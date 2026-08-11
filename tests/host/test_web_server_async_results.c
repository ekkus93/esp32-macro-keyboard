/* Worker-capable host regression for H6-062/H6-063.
 *
 * Unlike test_web_server_async_confirmation.c's deliberate dead-path
 * FreeRTOS canaries, this target supplies a small pthread-backed queue,
 * semaphore, task, and critical-section model so the real
 * web_server_async.c worker actually runs. The model is intentionally
 * limited to the one-slot queue/binary semaphore semantics this module
 * uses; it is not a general FreeRTOS emulator.
 *
 * The tests prove that handler/send failures never skip async request
 * completion, completion failures are observed in HTTP health, queue
 * failure completes the already-cloned request and releases in-flight
 * ownership, and a failed stop signal is returned/health-recorded while
 * leaving the worker state retryable for a later clean stop. */

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app_error.h"
#include "auth.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "fake_httpd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "http_health.h"
#include "macro_executor.h"
#include "storage.h"
#include "storage_blob.h"
#include "test_assert.h"
#include "usb_keyboard.h"
#include "web_api_core.h"
#include "web_http_status.h"
#include "web_server_internal.h"
#include "wifi_ap.h"

/* ------------------------------------------------------------------
 * Focused pthread-backed FreeRTOS subset.
 * ------------------------------------------------------------------ */

struct QueueDefinition {
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    bool active;
    bool full;
    void *value;
};

static struct QueueDefinition g_queue = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .changed = PTHREAD_COND_INITIALIZER,
};
static struct QueueDefinition g_semaphore = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .changed = PTHREAD_COND_INITIALIZER,
};
static bool g_fail_next_queue_send;

struct tskTaskControlBlock {
    pthread_t thread;
};

static struct tskTaskControlBlock g_task;
static pthread_mutex_t g_task_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_task_changed = PTHREAD_COND_INITIALIZER;
static bool g_task_running;
static TaskFunction_t g_task_function;
static void *g_task_context;

static pthread_mutex_t g_port_lock = PTHREAD_MUTEX_INITIALIZER;

/* The real async worker can spend up to APP_PHYSICAL_CONFIRM_TIMEOUT_MS inside
 * web_api_handle_call_with_body() while policy confirmation waits. These
 * synchronization points let H6-063 hold that worker deterministically without
 * sleeping for the production timeout. */
static pthread_mutex_t g_handler_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_handler_changed = PTHREAD_COND_INITIALIZER;
static bool g_handler_should_block;
static bool g_handler_entered;
static bool g_handler_release;

static pthread_mutex_t g_stop_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_stop_changed = PTHREAD_COND_INITIALIZER;
static bool g_stop_started;
static bool g_stop_finished;
static app_error_code_t g_stop_result;

static struct timespec realtime_deadline_after_ms(long milliseconds) {
    struct timespec deadline = {0};
    TEST_CHECK_EQ_INT(0, clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec += milliseconds / 1000L;
    deadline.tv_nsec += (milliseconds % 1000L) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec += 1;
        deadline.tv_nsec -= 1000000000L;
    }
    return deadline;
}

static int64_t monotonic_milliseconds(void) {
    struct timespec now = {0};
    TEST_CHECK_EQ_INT(0, clock_gettime(CLOCK_MONOTONIC, &now));
    return (int64_t)now.tv_sec * 1000LL + (int64_t)(now.tv_nsec / 1000000L);
}

static void handler_block_enable(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_handler_mutex));
    g_handler_should_block = true;
    g_handler_entered = false;
    g_handler_release = false;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_handler_mutex));
}

static void wait_for_handler_entry(void) {
    const struct timespec deadline = realtime_deadline_after_ms(1000L);
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_handler_mutex));
    while (!g_handler_entered) {
        TEST_CHECK_EQ_INT(0,
                          pthread_cond_timedwait(&g_handler_changed, &g_handler_mutex, &deadline));
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_handler_mutex));
}

