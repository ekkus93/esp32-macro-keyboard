/* Fail-closed HTTP regression for confirmation-required routes.
 *
 * This target links the real web_server_async.c but deliberately never starts
 * its FreeRTOS worker. A confirmation-gated request must therefore fail fast
 * with 503 Service Unavailable. It must not call
 * device_controls_wait_for_confirmation(), execute the protected operation, or
 * restart the device on the httpd task. This is the regression for post-v2
 * hardening H6-060.
 *
 * The actual FreeRTOS queue/task worker path remains outside this host target;
 * the stubbed queue/task/async-httpd functions below are hard-failure canaries
 * so this test cannot silently approximate concurrency semantics it does not
 * faithfully implement. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"
#include "device_controls.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_system.h"
#include "fake_httpd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "http_health.h"
#include "test_assert.h"
#include "web_api_response.h"
#include "web_diagnostics.h"
#include "web_server.h"
#include "web_server_internal.h"

#define CONFIRM_TEST_SESSION_TOKEN                                                                 \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

/* ---------------------------------------------------------------------- *
 * Test doubles for auth.c's, device_settings.c's, and device_controls.c's
 * public entry points -- none host-linkable (mbedtls/NVS/GPIO/FreeRTOS), the
 * same narrow-substitution technique test_web_server_administration_route.c
 * already uses for this exact set.
 * ---------------------------------------------------------------------- */

static app_error_code_t g_auth_session_validate_result;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_auth_session_validate_result;
}

/* Never exercised by any test below: no test here requests
 * GET /api/v1/auth/session. Present only so the linker resolves
 * handle_session()'s (web_api_administration.c) reference to it -- the same
 * never-exercised-stub technique this file already uses for
 * web_diagnostics_handle(). */
app_error_code_t auth_session_remaining(const char *session_token,
                                        uint32_t *out_idle_seconds_remaining,
                                        uint32_t *out_absolute_seconds_remaining) {
    (void)session_token;
    (void)out_idle_seconds_remaining;
    (void)out_absolute_seconds_remaining;
    TEST_CHECK(false);
    return APP_ERROR_INTERNAL;
}

static app_error_code_t g_password_verify_result;
static bool g_password_matches;

app_error_code_t auth_password_verify(const char *password, size_t password_length,
                                      const auth_password_record_t *record, bool *out_matches) {
    (void)password;
    (void)password_length;
    (void)record;
    if (g_password_verify_result != APP_ERROR_NONE) {
        return g_password_verify_result;
    }
    *out_matches = g_password_matches;
    return APP_ERROR_NONE;
}

app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record) {
    (void)password;
    (void)password_length;
    memset(out_record, 0, sizeof(*out_record));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    /* Non-zero: app_v2_password_change_prepare_candidate()'s
     * app_v2_device_settings_validate() call (settings_contract_v2.c)
     * rejects an all-zero salt/verifier the same way the real
     * auth_password_create() never would -- see
     * test_web_server_administration_route.c's identical fake for the same
     * reason. */
    memset(out_record->salt, 0x77, sizeof(out_record->salt));
    memset(out_record->hash, 0x88, sizeof(out_record->hash));
    return APP_ERROR_NONE;
}

static size_t g_logout_all_calls;

app_error_code_t auth_session_logout_all(void) {
    ++g_logout_all_calls;
    return APP_ERROR_NONE;
}

static app_v2_device_settings_t g_device_settings_record;

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    *out_settings = g_device_settings_record;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    g_device_settings_record = *settings;
    *out_changed = true;
    return APP_ERROR_NONE;
}

static size_t g_restart_calls;

app_error_code_t device_controls_restart(void) {
    ++g_restart_calls;
    return APP_ERROR_NONE;
}

static size_t g_reset_settings_calls;

device_controls_reset_settings_outcome_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = true,
        .restart_owned = true,
        .primary_error = APP_ERROR_NONE,
        .restart_error = APP_ERROR_NONE,
    };
}

static size_t g_factory_reset_calls;

device_controls_factory_reset_outcome_t device_controls_factory_reset(void) {
    ++g_factory_reset_calls;
    return (device_controls_factory_reset_outcome_t){
        .durably_accepted = true,
        .recovery_required = false,
        .primary_error = APP_ERROR_NONE,
    };
}

/* The one fake this file adds that test_web_server_administration_route.c
 * does not need working: there, this is an unconditional TEST_CHECK(false)
 * canary because require_physical_confirmation=false never reaches it. Here
 * it is the actual subject under test -- policy_confirm()
 * (web_server_api.c) calls this with APP_PHYSICAL_CONFIRM_TIMEOUT_MS, and
 * its result (APP_ERROR_NONE vs. a failure code) is what
 * enforce_physical_confirmation() (web_request_policy.c) turns into
 * "proceed to the handler" vs. "403 Forbidden before the handler ever
 * runs." */
