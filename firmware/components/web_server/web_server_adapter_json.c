#include "web_server_adapter_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../app_contracts_v2/include/app_limits_v2.h"
#include "app_error.h"
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
    /* SPEC_V2 13.2's exact error envelope -- no v1-style top-level "ok" key. */
    json_writer_t writer = {.buffer = output, .capacity = output_size};
    writer_append_text(&writer, "{\"error\":{\"code\":\"");
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

app_error_code_t web_adapter_build_limits_json(char *output, size_t output_size) {
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int length = snprintf(
        output, output_size,
        "{\"packageNameMaxBytes\":%lu,\"macroNameMaxBytes\":%lu,"
        "\"macroSourceMaxBytes\":%lu,\"compiledActionsMax\":%lu,"
        "\"delayDirectiveMaxMs\":%lu,\"keyPressMaxMs\":%lu,"
        "\"interKeyMaxMs\":%lu,\"estimatedDurationMaxMs\":%lu,"
        "\"executorAbsoluteDeadlineMs\":%lu,\"jsonBodyMaxBytes\":%lu,"
        "\"blobMaxBytes\":%lu,\"activeSessionsMax\":%lu,"
        "\"sessionIdleLifetimeSeconds\":%lu,\"sessionAbsoluteLifetimeSeconds\":%lu,"
        "\"serialConfirmationTimeoutSeconds\":%lu,\"adminPasswordMinBytes\":%lu,"
        "\"adminPasswordMaxBytes\":%lu,\"snapshotRetentionTargetMax\":%lu}",
        (unsigned long)APP_V2_PACKAGE_NAME_MAX_BYTES, (unsigned long)APP_V2_MACRO_NAME_MAX_BYTES,
        (unsigned long)APP_V2_MACRO_SOURCE_MAX_BYTES, (unsigned long)APP_V2_COMPILED_ACTIONS_MAX,
        (unsigned long)APP_V2_DELAY_DIRECTIVE_MAX_MS, (unsigned long)APP_V2_KEY_PRESS_MAX_MS,
        (unsigned long)APP_V2_INTER_KEY_MAX_MS, (unsigned long)APP_V2_ESTIMATED_DURATION_MAX_MS,
        (unsigned long)APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS,
        (unsigned long)APP_V2_JSON_BODY_MAX_BYTES, (unsigned long)APP_V2_BLOB_MAX_BYTES,
        (unsigned long)APP_V2_ACTIVE_SESSIONS_MAX,
        (unsigned long)APP_V2_SESSION_IDLE_LIFETIME_SECONDS,
        (unsigned long)APP_V2_SESSION_ABSOLUTE_LIFETIME_SECONDS,
        (unsigned long)APP_V2_SERIAL_CONFIRMATION_TIMEOUT_SECONDS,
        (unsigned long)APP_V2_ADMIN_PASSWORD_MIN_BYTES,
        (unsigned long)APP_V2_ADMIN_PASSWORD_MAX_BYTES,
        (unsigned long)APP_V2_SNAPSHOT_RETENTION_TARGET_MAX);
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

static void append_string_array(json_writer_t *writer,
                                const char (*values)[WEB_DIAGNOSTICS_INVALID_NAME_CAPACITY],
                                size_t count) {
    writer_append_text(writer, "[");
    for (size_t index = 0U; index < count; ++index) {
        if (index > 0U) {
            writer_append_text(writer, ",");
        }
        writer_append_text(writer, "\"");
        writer_append_escaped(writer, values[index]);
        writer_append_text(writer, "\"");
    }
    writer_append_text(writer, "]");
}

/* Matches SPEC_V2 13.13 / contracts/v2/api/examples.json's "diagnostics"
 * example exactly: firmwareVersion, buildId, resetReason, uptimeMs, memory{},
 * usb{}, wifi{}, storage{}, send{}, subsystems[]. */
app_error_code_t web_adapter_build_diagnostics_json(const web_diagnostics_snapshot_t *snapshot,
                                                    char *output, size_t output_size) {
    if (output != NULL && output_size > 0U) {
        output[0] = '\0';
    }
    if (snapshot == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (snapshot->blob_scan.invalid_names_truncated ||
        snapshot->blob_scan.reported_invalid_name_count != snapshot->blob_scan.invalid_name_count ||
        snapshot->blob_scan.temporary_files_truncated ||
        snapshot->blob_scan.reported_temporary_file_count !=
            snapshot->blob_scan.temporary_file_count) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    json_writer_t writer = {.buffer = output, .capacity = output_size};
    writer_append_text(&writer, "{\"firmwareVersion\":\"");
    writer_append_escaped(&writer, snapshot->firmware_version);
    writer_append_text(&writer, "\",\"buildId\":\"");
    writer_append_escaped(&writer, snapshot->build_id);
    writer_append_text(&writer, "\",\"resetReason\":\"");
    writer_append_escaped(&writer, snapshot->reset_reason);
    writer_append_text(&writer, "\",\"uptimeMs\":");
    append_uint64(&writer, snapshot->uptime_ms);

    writer_append_text(&writer, ",\"memory\":{\"freeHeapBytes\":");
    append_uint64(&writer, snapshot->free_heap_bytes);
    writer_append_text(&writer, ",\"minimumFreeHeapBytes\":");
    append_uint64(&writer, snapshot->minimum_free_heap_bytes);
    writer_append_text(&writer, ",\"largestFreeBlockBytes\":");
    append_uint64(&writer, snapshot->largest_free_block_bytes);
    writer_append_text(&writer, "}");

    writer_append_text(&writer, ",\"usb\":{\"state\":\"");
    writer_append_escaped(&writer, snapshot->usb_state);
    writer_append_text(&writer, "\"}");

    writer_append_text(&writer, ",\"wifi\":{\"accessPointState\":\"");
    writer_append_escaped(&writer, snapshot->access_point_state);
    writer_append_text(&writer, "\",\"stationState\":\"");
    writer_append_escaped(&writer, snapshot->station_state);
    writer_append_text(&writer, "\"}");

    writer_append_text(&writer, ",\"storage\":{\"state\":\"");
    writer_append_escaped(&writer, snapshot->storage_state);
    writer_append_text(&writer, "\",\"webfsTotalBytes\":");
    append_uint64(&writer, (uint64_t)snapshot->webfs.total_bytes);
    writer_append_text(&writer, ",\"webfsUsedBytes\":");
    append_uint64(&writer, (uint64_t)snapshot->webfs.used_bytes);
    writer_append_text(&writer, ",\"userdataTotalBytes\":");
    append_uint64(&writer, (uint64_t)snapshot->userdata.total_bytes);
    writer_append_text(&writer, ",\"userdataUsedBytes\":");
    append_uint64(&writer, (uint64_t)snapshot->userdata.used_bytes);
    writer_append_text(&writer, ",\"blobCount\":");
    append_uint64(&writer, (uint64_t)snapshot->blob_scan.blob_count);
    writer_append_text(&writer, ",\"invalidNames\":");
    append_string_array(&writer, snapshot->blob_scan.invalid_names,
                        snapshot->blob_scan.reported_invalid_name_count);
    writer_append_text(&writer, ",\"temporaryFiles\":");
    append_string_array(&writer, snapshot->blob_scan.temporary_files,
                        snapshot->blob_scan.reported_temporary_file_count);
    writer_append_text(&writer, "}");

    writer_append_text(&writer, ",\"send\":{\"present\":");
    writer_append_text(&writer, snapshot->send_present ? "true" : "false");
    if (snapshot->send_present && snapshot->send_state != NULL) {
        writer_append_text(&writer, ",\"state\":\"");
        writer_append_escaped(&writer, snapshot->send_state);
        writer_append_text(&writer, "\"");
    } else {
        writer_append_text(&writer, ",\"state\":null");
    }
    writer_append_text(&writer, "}");

    writer_append_text(&writer, ",\"subsystems\":[");
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
