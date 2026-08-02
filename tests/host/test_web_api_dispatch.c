#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

#define ARRAY_COUNT(values) (sizeof(values) / sizeof((values)[0]))

typedef enum {
    HANDLER_NONE = 0,
    HANDLER_SETS,
    HANDLER_MACROS,
    HANDLER_EXECUTION,
    HANDLER_ADMINISTRATION,
} handler_group_t;

typedef struct {
    web_api_route_t route;
    handler_group_t group;
} dispatch_case_t;

static handler_group_t seen_group;

static app_error_code_t record_handler(handler_group_t group, const web_api_call_t *call,
                                       web_api_response_t *response) {
    TEST_CHECK(call != NULL);
    TEST_CHECK(response != NULL);
    TEST_CHECK(seen_group == HANDLER_NONE);
    seen_group = group;
    return web_api_response_success(response, 200U, "{\"dispatched\":true}");
}

app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_response_t *response) {
    return record_handler(HANDLER_SETS, call, response);
}

app_error_code_t web_api_handle_macros(const web_api_call_t *call, web_api_response_t *response) {
    return record_handler(HANDLER_MACROS, call, response);
}

app_error_code_t web_api_handle_execution(const web_api_call_t *call,
                                          web_api_response_t *response) {
    return record_handler(HANDLER_EXECUTION, call, response);
}

app_error_code_t web_api_handle_administration(const web_api_call_t *call,
                                               web_api_response_t *response) {
    return record_handler(HANDLER_ADMINISTRATION, call, response);
}

static const dispatch_case_t dispatch_cases[] = {
    {WEB_API_ROUTE_AUTH_SESSION, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_SETS, HANDLER_SETS},
    {WEB_API_ROUTE_SETS_ORDER, HANDLER_SETS},
    {WEB_API_ROUTE_SET, HANDLER_SETS},
    {WEB_API_ROUTE_SET_DUPLICATE, HANDLER_SETS},
    {WEB_API_ROUTE_SET_SELECT, HANDLER_SETS},
    {WEB_API_ROUTE_SET_EXPORT, HANDLER_SETS},
    {WEB_API_ROUTE_SET_IMPORT, HANDLER_SETS},
    {WEB_API_ROUTE_SET_IMPORT_NEW, HANDLER_SETS},
    {WEB_API_ROUTE_SET_MACROS, HANDLER_MACROS},
    {WEB_API_ROUTE_SET_MACRO, HANDLER_MACROS},
    {WEB_API_ROUTE_SET_MACRO_VALIDATE, HANDLER_MACROS},
    {WEB_API_ROUTE_SET_MACRO_DUPLICATE, HANDLER_MACROS},
    {WEB_API_ROUTE_SET_MACROS_REORDER, HANDLER_MACROS},
    {WEB_API_ROUTE_EXECUTIONS, HANDLER_EXECUTION},
    {WEB_API_ROUTE_EXECUTION_CURRENT, HANDLER_EXECUTION},
    {WEB_API_ROUTE_EXECUTION_CANCEL, HANDLER_EXECUTION},
    {WEB_API_ROUTE_SETTINGS, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_DEVICE_RESTART, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_DEVICE_RESET_SETTINGS, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_DEVICE_FACTORY_RESET, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_DIAGNOSTICS_STORAGE, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_BACKUP, HANDLER_ADMINISTRATION},
    {WEB_API_ROUTE_RESTORE, HANDLER_ADMINISTRATION},
};

static void test_dispatch_matrix(void) {
    for (size_t index = 0U; index < ARRAY_COUNT(dispatch_cases); ++index) {
        seen_group = HANDLER_NONE;
        const web_api_call_t call = {
            .method = WEB_API_METHOD_GET,
            .path = {.route = dispatch_cases[index].route},
        };
        web_api_response_t response = {0};
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_dispatch(&call, &response));
        TEST_CHECK_EQ_INT(dispatch_cases[index].group, seen_group);
        TEST_CHECK_EQ_U64(200U, response.status);
        TEST_CHECK(response.body != NULL);
        TEST_CHECK(strstr(response.body, "\"dispatched\":true") != NULL);
        web_api_response_free(&response);
    }
}

static void test_invalid_dispatch_inputs(void) {
    web_api_response_t response = {0};
    const web_api_call_t unknown = {
        .method = WEB_API_METHOD_GET,
        .path = {.route = WEB_API_ROUTE_UNKNOWN},
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_dispatch(NULL, &response));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_dispatch(&unknown, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_dispatch(&unknown, &response));
    TEST_CHECK(response.body == NULL);
}

int main(void) {
    test_dispatch_matrix();
    test_invalid_dispatch_inputs();
    return 0;
}
