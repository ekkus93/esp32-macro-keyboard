#include "macro_executor.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "macro_executor_engine.h"
#include "usb_keyboard.h"

static const char *const TAG = "macro_executor";
static QueueHandle_t request_queue;
static SemaphoreHandle_t status_mutex;
static TaskHandle_t executor_task_handle;
static macro_executor_engine_t engine;

static bool adapter_lock(void *context)
{
    (void)context;
    return status_mutex != NULL && xSemaphoreTake(status_mutex, portMAX_DELAY) == pdTRUE;
}

static bool adapter_unlock(void *context)
{
    (void)context;
    return status_mutex != NULL && xSemaphoreGive(status_mutex) == pdTRUE;
}

static bool adapter_queue_send(void *context, const macro_execution_request_t *request)
{
    (void)context;
    return request_queue != NULL && request != NULL &&
           xQueueSend(request_queue, request, 0U) == pdTRUE;
}

static void adapter_notify(void *context)
{
    (void)context;
    if (executor_task_handle != NULL) {
        xTaskNotifyGive(executor_task_handle);
    }
}

static uint32_t adapter_now_ms(void *context)
{
    (void)context;
    const uint64_t milliseconds =
        (uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS;
    return (uint32_t)milliseconds;
}

static app_error_code_t adapter_wait_ms(void *context, uint32_t milliseconds)
{
    (void)context;
    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(milliseconds));
    return APP_ERROR_NONE;
}

static bool adapter_usb_ready(void *context)
{
    (void)context;
    return usb_keyboard_get_state() == USB_KEYBOARD_READY;
}

static app_error_code_t adapter_usb_press(void *context, uint8_t modifiers, uint8_t usage)
{
    (void)context;
    return usb_keyboard_press(modifiers, usage);
}

static app_error_code_t adapter_usb_release_all(void *context)
{
    (void)context;
    return usb_keyboard_release_all();
}

static void adapter_plan_free(macro_plan_t *plan)
{
    macro_plan_free(plan);
}

static void executor_task(void *context)
{
    (void)context;
    macro_execution_request_t request;
    while (true) {
        if (xQueueReceive(request_queue, &request, portMAX_DELAY) == pdTRUE) {
            const app_error_code_t result = macro_executor_engine_execute(&engine, &request);
            if (result == APP_ERROR_INTERNAL) {
                ESP_LOGE(TAG, "executor engine failed internally");
            }
        }
    }
}

app_error_code_t macro_executor_init(void)
{
    if (status_mutex != NULL || request_queue != NULL || executor_task_handle != NULL) {
        return APP_ERROR_CONFLICT;
    }
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
    const macro_executor_ops_t ops = {
        .context = NULL,
        .lock = adapter_lock,
        .unlock = adapter_unlock,
        .queue_send = adapter_queue_send,
        .notify_executor = adapter_notify,
        .now_ms = adapter_now_ms,
        .wait_ms = adapter_wait_ms,
        .usb_ready = adapter_usb_ready,
        .usb_press = adapter_usb_press,
        .usb_release_all = adapter_usb_release_all,
        .plan_free = adapter_plan_free,
    };
    app_error_code_t result = macro_executor_engine_init(&engine, &ops);
    if (result != APP_ERROR_NONE) {
        vQueueDelete(request_queue);
        request_queue = NULL;
        vSemaphoreDelete(status_mutex);
        status_mutex = NULL;
        return result;
    }
    if (xTaskCreate(executor_task,
                    "macro_executor",
                    4096U,
                    NULL,
                    8U,
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
    return macro_executor_engine_submit(&engine, request);
}

app_error_code_t macro_executor_cancel(void)
{
    return macro_executor_engine_cancel(&engine);
}

macro_execution_status_t macro_executor_get_status(void)
{
    return macro_executor_engine_get_status(&engine);
}
