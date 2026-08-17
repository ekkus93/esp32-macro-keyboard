#include "web_api_handlers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "auth.h"
#include "device_controls.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_response.h"
#include "web_auth_routes.h"
#include "web_device_actions.h"
#include "web_diagnostics.h"
#include "web_http_status.h"
#include "web_settings.h"
#include "web_settings_production_ops.h"

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

/* -------------------------------------------------------------------------
 * Shared device_settings / auth ops (settings routes and device actions
 * both need to read settings and, for two of them, verify a password
 * against the stored credential). The password-record wiring itself now
 * lives in web_settings_production_ops() (web_settings.c) so the physical
 * console's set-admin-password command can reuse it without duplicating it.
 * ---------------------------------------------------------------------- */

static app_error_code_t device_actions_ops_restart(void *context) {
    (void)context;
    return device_controls_restart();
}

static device_controls_reset_settings_outcome_t device_actions_ops_reset_settings(void *context) {
    (void)context;
    return device_controls_reset_settings();
}

static device_controls_factory_reset_outcome_t device_actions_ops_factory_reset(void *context) {
    (void)context;
    return device_controls_factory_reset();
}

static web_device_actions_ops_t device_actions_ops(void) {
    const web_settings_ops_t settings = web_settings_production_ops();
    return (web_device_actions_ops_t){
        .context = NULL,
        .settings_read = settings.settings_read,
        .password_verify = settings.password_verify,
        .restart = device_actions_ops_restart,
        .reset_settings = device_actions_ops_reset_settings,
        .factory_reset = device_actions_ops_factory_reset,
    };
}

/* True as long as the caller has not yet consumed `call->body` via a
 * wiping parser -- guards every body-required route below against a
 * present-but-empty body, whose backing storage
 * (authorize_and_read_api_call() in web_server_api.c) is the read-only
 * string literal "", not a heap buffer safe to wipe in place. */
static bool call_has_body(const web_api_call_t *call) {
    return call->body_length > 0U;
}

/* -------------------------------------------------------------------------
 * GET/PUT /api/v1/settings
 * ---------------------------------------------------------------------- */

static app_error_code_t handle_settings_get(web_api_response_t *response) {
    const web_settings_ops_t ops = web_settings_production_ops();
    char *json = NULL;
    const web_settings_get_outcome_t outcome = web_settings_get_handle(&ops, &json);
    app_error_code_t result;
    switch (outcome.result) {
    case WEB_SETTINGS_GET_OK:
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
        break;
    case WEB_SETTINGS_GET_BACKEND_UNAVAILABLE:
        result = web_api_handler_error(response, outcome.detail, "settings unavailable", NULL);
        break;
    case WEB_SETTINGS_GET_INTERNAL:
    default:
        result =
            web_api_handler_error(response, APP_ERROR_INTERNAL, "settings response failed", NULL);
        break;
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t settings_put_error(web_api_response_t *response,
                                           web_settings_put_outcome_t outcome) {
    switch (outcome.result) {
    case WEB_SETTINGS_PUT_INVALID_BODY:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid settings update request", NULL);
    case WEB_SETTINGS_PUT_EMPTY:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "update must include at least one field", NULL);
    case WEB_SETTINGS_PUT_INVALID_DEVICE_NAME:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT, "invalid device name",
                                     "deviceName");
    case WEB_SETTINGS_PUT_INVALID_SEND_MODE:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT, "invalid send mode",
                                     "sendMode");
    case WEB_SETTINGS_PUT_INVALID_SNAPSHOT_RETENTION_TARGET:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid snapshot retention target",
                                     "snapshotRetentionTarget");
    case WEB_SETTINGS_PUT_INVALID_LAST_SELECTED_PACKAGE_ID:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid last selected package id", "lastSelectedPackageId");
    case WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_SSID:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid access-point SSID", "accessPoint.ssid");
    case WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_PASSPHRASE:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid access-point passphrase", "accessPoint.passphrase");
    case WEB_SETTINGS_PUT_INVALID_STATION_SSID:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT, "invalid station SSID",
                                     "station.ssid");
    case WEB_SETTINGS_PUT_INVALID_STATION_PASSPHRASE:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid station passphrase", "station.passphrase");
    case WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE:
        return web_api_handler_error(response, outcome.detail, "settings unavailable", NULL);
    case WEB_SETTINGS_PUT_OK:
    case WEB_SETTINGS_PUT_INTERNAL:
    default:
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "settings update failed", NULL);
    }
}

