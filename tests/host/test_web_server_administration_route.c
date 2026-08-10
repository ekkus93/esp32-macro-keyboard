/* Live end-to-end HTTP tests for the administration route group (TODO_V2
 * V2-057's remaining items, see
 * docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md's
 * "What remains open" section): GET /api/v1/auth/session,
 * POST /api/v1/device/restart, GET/PUT /api/v1/settings,
 * POST /api/v1/settings/change-password,
 * POST /api/v1/device/reset-settings, POST /api/v1/device/factory-reset,
 * GET/POST /api/v1/setup (provisioned-mode conflict), and
 * GET /api/v1/diagnostics.
 *
 * Unlike test_web_api_administration.c (which calls
 * web_api_handle_administration() directly with a hand-built web_api_call_t)
 * and test_web_request_policy.c (which calls web_request_policy_evaluate()
 * directly), this drives the real api_handler() -- web_server_api.c's httpd
 * URI handler for the generic "/api/v1" wildcard -- against a fake
 * esp_http_server.h (fakes/esp_http_server_stub, fakes/fake_httpd.c), the
 * same technique test_web_server_blob.c/test_web_server_send_route.c use.
 * That proves a request actually flows through
 * method_from_request() -> web_api_parse_path() ->
 * web_request_policy_evaluate() -> web_api_dispatch() ->
 * web_api_handle_administration() -> the exact same pipeline a real device
 * uses, not just the last step of it.
 *
 * server_configuration.require_physical_confirmation is left false for
 * every test below. With it false, web_api_request_requires_worker()
 * (web_server_api.c) returns false for every route in this group (none of
 * them sets web_api_route_requires_worker() true either -- see
 * web_api_core.c), so api_handler() always takes the synchronous
 * web_api_handle_call() path under test rather than
 * web_server_async_dispatch()'s FreeRTOS worker queue, which is not
 * host-linkable. The confirmation-required=true path (which would route
 * restart/reset-settings/factory-reset/change-password through the async
 * worker and device_controls_wait_for_confirmation()) is therefore NOT
 * exercised here -- that is a distinct, FreeRTOS-dependent code path
 * (web_server_async.c) out of this track's scope; policy_confirm()'s
 * decision of *whether* confirmation is required for a route is already
 * covered by test_web_request_policy.c and test_web_api_core.c.
 *
 * diagnostics (GET /api/v1/diagnostics) is now included too: the prior
 * track's report framed the eight subsystem-health snapshot functions
 * web_diagnostics_handle()'s collect_diagnostics() (web_server_diagnostics.c)
 * calls as needing "host stand-ins ... none of which exist yet", but reading
 * each one's own module comment shows six of them
 * (app_lifecycle_health_snapshot(), storage_health_snapshot(),
 * auth_health_snapshot(), usb_health_snapshot(), executor_health_snapshot(),
 * http_health_snapshot()) are already portable C with no ESP-IDF dependency
 * -- they link in real, unfaked, the same way this file's own
 * device_controls_logic.c-backed device_controls_health_derive_state() and
 * wifi_ap_state.c-backed wifi_ap_health_derive_state() do. Only two
 * (device_controls_get_health(), wifi_ap_get_status()) live in
 * ESP-IDF/FreeRTOS-bound files and need fakes, both provided below using the
 * same narrow-substitution technique already used for this file's other
 * device_controls.h entry points. web_diagnostics_handle() itself, and the
 * esp_heap_caps.h/esp_system.h entry points collect_diagnostics() calls
 * (esp_reset_reason(), esp_get_free_heap_size(), esp_get_minimum_free_heap_size(),
 * heap_caps_get_largest_free_block()), are real too -- see
 * fakes/esp_idf_misc_stub/esp_system.h and the new
 * fakes/esp_idf_misc_stub/esp_heap_caps.h for those two headers' stand-ins.
 * Diagnostics' own JSON composition (schema, exact status codes,
 * secret-sentinel absence, bounded-output/corrupt-input handling) keeps its
 * existing deep, dedicated coverage in
 * test_web_server_adapter_diagnostics_json.inc; the tests added here prove
 * only that a live request reaches that composition through the same
 * policy/dispatch pipeline every other route in this file already proves,
 * plus one representative backend-failure mapping. */

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
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "fake_httpd.h"
#include "macro_executor.h"
#include "storage.h"
#include "storage_blob.h"
#include "test_assert.h"
#include "test_examples_fixture.h"
#include "usb_keyboard.h"
#include "web_api_response.h"
#include "web_diagnostics.h"
#include "web_server.h"
#include "web_server_internal.h"
#include "wifi_ap.h"