static void release_blocked_handler(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_handler_mutex));
    g_handler_release = true;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_handler_changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_handler_mutex));
}

static void reset_stop_state(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_stop_mutex));
    g_stop_started = false;
    g_stop_finished = false;
    g_stop_result = APP_ERROR_INTERNAL;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_stop_mutex));
}

static void *stop_thread_entry(void *unused) {
    (void)unused;
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_stop_mutex));
    g_stop_started = true;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_stop_changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_stop_mutex));

    const app_error_code_t result = web_server_async_stop();

    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_stop_mutex));
    g_stop_result = result;
    g_stop_finished = true;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_stop_changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_stop_mutex));
    return NULL;
}

static void wait_for_stop_start(void) {
    const struct timespec deadline = realtime_deadline_after_ms(1000L);
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_stop_mutex));
    while (!g_stop_started) {
        TEST_CHECK_EQ_INT(0, pthread_cond_timedwait(&g_stop_changed, &g_stop_mutex, &deadline));
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_stop_mutex));
}

static bool stop_finished_snapshot(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_stop_mutex));
    const bool finished = g_stop_finished;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_stop_mutex));
    return finished;
}

static void fake_freertos_wait_for_idle(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_task_mutex));
    while (g_task_running) {
        TEST_CHECK_EQ_INT(0, pthread_cond_wait(&g_task_changed, &g_task_mutex));
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_task_mutex));
}

static void fake_freertos_fail_next_queue_send(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_queue.mutex));
    g_fail_next_queue_send = true;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
}

QueueHandle_t xQueueCreate(UBaseType_t queue_length, UBaseType_t item_size) {
    if (queue_length != 1U || item_size != sizeof(void *)) {
        return NULL;
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_queue.mutex));
    g_queue.active = true;
    g_queue.full = false;
    g_queue.value = NULL;
    g_fail_next_queue_send = false;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
    return &g_queue;
}

BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait) {
    if (queue != &g_queue || item == NULL) {
        return pdFAIL;
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_queue.mutex));
    if (g_fail_next_queue_send) {
        g_fail_next_queue_send = false;
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
        return pdFAIL;
    }
    while (g_queue.active && g_queue.full) {
        if (ticks_to_wait == 0U) {
            TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
            return pdFAIL;
        }
        TEST_CHECK_EQ_INT(0, pthread_cond_wait(&g_queue.changed, &g_queue.mutex));
    }
    if (!g_queue.active) {
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
        return pdFAIL;
    }
    memcpy(&g_queue.value, item, sizeof(g_queue.value));
    g_queue.full = true;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_queue.changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
    return pdTRUE;
}

BaseType_t xQueueReceive(QueueHandle_t queue, void *out_item, TickType_t ticks_to_wait) {
    if (queue != &g_queue || out_item == NULL) {
        return pdFAIL;
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_queue.mutex));
    while (g_queue.active && !g_queue.full) {
        if (ticks_to_wait == 0U) {
            TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
            return pdFAIL;
        }
        TEST_CHECK_EQ_INT(0, pthread_cond_wait(&g_queue.changed, &g_queue.mutex));
    }
    if (!g_queue.active) {
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
        return pdFAIL;
    }
    memcpy(out_item, &g_queue.value, sizeof(g_queue.value));
    g_queue.value = NULL;
    g_queue.full = false;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_queue.changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
    return pdTRUE;
}

void vQueueDelete(QueueHandle_t queue) {
    TEST_CHECK(queue == &g_queue);
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_queue.mutex));
    g_queue.active = false;
    g_queue.full = false;
    g_queue.value = NULL;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_queue.changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_queue.mutex));
}

SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_semaphore.mutex));
    g_semaphore.active = true;
    g_semaphore.full = false;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
    return &g_semaphore;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore) {
    if (semaphore != &g_semaphore) {
        return pdFAIL;
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_semaphore.mutex));
    if (!g_semaphore.active) {
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
        return pdFAIL;
    }
    g_semaphore.full = true;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_semaphore.changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
    return pdTRUE;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t ticks_to_wait) {
    if (semaphore != &g_semaphore) {
        return pdFAIL;
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_semaphore.mutex));
    while (g_semaphore.active && !g_semaphore.full) {
        if (ticks_to_wait == 0U) {
            TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
            return pdFAIL;
        }
        TEST_CHECK_EQ_INT(0, pthread_cond_wait(&g_semaphore.changed, &g_semaphore.mutex));
    }
    if (!g_semaphore.active) {
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
        return pdFAIL;
    }
    g_semaphore.full = false;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
    return pdTRUE;
}

void vSemaphoreDelete(SemaphoreHandle_t semaphore) {
    TEST_CHECK(semaphore == &g_semaphore);
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_semaphore.mutex));
    g_semaphore.active = false;
    g_semaphore.full = false;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_semaphore.changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_semaphore.mutex));
}

static void *fake_task_entry(void *unused) {
    (void)unused;
    g_task_function(g_task_context);
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_task_mutex));
    g_task_running = false;
    TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_task_changed));
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_task_mutex));
    return NULL;
}

BaseType_t xTaskCreate(TaskFunction_t task_code, const char *name, uint32_t stack_depth,
                       void *parameters, UBaseType_t priority, TaskHandle_t *out_handle) {
    (void)name;
    (void)stack_depth;
    (void)priority;
    if (task_code == NULL || out_handle == NULL) {
        return pdFAIL;
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_task_mutex));
    if (g_task_running) {
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_task_mutex));
        return pdFAIL;
    }
    g_task_function = task_code;
    g_task_context = parameters;
    g_task_running = true;
    const int create_result = pthread_create(&g_task.thread, NULL, fake_task_entry, NULL);
    if (create_result != 0) {
        g_task_running = false;
        TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_task_mutex));
        return pdFAIL;
    }
    TEST_CHECK_EQ_INT(0, pthread_detach(g_task.thread));
    *out_handle = &g_task;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_task_mutex));
    return pdPASS;
}

void vTaskDelete(TaskHandle_t task) {
    TEST_CHECK(task == NULL);
}

void vPortEnterCritical(portMUX_TYPE *mux) {
    (void)mux;
    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_port_lock));
}

void vPortExitCritical(portMUX_TYPE *mux) {
    (void)mux;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_port_lock));
}

/* ------------------------------------------------------------------
 * HTTP/API seams around the real worker.
 * ------------------------------------------------------------------ */

static esp_err_t g_handler_result;
static bool g_handler_should_restart;
static size_t g_handler_calls;
static esp_err_t g_completion_result;
static size_t g_completion_calls;
static size_t g_begin_calls;
static esp_err_t g_status_send_result;
static size_t g_status_send_calls;
static unsigned int g_last_status;
static app_error_code_t g_last_status_code;
static size_t g_restart_calls;
static httpd_req_t g_async_request;

app_error_code_t web_api_read_route_body(httpd_req_t *request, char **out_body,
                                         size_t *out_length) {
    (void)request;
    *out_body = NULL;
    *out_length = 0U;
    return APP_ERROR_NOT_FOUND;
}

esp_err_t web_api_handle_call_with_body(httpd_req_t *request, char *preread_body,
                                        size_t preread_length, bool *out_should_restart) {
    (void)request;
    (void)preread_length;
    ++g_handler_calls;

    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_handler_mutex));
    if (g_handler_should_block) {
        g_handler_entered = true;
        TEST_CHECK_EQ_INT(0, pthread_cond_broadcast(&g_handler_changed));
        while (!g_handler_release) {
            TEST_CHECK_EQ_INT(0, pthread_cond_wait(&g_handler_changed, &g_handler_mutex));
        }
    }
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_handler_mutex));

    free(preread_body);
    *out_should_restart = g_handler_should_restart;
    return g_handler_result;
}