static app_error_code_t g_wait_for_confirmation_result;
static size_t g_wait_for_confirmation_calls;
static unsigned int g_wait_for_confirmation_last_timeout_ms;

app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms) {
    ++g_wait_for_confirmation_calls;
    g_wait_for_confirmation_last_timeout_ms = timeout_ms;
    return g_wait_for_confirmation_result;
}

static size_t g_esp_restart_calls;

void esp_restart(void) {
    ++g_esp_restart_calls;
}

/* Never exercised by any test below: WEB_API_ROUTE_DIAGNOSTICS_FULL is not a
 * confirmation-required route, and no test here requests it. Present only so
 * the linker resolves web_api_handle_administration()'s switch statement --
 * the same never-exercised-stub technique test_web_api_administration.c
 * uses for this exact symbol. */
app_error_code_t web_diagnostics_handle(web_api_response_t *response) {
    TEST_CHECK(false);
    return web_api_response_success(response, 200U, "{\"stub\":true}");
}

static size_t g_async_health_record_calls;
static http_async_failure_stage_t g_async_health_stage;
static app_error_code_t g_async_health_error;

void http_health_record_async_failure(http_async_failure_stage_t stage, app_error_code_t error) {
    ++g_async_health_record_calls;
    if (g_async_health_stage == HTTP_ASYNC_FAILURE_NONE) {
        g_async_health_stage = stage;
        g_async_health_error = error;
    }
}

/* ---------------------------------------------------------------------- *
 * Dead-path FreeRTOS/httpd-async canaries. The worker is intentionally
 * unavailable in this host regression, and H6-060 requires dispatch to
 * return 503 before any queue/task/async-httpd primitive is reached. Each
 * definition below therefore fails loudly if that invariant changes.
 * ---------------------------------------------------------------------- */

QueueHandle_t xQueueCreate(UBaseType_t queue_length, UBaseType_t item_size) {
    (void)queue_length;
    (void)item_size;
    TEST_CHECK(false);
    return NULL;
}

BaseType_t xQueueSend(QueueHandle_t queue, const void *item, TickType_t ticks_to_wait) {
    (void)queue;
    (void)item;
    (void)ticks_to_wait;
    TEST_CHECK(false);
    return pdFAIL;
}

BaseType_t xQueueReceive(QueueHandle_t queue, void *out_item, TickType_t ticks_to_wait) {
    (void)queue;
    (void)out_item;
    (void)ticks_to_wait;
    TEST_CHECK(false);
    return pdFAIL;
}

void vQueueDelete(QueueHandle_t queue) {
    (void)queue;
    TEST_CHECK(false);
}

SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    TEST_CHECK(false);
    return NULL;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t semaphore) {
    (void)semaphore;
    TEST_CHECK(false);
    return pdFAIL;
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t semaphore, TickType_t ticks_to_wait) {
    (void)semaphore;
    (void)ticks_to_wait;
    TEST_CHECK(false);
    return pdFAIL;
}

void vSemaphoreDelete(SemaphoreHandle_t semaphore) {
    (void)semaphore;
    TEST_CHECK(false);
}

BaseType_t xTaskCreate(TaskFunction_t task_code, const char *name, uint32_t stack_depth,
                       void *parameters, UBaseType_t priority, TaskHandle_t *out_handle) {
    (void)task_code;
    (void)name;
    (void)stack_depth;
    (void)parameters;
    (void)priority;
    (void)out_handle;
    TEST_CHECK(false);
    return pdFAIL;
}

void vTaskDelete(TaskHandle_t task) {
    (void)task;
    TEST_CHECK(false);
}

void vPortEnterCritical(portMUX_TYPE *mux) {
    (void)mux;
    TEST_CHECK(false);
}

void vPortExitCritical(portMUX_TYPE *mux) {
    (void)mux;
    TEST_CHECK(false);
}

