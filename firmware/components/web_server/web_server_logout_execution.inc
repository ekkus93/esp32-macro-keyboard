static esp_err_t logout_handler(httpd_req_t *request)
{
    char session_token[AUTH_TOKEN_HEX_BYTES];
    const app_error_code_t authorization = authorize_mutation(request, session_token);
    if (authorization != APP_ERROR_NONE) {
        return send_error(request,
                          "401 Unauthorized",
                          authorization,
                          "authentication required");
    }
    const app_error_code_t result = auth_session_logout(session_token);
    if (result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          result,
                          "logout failed");
    }
    if (httpd_resp_set_hdr(request,
                           "Set-Cookie",
                           SESSION_COOKIE_NAME "=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0") !=
        ESP_OK) {
        return ESP_FAIL;
    }
    return send_json(request, "{\"ok\":true,\"data\":{}}", "200 OK");
}

static esp_err_t execution_handler(httpd_req_t *request)
{
    const macro_execution_status_t execution = macro_executor_get_status();
    char response[384U];
    const int length = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"data\":{\"state\":\"%s\",\"error\":\"%s\","
        "\"releaseError\":\"%s\",\"actionIndex\":%u,\"actionCount\":%u}}",
        execution_state_string(execution.state),
        app_error_code_string(execution.error),
        app_error_code_string(execution.release_error),
        (unsigned int)execution.action_index,
        (unsigned int)execution.action_count);
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request,
                          "500 Internal Server Error",
                          APP_ERROR_INTERNAL,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