static app_error_code_t handle_settings_put(const web_api_call_t *call,
                                            web_api_response_t *response) {
    if (!call_has_body(call)) {
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid settings update request", NULL);
    }
    const web_settings_ops_t ops = web_settings_production_ops();
    char *json = NULL;
    const web_settings_put_outcome_t outcome =
        web_settings_put_handle((char *)call->body, call->body_length + 1U, &ops, &json);
    const app_error_code_t result =
        outcome.result == WEB_SETTINGS_PUT_OK
            ? web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json)
            : settings_put_error(response, outcome);
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t handle_settings(const web_api_call_t *call, web_api_response_t *response) {
    return call->method == WEB_API_METHOD_GET ? handle_settings_get(response)
                                              : handle_settings_put(call, response);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/settings/change-password
 * ---------------------------------------------------------------------- */

static app_error_code_t change_password_error(web_api_response_t *response,
                                              web_change_password_outcome_t outcome) {
    switch (outcome.result) {
    case WEB_CHANGE_PASSWORD_INVALID_BODY:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid change-password request", NULL);
    case WEB_CHANGE_PASSWORD_INVALID_NEW_PASSWORD:
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT, "invalid new password",
                                     "newPassword");
    case WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD:
        /* SPEC_V2 13.9: "An incorrect current password returns 403." No
         * app_error_code_t maps to 403 via web_api_http_status_for_error()
         * (only 401/other codes exist there), so this builds the error
         * envelope directly with an explicit status, exactly like
         * web_server_send.c's set_error_response() already does for other
         * routes outside this generic dispatch pipeline. */
        return web_api_response_error(response, &(web_api_error_spec_t){
                                                    .status = WEB_HTTP_STATUS_FORBIDDEN,
                                                    .code = APP_ERROR_AUTH_FAILED,
                                                    .message = "incorrect current password",
                                                });
    case WEB_CHANGE_PASSWORD_COMMITTED_SESSION_INVALIDATION_INCOMPLETE:
        /* The durable credential and RAM verifier are already the new password.
         * This must be programmatically distinguishable from a pre-commit
         * failure so a caller never tells the owner to keep using the old
         * password. */
        return web_api_response_error(response,
                                      &(web_api_error_spec_t){
                                          .status = WEB_HTTP_STATUS_CONFLICT,
                                          .code = APP_ERROR_AUTH_STATE_INCOMPLETE,
                                          .message = "password changed; session invalidation "
                                                     "incomplete; sign in with the new password",
                                      });
    case WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE:
        if (outcome.detail == APP_ERROR_CONFLICT) {
            /* A credential transition is already active, but this request has
             * not committed anything. Keep this status distinct from H2's 409
             * committed-partial result so the cookie-clearing layer cannot
             * confuse "nothing changed" with "new password authoritative". */
            return web_api_response_error(response,
                                          &(web_api_error_spec_t){
                                              .status = WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                                              .code = APP_ERROR_CONFLICT,
                                              .message = "password change already in progress",
                                          });
        }
        return web_api_handler_error(response, outcome.detail, "settings unavailable", NULL);
    case WEB_CHANGE_PASSWORD_OK:
    case WEB_CHANGE_PASSWORD_INTERNAL:
    default:
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "password change failed", NULL);
    }
}

static app_error_code_t handle_change_password(const web_api_call_t *call,
                                               web_api_response_t *response) {
    if (!call_has_body(call)) {
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid change-password request", NULL);
    }
    const web_settings_ops_t ops = web_settings_production_ops();
    const web_change_password_outcome_t outcome =
        web_change_password_handle((char *)call->body, call->body_length + 1U, &ops);
    if (outcome.result != WEB_CHANGE_PASSWORD_OK) {
        return change_password_error(response, outcome);
    }
    return web_api_handler_no_content(response, WEB_HTTP_STATUS_NO_CONTENT);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/restart
 * ---------------------------------------------------------------------- */

static app_error_code_t handle_device_restart(web_api_response_t *response) {
    const web_device_actions_ops_t ops = device_actions_ops();
    const web_device_restart_outcome_t outcome = web_device_restart_handle(&ops);
    switch (outcome.result) {
    case WEB_DEVICE_RESTART_OK:
        break;
    case WEB_DEVICE_RESTART_BACKEND_UNAVAILABLE:
        return web_api_handler_error(response, outcome.detail, "restart could not be scheduled",
                                     NULL);
    case WEB_DEVICE_RESTART_INTERNAL:
    default:
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "restart could not be scheduled",
                                     NULL);
    }
    char *json = NULL;
    if (web_device_restart_accepted_json(&json) != APP_ERROR_NONE) {
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }
    const app_error_code_t result =
        web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED, json);
    web_api_handler_json_free(json);
    return result;
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/reset-settings
 * ---------------------------------------------------------------------- */

