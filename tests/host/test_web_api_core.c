#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_core.h"

static void expect_route(const char *path, web_api_route_t route) {
    web_api_path_t parsed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_parse_path(path, &parsed));
    TEST_CHECK_EQ_INT(route, parsed.route);
}

static void expect_absent(const char *path) {
    web_api_path_t parsed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_parse_path(path, &parsed));
    TEST_CHECK_EQ_INT(WEB_API_ROUTE_UNKNOWN, parsed.route);
}

static void test_active_routes(void) {
    expect_route("/api/v1/auth/session", WEB_API_ROUTE_AUTH_SESSION);
    expect_route("/api/v1/auth/login", WEB_API_ROUTE_AUTH_LOGIN);
    expect_route("/api/v1/auth/logout", WEB_API_ROUTE_AUTH_LOGOUT);
    expect_route("/api/v1/status", WEB_API_ROUTE_STATUS);
    expect_route("/api/v1/limits", WEB_API_ROUTE_LIMITS);
    expect_route("/api/v1/blob", WEB_API_ROUTE_BLOB_COLLECTION);
    expect_route("/api/v1/blob/7", WEB_API_ROUTE_BLOB_ITEM);
    expect_route("/api/v1/send", WEB_API_ROUTE_SEND);
    expect_route("/api/v1/settings", WEB_API_ROUTE_SETTINGS);
    expect_route("/api/v1/settings/change-password", WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD);
    expect_route("/api/v1/device/restart", WEB_API_ROUTE_DEVICE_RESTART);
    expect_route("/api/v1/device/reset-settings", WEB_API_ROUTE_DEVICE_RESET_SETTINGS);
    expect_route("/api/v1/device/factory-reset", WEB_API_ROUTE_DEVICE_FACTORY_RESET);
    expect_route("/api/v1/diagnostics", WEB_API_ROUTE_DIAGNOSTICS_FULL);
    expect_route("/api/v1/setup", WEB_API_ROUTE_SETUP);
}

static void test_retired_routes_are_absent(void) {
    static const char *const paths[] = {
        "/api/v1/package",
        "/api/v1/package/order",
        "/api/v1/package/import",
        "/api/v1/package/11111111-1111-4111-8111-111111111111",
        "/api/v1/package/11111111-1111-4111-8111-111111111111/macros",
        "/api/v1/executions",
        "/api/v1/executions/current",
        "/api/v1/executions/current/cancel",
        "/api/v1/repository",
        "/api/v1/restore",
        "/api/v1/diagnostics/storage",
        "/api/v1/diagnostics/storage/check",
    };
    for (size_t index = 0U; index < sizeof(paths) / sizeof(paths[0]); ++index) {
        expect_absent(paths[index]);
    }
}

static void test_invalid_paths(void) {
    web_api_path_t parsed = {.route = WEB_API_ROUTE_SETTINGS};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_parse_path(NULL, &parsed));
    TEST_CHECK_EQ_INT(WEB_API_ROUTE_UNKNOWN, parsed.route);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_parse_path("/api/v1/settings", NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("api/v1/settings", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/settings?x=1", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/settings#fragment", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/settings/%2f", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api//v1/settings", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/../settings", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_parse_path("/api/v1/unknown", &parsed));
}

static void test_method_and_body_policy(void) {
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_AUTH_SESSION, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_AUTH_SESSION, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_PUT));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_BLOB_ITEM, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_BLOB_ITEM, WEB_API_METHOD_DELETE));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_BLOB_ITEM, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_POST));
    TEST_CHECK(
        web_api_route_allows_method(WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, WEB_API_METHOD_POST));
    TEST_CHECK(
        !web_api_route_allows_method(WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_DEVICE_RESTART, WEB_API_METHOD_POST));
    TEST_CHECK(
        web_api_route_allows_method(WEB_API_ROUTE_DEVICE_RESET_SETTINGS, WEB_API_METHOD_POST));
    TEST_CHECK(
        web_api_route_allows_method(WEB_API_ROUTE_DEVICE_FACTORY_RESET, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_UNKNOWN, WEB_API_METHOD_GET));

    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_STATUS, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_STATUS, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_STATUS, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_STATUS, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_LIMITS, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_LIMITS, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_AUTH_LOGIN, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_AUTH_LOGIN, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_AUTH_LOGOUT, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_AUTH_LOGOUT, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SEND, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SEND, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SEND, WEB_API_METHOD_DELETE));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SEND, WEB_API_METHOD_PUT));

    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_STATUS, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_LIMITS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_AUTH_LOGIN, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_AUTH_LOGOUT, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_SEND, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_SEND, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_SEND, WEB_API_METHOD_DELETE));

    /* WEB_API_ROUTE_SETUP: reachable only once provisioned (SPEC 13.4), where
     * GET answers 404 and POST answers 409 -- see web_api_administration.c. */
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETUP, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETUP, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETUP, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETUP, WEB_API_METHOD_DELETE));

    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(
        web_api_route_requires_body(WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_DEVICE_RESTART, WEB_API_METHOD_POST));
    TEST_CHECK(
        web_api_route_requires_body(WEB_API_ROUTE_DEVICE_RESET_SETTINGS, WEB_API_METHOD_POST));
    TEST_CHECK(
        web_api_route_requires_body(WEB_API_ROUTE_DEVICE_FACTORY_RESET, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_UNKNOWN, WEB_API_METHOD_POST));
    /* A POST /api/v1/setup submission body on a provisioned device must not
     * be rejected with 422 before it can reach the 409 conflict response. */
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_SETUP, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_SETUP, WEB_API_METHOD_GET));
}

