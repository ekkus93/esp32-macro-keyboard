#include "web_server_internal.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "macro_limits.h"
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
        httpd_req_t *request = NULL;
        if (xQueueReceive(async_queue, (void *)&request, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        if (request == NULL) {
            break; /* stop sentinel */
        }

        bool should_restart = false;
        const esp_err_t send_result = web_api_handle_call(request, &should_restart);
        (void)send_result;

        /* Always complete, on every path. An async request left incomplete
         * never releases its socket, and once those run out the server stops
         * accepting connections entirely. */
        (void)httpd_req_async_handler_complete(request);
        release_in_flight();

        if (should_restart) {
            esp_restart();
        }
    }
    (void)xSemaphoreGive(async_stopped);
    vTaskDelete(NULL);
}

app_error_code_t web_server_async_start(void) {
    if (async_task_handle != NULL || async_queue != NULL || async_stopped != NULL) {
        return APP_ERROR_CONFLICT;
    }
    async_queue = xQueueCreate(WEB_ASYNC_QUEUE_DEPTH, sizeof(httpd_req_t *));
    if (async_queue == NULL) {
        return APP_ERROR_INTERNAL;
    }
    async_stopped = xSemaphoreCreateBinary();
    if (async_stopped == NULL) {
        vQueueDelete(async_queue);
        async_queue = NULL;
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
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

app_error_code_t web_server_async_stop(void) {
    if (async_task_handle == NULL || async_queue == NULL || async_stopped == NULL) {
        return APP_ERROR_NONE;
    }
    httpd_req_t *sentinel = NULL;
    /* Blocks until the queue drains, so a request already waiting for the
     * button is finished and completed rather than abandoned. */
    if (xQueueSend(async_queue, (const void *)&sentinel,
                   pdMS_TO_TICKS(WEB_ASYNC_STOP_TIMEOUT_MS)) != pdTRUE) {
        return APP_ERROR_TIMEOUT;
    }
    if (xSemaphoreTake(async_stopped, pdMS_TO_TICKS(WEB_ASYNC_STOP_TIMEOUT_MS)) != pdTRUE) {
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
        /* Worker unavailable: answer on the httpd task rather than drop the
         * request. This blocks other clients for the confirmation window, which
         * is the pre-existing behaviour and strictly better than failing. */
        bool should_restart = false;
        const esp_err_t result = web_api_handle_call(request, &should_restart);
        if (should_restart) {
            esp_restart();
        }
        return result;
    }
    if (!claim_in_flight()) {
        return web_api_send_status_error(request, WEB_HTTP_STATUS_CONFLICT, APP_ERROR_CONFLICT,
                                         "another request is already awaiting confirmation");
    }
    httpd_req_t *async_request = NULL;
    if (httpd_req_async_handler_begin(request, &async_request) != ESP_OK || async_request == NULL) {
        release_in_flight();
        return web_api_send_status_error(request, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                         APP_ERROR_INTERNAL, "could not start confirmation");
    }
    if (xQueueSend(async_queue, (const void *)&async_request, 0) != pdTRUE) {
        /* Unreachable while in_flight gates entry, but the request must still
         * be answered and completed or its socket leaks permanently. */
        const esp_err_t result =
            web_api_send_status_error(async_request, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                      APP_ERROR_INTERNAL, "could not queue confirmation");
        (void)httpd_req_async_handler_complete(async_request);
        release_in_flight();
        return result;
    }
    return ESP_OK;
}