esp_err_t web_api_send_status_error(httpd_req_t *request, unsigned int status,
                                    app_error_code_t code, const char *message) {
    (void)request;
    (void)message;
    ++g_status_send_calls;
    g_last_status = status;
    g_last_status_code = code;
    return g_status_send_result;
}

unsigned int web_api_http_status_for_error(app_error_code_t error) {
    (void)error;
    return 500U;
}

esp_err_t httpd_req_async_handler_begin(httpd_req_t *request, httpd_req_t **out_request) {
    ++g_begin_calls;
    g_async_request = *request;
    *out_request = &g_async_request;
    return ESP_OK;
}

esp_err_t httpd_req_async_handler_complete(httpd_req_t *request) {
    TEST_CHECK(request == &g_async_request);
    ++g_completion_calls;
    return g_completion_result;
}

void esp_restart(void) {
    ++g_restart_calls;
}

#define STATUS_TEST_SESSION_TOKEN                                                                  \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

/* Real GET /api/v1/status dependencies. Keeping this route real is important
 * for H6-063: the test must prove that an unrelated production handler remains
 * responsive while the async confirmation worker is blocked. */
static app_error_code_t g_status_auth_result;
static app_error_code_t g_status_settings_result;
static app_v2_device_settings_t g_status_settings;
static wifi_ap_status_t g_status_wifi;
static macro_execution_status_t g_status_execution;
static usb_keyboard_state_t g_status_usb;
static storage_mount_state_t g_status_mount;
static app_error_code_t g_status_capacity_result;
static size_t g_status_total_bytes;
static size_t g_status_used_bytes;
static app_error_code_t g_status_blob_result;
static size_t g_status_blob_count;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_status_auth_result;
}

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    if (g_status_settings_result != APP_ERROR_NONE) {
        return g_status_settings_result;
    }
    *out_settings = g_status_settings;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    (void)settings;
    (void)out_changed;
    TEST_CHECK(false);
    return APP_ERROR_INTERNAL;
}

wifi_ap_status_t wifi_ap_get_status(void) {
    return g_status_wifi;
}

macro_execution_status_t macro_executor_get_status(void) {
    return g_status_execution;
}

usb_keyboard_state_t usb_keyboard_get_state(void) {
    return g_status_usb;
}

storage_mount_state_t storage_mount_state(void) {
    return g_status_mount;
}

