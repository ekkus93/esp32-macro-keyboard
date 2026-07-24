app_error_code_t web_adapter_build_error_json(app_error_code_t code,
                                               const char *message,
                                               char *output,
                                               size_t output_size)
{
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (message == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    json_writer_t writer = {.buffer = output, .capacity = output_size};
    writer_append_text(&writer, "{\"ok\":false,\"error\":{\"code\":\"");
    writer_append_escaped(&writer, app_error_code_string(code));
    writer_append_text(&writer, "\",\"message\":\"");
    writer_append_escaped(&writer, message);
    writer_append_text(&writer, "\"}}");
    const app_error_code_t result = writer_finish(&writer);
    if (result != APP_ERROR_NONE) {
        output[0] = '\0';
    }
    return result;
}

app_error_code_t web_adapter_build_status_json(const char *version,
                                                const char *idf_version,
                                                const char *usb_state,
                                                const char *wifi_state,
                                                uint32_t wifi_clients,
                                                const char *execution_state,
                                                char *output,
                                                size_t output_size)
{
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (version == NULL || idf_version == NULL || usb_state == NULL ||
        wifi_state == NULL || execution_state == NULL || output == NULL ||
        output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char clients[16U];
    const int client_length = snprintf(clients, sizeof(clients), "%u", wifi_clients);
    if (client_length < 0 || (size_t)client_length >= sizeof(clients)) {
        return APP_ERROR_INTERNAL;
    }
    json_writer_t writer = {.buffer = output, .capacity = output_size};
    writer_append_text(&writer, "{\"ok\":true,\"data\":{\"version\":\"");
    writer_append_escaped(&writer, version);
    writer_append_text(&writer, "\",\"idf\":\"");
    writer_append_escaped(&writer, idf_version);
    writer_append_text(&writer, "\",\"usbState\":\"");
    writer_append_escaped(&writer, usb_state);
    writer_append_text(&writer, "\",\"wifiState\":\"");
    writer_append_escaped(&writer, wifi_state);
    writer_append_text(&writer, "\",\"wifiClients\":");
    writer_append_text(&writer, clients);
    writer_append_text(&writer, ",\"executionState\":\"");
    writer_append_escaped(&writer, execution_state);
    writer_append_text(&writer, "\"}}");
    const app_error_code_t result = writer_finish(&writer);
    if (result != APP_ERROR_NONE) {
        output[0] = '\0';
    }
    return result;
}

app_error_code_t web_adapter_build_limits_json(char *output, size_t output_size)
{
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int length = snprintf(
        output,
        output_size,
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
    if (length < 0 || (size_t)length >= output_size) {
        output[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}