#define ADMIN_TEST_SESSION_TOKEN                                                                   \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

/* ---------------------------------------------------------------------- *
 * Test doubles for auth.c's, device_settings.c's, and device_controls.c's
 * public entry points -- none host-linkable (mbedtls/NVS/GPIO/FreeRTOS),
 * the same narrow-substitution technique test_web_api_administration.c and
 * test_web_server_send_route.c already use -- plus esp_restart()
 * (esp_system.h has no real host implementation either).
 * ---------------------------------------------------------------------- */

static app_error_code_t g_auth_session_validate_result;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_auth_session_validate_result;
}

static app_error_code_t g_session_remaining_result;
static uint32_t g_idle_seconds;
static uint32_t g_absolute_seconds;

app_error_code_t auth_session_remaining(const char *session_token,
                                        uint32_t *out_idle_seconds_remaining,
                                        uint32_t *out_absolute_seconds_remaining) {
    (void)session_token;
    if (g_session_remaining_result != APP_ERROR_NONE) {
        return g_session_remaining_result;
    }
    *out_idle_seconds_remaining = g_idle_seconds;
    *out_absolute_seconds_remaining = g_absolute_seconds;
    return APP_ERROR_NONE;
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

static app_error_code_t g_password_create_result;

app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record) {
    (void)password;
    (void)password_length;
    if (g_password_create_result != APP_ERROR_NONE) {
        return g_password_create_result;
    }
    memset(out_record, 0, sizeof(*out_record));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    /* Non-zero: app_v2_password_change_prepare_candidate()'s
     * app_v2_device_settings_validate() call (settings_contract_v2.c)
     * rejects an all-zero salt/verifier the same way the real
     * auth_password_create() never would. */
    memset(out_record->salt, 0x77, sizeof(out_record->salt));
    memset(out_record->hash, 0x88, sizeof(out_record->hash));
    return APP_ERROR_NONE;
}

static app_error_code_t g_logout_all_result;
static size_t g_logout_all_calls;

app_error_code_t auth_session_logout_all(void) {
    ++g_logout_all_calls;
    return g_logout_all_result;
}

static app_v2_device_settings_t g_device_settings_record;
static app_error_code_t g_device_settings_read_result;
static app_error_code_t g_device_settings_replace_result;
static bool g_device_settings_replace_changed;

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    if (g_device_settings_read_result != APP_ERROR_NONE) {
        return g_device_settings_read_result;
    }
    *out_settings = g_device_settings_record;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    if (g_device_settings_replace_result != APP_ERROR_NONE) {
        return g_device_settings_replace_result;
    }
    g_device_settings_record = *settings;
    *out_changed = g_device_settings_replace_changed;
    return APP_ERROR_NONE;
}

static app_error_code_t g_restart_result;
static size_t g_restart_calls;

app_error_code_t device_controls_restart(void) {
    ++g_restart_calls;
    return g_restart_result;
}

static app_error_code_t g_reset_settings_result;
static size_t g_reset_settings_calls;

app_error_code_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return g_reset_settings_result;
}

static app_error_code_t g_factory_reset_result;
static size_t g_factory_reset_calls;

app_error_code_t device_controls_factory_reset(void) {
    ++g_factory_reset_calls;
    return g_factory_reset_result;
}

app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms) {
    (void)timeout_ms;
    /* Never reached: server_configuration.require_physical_confirmation is
     * false for every test below -- see the file header comment. */
    TEST_CHECK(false);
    return APP_ERROR_INTERNAL;
}

static size_t g_esp_restart_calls;

