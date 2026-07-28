#include "web_server_internal.h"

#include "app_error.h"
#include "auth.h"
#include "macro_executor.h"
#include "web_api_core.h"
#include "web_http_status.h"

static const char *cancel_status_text(unsigned int status) {
    switch (status) {
    case WEB_HTTP_STATUS_NOT_FOUND:
        return "404 Not Found";
    case WEB_HTTP_STATUS_CONFLICT:
        return "409 Conflict";
    case WEB_HTTP_STATUS_SERVICE_UNAVAILABLE:
        return "503 Service Unavailable";
    case WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR:
    default:
        return "500 Internal Server Error";
    }
}

esp_err_t cancel_handler(httpd_req_t *request) {
    char session_token[AUTH_TOKEN_HEX_BYTES];
    const app_error_code_t authorization = authorize_mutation(request, session_token);
    if (authorization != APP_ERROR_NONE) {
        return send_error(request, "401 Unauthorized", authorization, "authentication required");
    }
    const macro_execution_status_t status = macro_executor_get_status();
    if (!status.available) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_STORAGE_UNAVAILABLE,
                          "executor unavailable");
    }
    if (status.cancellation_requested) {
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT,
                          "cancellation already requested");
    }
    const app_error_code_t result = macro_executor_cancel();
    if (result != APP_ERROR_NONE) {
        return send_error(request, cancel_status_text(web_api_cancel_http_status(&status, result)),
                          result, "no cancellable execution");
    }
    return send_json(request, "{\"ok\":true,\"data\":{\"cancelRequested\":true}}", "202 Accepted");
}
