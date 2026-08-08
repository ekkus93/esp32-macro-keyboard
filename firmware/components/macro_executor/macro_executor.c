#include "macro_executor.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "macro_executor_engine.h"
#include "macro_executor_ops.h"
#include "macro_parser.h"
#include "usb_keyboard.h"

/* Bounded shutdown timeouts (FIX1 §12.2): time to enqueue the stop message, and
 * time to wait for the worker task to acknowledge it has exited. */
#define SHUTDOWN_ENQUEUE_MS 100U
#define SHUTDOWN_WAIT_MS 2000U

static const char *const TAG = "macro_executor";

/* Tagged worker message (FIX1 §12.1): an execution request, or a cooperative stop
 * that makes the task leave its loop and delete itself cleanly. */
typedef enum {
    MACRO_EXECUTOR_MESSAGE_EXECUTE = 0,
    MACRO_EXECUTOR_MESSAGE_STOP,
} macro_executor_message_type_t;

typedef struct {
    macro_executor_message_type_t type;
    macro_execution_request_t request;
} macro_executor_message_t;

static QueueHandle_t request_queue;
static SemaphoreHandle_t status_mutex;
static SemaphoreHandle_t executor_stopped;
static TaskHandle_t executor_task_handle;
static macro_executor_engine_t engine;
/* Set once deinit begins so no new submission is accepted while the worker is
 * being torn down. */
static volatile bool shutting_down;
/* Health: the worker could not signal its own exit; folded into the deinit
 * result so a failed stop is never silently ignored. */
static volatile bool executor_stop_signal_failed;

static bool adapter_lock(void *context) {
    (void)context;
    return status_mutex != NULL && xSemaphoreTake(status_mutex, portMAX_DELAY) == pdTRUE;
}

static bool adapter_unlock(void *context) {
    (void)context;
    return status_mutex != NULL && xSemaphoreGive(status_mutex) == pdTRUE;
}

static bool adapter_queue_send(void *context, const macro_execution_request_t *request) {
    (void)context;
    if (request_queue == NULL || request == NULL) {
        return false;
    }
    const macro_executor_message_t message = {
        .type = MACRO_EXECUTOR_MESSAGE_EXECUTE,
        .request = *request,
    };
    return xQueueSend(request_queue, &message, 0U) == pdTRUE;
}

static void adapter_notify(void *context) {
    (void)context;
    if (executor_task_handle != NULL) {
        xTaskNotifyGive(executor_task_handle);
    }
}

