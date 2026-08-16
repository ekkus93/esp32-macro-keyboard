#include "web_server_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "app_error.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "http_health.h"
#include "macro_limits.h"
#include "web_api_core.h"
#include "web_http_status.h"

/* Physical confirmation blocks the request handler for up to
 * APP_PHYSICAL_CONFIRM_TIMEOUT_MS while device_controls waits for the button.
 * esp_http_server runs one task with select() over every socket - there is no
 * worker pool - so performing that wait on the httpd task freezes every other
 * client for the entire window. Measured on hardware before this change: an
 * unrelated GET /api/v1/status took 18.0 s while a restore sat in the wait,
 * against a 25-43 ms idle baseline.
 *
 * Confirmation-gated requests are therefore moved onto this worker via
 * httpd_req_async_handler_begin(), which releases the httpd task immediately. */

/* The request and the body that arrived with it. esp_http_server gives an async
 * handler the request but not its unread payload, so the body has to be read on
 * the httpd task and carried across. Without this, restore and import reached
 * their handlers with body_length 0 and answered 422 having done nothing. */
typedef struct {
    httpd_req_t *request;
    char *body;
    size_t body_length;
} async_item_t;

#define WEB_ASYNC_QUEUE_DEPTH 1U
#define WEB_ASYNC_TASK_PRIORITY 5

/* Same budget as the httpd task, because the deepest call chains in the
 * codebase now run here instead of there: restore_locked (~20 KB) and
 * import_locked (~19 KB) are both reached only through confirmation-gated
 * routes. scripts/check-stack-usage.sh measures those frames against this. */
#define WEB_ASYNC_TASK_STACK_BYTES 24576U

/* Stopping has to outlast a confirmation wait already in progress, because the
 * worker owns an httpd request that must be completed before httpd_stop(). */
#define WEB_ASYNC_STOP_TIMEOUT_MS (APP_PHYSICAL_CONFIRM_TIMEOUT_MS + 5000U)

static TaskHandle_t async_task_handle;
static QueueHandle_t async_queue;
static SemaphoreHandle_t async_stopped;

/* Guards in_flight. A single button press cannot disambiguate two pending
 * confirmations, so at most one such request is accepted at a time. */
static portMUX_TYPE async_lock = portMUX_INITIALIZER_UNLOCKED;
static bool async_in_flight;

static bool claim_in_flight(void) {
    bool claimed = false;
    portENTER_CRITICAL(&async_lock);
    if (!async_in_flight) {
        async_in_flight = true;
        claimed = true;
    }
    portEXIT_CRITICAL(&async_lock);
    return claimed;
}

static void release_in_flight(void) {
    portENTER_CRITICAL(&async_lock);
    async_in_flight = false;
    portEXIT_CRITICAL(&async_lock);
}

static void async_worker(void *context) {
    (void)context;
    while (true) {
        async_item_t *item = NULL;
        if (xQueueReceive(async_queue, (void *)&item, portMAX_DELAY) != pdTRUE) {
            http_health_record_async_failure(HTTP_ASYNC_FAILURE_QUEUE, APP_ERROR_INTERNAL);
            continue;
        }
        if (item == NULL) {
            break; /* stop sentinel */
        }
        httpd_req_t *request = item->request;

        /* Ownership of item->body passes to the call, which frees it. */
        const esp_err_t send_result =
            web_api_handle_call_with_body(request, item->body, item->body_length);
        if (send_result != ESP_OK) {
            http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_RUN, APP_ERROR_IO);
        }
        free(item);

        /* Always complete, on every path. An async request left incomplete
         * never releases its socket, and once those run out the server stops
         * accepting connections entirely. */
        const esp_err_t completion_result = httpd_req_async_handler_complete(request);
        if (completion_result != ESP_OK) {
            http_health_record_async_failure(HTTP_ASYNC_FAILURE_COMPLETION, APP_ERROR_IO);
        }
        release_in_flight();
    }
    if (xSemaphoreGive(async_stopped) != pdTRUE) {
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_STOP, APP_ERROR_INTERNAL);
    }
    vTaskDelete(NULL);
}

app_error_code_t web_server_async_start(void) {
    if (async_task_handle != NULL || async_queue != NULL || async_stopped != NULL) {
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_START, APP_ERROR_CONFLICT);
        return APP_ERROR_CONFLICT;
    }
    async_queue = xQueueCreate(WEB_ASYNC_QUEUE_DEPTH, sizeof(async_item_t *));
    if (async_queue == NULL) {
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_START, APP_ERROR_INTERNAL);
        return APP_ERROR_INTERNAL;
    }
    async_stopped = xSemaphoreCreateBinary();
    if (async_stopped == NULL) {
        vQueueDelete(async_queue);
        async_queue = NULL;
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_START, APP_ERROR_INTERNAL);
        return APP_ERROR_INTERNAL;
    }
    async_in_flight = false;
    if (xTaskCreate(async_worker, "web_async", WEB_ASYNC_TASK_STACK_BYTES, NULL,
                    WEB_ASYNC_TASK_PRIORITY, &async_task_handle) != pdPASS) {
        vSemaphoreDelete(async_stopped);
        async_stopped = NULL;
        vQueueDelete(async_queue);
        async_queue = NULL;
        async_task_handle = NULL;
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_START, APP_ERROR_INTERNAL);
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

