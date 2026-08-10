/* HTTP-adapter-level tests for GET /api/v1/status and GET /api/v1/limits
 * (firmware/components/web_server/web_server_status_limits.c; SPEC_V2 13.6/
 * 13.5, TODO_V2 V2-057). Unlike test_web_status_limits.c (which only exercises
 * the pure web_status_response_json() JSON builder), this drives the real
 * status_handler()/limits_handler() httpd_req_t-taking functions against a
 * fake esp_http_server.h (fakes/esp_http_server_stub, fakes/fake_httpd.c) --
 * the same technique test_web_server_blob.c uses -- so it also covers the
 * auth gate (authorize_mutation) and the subsystem snapshot composition
 * (device_settings/wifi_ap/macro_executor/usb_keyboard/storage/storage_blob)
 * that the pure JSON-builder test cannot reach.
 *
 * Both routes are bodyless, fixed-URI, single-method GET handlers registered
 * directly ahead of the generic "/api/v1/" wildcard route (see
 * web_server_common.c's route table) -- they never go through
 * web_api_parse_path()/web_api_route_allows_method(), so "malformed-path" and
 * "method-error" for these two routes are covered instead at the pure-
 * function level in test_web_api_core.c (WEB_API_ROUTE_STATUS/LIMITS cases in
 * test_method_and_body_policy()/test_active_routes()) -- that is the code
 * path that actually answers a wrong-method or malformed-path request for
 * these routes in production (the wildcard fallback into api_handler).
 * "wrong-type"/"wrong-content-type"/"oversized"/"extra"/"missing" body
 * categories do not apply: neither route reads a request body. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_app_desc.h"
#include "esp_timer.h"
#include "fake_httpd.h"
#include "macro_executor.h"
#include "storage.h"
#include "storage_blob.h"
#include "test_assert.h"
#include "test_examples_fixture.h"
#include "usb_keyboard.h"
#include "web_server_internal.h"
#include "wifi_ap.h"

#define STATUS_TEST_SESSION_TOKEN                                                                  \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

/* ---------------------------------------------------------------------- *
 * Test doubles: every dependency status_handler()/limits_handler() reach
 * that is NVS/FreeRTOS/hardware-backed in production (auth, device_settings,
 * wifi_ap, macro_executor, usb_keyboard, storage, storage_blob) or ESP-IDF-
 * build-only (esp_app_desc.h, esp_timer.h) is substituted here with a small,
 * behavior-controllable stand-in matching the real production signature --
 * the same technique test_web_server_blob_fixture.inc uses for
 * auth_session_validate/storage_blob.
 * ---------------------------------------------------------------------- */

static app_error_code_t g_auth_result;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_auth_result;
}

static app_error_code_t g_settings_read_result;
static app_v2_device_settings_t g_settings_record;

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    if (g_settings_read_result != APP_ERROR_NONE) {
        return g_settings_read_result;
    }
    *out_settings = g_settings_record;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    (void)settings;
    (void)out_changed;
    TEST_CHECK(false); /* status/limits never call this. */
    return APP_ERROR_INTERNAL;
}

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

static app_error_code_t g_capacity_result;
static size_t g_capacity_total_bytes;
static size_t g_capacity_used_bytes;

app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes) {
    TEST_CHECK_EQ_STRING(STORAGE_DATA_PARTITION, partition_label);
    if (g_capacity_result != APP_ERROR_NONE) {
        return g_capacity_result;
    }
    *out_total_bytes = g_capacity_total_bytes;
    *out_used_bytes = g_capacity_used_bytes;
    return APP_ERROR_NONE;
}

static app_error_code_t g_blob_list_result;
static size_t g_blob_valid_count;

app_error_code_t storage_blob_list(const storage_blob_scan_observer_t *observer,
                                   storage_blob_scan_summary_t *out_summary) {
    (void)observer;
    if (g_blob_list_result != APP_ERROR_NONE) {
        return g_blob_list_result;
    }
    *out_summary = (storage_blob_scan_summary_t){.valid_count = g_blob_valid_count};
    return APP_ERROR_NONE;
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
    return 123456000; /* 123456 ms, matching valid_snapshot()'s uptime fixture below. */
}

/* ---------------------------------------------------------------------- */

/* Every value below (device name, AP ssid/clientCount, storage
 * total/used/blob-count, esp_timer_get_time()'s fixed 123456 ms above) is
 * chosen to equal contracts/v2/api/examples.json's own "status" example
 * exactly, not just plausibly -- see test_status_valid_schema()'s
 * cJSON_Compare() against that checked-in fixture. "limits" needs no fake
 * tuning: web_adapter_build_limits_json() serializes fixed macro_limits.h/
 * app_limits_v2.h constants with no backend dependency at all. */