void esp_restart(void) {
    ++g_esp_restart_calls;
}

/* ---------------------------------------------------------------------- *
 * Test doubles for GET /api/v1/diagnostics's own dependencies --
 * web_diagnostics_handle() itself (web_server_diagnostics.c) links in real
 * (see the file header comment for why); these are the handful of
 * NVS/FreeRTOS/hardware-backed entry points its collect_diagnostics() calls
 * that are not host-linkable, the same narrow-substitution technique used
 * throughout this file. Matches test_web_server_status_limits_route.c's
 * device_settings_read/wifi_ap_get_status/macro_executor_get_status/
 * usb_keyboard_get_state/storage_mount_state/storage_partition_capacity
 * fakes exactly (status_handler() and collect_diagnostics() read the same
 * subsystems), plus storage_blob_collect_diagnostics()/
 * device_controls_get_health(), which status/limits do not need.
 * ---------------------------------------------------------------------- */

static wifi_ap_status_t g_wifi_status;

wifi_ap_status_t wifi_ap_get_status(void) {
    return g_wifi_status;
}

static macro_execution_status_t g_execution_status;

macro_execution_status_t macro_executor_get_status(void) {
    return g_execution_status;
}

static usb_keyboard_state_t g_usb_state;

usb_keyboard_state_t usb_keyboard_get_state(void) {
    return g_usb_state;
}

static storage_mount_state_t g_mount_state;

storage_mount_state_t storage_mount_state(void) {
    return g_mount_state;
}

static app_error_code_t g_webfs_capacity_result;
static size_t g_webfs_total_bytes;
static size_t g_webfs_used_bytes;
static app_error_code_t g_userdata_capacity_result;
static size_t g_userdata_total_bytes;
static size_t g_userdata_used_bytes;

app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes) {
    if (strcmp(partition_label, STORAGE_WEB_PARTITION) == 0) {
        if (g_webfs_capacity_result != APP_ERROR_NONE) {
            return g_webfs_capacity_result;
        }
        *out_total_bytes = g_webfs_total_bytes;
        *out_used_bytes = g_webfs_used_bytes;
        return APP_ERROR_NONE;
    }
    TEST_CHECK_EQ_STRING(STORAGE_DATA_PARTITION, partition_label);
    if (g_userdata_capacity_result != APP_ERROR_NONE) {
        return g_userdata_capacity_result;
    }
    *out_total_bytes = g_userdata_total_bytes;
    *out_used_bytes = g_userdata_used_bytes;
    return APP_ERROR_NONE;
}

static app_error_code_t g_blob_diagnostics_result;
static size_t g_blob_valid_count;

app_error_code_t storage_blob_collect_diagnostics(storage_blob_diagnostics_t *out_diagnostics) {
    if (g_blob_diagnostics_result != APP_ERROR_NONE) {
        return g_blob_diagnostics_result;
    }
    *out_diagnostics = (storage_blob_diagnostics_t){0};
    out_diagnostics->summary.valid_count = g_blob_valid_count;
    return APP_ERROR_NONE;
}

static device_controls_health_t g_device_controls_health;

device_controls_health_t device_controls_get_health(void) {
    return g_device_controls_health;
}

const esp_app_desc_t *esp_app_get_description(void) {
    static const esp_app_desc_t description = {.version = "0.2.0"};
    return &description;
}

int esp_app_get_elf_sha256(char *dst, size_t size) {
    (void)snprintf(dst, size, "%s", "git-abcdef0");
    return 0;
}

int64_t esp_timer_get_time(void) {
    return 123456000; /* 123456 ms. */
}

static esp_reset_reason_t g_reset_reason;

esp_reset_reason_t esp_reset_reason(void) {
    return g_reset_reason;
}

static uint32_t g_free_heap_bytes;

uint32_t esp_get_free_heap_size(void) {
    return g_free_heap_bytes;
}

static uint32_t g_minimum_free_heap_bytes;

uint32_t esp_get_minimum_free_heap_size(void) {
    return g_minimum_free_heap_bytes;
}

static size_t g_largest_free_block_bytes;

