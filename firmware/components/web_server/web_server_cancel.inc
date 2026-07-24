static esp_err_t cancel_handler(httpd_req_t *request)
{
    char session_token[AUTH_TOKEN_HEX_BYTES];
    const app_error_code_t authorization = authorize_mutation(request, session_token);
    if (authorization != APP_ERROR_NONE) {
        return send_error(request,
                          "401 Unauthorized",
                          authorization,
                          "authentication required");
    }
    const app_error_code_t result = macro_executor_cancel();
    if (result != APP_ERROR_NONE) {
        return send_error(request,
                          result == APP_ERROR_NOT_FOUND ? "404 Not Found" : "409 Conflict",
                          result,
                          "no cancellable execution");
    }
    return send_json(request,
                     "{\"ok\":true,\"data\":{\"cancelRequested\":true}}",
                     "202 Accepted");
}

