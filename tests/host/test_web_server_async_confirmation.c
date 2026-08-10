/* Live end-to-end HTTP test for the physical-confirmation-required=true path
 * (TODO_V2 V2-057 / Phase 5 exit gate's last open contract/security-test
 * gap): POST /api/v1/device/restart, /device/reset-settings,
 * /device/factory-reset, and /settings/change-password with
 * server_configuration.require_physical_confirmation = true.
 *
 * WHAT THIS DOES AND DOES NOT PROVE -- read before trusting this file's
 * coverage claims:
 *
 * api_handler() (web_server_api.c) routes a confirmation-required route down
 * one of two branches once web_api_request_requires_worker() answers true:
 *
 *   1. web_server_async_dispatch() (web_server_async.c) queues the request
 *      onto a FreeRTOS worker task and returns immediately, freeing the
 *      httpd task; a second thread later dequeues it, waits on
 *      device_controls_wait_for_confirmation() on ITS OWN stack, and
 *      completes the (by-then-async) request.
 *   2. If the worker was never started (async_queue/async_task_handle are
 *      still NULL -- web_server_async_dispatch()'s own "worker unavailable:
 *      answer on the httpd task rather than drop the request" branch, its
 *      documented, production fallback, not a test-only shortcut), the SAME
 *      function instead calls web_api_handle_call() synchronously, on the
 *      calling thread, and returns its result directly.
 *
 * This file exercises branch 2 only. It links the real web_server_async.c
 * (not a hand-rolled stub of web_server_async_dispatch(), unlike
 * test_web_server_administration_route.c, which never reaches
 * confirmation-required=true at all and stubs the function out entirely),
 * but every test below deliberately never calls web_server_async_start().
 * That keeps async_queue/async_task_handle at their zero-initialized NULL
 * state for the whole process, so every call here takes branch 2 -- for
 * real, in the actual compiled web_server_async_dispatch(), not a
 * reimplementation of its logic. This proves, against the live
 * api_handler() pipeline: that confirmation-required routes are correctly
 * classified and routed into web_server_async_dispatch() at all (never
 * proven anywhere else -- test_web_server_administration_route.c
 * specifically avoids this by setting require_physical_confirmation=false),
 * that device_controls_wait_for_confirmation() is invoked with the correct
 * timeout, that a successful confirmation reaches the exact same handler and
 * produces the exact same response the confirmation-disabled tests in
 * test_web_server_administration_route.c already verify, that a failed/
 * timed-out confirmation is correctly rejected with 403 Forbidden before the
 * handler ever runs, and that a route NOT requiring confirmation is
 * unaffected by the flag even when it is globally enabled.
 *
 * It does NOT prove branch 1 works: the actual FreeRTOS queue/task worker
 * machinery (web_server_async_start()/_stop(), async_worker(), the
 * xQueueSend()/xQueueReceive() handoff, claim_in_flight()/
 * release_in_flight()'s mutual exclusion) is not exercised here at all, and
 * remains genuinely untested at the host level -- see
 * fakes/freertos_stub/freertos/FreeRTOS.h's header comment and
 * docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md for exactly
 * why that gap is not closed by this file and is not believed closable
 * without a substantially larger, riskier faithful FreeRTOS host mock. */

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

app_error_code_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return APP_ERROR_NONE;
}

static size_t g_factory_reset_calls;