size_t heap_caps_get_largest_free_block(uint32_t caps) {
    TEST_CHECK_EQ_U64((uint64_t)MALLOC_CAP_8BIT, (uint64_t)caps);
    return g_largest_free_block_bytes;
}

esp_err_t web_server_async_dispatch(httpd_req_t *request) {
    (void)request;
    /* Never reached: web_api_request_requires_worker() (web_server_api.c)
     * only returns true when a route requires the FreeRTOS async worker
     * (none currently do -- web_api_route_requires_worker() is
     * unconditionally false) or physical confirmation is required, which
     * server_configuration.require_physical_confirmation=false disables for
     * every test below -- see the file header comment. Present only so the
     * linker resolves api_handler()'s reference to it. */
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
    g_session_remaining_result = APP_ERROR_NONE;
    g_idle_seconds = 86400U;
    g_absolute_seconds = 604800U;
    g_password_verify_result = APP_ERROR_NONE;
    g_password_matches = true;
    g_password_create_result = APP_ERROR_NONE;
    g_logout_all_result = APP_ERROR_NONE;
    g_logout_all_calls = 0U;
    g_device_settings_record = provisioned_settings();
    g_device_settings_read_result = APP_ERROR_NONE;
    g_device_settings_replace_result = APP_ERROR_NONE;
    g_device_settings_replace_changed = true;
    g_restart_result = APP_ERROR_NONE;
    g_restart_calls = 0U;
    g_reset_settings_result = APP_ERROR_NONE;
    g_reset_settings_calls = 0U;
    g_factory_reset_result = APP_ERROR_NONE;
    g_factory_reset_calls = 0U;
    g_esp_restart_calls = 0U;

    g_wifi_status = (wifi_ap_status_t){.state = WIFI_AP_READY, .client_count = 1U};
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    g_usb_state = USB_KEYBOARD_READY;
    g_mount_state = (storage_mount_state_t){.web_mounted = true, .data_mounted = true};
    /* webfsTotalBytes/webfsUsedBytes/userdataTotalBytes/userdataUsedBytes/
     * blobCount below are chosen to equal contracts/v2/api/examples.json's
     * "diagnostics" fixture exactly (not just plausible-looking numbers), so
     * test_diagnostics_get_valid() can compare the real response against
     * that checked-in example directly -- see its own comment. */
    g_webfs_capacity_result = APP_ERROR_NONE;
    g_webfs_total_bytes = 1048576U;
    g_webfs_used_bytes = 500000U;
    g_userdata_capacity_result = APP_ERROR_NONE;
    g_userdata_total_bytes = 524288U;
    g_userdata_used_bytes = 4096U;
    g_blob_diagnostics_result = APP_ERROR_NONE;
    g_blob_valid_count = 3U;
    g_device_controls_health = (device_controls_health_t){.task_running = true};
    g_reset_reason = ESP_RST_POWERON;
    g_free_heap_bytes = 200000U;
    g_minimum_free_heap_bytes = 180000U;
    g_largest_free_block_bytes = 120000U;

    server_configuration = (web_server_config_t){0};
    server_configuration.require_physical_confirmation = false;
}

static void authenticate(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" ADMIN_TEST_SESSION_TOKEN);
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
 * GET /api/v1/auth/session
 * ---------------------------------------------------------------------- */

static void test_session_get_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/auth/session", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    TEST_CHECK(fake_httpd_response_header(&fake, "X-Request-ID") != NULL);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "authenticated")));
    TEST_CHECK_EQ_U64(
        86400U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "idleExpiresInSeconds")->valuedouble);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": reset_fakes()'s g_idle_seconds/g_absolute_seconds
     * already equal contracts/v2/api/examples.json's "session" example
     * exactly, so this diffs the whole live response against it wholesale. */
    const cJSON *example = test_examples_fixture_get("session");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_session_get_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/auth/session", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING(
        "auth_required",
        cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root, "error"), "code")
            ->valuestring);
    cJSON_Delete(root);
}

static void test_session_get_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_session_validate_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/auth/session", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/restart
 * ---------------------------------------------------------------------- */

