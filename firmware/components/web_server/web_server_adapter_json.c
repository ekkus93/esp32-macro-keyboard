#include "web_server_adapter_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "macro_limits.h"
#include "subsystem_health.h"
#include "web_diagnostics.h"

#define WEB_ADAPTER_UINT64_TEXT_MAX_BYTES 24U

app_error_code_t web_adapter_build_error_json(app_error_code_t code, const char *message,
                                              char *output, size_t output_size) {
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

app_error_code_t web_adapter_build_status_json(const char *version, const char *idf_version,
                                               const char *usb_state, const char *wifi_state,
                                               uint32_t wifi_clients, const char *execution_state,
                                               char *output, size_t output_size) {
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (version == NULL || idf_version == NULL || usb_state == NULL || wifi_state == NULL ||
        execution_state == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char clients[16U];
    const int client_length =
        snprintf(clients, sizeof(clients), "%lu", (unsigned long)wifi_clients);
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

app_error_code_t web_adapter_build_limits_json(char *output, size_t output_size) {
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int length = snprintf(
        output, output_size,
        "{\"ok\":true,\"data\":{\"macroNameBytes\":%lu,\"macroSourceBytes\":%lu,"
        "\"compiledActions\":%lu,\"delayMs\":%lu,\"durationMs\":%lu,"
        "\"macrosPerPackage\":%lu,\"packages\":%lu,\"importBytes\":%lu}}",
        (unsigned long)APP_MACRO_NAME_MAX_BYTES, (unsigned long)APP_MACRO_SOURCE_MAX_BYTES,
        (unsigned long)APP_COMPILED_ACTION_MAX, (unsigned long)APP_DELAY_MAX_MS,
        (unsigned long)APP_ESTIMATED_DURATION_MAX_MS, (unsigned long)APP_MACROS_PER_SET_MAX,
        (unsigned long)APP_MACRO_SETS_MAX, (unsigned long)APP_IMPORT_PACKAGE_MAX_BYTES);
    if (length < 0 || (size_t)length >= output_size) {
        output[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

static void append_uint64(json_writer_t *writer, uint64_t value) {
    char buffer[WEB_ADAPTER_UINT64_TEXT_MAX_BYTES];
    const int length = snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value);
    if (length < 0 || (size_t)length >= sizeof(buffer)) {
        writer->failed = true;
        return;
    }
    writer_append_text(writer, buffer);
}

static void append_capacity(json_writer_t *writer, const char *key,
                            const web_diagnostics_capacity_t *capacity) {
    writer_append_text(writer, "\"");
    writer_append_text(writer, key);
    writer_append_text(writer, "\":{\"ok\":");
    writer_append_text(writer, capacity->ok ? "true" : "false");
    writer_append_text(writer, ",\"totalBytes\":");
    append_uint64(writer, (uint64_t)capacity->total_bytes);
    writer_append_text(writer, ",\"usedBytes\":");
    append_uint64(writer, (uint64_t)capacity->used_bytes);
    writer_append_text(writer, "}");
}

app_error_code_t web_adapter_build_diagnostics_json(const web_diagnostics_snapshot_t *snapshot,
                                                    char *output, size_t output_size) {
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (snapshot == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    json_writer_t writer = {.buffer = output, .capacity = output_size};
    writer_append_text(&writer, "{\"buildId\":\"");
    writer_append_escaped(&writer, snapshot->build_id);
    writer_append_text(&writer, "\",\"firmwareVersion\":\"");
    writer_append_escaped(&writer, snapshot->firmware_version);
    writer_append_text(&writer, "\",\"schemaVersion\":");
    append_uint64(&writer, snapshot->schema_version);
    writer_append_text(&writer, ",\"resetReason\":\"");
    writer_append_escaped(&writer, snapshot->reset_reason);
    writer_append_text(&writer, "\",\"uptimeMs\":");
    append_uint64(&writer, snapshot->uptime_ms);
    writer_append_text(&writer, ",\"freeHeapBytes\":");
    append_uint64(&writer, snapshot->free_heap_bytes);
    writer_append_text(&writer, ",\"minFreeHeapBytes\":");
    append_uint64(&writer, snapshot->min_free_heap_bytes);
    writer_append_text(&writer, ",\"stack\":{\"controlsWords\":");
    append_uint64(&writer, (uint64_t)snapshot->controls_stack_high_water_mark);
    writer_append_text(&writer, ",\"executorWords\":");
    append_uint64(&writer, (uint64_t)snapshot->executor_stack_high_water_mark);
    writer_append_text(&writer, "},");
    append_capacity(&writer, "webfs", &snapshot->webfs);
    writer_append_text(&writer, ",");
    append_capacity(&writer, "userdata", &snapshot->userdata);
    writer_append_text(&writer, ",\"executionState\":\"");
    writer_append_escaped(&writer, snapshot->execution_state);
    writer_append_text(&writer, "\",\"subsystems\":[");
    for (size_t index = 0U; index < WEB_DIAGNOSTICS_SUBSYSTEM_COUNT; ++index) {
        if (index > 0U) {
            writer_append_text(&writer, ",");
        }
        writer_append_text(&writer, "{\"name\":\"");
        writer_append_escaped(&writer, snapshot->subsystems[index].name);
        writer_append_text(&writer, "\",\"state\":\"");
        writer_append_escaped(&writer,
                              subsystem_health_state_string(snapshot->subsystems[index].state));
        writer_append_text(&writer, "\"}");
    }
    writer_append_text(&writer, "]}");
    const app_error_code_t result = writer_finish(&writer);
    if (result != APP_ERROR_NONE) {
        output[0] = '\0';
    }
    return result;
}