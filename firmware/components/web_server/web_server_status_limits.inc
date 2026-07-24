static esp_err_t status_handler(httpd_req_t *request)
{
    const wifi_ap_status_t wifi = wifi_ap_get_status();
    const macro_execution_status_t execution = macro_executor_get_status();
    char response[512U];
    const int length = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"data\":{\"version\":\"0.1.0\",\"idf\":\"v5.5.5\","
        "\"usbState\":\"%s\",\"wifiState\":\"%s\",\"wifiClients\":%u,"
        "\"executionState\":\"%s\"}}",
        usb_state_string(usb_keyboard_get_state()),
        wifi_state_string(wifi.state),
        (unsigned int)wifi.client_count,
        execution_state_string(execution.state));
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request,
                          "500 Internal Server Error",
                          APP_ERROR_INTERNAL,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

static esp_err_t limits_handler(httpd_req_t *request)
{
    char response[512U];
    const int length = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"data\":{\"macroNameBytes\":%u,\"macroSourceBytes\":%u,"
        "\"compiledActions\":%u,\"delayMs\":%u,\"durationMs\":%u,"
        "\"macrosPerSet\":%u,\"proceduresPerSet\":%u,\"stepsPerProcedure\":%u,"
        "\"macroSets\":%u,\"importBytes\":%u}}",
        APP_MACRO_NAME_MAX_BYTES,
        APP_MACRO_SOURCE_MAX_BYTES,
        APP_COMPILED_ACTION_MAX,
        APP_DELAY_MAX_MS,
        APP_ESTIMATED_DURATION_MAX_MS,
        APP_MACROS_PER_SET_MAX,
        APP_PROCEDURES_PER_SET_MAX,
        APP_STEPS_PER_PROCEDURE_MAX,
        APP_MACRO_SETS_MAX,
        APP_IMPORT_PACKAGE_MAX_BYTES);
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request,
                          "500 Internal Server Error",
                          APP_ERROR_INTERNAL,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}