static void test_restart_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_restart_calls);
    /* api_handler() itself calls esp_restart() after a successful 202
     * restart/factory-reset response is sent -- this is the one behavior
     * only the live api_handler() path (not web_api_handle_administration()
     * called directly) can prove. */
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_esp_restart_calls);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_device_restart_accepted_json() has no backend
     * dependency (every field is a fixed literal), so this diffs the live
     * response against contracts/v2/api/examples.json's "restartAccepted"
     * example wholesale. */
    const cJSON *example = test_examples_fixture_get("restartAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_restart_post_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

/* -------------------------------------------------------------------------
 * GET/PUT /api/v1/settings
 * ---------------------------------------------------------------------- */

static void test_settings_get_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/settings", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard",
                         cJSON_GetObjectItemCaseSensitive(root, "deviceName")->valuestring);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": provisioned_settings() above already produces every
     * field contracts/v2/api/examples.json's "settings" example claims
     * (app_v2_device_settings_init_unprovisioned()'s own defaults already
     * match sendMode/snapshotRetentionTarget/showMacroSourcePreviews/
     * requireSerialConfirmation/lastSelectedPackageId, and
     * provisioned_settings() overrides deviceName/apSsid to the same
     * strings), so this diffs the whole live response against it wholesale. */
    const cJSON *example = test_examples_fixture_get("settings");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_settings_put_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings", HTTP_PUT,
                   "{\"deviceName\":\"New Device Name\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("New Device Name", g_device_settings_record.device_name);
    cJSON *root = parse_response(&fake);
    const cJSON *settings = cJSON_GetObjectItemCaseSensitive(root, "settings");
    TEST_CHECK(settings != NULL);
    TEST_CHECK_EQ_STRING("New Device Name",
                         cJSON_GetObjectItemCaseSensitive(settings, "deviceName")->valuestring);
    cJSON_Delete(root);
}

/* TODO_V2 V2-057 "consume the same checked-in examples from C and TypeScript
 * tests": submits contracts/v2/api/examples.json's own "settingsUpdate" body
 * verbatim against the "settings" example's baseline (provisioned_settings()
 * above already matches it, see test_settings_get_valid()), so the response
 * can be diffed against "settingsUpdated" wholesale, including
 * "restartRequired"/"reconnectRequired".
 *
 * Both examples.json's "settingsUpdated" AND docs/SPEC_V2.md 13.9's own
 * inline JSON for this exact request previously showed both flags false,
 * which contradicted the very next sentence in SPEC_V2 13.9 ("Changing
 * access-point credentials sets both flags to true") and
 * apply_access_point() (settings_contract_v2.c), which does exactly that
 * unconditionally whenever a request carries "accessPoint" -- which this
 * request does. Confirmed as a stale example (not a code defect), reported
 * to and fixed by Phil per this project's frozen-spec discipline -- see
 * docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md for the
 * original discrepancy writeup. examples.json and SPEC_V2.md now both read
 * true/true, matching this test's separate exact assertion below. */
static void test_settings_put_valid_matches_example(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings", HTTP_PUT,
                   "{\"deviceName\":\"Desk Macro Keyboard\",\"requireSerialConfirmation\":false,"
                   "\"sendMode\":\"quick\",\"snapshotRetentionTarget\":5,"
                   "\"showMacroSourcePreviews\":false,\"lastSelectedPackageId\":null,"
                   "\"accessPoint\":{\"ssid\":\"MacroKeyboard\",\"passphrase\":\"new-example-"
                   "passphrase\"},\"station\":{\"ssid\":\"OfficeWiFi\",\"passphrase\":\"station-"
                   "example-passphrase\"}}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);

    cJSON *root = parse_response(&fake);
    /* The real, prose-conformant values for this exact request -- see this
     * test's own comment. */
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "restartRequired")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "reconnectRequired")));

    const cJSON *example = test_examples_fixture_get("settingsUpdated");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_settings_put_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_session_validate_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings", HTTP_PUT,
                   "{\"deviceName\":\"New Device Name\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    /* The policy gate ran (and rejected) before dispatch reached
     * handle_settings_put(), so the backend was never touched. */
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard", g_device_settings_record.device_name);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/settings/change-password
 * ---------------------------------------------------------------------- */

