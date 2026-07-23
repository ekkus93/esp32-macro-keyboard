#include "macro_executor.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "macro_limits.h"
#include "usb_keyboard.h"

#define EXECUTION_WATCHDOG_MARGIN_MS 1000U
#define CANCELLATION_SLICE_MS 10U

static QueueHandle_t request_queue;
static SemaphoreHandle_t status_mutex;
static TaskHandle_t executor_task_handle;
static macro_execution_status_t status;
static bool busy;
static bool cancellation_requested;

static bool lock_status(void)
{
    return status_mutex != NULL && xSemaphoreTake(status_mutex, portMAX_DELAY) == pdTRUE;
}

static void unlock_status(void)
{
    (void)xSemaphoreGive(status_mutex);
}

static void status_update(macro_execution_status_t next)
{
    if (lock_status()) {
        status = next;
        unlock_status();
    }
}

macro_execution_status_t macro_executor_get_status(void)
{
    macro_execution_status_t snapshot = {.state = EXECUTION_FAILED, .error = APP_ERROR_INTERNAL};
    if (lock_status()) {
        snapshot = status;
        unlock_status();
    }
    return snapshot;
}

static bool cancelled(void)
{
    bool value = true;
    if (lock_status()) {
        value = cancellation_requested;
        unlock_status();
    }
    return value;
}

static bool deadline_expired(TickType_t deadline)
{
    return (int32_t)(xTaskGetTickCount() - deadline) >= 0;
}

static app_error_code_t cancellable_delay(uint32_t delay_ms, TickType_t deadline)
{
    uint32_t remaining = delay_ms;
    while (remaining > 0U) {
        if (cancelled()) {
            return APP_ERROR_EXECUTION_CANCELLED;
        }
        if (deadline_expired(deadline)) {
            return APP_ERROR_TIMEOUT;
        }
        const uint32_t slice = remaining > CANCELLATION_SLICE_MS ? CANCELLATION_SLICE_MS : remaining;
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(slice));
        remaining -= slice;
    }
    return APP_ERROR_NONE;
}

static void finish(macro_execution_status_t current,
                   execution_state_t terminal_state,
                   app_error_code_t error)
{
    current.release_error = usb_keyboard_release_all();
    current.state = terminal_state;
    current.error = error;
    status_update(current);

    if (lock_status()) {
        busy = false;
        cancellation_requested = false;
        unlock_status();
    }
}

static void execute_request(macro_execution_request_t *request)
{
    macro_execution_status_t current = {
        .state = EXECUTION_RUNNING,
        .error = APP_ERROR_NONE,
        .release_error = APP_ERROR_NONE,
        .execution_id = request->execution_id,
        .action_index = 0U,
        .action_count = request->plan.action_count,
    };
    status_update(current);

    const uint32_t watchdog_ms = request->plan.estimated_duration_ms + EXECUTION_WATCHDOG_MARGIN_MS;
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(watchdog_ms);
    app_error_code_t result = APP_ERROR_NONE;
    for (size_t index = 0U; index < request->plan.action_count; ++index) {
        current.action_index = index;
        status_update(current);
        if (cancelled()) {
            result = APP_ERROR_EXECUTION_CANCELLED;
            break;
        }
        if (deadline_expired(deadline)) {
            result = APP_ERROR_TIMEOUT;
            break;
        }

        const macro_action_t action = request->plan.actions[index];
        if (action.type == MACRO_ACTION_DELAY) {
            result = cancellable_delay(action.delay_ms, deadline);
        } else {
            result = usb_keyboard_press(action.modifiers, action.usage);
            if (result == APP_ERROR_NONE) {
                result = cancellable_delay(request->key_press_ms, deadline);
            }
            const app_error_code_t release_result = usb_keyboard_release_all();
            if (result == APP_ERROR_NONE) {
                result = release_result;
            }
            if (result == APP_ERROR_NONE) {
                result = cancellable_delay(request->inter_key_ms, deadline);
            }
        }
        if (result != APP_ERROR_NONE) {
            break;
        }
    }

    macro_plan_free(&request->plan);
    if (result == APP_ERROR_NONE) {
        finish(current, EXECUTION_COMPLETED, APP_ERROR_NONE);
    } else if (result == APP_ERROR_EXECUTION_CANCELLED) {
        finish(current, EXECUTION_CANCELLED, result);
    } else {
        finish(current, EXECUTION_FAILED, result);
    }
}

static void executor_task(void *context)
{
    (void)context;
    macro_execution_request_t request;
    while (true) {
        if (xQueueReceive(request_queue, &request, portMAX_DELAY) == pdTRUE) {
            execute_request(&request);
        }
    }
}

app_error_code_t macro_executor_init(void)
{
    status_mutex = xSemaphoreCreateMutex();
    if (status_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    request_queue = xQueueCreate(1U, sizeof(macro_execution_request_t));
    if (request_queue == NULL) {
        vSemaphoreDelete(status_mutex);
        status_mutex = NULL;
        return APP_ERROR_INTERNAL;
    }
    status = (macro_execution_status_t){.state = EXECUTION_IDLE};
    if (xTaskCreate(executor_task, "macro_executor", 4096U, NULL, 8U,
                    &executor_task_handle) != pdPASS) {
        vQueueDelete(request_queue);
        request_queue = NULL;
        vSemaphoreDelete(status_mutex);
        status_mutex = NULL;
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_submit(macro_execution_request_t *request)
{
    if (request == NULL || request->plan.actions == NULL || request->plan.action_count == 0U ||
        request->plan.action_count > APP_COMPILED_ACTION_MAX || request->key_press_ms == 0U ||
        request->key_press_ms > APP_DELAY_MAX_MS || request->inter_key_ms > APP_DELAY_MAX_MS ||
        !app_uuid_is_valid_string(request->execution_id.value) ||
        !app_uuid_is_valid_string(request->set_id.value) ||
        !app_uuid_is_valid_string(request->macro_id.value) || request->macro_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (usb_keyboard_get_state() != USB_KEYBOARD_READY) {
        return APP_ERROR_USB_NOT_READY;
    }
    if (!lock_status()) {
        return APP_ERROR_INTERNAL;
    }
    if (busy) {
        unlock_status();
        return APP_ERROR_EXECUTOR_BUSY;
    }
    busy = true;
    cancellation_requested = false;
    unlock_status();

    if (xQueueSend(request_queue, request, 0U) != pdTRUE) {
        if (lock_status()) {
            busy = false;
            unlock_status();
        }
        return APP_ERROR_INTERNAL;
    }
    request->plan.actions = NULL;
    request->plan.action_count = 0U;
    request->plan.estimated_duration_ms = 0U;
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_cancel(void)
{
    if (!lock_status()) {
        return APP_ERROR_INTERNAL;
    }
    if (!busy) {
        unlock_status();
        return APP_ERROR_NOT_FOUND;
    }
    cancellation_requested = true;
    unlock_status();
    if (executor_task_handle == NULL) {
        return APP_ERROR_INTERNAL;
    }
    xTaskNotifyGive(executor_task_handle);
    return APP_ERROR_NONE;
}