app_error_code_t web_server_async_stop(void) {
    if (async_task_handle == NULL && async_queue == NULL && async_stopped == NULL) {
        return APP_ERROR_NONE;
    }
    if (async_task_handle == NULL || async_queue == NULL || async_stopped == NULL) {
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_STOP, APP_ERROR_INTERNAL);
        return APP_ERROR_INTERNAL;
    }
    async_item_t *sentinel = NULL;
    /* Blocks until the queue drains, so a request already waiting for the
     * button is finished and completed rather than abandoned. */
    if (xQueueSend(async_queue, (const void *)&sentinel,
                   pdMS_TO_TICKS(WEB_ASYNC_STOP_TIMEOUT_MS)) != pdTRUE) {
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_STOP, APP_ERROR_TIMEOUT);
        return APP_ERROR_TIMEOUT;
    }
    if (xSemaphoreTake(async_stopped, pdMS_TO_TICKS(WEB_ASYNC_STOP_TIMEOUT_MS)) != pdTRUE) {
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_STOP, APP_ERROR_TIMEOUT);
        return APP_ERROR_TIMEOUT;
    }
    vSemaphoreDelete(async_stopped);
    async_stopped = NULL;
    vQueueDelete(async_queue);
    async_queue = NULL;
    async_task_handle = NULL;
    async_in_flight = false;
    return APP_ERROR_NONE;
}

esp_err_t web_server_async_dispatch(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    if (async_queue == NULL || async_task_handle == NULL) {
        /* Confirmation-gated work must never fall back to the httpd task:
         * waiting there blocks the single server task and turns an async
         * subsystem fault into whole-server unavailability. Fail closed and
         * leave the requested operation untouched. */
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_START, APP_ERROR_INTERNAL);
        return web_api_send_status_error(request, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                         APP_ERROR_INTERNAL, "confirmation service unavailable");
    }
    if (!claim_in_flight()) {
        return web_api_send_status_error(request, WEB_HTTP_STATUS_CONFLICT, APP_ERROR_CONFLICT,
                                         "another request is already awaiting confirmation");
    }
    /* Read the body here, on the httpd task, while the payload is still
     * readable. After httpd_req_async_handler_begin() it is gone: the async
     * handler receives the request, not its unread content. */
    char *body = NULL;
    size_t body_length = 0U;
    const app_error_code_t body_result = web_api_read_route_body(request, &body, &body_length);
    if (body_result != APP_ERROR_NONE && body_result != APP_ERROR_NOT_FOUND) {
        release_in_flight();
        return web_api_send_status_error(request, web_api_http_status_for_error(body_result),
                                         body_result, "could not read request body");
    }

    async_item_t *item = calloc(1U, sizeof(*item));
    if (item == NULL) {
        free(body);
        release_in_flight();
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_RUN, APP_ERROR_INTERNAL);
        return web_api_send_status_error(request, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                         APP_ERROR_INTERNAL, "could not start confirmation");
    }
    item->body = body;
    item->body_length = body_length;

    httpd_req_t *async_request = NULL;
    if (httpd_req_async_handler_begin(request, &async_request) != ESP_OK || async_request == NULL) {
        free(item->body);
        free(item);
        release_in_flight();
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_WORKER_RUN, APP_ERROR_INTERNAL);
        return web_api_send_status_error(request, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                         APP_ERROR_INTERNAL, "could not start confirmation");
    }
    item->request = async_request;
    if (xQueueSend(async_queue, (const void *)&item, 0) != pdTRUE) {
        /* Unreachable while in_flight gates entry, but the request must still
         * be answered and completed or its socket leaks permanently. */
        http_health_record_async_failure(HTTP_ASYNC_FAILURE_QUEUE, APP_ERROR_INTERNAL);
        const esp_err_t result =
            web_api_send_status_error(async_request, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                      APP_ERROR_INTERNAL, "could not queue confirmation");
        if (httpd_req_async_handler_complete(async_request) != ESP_OK) {
            http_health_record_async_failure(HTTP_ASYNC_FAILURE_COMPLETION, APP_ERROR_IO);
        }
        free(item->body);
        free(item);
        release_in_flight();
        return result;
    }
    return ESP_OK;
}