static void test_session_confirmation_and_worker_policy(void) {
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_BLOB_COLLECTION));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_DIAGNOSTICS_FULL));
    TEST_CHECK(!web_api_route_requires_session(WEB_API_ROUTE_UNKNOWN));
    /* SPEC 13.4: the 404/409 setup response must not require a session. */
    TEST_CHECK(!web_api_route_requires_session(WEB_API_ROUTE_SETUP));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_STATUS));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_LIMITS));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_SEND));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_AUTH_LOGOUT));
    /* contracts/v2/api/routes.json: login is "none-provisioned-only" -- it
     * establishes a session, so it cannot itself require one. */
    TEST_CHECK(!web_api_route_requires_session(WEB_API_ROUTE_AUTH_LOGIN));

    TEST_CHECK(
        web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_RESTART));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_RESET_SETTINGS));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_BLOB_COLLECTION));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SETTINGS));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SETUP));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_FACTORY_RESET, false));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_SETTINGS, true));
}

static void test_blob_id_policy(void) {
    uint64_t blob_id = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_parse_blob_id("/api/v1/blob/1", &blob_id));
    TEST_CHECK_EQ_U64(1U, blob_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_parse_blob_id("/api/v1/blob/18446744073709551615", &blob_id));
    TEST_CHECK_EQ_U64(UINT64_MAX, blob_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_blob_id("/api/v1/blob/", &blob_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_blob_id("/api/v1/blob/0", &blob_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_blob_id("/api/v1/blob/01", &blob_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_blob_id("/api/v1/blob/a", &blob_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_blob_id("/api/v1/blob/18446744073709551616", &blob_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_blob_id("/api/v1/blob/1/2", &blob_id));
}

static void test_content_type_policy(void) {
    TEST_CHECK(!web_api_content_type_is_json(NULL));
    TEST_CHECK(web_api_content_type_is_json("application/json"));
    TEST_CHECK(web_api_content_type_is_json("  application/json  "));
    TEST_CHECK(web_api_content_type_is_json(" Application/JSON ;  charset=UTF-8"));
    TEST_CHECK(!web_api_content_type_is_json(""));
    TEST_CHECK(!web_api_content_type_is_json("text/plain"));
    TEST_CHECK(!web_api_content_type_is_json("application/json;"));
    TEST_CHECK(!web_api_content_type_is_json("application/json; charset=ascii"));
    TEST_CHECK(!web_api_content_type_is_json("application/json; charset=utf-8; version=1"));
    TEST_CHECK(!web_api_content_type_is_json("application/jsonx"));

    TEST_CHECK(!web_api_content_type_is_gzip(NULL));
    TEST_CHECK(web_api_content_type_is_gzip("application/gzip"));
    TEST_CHECK(web_api_content_type_is_gzip("  Application/GZip  "));
    TEST_CHECK(!web_api_content_type_is_gzip(""));
    TEST_CHECK(!web_api_content_type_is_gzip("application/x-gzip"));
    TEST_CHECK(!web_api_content_type_is_gzip("application/gzip; charset=binary"));
    TEST_CHECK(!web_api_content_type_is_gzip("application/gzipx"));
}

static void test_request_id_policy(void) {
    TEST_CHECK(!web_api_request_id_is_valid(NULL));
    TEST_CHECK(!web_api_request_id_is_valid(""));
    TEST_CHECK(web_api_request_id_is_valid("request-123_abc.def:4"));
    TEST_CHECK(!web_api_request_id_is_valid("contains/slash"));
    TEST_CHECK(!web_api_request_id_is_valid("contains space"));

    char maximum[WEB_API_REQUEST_ID_MAX_BYTES + 1U];
    memset(maximum, 'a', sizeof(maximum));
    maximum[WEB_API_REQUEST_ID_MAX_BYTES] = '\0';
    TEST_CHECK(web_api_request_id_is_valid(maximum));

    char too_long[WEB_API_REQUEST_ID_MAX_BYTES + 2U];
    memset(too_long, 'b', sizeof(too_long));
    too_long[WEB_API_REQUEST_ID_MAX_BYTES + 1U] = '\0';
    TEST_CHECK(!web_api_request_id_is_valid(too_long));
}

static void test_error_status_mapping(void) {
    TEST_CHECK_EQ_U64(200U, web_api_http_status_for_error(APP_ERROR_NONE));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_INVALID_ARGUMENT));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_MACRO_SYNTAX));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_MACRO_LIMIT));
    TEST_CHECK_EQ_U64(404U, web_api_http_status_for_error(APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_CONFLICT));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_AUTH_STATE_INCOMPLETE));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_EXECUTOR_BUSY));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_EXECUTION_CANCELLED));
    TEST_CHECK_EQ_U64(507U, web_api_http_status_for_error(APP_ERROR_STORAGE_FULL));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_STORAGE_UNAVAILABLE));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_STORAGE_CORRUPT));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_USB_NOT_READY));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_RESET_RECOVERY_REQUIRED));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_TIMEOUT));
    TEST_CHECK_EQ_U64(401U, web_api_http_status_for_error(APP_ERROR_AUTH_REQUIRED));
    TEST_CHECK_EQ_U64(401U, web_api_http_status_for_error(APP_ERROR_AUTH_FAILED));
    TEST_CHECK_EQ_U64(429U, web_api_http_status_for_error(APP_ERROR_RATE_LIMITED));
    TEST_CHECK_EQ_U64(500U, web_api_http_status_for_error(APP_ERROR_IO));
    TEST_CHECK_EQ_U64(500U, web_api_http_status_for_error(APP_ERROR_INTERNAL));
}

int main(void) {
    test_active_routes();
    test_retired_routes_are_absent();
    test_invalid_paths();
    test_method_and_body_policy();
    test_session_confirmation_and_worker_policy();
    test_blob_id_policy();
    test_content_type_policy();
    test_request_id_policy();
    test_error_status_mapping();
    return 0;
}