app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes) {
    TEST_CHECK_EQ_STRING(STORAGE_DATA_PARTITION, partition_label);
    if (g_status_capacity_result != APP_ERROR_NONE) {
        return g_status_capacity_result;
    }
    *out_total_bytes = g_status_total_bytes;
    *out_used_bytes = g_status_used_bytes;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_list(const storage_blob_scan_observer_t *observer,
                                   storage_blob_scan_summary_t *out_summary) {
    (void)observer;
    if (g_status_blob_result != APP_ERROR_NONE) {
        return g_status_blob_result;
    }
    *out_summary = (storage_blob_scan_summary_t){.valid_count = g_status_blob_count};
    return APP_ERROR_NONE;
}

const esp_app_desc_t *esp_app_get_description(void) {
    static const esp_app_desc_t description = {.version = "0.2.0"};
    return &description;
}

int esp_app_get_elf_sha256(char *dst, size_t size) {
    (void)snprintf(dst, size, "%s", "git-h6063");
    return 0;
}

int64_t esp_timer_get_time(void) {
    return 123456000;
}

static void authenticate_status(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" STATUS_TEST_SESSION_TOKEN);
}

static void reset_fakes(void) {
    fake_freertos_wait_for_idle();
    g_handler_result = ESP_OK;
    g_handler_should_restart = false;
    g_handler_calls = 0U;
    g_completion_result = ESP_OK;
    g_completion_calls = 0U;
    g_begin_calls = 0U;
    g_status_send_result = ESP_OK;
    g_status_send_calls = 0U;
    g_last_status = 0U;
    g_last_status_code = APP_ERROR_NONE;
    g_restart_calls = 0U;

    TEST_CHECK_EQ_INT(0, pthread_mutex_lock(&g_handler_mutex));
    g_handler_should_block = false;
    g_handler_entered = false;
    g_handler_release = false;
    TEST_CHECK_EQ_INT(0, pthread_mutex_unlock(&g_handler_mutex));
    reset_stop_state();

    g_status_auth_result = APP_ERROR_NONE;
    g_status_settings_result = APP_ERROR_NONE;
    g_status_settings = (app_v2_device_settings_t){0};
    g_status_settings.provisioned = true;
    (void)snprintf(g_status_settings.device_name, sizeof(g_status_settings.device_name), "%s",
                   "Desk Macro Keyboard");
    (void)snprintf(g_status_settings.ap_ssid, sizeof(g_status_settings.ap_ssid), "%s",
                   "MacroKeyboard");
    g_status_wifi = (wifi_ap_status_t){.state = WIFI_AP_READY, .client_count = 1U};
    g_status_execution = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    g_status_usb = USB_KEYBOARD_READY;
    g_status_mount = (storage_mount_state_t){.web_mounted = true, .data_mounted = true};
    g_status_capacity_result = APP_ERROR_NONE;
    g_status_total_bytes = 524288U;
    g_status_used_bytes = 4096U;
    g_status_blob_result = APP_ERROR_NONE;
    g_status_blob_count = 3U;

    http_health_reset();
}

static httpd_req_t request_fixture(void) {
    httpd_req_t request = {0};
    request.method = HTTP_POST;
    memcpy(request.uri, "/api/v1/device/restart", sizeof("/api/v1/device/restart"));
    return request;
}

static void assert_async_health(http_async_failure_stage_t stage, app_error_code_t error) {
    const http_health_t health = http_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_EQ_INT(stage, health.async_failure_stage);
    TEST_CHECK_APP_ERROR(error, health.async_error);
}

static void test_handler_failure_still_completes_request(void) {
    reset_fakes();
    g_handler_result = ESP_FAIL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_start());
    httpd_req_t request = request_fixture();

    TEST_CHECK_EQ_INT(ESP_OK, web_server_async_dispatch(&request));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_stop());
    fake_freertos_wait_for_idle();

    TEST_CHECK_EQ_U64(1U, (uint64_t)g_handler_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_completion_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
    assert_async_health(HTTP_ASYNC_FAILURE_WORKER_RUN, APP_ERROR_IO);
}

static void test_completion_failure_is_observed_and_stop_remains_safe(void) {
    reset_fakes();
    g_completion_result = ESP_FAIL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_start());
    httpd_req_t request = request_fixture();

    TEST_CHECK_EQ_INT(ESP_OK, web_server_async_dispatch(&request));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_stop());
    fake_freertos_wait_for_idle();

    TEST_CHECK_EQ_U64(1U, (uint64_t)g_handler_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_completion_calls);
    assert_async_health(HTTP_ASYNC_FAILURE_COMPLETION, APP_ERROR_IO);
}

static void test_queue_failure_completes_clone_and_releases_in_flight(void) {
    reset_fakes();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_start());
    httpd_req_t first = request_fixture();
    fake_freertos_fail_next_queue_send();

    TEST_CHECK_EQ_INT(ESP_OK, web_server_async_dispatch(&first));
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_begin_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_status_send_calls);
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_SERVICE_UNAVAILABLE, (uint64_t)g_last_status);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, g_last_status_code);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_completion_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_handler_calls);
    assert_async_health(HTTP_ASYNC_FAILURE_QUEUE, APP_ERROR_INTERNAL);

    /* The failed queue attempt must relinquish in-flight ownership.
     * A second request can enter the real worker instead of being
     * rejected as a phantom pending confirmation. */
    httpd_req_t second = request_fixture();
    TEST_CHECK_EQ_INT(ESP_OK, web_server_async_dispatch(&second));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_stop());
    fake_freertos_wait_for_idle();
    TEST_CHECK_EQ_U64(2U, (uint64_t)g_begin_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_handler_calls);
    TEST_CHECK_EQ_U64(2U, (uint64_t)g_completion_calls);
}