esp_err_t httpd_req_async_handler_begin(httpd_req_t *request, httpd_req_t **out_request) {
    (void)request;
    (void)out_request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t httpd_req_async_handler_complete(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

/* ---------------------------------------------------------------------- */

static app_v2_device_settings_t provisioned_settings(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.provisioned = true;
    settings.credential_version = APP_V2_CREDENTIAL_VERSION;
    settings.password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings.password_iterations = 120000U;
    memset(settings.password_salt, 0x11, sizeof(settings.password_salt));
    memset(settings.password_verifier, 0x22, sizeof(settings.password_verifier));
    memcpy(settings.device_name, "Desk Macro Keyboard", sizeof("Desk Macro Keyboard"));
    memcpy(settings.ap_ssid, "MacroKeyboard", sizeof("MacroKeyboard"));
    memcpy(settings.ap_passphrase, "example-passphrase", sizeof("example-passphrase"));
    return settings;
}

static void reset_fakes(void) {
    g_auth_session_validate_result = APP_ERROR_NONE;
    g_password_verify_result = APP_ERROR_NONE;
    g_password_matches = true;
    g_logout_all_calls = 0U;
    g_device_settings_record = provisioned_settings();
    g_restart_calls = 0U;
    g_reset_settings_calls = 0U;
    g_factory_reset_calls = 0U;
    g_wait_for_confirmation_result = APP_ERROR_NONE;
    g_wait_for_confirmation_calls = 0U;
    g_wait_for_confirmation_last_timeout_ms = 0U;
    g_esp_restart_calls = 0U;
    g_async_health_record_calls = 0U;
    g_async_health_stage = HTTP_ASYNC_FAILURE_NONE;
    g_async_health_error = APP_ERROR_NONE;

    /* Confirmation is required globally, while the FreeRTOS worker is
     * intentionally never started. Confirmation-gated routes must fail closed
     * before policy confirmation or handler execution. */
    server_configuration = (web_server_config_t){0};
    server_configuration.require_physical_confirmation = true;
}

static void authenticate(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" CONFIRM_TEST_SESSION_TOKEN);
}

static cJSON *parse_response(const fake_httpd_request_t *fake) {
    cJSON *root = cJSON_ParseWithLength(fake->response_body, fake->response_body_length);
    TEST_CHECK(root != NULL);
    return root;
}

static void bind_bodyless(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                          httpd_method_t method) {
    fake_httpd_bind(request, fake, uri, 0U);
    fake_httpd_set_method(request, method);
}

static void bind_json_body(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                           httpd_method_t method, const char *body) {
    fake_httpd_add_request_header(fake, "Content-Type", "application/json");
    const size_t length = strlen(body);
    fake_httpd_set_body(fake, body, length, 0U);
    fake_httpd_bind(request, fake, uri, length);
    fake_httpd_set_method(request, method);
}

/* -------------------------------------------------------------------------
 * Worker-unavailable fail-closed behavior.
 * ---------------------------------------------------------------------- */

static void assert_confirmation_service_unavailable(const fake_httpd_request_t *fake) {
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake->response_status);
    cJSON *root = parse_response(fake);
    cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK(cJSON_IsObject(error));
    cJSON *code = cJSON_GetObjectItemCaseSensitive(error, "code");
    cJSON *message = cJSON_GetObjectItemCaseSensitive(error, "message");
    TEST_CHECK(cJSON_IsString(code));
    TEST_CHECK(cJSON_IsString(message));
    TEST_CHECK_EQ_STRING("internal", code->valuestring);
    TEST_CHECK_EQ_STRING("confirmation service unavailable", message->valuestring);
    cJSON_Delete(root);
}

static void assert_worker_unavailable_health(void) {
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_async_health_record_calls);
    TEST_CHECK_EQ_INT(HTTP_ASYNC_FAILURE_WORKER_START, g_async_health_stage);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, g_async_health_error);
}

static void test_restart_worker_unavailable_fails_closed(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    assert_confirmation_service_unavailable(&fake);
    assert_worker_unavailable_health();
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

static void test_change_password_worker_unavailable_fails_closed(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    assert_confirmation_service_unavailable(&fake);
    assert_worker_unavailable_health();
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_logout_all_calls);
    TEST_CHECK(fake_httpd_response_header(&fake, "Set-Cookie") == NULL);
}

static void test_reset_settings_worker_unavailable_fails_closed(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/reset-settings", HTTP_POST,
                   "{\"confirmation\":\"RESET SETTINGS\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    assert_confirmation_service_unavailable(&fake);
    assert_worker_unavailable_health();
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_reset_settings_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

static void test_factory_reset_worker_unavailable_fails_closed(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    assert_confirmation_service_unavailable(&fake);
    assert_worker_unavailable_health();
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_factory_reset_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

static void test_settings_get_unaffected_by_worker_unavailability(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/settings", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_async_health_record_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
}

int main(void) {
    test_restart_worker_unavailable_fails_closed();
    test_change_password_worker_unavailable_fails_closed();
    test_reset_settings_worker_unavailable_fails_closed();
    test_factory_reset_worker_unavailable_fails_closed();
    test_settings_get_unaffected_by_worker_unavailability();

    puts("web server async confirmation fail-closed tests passed");
    return 0;
}