static uint32_t adapter_now_ms(void *context) {
    (void)context;
    const uint64_t milliseconds = (uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS;
    return (uint32_t)milliseconds;
}

static app_error_code_t adapter_wait_ms(void *context, uint32_t milliseconds) {
    (void)context;
    (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(milliseconds));
    return APP_ERROR_NONE;
}

static bool adapter_usb_ready(void *context) {
    (void)context;
    return usb_keyboard_get_state() == USB_KEYBOARD_READY;
}

static app_error_code_t adapter_usb_press(void *context, uint8_t modifiers, uint8_t usage) {
    (void)context;
    return usb_keyboard_press(modifiers, usage);
}

static app_error_code_t adapter_usb_release_all(void *context) {
    (void)context;
    return usb_keyboard_release_all();
}

static void adapter_plan_free(macro_plan_t *plan) {
    macro_plan_free(plan);
}

/* Cooperative worker loop (FIX1 §12.1): run execution requests until a stop
 * message arrives, then signal completion and delete the task. A forced
 * vTaskDelete is never used, so a request in flight always reaches its own
 * terminal cleanup (key release) first. */
static void executor_task(void *context) {
    (void)context;
    macro_executor_message_t message;
    while (true) {
        if (xQueueReceive(request_queue, &message, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (message.type == MACRO_EXECUTOR_MESSAGE_STOP) {
            break;
        }
        const app_error_code_t result = macro_executor_engine_execute(&engine, &message.request);
        if (result != APP_ERROR_NONE) {
            ESP_LOGE(TAG, "execution ended with %s", app_error_code_string(result));
        }
    }
    if (xSemaphoreGive(executor_stopped) != pdTRUE) {
        executor_stop_signal_failed = true;
        ESP_LOGE(TAG, "failed to signal executor stop");
    }
    vTaskDelete(NULL);
}

static void cleanup_partial_init(void) {
    if (request_queue != NULL) {
        vQueueDelete(request_queue);
        request_queue = NULL;
    }
    if (executor_stopped != NULL) {
        vSemaphoreDelete(executor_stopped);
        executor_stopped = NULL;
    }
    if (status_mutex != NULL) {
        vSemaphoreDelete(status_mutex);
        status_mutex = NULL;
    }
}

app_error_code_t macro_executor_init(void) {
    if (status_mutex != NULL || request_queue != NULL || executor_stopped != NULL ||
        executor_task_handle != NULL) {
        return APP_ERROR_CONFLICT;
    }
    shutting_down = false;
    executor_stop_signal_failed = false;
    status_mutex = xSemaphoreCreateMutex();
    executor_stopped = xSemaphoreCreateBinary();
    /* Depth 2 so a cooperative stop can always be enqueued alongside at most one
     * in-flight execution request. */
    request_queue = xQueueCreate(2U, sizeof(macro_executor_message_t));
    if (status_mutex == NULL || executor_stopped == NULL || request_queue == NULL) {
        cleanup_partial_init();
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
        cleanup_partial_init();
        return result;
    }
    if (xTaskCreate(executor_task, "macro_executor", 4096U, NULL, 8U, &executor_task_handle) !=
        pdPASS) {
        cleanup_partial_init();
        memset(&engine, 0, sizeof(engine));
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

/* Ask the worker to stop cooperatively and wait, bounded, for it to exit. Sets
 * *out_stopped only when the task confirmed exit (so the caller knows the queue
 * and semaphores are safe to delete). Returns the first error encountered. */
static app_error_code_t stop_executor_task(bool *out_stopped) {
    *out_stopped = false;
    app_error_code_t first_error = APP_ERROR_NONE;
    const app_error_code_t cancel_result = macro_executor_engine_cancel(&engine);
    if (cancel_result != APP_ERROR_NONE && cancel_result != APP_ERROR_NOT_FOUND) {
        first_error = cancel_result;
    }
    const macro_executor_message_t stop = {.type = MACRO_EXECUTOR_MESSAGE_STOP};
    if (xQueueSend(request_queue, &stop, pdMS_TO_TICKS(SHUTDOWN_ENQUEUE_MS)) != pdTRUE &&
        first_error == APP_ERROR_NONE) {
        first_error = APP_ERROR_INTERNAL;
    }
    if (xSemaphoreTake(executor_stopped, pdMS_TO_TICKS(SHUTDOWN_WAIT_MS)) == pdTRUE) {
        *out_stopped = true;
    } else if (first_error == APP_ERROR_NONE) {
        first_error = APP_ERROR_TIMEOUT;
    }
    return first_error;
}

/* Free the plan of any execution request still queued (never dequeued by the
 * worker), so a shutdown with pending work does not leak the compiled plan. */
static void drain_pending_plans(void) {
    if (request_queue == NULL) {
        return;
    }
    macro_executor_message_t message;
    while (xQueueReceive(request_queue, &message, 0U) == pdTRUE) {
        if (message.type == MACRO_EXECUTOR_MESSAGE_EXECUTE) {
            macro_plan_free(&message.request.plan);
        }
    }
}

app_error_code_t macro_executor_deinit(void) {
    if (status_mutex == NULL && request_queue == NULL && executor_stopped == NULL &&
        executor_task_handle == NULL) {
        return APP_ERROR_NONE;
    }
    shutting_down = true;
    app_error_code_t first_error = APP_ERROR_NONE;

    if (executor_task_handle != NULL) {
        bool stopped = false;
        first_error = stop_executor_task(&stopped);
        if (!stopped) {
            /* The worker did not confirm exit; deleting the queue and semaphores it
             * may still touch would be a use-after-free. Leave them in place and
             * report the failure (fail-closed) rather than risk a crash. */
            return first_error;
        }
        executor_task_handle = NULL;
    }

    const app_error_code_t release_result = usb_keyboard_release_all();
    if (release_result != APP_ERROR_NONE && first_error == APP_ERROR_NONE) {
        first_error = release_result;
    }
    drain_pending_plans();

    if (request_queue != NULL) {
        vQueueDelete(request_queue);
        request_queue = NULL;
    }
    if (executor_stopped != NULL) {
        vSemaphoreDelete(executor_stopped);
        executor_stopped = NULL;
    }
    if (status_mutex != NULL) {
        vSemaphoreDelete(status_mutex);
        status_mutex = NULL;
    }
    memset(&engine, 0, sizeof(engine));
    if (executor_stop_signal_failed && first_error == APP_ERROR_NONE) {
        first_error = APP_ERROR_INTERNAL;
    }
    executor_stop_signal_failed = false;
    shutting_down = false;
    return first_error;
}

app_error_code_t macro_executor_submit(macro_execution_request_t *request) {
    if (shutting_down) {
        return APP_ERROR_CONFLICT;
    }
    return macro_executor_engine_submit(&engine, request);
}

app_error_code_t macro_executor_cancel(void) {
    return macro_executor_engine_cancel(&engine);
}

app_error_code_t macro_executor_confirm(void) {
    return macro_executor_engine_confirm(&engine);
}

macro_execution_status_t macro_executor_get_status(void) {
    return macro_executor_engine_get_status(&engine);
}

size_t macro_executor_stack_high_water_mark(void) {
    return executor_task_handle != NULL ? (size_t)uxTaskGetStackHighWaterMark(executor_task_handle)
                                        : 0U;
}
