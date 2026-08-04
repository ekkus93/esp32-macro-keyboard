#include <stdbool.h>
#include <stddef.h>

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
    expect_route("/api/v1/settings", WEB_API_ROUTE_SETTINGS);
    expect_route("/api/v1/settings/change-password", WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD);
    expect_route("/api/v1/device/restart", WEB_API_ROUTE_DEVICE_RESTART);
    expect_route("/api/v1/device/reset-settings", WEB_API_ROUTE_DEVICE_RESET_SETTINGS);
    expect_route("/api/v1/device/factory-reset", WEB_API_ROUTE_DEVICE_FACTORY_RESET);
    expect_route("/api/v1/diagnostics", WEB_API_ROUTE_DIAGNOSTICS_FULL);
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

static void test_path_and_method_policy(void) {
    web_api_path_t parsed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_parse_path(NULL, &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/settings?x=1", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/settings/%2f", &parsed));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_parse_path("/api/v1/unknown", &parsed));

    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_AUTH_SESSION, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_AUTH_SESSION, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_DEVICE_RESTART, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_DIAGNOSTICS_FULL));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_FACTORY_RESET, false));
    TEST_CHECK(!web_api_route_requires_worker(WEB_API_ROUTE_SETTINGS));
}

static void test_content_type_request_id_and_status(void) {
    TEST_CHECK(web_api_content_type_is_json("application/json"));
    TEST_CHECK(web_api_content_type_is_json(" Application/JSON; charset=UTF-8"));
    TEST_CHECK(!web_api_content_type_is_json("text/plain"));
    TEST_CHECK(web_api_request_id_is_valid("request-123_abc.def:4"));
    TEST_CHECK(!web_api_request_id_is_valid("contains/slash"));
    TEST_CHECK_EQ_U64(404U, web_api_http_status_for_error(APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_INVALID_ARGUMENT));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_STORAGE_UNAVAILABLE));
}

int main(void) {
    test_active_routes();
    test_retired_routes_are_absent();
    test_path_and_method_policy();
    test_content_type_request_id_and_status();
    return 0;
}