static void test_change_password_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    /* This is exactly the case that caught web_server_api.c's status_text()
     * bug: WEB_HTTP_STATUS_NO_CONTENT (204) had no case in that switch, so
     * a *successful* change-password response was previously sent to the
     * client as "500 Internal Server Error" even though the password was
     * already changed and every session already invalidated -- a defect no
     * existing test (all of which call web_api_handle_administration()
     * directly and only ever inspect response.status, not the wire status
     * line send_api_response()/status_text() actually produce) could catch.
     * Fixed in the same commit as this test, see web_server_api.c. */
    TEST_CHECK_EQ_STRING("204 No Content", fake.response_status);
    TEST_CHECK_EQ_U64(0U, fake.response_body_length);
    TEST_CHECK_EQ_STRING("MKSESSION=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0",
                         fake_httpd_response_header(&fake, "Set-Cookie"));
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_logout_all_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

static void test_change_password_post_incorrect_current_password(void) {
    reset_fakes();
    g_password_matches = false;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"wrong-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("403 Forbidden", fake.response_status);
    TEST_CHECK(fake_httpd_response_header(&fake, "Set-Cookie") == NULL);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/reset-settings
 * ---------------------------------------------------------------------- */

static void test_reset_settings_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/reset-settings", HTTP_POST,
                   "{\"confirmation\":\"RESET SETTINGS\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_reset_settings_calls);
    /* DEVICE_RESET_SETTINGS is not in restart_after_response()'s route set
     * (only DEVICE_RESTART/DEVICE_FACTORY_RESET are) -- device_controls
     * itself performs the reboot, web_server_api.c does not call
     * esp_restart() a second time. */
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_device_reset_accepted_json(false, true, ...)'s
     * two bool arguments (web_api_administration.c's call site) have no
     * backend dependency (both are fixed literals for this route), so this
     * diffs the live response against contracts/v2/api/examples.json's
     * "resetSettingsAccepted" example wholesale. */
    cJSON *root = parse_response(&fake);
    const cJSON *example = test_examples_fixture_get("resetSettingsAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_reset_settings_post_wrong_confirmation(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/reset-settings", HTTP_POST,
                   "{\"confirmation\":\"wrong\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    /* APP_ERROR_INVALID_ARGUMENT maps to 422 (web_api_http_status_for_error()),
     * not 400 -- SPEC_V2 13.2's validation-failure status. */
    TEST_CHECK_EQ_STRING("422 Unprocessable Entity", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_reset_settings_calls);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset
 * ---------------------------------------------------------------------- */

static void test_factory_reset_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_factory_reset_calls);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_esp_restart_calls);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_device_reset_accepted_json(true, false, ...)'s
     * call site for this route (web_api_administration.c) also passes two
     * fixed literals, so this diffs the live response against
     * contracts/v2/api/examples.json's "factoryResetAccepted" example
     * wholesale. */
    cJSON *root = parse_response(&fake);
    const cJSON *example = test_examples_fixture_get("factoryResetAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_factory_reset_post_incorrect_password(void) {
    reset_fakes();
    g_password_matches = false;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"wrong-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("403 Forbidden", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_factory_reset_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

/* -------------------------------------------------------------------------
 * GET/POST /api/v1/setup (provisioned-mode conflict; SPEC_V2 13.4). No
 * session is required (WEB_API_ROUTE_SETUP is exempted by
 * web_api_route_requires_session()), so these two need no Cookie header --
 * the live pipeline still reaches setup_route_response() the same way an
 * unauthenticated caller's real request would.
 * ---------------------------------------------------------------------- */

static void test_setup_get_not_found_when_provisioned(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/setup", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
}

static void test_setup_post_conflict_when_provisioned(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    /* web_api_route_requires_body(WEB_API_ROUTE_SETUP, POST) is true (so an
     * already-provisioned device's real setup submission body is accepted
     * up through the policy gate rather than rejected with 422 before it
     * can reach the 409 conflict response setup_route_response() answers --
     * see web_api_core.c's comment on that route) -- so this needs a real
     * JSON body/Content-Type, unlike the GET case above. */
    bind_json_body(&request, &fake, "/api/v1/setup", HTTP_POST, "{}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("409 Conflict", fake.response_status);
}

/* -------------------------------------------------------------------------
 * GET /api/v1/diagnostics
 * ---------------------------------------------------------------------- */

static void test_diagnostics_get_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);

    cJSON *root = parse_response(&fake);
    /* reset_fakes() above chose every backend value (heap sizes, reset
     * reason, webfs/userdata capacity, blob count, USB/Wi-Fi/send state) to
     * equal contracts/v2/api/examples.json's own "diagnostics" fixture
     * exactly -- the same file webapp/tests/v2-api-contracts.test.ts
     * validates isDiagnosticsResponse(examples.diagnostics) against
     * (TODO_V2 V2-057's "consume the same checked-in examples from C and
     * TypeScript tests" bullet). The one field that cannot match literally
     * is "subsystems": the example leaves it "[]" as a documentation
     * placeholder, but a real device always reports all
     * WEB_DIAGNOSTICS_SUBSYSTEM_COUNT (8) entries (asserted separately
     * below) -- so this compares a copy of the response with "subsystems"
     * normalized to an empty array against the example, deep and
     * order-independent, rather than skip the comparison. */
    cJSON *comparable = cJSON_Duplicate(root, true);
    TEST_CHECK(comparable != NULL);
    TEST_CHECK(
        cJSON_ReplaceItemInObjectCaseSensitive(comparable, "subsystems", cJSON_CreateArray()));
    const cJSON *example = test_examples_fixture_get("diagnostics");
    TEST_CHECK(cJSON_Compare(comparable, example, true) != 0);
    cJSON_Delete(comparable);

    const cJSON *subsystems = cJSON_GetObjectItemCaseSensitive(root, "subsystems");
    TEST_CHECK_EQ_INT(8, cJSON_GetArraySize(subsystems));
    /* device_controls_get_health() above reports task_running=true and no
     * errors, so device_controls_health_derive_state() (the real,
     * unfaked device_controls_logic.c function) derives SUBSYSTEM_HEALTH_HEALTHY
     * -- proving the fake-to-pure-logic-to-JSON chain, not just that some
     * string came out. */
    const cJSON *controls_entry = NULL;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, subsystems) {
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (strcmp(name->valuestring, "controls") == 0) {
            controls_entry = entry;
        }
    }
    TEST_CHECK(controls_entry != NULL);
    TEST_CHECK_EQ_STRING("healthy",
                         cJSON_GetObjectItemCaseSensitive(controls_entry, "state")->valuestring);
    cJSON_Delete(root);
}

static void test_diagnostics_get_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_diagnostics_get_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_session_validate_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_diagnostics_get_blob_scan_failure_maps_to_503(void) {
    reset_fakes();
    g_blob_diagnostics_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

int main(void) {
    test_session_get_valid();
    test_session_get_unauthorized_without_cookie();
    test_session_get_unauthorized_expired_session();

    test_restart_post_valid();
    test_restart_post_unauthorized_without_cookie();

    test_settings_get_valid();
    test_settings_put_valid();
    test_settings_put_valid_matches_example();
    test_settings_put_unauthorized_expired_session();

    test_change_password_post_valid();
    test_change_password_post_incorrect_current_password();

    test_reset_settings_post_valid();
    test_reset_settings_post_wrong_confirmation();

    test_factory_reset_post_valid();
    test_factory_reset_post_incorrect_password();

    test_setup_get_not_found_when_provisioned();
    test_setup_post_conflict_when_provisioned();

    test_diagnostics_get_valid();
    test_diagnostics_get_unauthorized_without_cookie();
    test_diagnostics_get_unauthorized_expired_session();
    test_diagnostics_get_blob_scan_failure_maps_to_503();

    puts("web server administration route tests passed");
    return 0;
}
