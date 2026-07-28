#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "macro_executor.h"
#include "test_assert.h"
#include "web_api_core.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define MACRO_ID "22222222-2222-4222-8222-222222222222"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define EXECUTION_ID "44444444-4444-4444-8444-444444444444"

static void check_route(const char *uri, web_api_route_t expected_route) {
    web_api_path_t path = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_parse_path(uri, &path));
    TEST_CHECK_EQ_INT(expected_route, path.route);
}

static void test_content_type_and_request_id(void) {
    TEST_CHECK(web_api_content_type_is_json("application/json"));
    TEST_CHECK(web_api_content_type_is_json("application/json; charset=utf-8"));
    TEST_CHECK(web_api_content_type_is_json(" Application/JSON; charset=UTF-8"));
    TEST_CHECK(!web_api_content_type_is_json("text/plain"));
    TEST_CHECK(!web_api_content_type_is_json("application/json; charset=latin1"));
    TEST_CHECK(!web_api_content_type_is_json(NULL));

    TEST_CHECK(web_api_request_id_is_valid("request-123_abc.def:4"));
    TEST_CHECK(!web_api_request_id_is_valid(""));
    TEST_CHECK(!web_api_request_id_is_valid("contains space"));
    TEST_CHECK(!web_api_request_id_is_valid("contains/slash"));
}

static void test_route_parsing(void) {
    check_route("/api/v1/auth/session", WEB_API_ROUTE_AUTH_SESSION);
    check_route("/api/v1/sets", WEB_API_ROUTE_SETS);
    check_route("/api/v1/sets/import", WEB_API_ROUTE_SET_IMPORT);
    check_route("/api/v1/sets/" SET_ID, WEB_API_ROUTE_SET);
    check_route("/api/v1/sets/" SET_ID "/select", WEB_API_ROUTE_SET_SELECT);
    check_route("/api/v1/sets/" SET_ID "/macros", WEB_API_ROUTE_SET_MACROS);
    check_route("/api/v1/sets/" SET_ID "/macros/" MACRO_ID,
                WEB_API_ROUTE_SET_MACRO);
    check_route("/api/v1/sets/" SET_ID "/macros/" MACRO_ID "/validate",
                WEB_API_ROUTE_SET_MACRO_VALIDATE);
    check_route("/api/v1/sets/" SET_ID "/procedures/" PROCEDURE_ID "/progress/skip",
                WEB_API_ROUTE_PROGRESS_SKIP);
    check_route("/api/v1/global/macros", WEB_API_ROUTE_GLOBAL_MACROS);
    check_route("/api/v1/global/macros/" MACRO_ID "/duplicate",
                WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE);
    check_route("/api/v1/executions", WEB_API_ROUTE_EXECUTIONS);
    check_route("/api/v1/executions/current", WEB_API_ROUTE_EXECUTION_CURRENT);
    check_route("/api/v1/executions/" EXECUTION_ID "/cancel",
                WEB_API_ROUTE_EXECUTION_CANCEL);
    check_route("/api/v1/settings/change-password", WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD);
    check_route("/api/v1/device/factory-reset", WEB_API_ROUTE_DEVICE_FACTORY_RESET);
    check_route("/api/v1/diagnostics/storage/check",
                WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK);
    check_route("/api/v1/backup", WEB_API_ROUTE_BACKUP);

    web_api_path_t path = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/%2fetc", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/../etc", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets//macros", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/not-a-uuid", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         web_api_parse_path("/api/v1/unknown", &path));
}

static void test_route_policy(void) {
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_EXECUTION_CANCEL, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_SETTINGS));
    TEST_CHECK(web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_requires_physical_confirmation(
        WEB_API_ROUTE_DEVICE_FACTORY_RESET));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_MACRO));
}

static void test_status_mapping(void) {
    TEST_CHECK_EQ_U64(404U, web_api_http_status_for_error(APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_CONFLICT));
    TEST_CHECK_EQ_U64(507U, web_api_http_status_for_error(APP_ERROR_STORAGE_FULL));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_USB_NOT_READY));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_MACRO_SYNTAX));

    macro_execution_status_t status = {.state = EXECUTION_IDLE};
    TEST_CHECK_EQ_U64(404U, web_api_cancel_http_status(&status, APP_ERROR_NOT_FOUND));
    status.state = EXECUTION_COMPLETED;
    TEST_CHECK_EQ_U64(409U, web_api_cancel_http_status(&status, APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(202U, web_api_cancel_http_status(&status, APP_ERROR_NONE));
    TEST_CHECK_EQ_U64(500U, web_api_cancel_http_status(&status, APP_ERROR_INTERNAL));
}

int main(void) {
    test_content_type_and_request_id();
    test_route_parsing();
    test_route_policy();
    test_status_mapping();
    return 0;
}
