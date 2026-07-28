#include "web_api_handlers.h"

#include <stdbool.h>

#include "app_error.h"
#include "web_api_core.h"
#include "web_api_response.h"

static bool is_set_route(web_api_route_t route) {
    switch (route) {
    case WEB_API_ROUTE_SETS:
    case WEB_API_ROUTE_SET:
    case WEB_API_ROUTE_SET_DUPLICATE:
    case WEB_API_ROUTE_SET_SELECT:
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_SET_IMPORT:
        return true;
    default:
        return false;
    }
}

static bool is_macro_route(web_api_route_t route) {
    switch (route) {
    case WEB_API_ROUTE_SET_MACROS:
    case WEB_API_ROUTE_SET_MACRO:
    case WEB_API_ROUTE_SET_MACRO_VALIDATE:
    case WEB_API_ROUTE_SET_MACRO_DUPLICATE:
    case WEB_API_ROUTE_SET_MACROS_REORDER:
    case WEB_API_ROUTE_GLOBAL_MACROS:
    case WEB_API_ROUTE_GLOBAL_MACRO:
    case WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE:
    case WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE:
    case WEB_API_ROUTE_GLOBAL_MACROS_REORDER:
        return true;
    default:
        return false;
    }
}

static bool is_procedure_route(web_api_route_t route) {
    switch (route) {
    case WEB_API_ROUTE_SET_PROCEDURES:
    case WEB_API_ROUTE_SET_PROCEDURE:
    case WEB_API_ROUTE_SET_PROCEDURES_REORDER:
    case WEB_API_ROUTE_PROCEDURE_PROGRESS:
    case WEB_API_ROUTE_PROGRESS_COMPLETE:
    case WEB_API_ROUTE_PROGRESS_SKIP:
        return true;
    default:
        return false;
    }
}

static bool is_execution_route(web_api_route_t route) {
    switch (route) {
    case WEB_API_ROUTE_EXECUTIONS:
    case WEB_API_ROUTE_EXECUTION_CURRENT:
    case WEB_API_ROUTE_EXECUTION_CANCEL:
    case WEB_API_ROUTE_EXECUTION_CONFIRM:
        return true;
    default:
        return false;
    }
}

app_error_code_t web_api_dispatch(const web_api_call_t *call, web_api_response_t *response) {
    if (call == NULL || response == NULL || call->path.route == WEB_API_ROUTE_UNKNOWN) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (is_set_route(call->path.route)) {
        return web_api_handle_sets(call, response);
    }
    if (is_macro_route(call->path.route)) {
        return web_api_handle_macros(call, response);
    }
    if (is_procedure_route(call->path.route)) {
        return web_api_handle_procedures(call, response);
    }
    if (is_execution_route(call->path.route)) {
        return web_api_handle_execution(call, response);
    }
    return web_api_handle_administration(call, response);
}
