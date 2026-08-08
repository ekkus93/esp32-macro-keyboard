#include "web_api_handlers.h"

#include <stddef.h>

#include "app_error.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_response.h"
#include "web_http_status.h"
#include "web_server_internal.h"

static app_error_code_t handle_session(const web_api_call_t *call, web_api_response_t *response) {
    (void)call;
    char *json = NULL;
    app_error_code_t result = web_api_handler_session_json(&json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    } else {
        result = web_api_handler_error(response, result, "session unavailable", NULL);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t v2_configuration_route_pending(web_api_response_t *response) {
    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE,
                                 "V2 configuration route is not enabled yet", NULL);
}

app_error_code_t web_api_handle_administration(const web_api_call_t *call,
                                               web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_AUTH_SESSION:
        return handle_session(call, response);
    case WEB_API_ROUTE_DEVICE_RESTART:
        return web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED,
                                            "{\"accepted\":true,\"connectionWillClose\":true,"
                                            "\"reprovisioningRequired\":false}");
    case WEB_API_ROUTE_DIAGNOSTICS_FULL:
        return web_diagnostics_handle(response);
    case WEB_API_ROUTE_SETTINGS:
    case WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD:
    case WEB_API_ROUTE_DEVICE_RESET_SETTINGS:
    case WEB_API_ROUTE_DEVICE_FACTORY_RESET:
        return v2_configuration_route_pending(response);
    case WEB_API_ROUTE_UNKNOWN:
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