static void reset_fakes(void) {
    g_auth_result = APP_ERROR_NONE;

    g_settings_read_result = APP_ERROR_NONE;
    g_settings_record = (app_v2_device_settings_t){0};
    g_settings_record.provisioned = true;
    (void)snprintf(g_settings_record.device_name, sizeof(g_settings_record.device_name), "%s",
                   "Desk Macro Keyboard");
    (void)snprintf(g_settings_record.ap_ssid, sizeof(g_settings_record.ap_ssid), "%s",
                   "MacroKeyboard");
    g_settings_record.station_configured = false;

    g_wifi_status = (wifi_ap_status_t){.state = WIFI_AP_READY, .client_count = 1U};
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    g_usb_state = USB_KEYBOARD_READY;
    g_mount_state = (storage_mount_state_t){.web_mounted = true, .data_mounted = true};

    g_capacity_result = APP_ERROR_NONE;
    g_capacity_total_bytes = 524288U;
    g_capacity_used_bytes = 4096U;

    g_blob_list_result = APP_ERROR_NONE;
    g_blob_valid_count = 3U;
}

static void authenticate(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" STATUS_TEST_SESSION_TOKEN);
}

static cJSON *parse_response(const fake_httpd_request_t *fake) {
    cJSON *root = cJSON_ParseWithLength(fake->response_body, fake->response_body_length);
    TEST_CHECK(root != NULL);
    return root;
}

/* -------------------------------------------------------------------------
 * GET /api/v1/status
 * ---------------------------------------------------------------------- */

static void test_status_valid_schema(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    TEST_CHECK_EQ_STRING("no-store", fake_httpd_response_header(&fake, "Cache-Control"));

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "provisioned")));
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard",
                         cJSON_GetObjectItemCaseSensitive(root, "deviceName")->valuestring);
    TEST_CHECK_EQ_STRING("0.2.0",
                         cJSON_GetObjectItemCaseSensitive(root, "firmwareVersion")->valuestring);
    TEST_CHECK_EQ_STRING("git-abcdef0",
                         cJSON_GetObjectItemCaseSensitive(root, "buildId")->valuestring);
    const cJSON *access_point = cJSON_GetObjectItemCaseSensitive(root, "accessPoint");
    TEST_CHECK_EQ_STRING("running",
                         cJSON_GetObjectItemCaseSensitive(access_point, "state")->valuestring);
    const cJSON *storage = cJSON_GetObjectItemCaseSensitive(root, "storage");
    TEST_CHECK_EQ_U64(
        3U, (uint64_t)cJSON_GetObjectItemCaseSensitive(storage, "blobCount")->valuedouble);
    const cJSON *send = cJSON_GetObjectItemCaseSensitive(root, "send");
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(send, "present")));

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": diff the whole live response against
     * contracts/v2/api/examples.json's "status" example, not just the
     * handful of fields spot-checked above. */
    const cJSON *example = test_examples_fixture_get("status");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_status_send_present_reflects_executor_state(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_RUNNING, .available = true};
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    cJSON *root = parse_response(&fake);
    const cJSON *send = cJSON_GetObjectItemCaseSensitive(root, "send");
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(send, "present")));
    TEST_CHECK_EQ_STRING("running", cJSON_GetObjectItemCaseSensitive(send, "state")->valuestring);
    cJSON_Delete(root);
}

static void test_status_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("auth_required",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    cJSON_Delete(root);
}

static void test_status_unauthorized_with_expired_session(void) {
    reset_fakes();
    g_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_status_settings_read_failure_maps_to_503(void) {
    reset_fakes();
    g_settings_read_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

static void test_status_capacity_failure_maps_to_503(void) {
    reset_fakes();
    g_capacity_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

static void test_status_capacity_used_exceeds_total_maps_to_503(void) {
    reset_fakes();
    g_capacity_used_bytes = g_capacity_total_bytes + 1U;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

static void test_status_blob_listing_failure_maps_to_503(void) {
    reset_fakes();
    g_blob_list_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/status", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, status_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

/* -------------------------------------------------------------------------
 * GET /api/v1/limits
 * ---------------------------------------------------------------------- */

static void test_limits_valid_schema(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/limits", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, limits_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    TEST_CHECK_EQ_STRING("no-store", fake_httpd_response_header(&fake, "Cache-Control"));

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "ok") == NULL);
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "error") == NULL);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_adapter_build_limits_json() has no backend
     * dependency (every field is a fixed macro_limits.h/app_limits_v2.h
     * constant), so this diffs the live response against
     * contracts/v2/api/examples.json's "limits" example wholesale. */
    const cJSON *example = test_examples_fixture_get("limits");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_limits_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/limits", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, limits_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("auth_required",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    cJSON_Delete(root);
}

static void test_limits_unauthorized_with_expired_session(void) {
    reset_fakes();
    g_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/limits", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, limits_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

int main(void) {
    test_status_valid_schema();
    test_status_send_present_reflects_executor_state();
    test_status_unauthorized_without_cookie();
    test_status_unauthorized_with_expired_session();
    test_status_settings_read_failure_maps_to_503();
    test_status_capacity_failure_maps_to_503();
    test_status_capacity_used_exceeds_total_maps_to_503();
    test_status_blob_listing_failure_maps_to_503();

    test_limits_valid_schema();
    test_limits_unauthorized_without_cookie();
    test_limits_unauthorized_with_expired_session();

    puts("web server status/limits route tests passed");
    return 0;
}