static void test_stop_signal_failure_is_returned_and_retryable(void) {
    reset_fakes();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_start());
    fake_freertos_fail_next_queue_send();

    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, web_server_async_stop());
    assert_async_health(HTTP_ASYNC_FAILURE_WORKER_STOP, APP_ERROR_TIMEOUT);

    /* Stop failure must not destroy queue/semaphore/task ownership.
     * Retrying the stop sends a fresh sentinel and completes cleanup. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_stop());
    fake_freertos_wait_for_idle();
}

static void test_stop_while_confirmation_pending_waits_for_request_cleanup(void) {
    reset_fakes();
    handler_block_enable();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_start());
    httpd_req_t request = request_fixture();

    TEST_CHECK_EQ_INT(ESP_OK, web_server_async_dispatch(&request));
    wait_for_handler_entry();

    pthread_t stop_thread;
    TEST_CHECK_EQ_INT(0, pthread_create(&stop_thread, NULL, stop_thread_entry, NULL));
    wait_for_stop_start();

    /* The worker still owns an incomplete async request. Stop must wait for it
     * instead of abandoning the request/socket or destroying worker state. */
    TEST_CHECK(!stop_finished_snapshot());
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_completion_calls);

    const int64_t release_at_ms = monotonic_milliseconds();
    release_blocked_handler();
    TEST_CHECK_EQ_INT(0, pthread_join(stop_thread, NULL));
    const int64_t stop_after_release_ms = monotonic_milliseconds() - release_at_ms;

    TEST_CHECK(stop_after_release_ms >= 0);
    TEST_CHECK(stop_after_release_ms < 1000);
    TEST_CHECK(stop_finished_snapshot());
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, g_stop_result);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_handler_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_completion_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
}

static void test_status_remains_responsive_while_confirmation_waits(void) {
    reset_fakes();
    handler_block_enable();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_start());
    httpd_req_t confirmation_request = request_fixture();

    TEST_CHECK_EQ_INT(ESP_OK, web_server_async_dispatch(&confirmation_request));
    wait_for_handler_entry();
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_completion_calls);

    fake_httpd_request_t fake_status;
    fake_httpd_reset(&fake_status);
    authenticate_status(&fake_status);
    httpd_req_t status_request;
    fake_httpd_bind(&status_request, &fake_status, "/api/v1/status", 0U);
    fake_httpd_set_method(&status_request, HTTP_GET);

    const int64_t status_start_ms = monotonic_milliseconds();
    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&status_request));
    const int64_t status_elapsed_ms = monotonic_milliseconds() - status_start_ms;

    /* A regression to synchronous confirmation would consume the sole httpd
     * task for the physical-confirmation timeout. The real status handler must
     * instead finish independently while the worker remains blocked. */
    TEST_CHECK(status_elapsed_ms >= 0);
    TEST_CHECK(status_elapsed_ms < 1000);
    TEST_CHECK_EQ_STRING("200 OK", fake_status.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_completion_calls);

    release_blocked_handler();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_async_stop());
    fake_freertos_wait_for_idle();
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_handler_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_completion_calls);
}

int main(void) {
    test_handler_failure_still_completes_request();
    test_completion_failure_is_observed_and_stop_remains_safe();
    test_queue_failure_completes_clone_and_releases_in_flight();
    test_stop_signal_failure_is_returned_and_retryable();
    test_stop_while_confirmation_pending_waits_for_request_cleanup();
    test_status_remains_responsive_while_confirmation_waits();
    puts("web server async result/cleanup/responsiveness tests passed");
    return 0;
}
