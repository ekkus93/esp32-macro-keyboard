static esp_err_t login_handler(httpd_req_t *request)
{
    if (!server_configuration.login_enabled) {
        return send_error(request,
                          "503 Service Unavailable",
                          APP_ERROR_AUTH_REQUIRED,
                          "login is not provisioned");
    }
    uint32_t retry_after = 0U;
    const app_error_code_t allowed = auth_login_attempt_allowed(&retry_after);
    if (allowed == APP_ERROR_RATE_LIMITED) {
        char retry_value[16U];
        const int written = snprintf(retry_value, sizeof(retry_value), "%u", retry_after);
        if (written < 0 || (size_t)written >= sizeof(retry_value) ||
            httpd_resp_set_hdr(request, "Retry-After", retry_value) != ESP_OK) {
            return ESP_FAIL;
        }
        return send_error(request,
                          "429 Too Many Requests",
                          APP_ERROR_RATE_LIMITED,
                          "too many login attempts");
    }
    if (allowed != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          allowed,
                          "login throttle unavailable");
    }

    char body[LOGIN_BODY_MAX_BYTES + 1U];
    const app_error_code_t body_result =
        read_bounded_body(request, body, sizeof(body), LOGIN_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        return send_error(request,
                          "400 Bad Request",
                          body_result,
                          "invalid request body");
