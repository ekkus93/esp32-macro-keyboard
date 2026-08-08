#include "web_api_handlers.h"

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "auth.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_response.h"
#include "web_auth_routes.h"
#include "web_http_status.h"
#include "web_server_internal.h"

/* The request policy layer (web_request_policy.c) has already validated (and
 * thereby refreshed the idle deadline of) call->session_token before
 * dispatch reaches here -- see web_request_policy_evaluate()'s
 * enforce_session() step -- so this reads the resulting TTLs without a
 * second refresh. */
static app_error_code_t handle_session(const web_api_call_t *call, web_api_response_t *response) {
    uint32_t idle_seconds = 0U;
    uint32_t absolute_seconds = 0U;
    app_error_code_t result =
        auth_session_remaining(call->session_token, &idle_seconds, &absolute_seconds);
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        result = web_auth_session_response_json(idle_seconds, absolute_seconds, &json);
    }
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

/* Reached only once provisioned (the setup route table itself has no
 * wildcard fallthrough into this dispatcher while unprovisioned -- see
 * web_server_lifecycle.c). GET matches the unprovisioned-only semantics of
 * setup_state_handler by answering 404; POST answers the 409 SPEC 13.4
 * requires for a setup submission after provisioning. Neither branch reads
 * `response`'s request body. */
static app_error_code_t setup_route_response(const web_api_call_t *call,
                                             web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        return web_api_handler_error(response, APP_ERROR_NOT_FOUND, "route not found", NULL);
    }
    return web_api_handler_error(response, APP_ERROR_CONFLICT, "device is already provisioned",
                                 NULL);
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
    case WEB_API_ROUTE_SETUP:
        return setup_route_response(call, response);
    case WEB_API_ROUTE_UNKNOWN:
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
