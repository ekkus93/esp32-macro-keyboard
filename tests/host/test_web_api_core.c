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
    check_route("/api/v1/sets/order", WEB_API_ROUTE_SETS_ORDER);
    check_route("/api/v1/sets/import", WEB_API_ROUTE_SET_IMPORT);
    check_route("/api/v1/sets/import-new", WEB_API_ROUTE_SET_IMPORT_NEW);
    check_route("/api/v1/sets/" SET_ID, WEB_API_ROUTE_SET);
    check_route("/api/v1/sets/" SET_ID "/select", WEB_API_ROUTE_SET_SELECT);
    check_route("/api/v1/sets/" SET_ID "/macros", WEB_API_ROUTE_SET_MACROS);
    check_route("/api/v1/sets/" SET_ID "/macros/" MACRO_ID, WEB_API_ROUTE_SET_MACRO);
    check_route("/api/v1/sets/" SET_ID "/macros/" MACRO_ID "/validate",
                WEB_API_ROUTE_SET_MACRO_VALIDATE);
    check_route("/api/v1/sets/" SET_ID "/procedures/" PROCEDURE_ID "/progress/skip",
                WEB_API_ROUTE_PROGRESS_SKIP);
    check_route("/api/v1/executions", WEB_API_ROUTE_EXECUTIONS);
    check_route("/api/v1/executions/current", WEB_API_ROUTE_EXECUTION_CURRENT);
    check_route("/api/v1/executions/current/cancel", WEB_API_ROUTE_EXECUTION_CANCEL);
    check_route("/api/v1/executions/" EXECUTION_ID "/cancel", WEB_API_ROUTE_EXECUTION_CANCEL);
    check_route("/api/v1/settings/change-password", WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD);
    check_route("/api/v1/device/factory-reset", WEB_API_ROUTE_DEVICE_FACTORY_RESET);
    check_route("/api/v1/diagnostics/storage/check", WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK);
    check_route("/api/v1/diagnostics/quarantine", WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE);
    check_route("/api/v1/diagnostics", WEB_API_ROUTE_DIAGNOSTICS_FULL);
    check_route("/api/v1/backup", WEB_API_ROUTE_BACKUP);

    web_api_path_t path = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         web_api_parse_path("/api/v1/executions/" EXECUTION_ID "/confirm", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/%2fetc", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/../etc", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets//macros", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/not-a-uuid", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_parse_path("/api/v1/unknown", &path));
    /* Global macros were removed (SPEC §7.2): every macro is reached through
     * its set, so the old /global tree must not resolve to anything. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_parse_path("/api/v1/global/macros", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         web_api_parse_path("/api/v1/global/macros/" MACRO_ID, &path));
}

static void test_route_policy(void) {
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETS, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_requires_body(WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE));
    TEST_CHECK(!web_api_route_requires_body(WEB_API_ROUTE_EXECUTION_CANCEL, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_SETTINGS));
    TEST_CHECK(web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_requires_csrf(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_IMPORT));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_IMPORT_NEW));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SET_IMPORT_NEW, WEB_API_METHOD_POST));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SET_IMPORT_NEW, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_EXECUTIONS));
    TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, true));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, false));
    /* Every gated route honours the setting - none may demand a button press
     * unconditionally, or the device is unusable without hardware on it. */
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_FACTORY_RESET, false));
    TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_FACTORY_RESET, true));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_RESTORE, false));
    TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_RESTORE, true));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_SET_IMPORT, false));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_RESTART, false));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_RESET_SETTINGS, false));
    TEST_CHECK(
        !web_api_physical_confirmation_required(WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, false));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_SET_MACRO, true));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_MACRO));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_DELETE));
    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_GET));
    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_POST));
    TEST_CHECK(web_api_route_requires_session(WEB_API_ROUTE_DIAGNOSTICS_FULL));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DIAGNOSTICS_FULL));
}

static void test_error_status_mapping(void) {
    TEST_CHECK_EQ_U64(200U, web_api_http_status_for_error(APP_ERROR_NONE));
    TEST_CHECK_EQ_U64(401U, web_api_http_status_for_error(APP_ERROR_AUTH_REQUIRED));
    TEST_CHECK_EQ_U64(404U, web_api_http_status_for_error(APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_CONFLICT));
    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_EXECUTOR_BUSY));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_INVALID_ARGUMENT));
    TEST_CHECK_EQ_U64(422U, web_api_http_status_for_error(APP_ERROR_MACRO_SYNTAX));
    TEST_CHECK_EQ_U64(429U, web_api_http_status_for_error(APP_ERROR_RATE_LIMITED));
    TEST_CHECK_EQ_U64(500U, web_api_http_status_for_error(APP_ERROR_IO));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_STORAGE_UNAVAILABLE));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_STORAGE_CORRUPT));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_USB_NOT_READY));
    TEST_CHECK_EQ_U64(503U, web_api_http_status_for_error(APP_ERROR_TIMEOUT));
    TEST_CHECK_EQ_U64(507U, web_api_http_status_for_error(APP_ERROR_STORAGE_FULL));
}

static void test_cancellation_status_matrix(void) {
    macro_execution_status_t status = {.state = EXECUTION_IDLE, .available = true};
    TEST_CHECK_EQ_U64(202U, web_api_cancel_http_status(&status, APP_ERROR_NONE));
    TEST_CHECK_EQ_U64(500U, web_api_cancel_http_status(&status, APP_ERROR_INTERNAL));
    TEST_CHECK_EQ_U64(503U, web_api_cancel_http_status(&status, APP_ERROR_STORAGE_UNAVAILABLE));
    TEST_CHECK_EQ_U64(503U, web_api_cancel_http_status(&status, APP_ERROR_USB_NOT_READY));
    TEST_CHECK_EQ_U64(404U, web_api_cancel_http_status(&status, APP_ERROR_NOT_FOUND));

    status.state = EXECUTION_RUNNING;
    TEST_CHECK_EQ_U64(404U, web_api_cancel_http_status(&status, APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(409U, web_api_cancel_http_status(&status, APP_ERROR_CONFLICT));

    static const execution_state_t terminal_states[] = {
        EXECUTION_COMPLETED,
        EXECUTION_CANCELLED,
        EXECUTION_FAILED,
        EXECUTION_TIMED_OUT,
    };
    for (size_t index = 0U; index < sizeof(terminal_states) / sizeof(terminal_states[0]); ++index) {
        status.state = terminal_states[index];
        TEST_CHECK_EQ_U64(409U, web_api_cancel_http_status(&status, APP_ERROR_NOT_FOUND));
    }

    TEST_CHECK_EQ_U64(409U, web_api_cancel_http_status(NULL, APP_ERROR_NOT_FOUND));
    TEST_CHECK_EQ_U64(409U, web_api_cancel_http_status(NULL, APP_ERROR_EXECUTION_CANCELLED));
}

int main(void) {
    test_content_type_and_request_id();
    test_route_parsing();
    test_route_policy();
    test_error_status_mapping();
    test_cancellation_status_matrix();
    return 0;
}
