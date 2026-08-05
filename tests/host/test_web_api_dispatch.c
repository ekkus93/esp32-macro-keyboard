#include <stddef.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

static size_t administration_calls;

app_error_code_t web_api_handle_administration(const web_api_call_t *call,
                                               web_api_response_t *response) {
    TEST_CHECK(call != NULL);
    TEST_CHECK(response != NULL);
    ++administration_calls;
    return web_api_response_success(response, 200U, "{\"dispatched\":true}");
}

static void test_all_routes_delegate_to_administration(void) {
    static const web_api_route_t routes[] = {
        WEB_API_ROUTE_AUTH_SESSION,
        WEB_API_ROUTE_SETTINGS,
        WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD,
        WEB_API_ROUTE_DEVICE_RESTART,
        WEB_API_ROUTE_DEVICE_RESET_SETTINGS,
        WEB_API_ROUTE_DEVICE_FACTORY_RESET,
        WEB_API_ROUTE_DIAGNOSTICS_FULL,
    };
    for (size_t index = 0U; index < sizeof(routes) / sizeof(routes[0]); ++index) {
        const web_api_call_t call = {
            .method = WEB_API_METHOD_GET,
            .path = {.route = routes[index]},
        };
        web_api_response_t response = {0};
        const size_t before = administration_calls;
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_dispatch(&call, &response));
        TEST_CHECK_EQ_U64(before + 1U, administration_calls);
        TEST_CHECK_EQ_U64(200U, response.status);
        web_api_response_free(&response);
    }
}

static void test_invalid_inputs_and_unknown_route(void) {
    web_api_response_t response = {0};
    const web_api_call_t unknown = {.path = {.route = WEB_API_ROUTE_UNKNOWN}};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_dispatch(NULL, &response));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_dispatch(&unknown, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_dispatch(&unknown, &response));
    TEST_CHECK_EQ_U64(0U, administration_calls);
}

int main(void) {
    test_invalid_inputs_and_unknown_route();
    test_all_routes_delegate_to_administration();
    return 0;
}