app_error_code_t device_controls_factory_reset(void) {
    ++g_factory_reset_calls;
    return APP_ERROR_NONE;
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

/* ---------------------------------------------------------------------- *
 * Dead-path FreeRTOS/httpd-async canaries. Every test below deliberately
 * never calls web_server_async_start(), so web_server_async.c's static
 * async_queue/async_task_handle stay NULL for the whole process and
 * web_server_async_dispatch() always takes its synchronous "worker
 * unavailable" fallback branch, which returns before any of these ten
 * symbols is reached -- see this file's own header comment and
 * fakes/freertos_stub/freertos/FreeRTOS.h's header comment for the full
 * argument. Each definition below is a hard-failure canary, not a working
 * implementation: if a future change ever makes one of these dead paths
 * reachable without updating this file, the test fails loudly here instead
 * of silently running against an approximated concurrency primitive.
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

    /* The subject of this whole file: confirmation is required globally, but
     * the FreeRTOS worker is never started (see the file header comment), so
     * every request below takes web_server_async_dispatch()'s synchronous
     * fallback branch. */
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
 * POST /api/v1/device/restart
 * ---------------------------------------------------------------------- */

static void test_restart_post_confirmation_granted(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(APP_PHYSICAL_CONFIRM_TIMEOUT_MS,
                      (uint64_t)g_wait_for_confirmation_last_timeout_ms);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_restart_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_esp_restart_calls);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));
    cJSON_Delete(root);
}

static void test_restart_post_confirmation_timed_out(void) {
    reset_fakes();
    g_wait_for_confirmation_result = APP_ERROR_TIMEOUT;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    /* web_request_policy_http_status(): WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION
     * maps to 403 regardless of the underlying app_error_code_t -- the
     * handler is never reached, so device_controls_restart()/esp_restart()
     * see zero calls. */
    TEST_CHECK_EQ_STRING("403 Forbidden", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);

    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING("timeout", cJSON_GetObjectItemCaseSensitive(
                                        cJSON_GetObjectItemCaseSensitive(root, "error"), "code")
                                        ->valuestring);
    cJSON_Delete(root);
}

static void test_restart_post_unauthorized_before_confirmation(void) {
    /* enforce_session() (web_request_policy.c) runs before
     * enforce_physical_confirmation() -- an unauthenticated caller must never
     * trigger a physical confirmation wait at all. */
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/settings/change-password
 * ---------------------------------------------------------------------- */

static void test_change_password_post_confirmation_granted(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("204 No Content", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_logout_all_calls);
    TEST_CHECK_EQ_STRING("MKSESSION=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0",
                         fake_httpd_response_header(&fake, "Set-Cookie"));
}

static void test_change_password_post_confirmation_denied(void) {
    reset_fakes();
    /* APP_ERROR_CONFLICT: device_controls_wait_for_confirmation()'s other
     * real failure mode (device_controls.c), returned when the controls
     * task is not running or a stop was requested while waiting -- not just
     * a timeout, see device_controls.c's own implementation. */
    g_wait_for_confirmation_result = APP_ERROR_CONFLICT;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("403 Forbidden", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_logout_all_calls);
    TEST_CHECK(fake_httpd_response_header(&fake, "Set-Cookie") == NULL);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/reset-settings
 * ---------------------------------------------------------------------- */

static void test_reset_settings_post_confirmation_granted(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/reset-settings", HTTP_POST,
                   "{\"confirmation\":\"RESET SETTINGS\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_reset_settings_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset
 * ---------------------------------------------------------------------- */

static void test_factory_reset_post_confirmation_granted(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_wait_for_confirmation_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_factory_reset_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_esp_restart_calls);
}

/* -------------------------------------------------------------------------
 * GET /api/v1/settings -- not a confirmation-required route. Proves
 * web_api_physical_confirmation_required()'s per-route allowlist, not just
 * the global require_physical_confirmation flag, is what api_handler()
 * actually consults: device_controls_wait_for_confirmation() must see zero
 * calls even though the flag is on.
 * ---------------------------------------------------------------------- */

static void test_settings_get_unaffected_by_confirmation_flag(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/settings", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_wait_for_confirmation_calls);
}

int main(void) {
    test_restart_post_confirmation_granted();
    test_restart_post_confirmation_timed_out();
    test_restart_post_unauthorized_before_confirmation();

    test_change_password_post_confirmation_granted();
    test_change_password_post_confirmation_denied();

    test_reset_settings_post_confirmation_granted();

    test_factory_reset_post_confirmation_granted();

    test_settings_get_unaffected_by_confirmation_flag();

    puts("web server async confirmation tests passed");
    return 0;
}