static app_error_code_t handle_device_reset_settings(const web_api_call_t *call,
                                                     web_api_response_t *response) {
    if (!call_has_body(call)) {
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid reset-settings request", NULL);
    }

    char *json = NULL;
    web_api_response_t accepted_response = {0};
    if (web_device_reset_accepted_json(false, true, &json) != APP_ERROR_NONE) {
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }
    const app_error_code_t prepare_result =
        web_api_handler_success_json(&accepted_response, WEB_HTTP_STATUS_ACCEPTED, json);
    web_api_handler_json_free(json);
    if (prepare_result != APP_ERROR_NONE) {
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }

    const web_device_actions_ops_t ops = device_actions_ops();
    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle((char *)call->body, call->body_length + 1U, &ops);
    switch (outcome.result) {
    case WEB_DEVICE_RESET_SETTINGS_OK:
    case WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED:
        *response = accepted_response;
        return APP_ERROR_NONE;
    case WEB_DEVICE_RESET_SETTINGS_INVALID_BODY:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid reset-settings request", NULL);
    case WEB_DEVICE_RESET_SETTINGS_INVALID_CONFIRMATION:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "incorrect confirmation phrase", "confirmation");
    case WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, outcome.detail, "reset-settings unavailable", NULL);
    case WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE:
        web_api_response_free(&accepted_response);
        return web_api_response_error(
            response,
            &(web_api_error_spec_t){
                .status = WEB_HTTP_STATUS_CONFLICT,
                .code = APP_ERROR_RESET_SETTINGS_INCOMPLETE,
                .message = "settings reset; automatic restart incomplete; restart the device",
            });
    case WEB_DEVICE_RESET_SETTINGS_INTERNAL:
    default:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "reset-settings failed", NULL);
    }
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset
 * ---------------------------------------------------------------------- */

static app_error_code_t handle_device_factory_reset(const web_api_call_t *call,
                                                    web_api_response_t *response) {
    if (!call_has_body(call)) {
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid factory-reset request", NULL);
    }

    char *json = NULL;
    web_api_response_t accepted_response = {0};
    if (web_device_reset_accepted_json(true, false, &json) != APP_ERROR_NONE) {
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }
    const app_error_code_t prepare_result =
        web_api_handler_success_json(&accepted_response, WEB_HTTP_STATUS_ACCEPTED, json);
    web_api_handler_json_free(json);
    if (prepare_result != APP_ERROR_NONE) {
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }

    const web_device_actions_ops_t ops = device_actions_ops();
    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle((char *)call->body, call->body_length + 1U, &ops);
    switch (outcome.result) {
    case WEB_DEVICE_FACTORY_RESET_OK:
    case WEB_DEVICE_FACTORY_RESET_RECOVERY_REQUIRED:
        *response = accepted_response;
        return APP_ERROR_NONE;
    case WEB_DEVICE_FACTORY_RESET_INVALID_BODY:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid factory-reset request", NULL);
    case WEB_DEVICE_FACTORY_RESET_INVALID_CONFIRMATION:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "incorrect confirmation phrase", "confirmation");
    case WEB_DEVICE_FACTORY_RESET_INCORRECT_PASSWORD:
        web_api_response_free(&accepted_response);
        return web_api_response_error(response, &(web_api_error_spec_t){
                                                    .status = WEB_HTTP_STATUS_FORBIDDEN,
                                                    .code = APP_ERROR_AUTH_FAILED,
                                                    .message = "incorrect administrator password",
                                                    .field = "adminPassword",
                                                });
    case WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, outcome.detail, "factory-reset unavailable", NULL);
    case WEB_DEVICE_FACTORY_RESET_INTERNAL:
    default:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "factory-reset failed", NULL);
    }
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
        return handle_device_restart(response);
    case WEB_API_ROUTE_DIAGNOSTICS_FULL:
        return web_diagnostics_handle(response);
    case WEB_API_ROUTE_SETTINGS:
        return handle_settings(call, response);
    case WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD:
        return handle_change_password(call, response);
    case WEB_API_ROUTE_DEVICE_RESET_SETTINGS:
        return handle_device_reset_settings(call, response);
    case WEB_API_ROUTE_DEVICE_FACTORY_RESET:
        return handle_device_factory_reset(call, response);
    case WEB_API_ROUTE_SETUP:
        return setup_route_response(call, response);
    case WEB_API_ROUTE_UNKNOWN:
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
