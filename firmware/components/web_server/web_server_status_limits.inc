static esp_err_t status_handler(httpd_req_t *request)
{
    const wifi_ap_status_t wifi = wifi_ap_get_status();
    const macro_execution_status_t execution = macro_executor_get_status();
    char response[512U];
    const app_error_code_t result = web_adapter_build_status_json(
        "0.1.0",
        "v5.5.5",
        usb_state_string(usb_keyboard_get_state()),
        wifi_state_string(wifi.state),
        wifi.client_count,
        execution_state_string(execution.state),
        response,
        sizeof(response));
    if (result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          result,
                          "response encoding failed");
    }
    return send_json(request, response, "200 OK");
}

static esp_err_t limits_handler(httpd_req_t *request)
{
    char response[512U];
    const app_error_code_t result = web_adapter_build_limits_json(response, sizeof(response));
    if (result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          result,
                          "response encoding failed");
    }
    return send_json(request, response, "200 OK");
}
