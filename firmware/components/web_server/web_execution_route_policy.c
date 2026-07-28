#include "web_execution_route_policy.h"

#include <stdbool.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"
#include "web_api_core.h"
#include "web_http_status.h"

static bool execution_is_terminal(execution_state_t state) {
    return state == EXECUTION_COMPLETED || state == EXECUTION_CANCELLED ||
           state == EXECUTION_FAILED || state == EXECUTION_TIMED_OUT;
}

static void reject(web_execution_cancel_policy_t *policy, unsigned int status,
                   app_error_code_t error, const char *message) {
    *policy = (web_execution_cancel_policy_t){
        .permitted = false,
        .status = status,
        .error = error,
        .message = message,
    };
}

app_error_code_t web_execution_cancel_policy_evaluate(
    const macro_execution_status_t *execution_status, const web_api_path_t *request_path,
    web_execution_cancel_policy_t *out_policy) {
    if (out_policy != NULL) {
        memset(out_policy, 0, sizeof(*out_policy));
    }
    if (execution_status == NULL || request_path == NULL || out_policy == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!execution_status->available) {
        reject(out_policy, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE, APP_ERROR_STORAGE_UNAVAILABLE,
               "executor unavailable");
        return APP_ERROR_NONE;
    }
    if (!request_path->has_execution_id ||
        !app_uuid_is_valid_string(execution_status->execution_id.value) ||
        !app_uuid_equal(&execution_status->execution_id, &request_path->execution_id)) {
        reject(out_policy, WEB_HTTP_STATUS_NOT_FOUND, APP_ERROR_NOT_FOUND,
               "execution not found");
        return APP_ERROR_NONE;
    }
    if (execution_status->cancellation_requested) {
        reject(out_policy, WEB_HTTP_STATUS_CONFLICT, APP_ERROR_CONFLICT,
               "cancellation already requested");
        return APP_ERROR_NONE;
    }
    if (execution_is_terminal(execution_status->state)) {
        reject(out_policy, WEB_HTTP_STATUS_CONFLICT, APP_ERROR_CONFLICT,
               "execution is already terminal");
        return APP_ERROR_NONE;
    }
    *out_policy = (web_execution_cancel_policy_t){
        .permitted = true,
        .error = APP_ERROR_NONE,
    };
    return APP_ERROR_NONE;
}
