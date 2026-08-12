#include "web_server_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_app_desc.h"
#include "esp_timer.h"
#include "factory_reset_state.h"
#include "macro_executor.h"
#include "storage.h"
#include "storage_blob.h"
#include "usb_keyboard.h"
#include "web_server_adapter.h"
#include "web_status_limits.h"
#include "wifi_ap.h"

#define STATUS_BUILD_ID_MAX_BYTES 40U
#define STATUS_VERSION_MAX_BYTES 32U
#define MICROSECONDS_PER_MILLISECOND 1000U

static const char *access_point_state_string(wifi_ap_state_t state) {
    switch (state) {
    case WIFI_AP_STOPPED:
        return "stopped";
    case WIFI_AP_STARTING:
        return "starting";
    case WIFI_AP_READY:
        return "running";
    case WIFI_AP_ERROR:
    default:
        return "error";
    }
}

/* No live station-connection tracking exists yet anywhere in the firmware
 * (station join is attempted once, blockingly, at boot -- see app_core.c --
 * and its outcome is not retained); adding that is Wi-Fi/reset semantics
 * (V2-044) scope, not this HTTP route's. This reports the one thing that is
 * knowable without it: whether a station network is configured at all. */
static const char *station_state_string(bool configured) {
    return configured ? "unknown" : "disabled";
}

static app_error_code_t blob_count(uint32_t *out_count) {
    *out_count = 0U;
    storage_blob_scan_summary_t summary = {0};
    const storage_blob_scan_observer_t observer = {
        .context = NULL,
        .visit_entry = NULL,
        .visit_invalid_name = NULL,
    };
    const app_error_code_t result = storage_blob_list(&observer, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    *out_count = (uint32_t)summary.valid_count;
    return APP_ERROR_NONE;
}

esp_err_t status_handler(httpd_req_t *request) {
    char session_token[AUTH_TOKEN_HEX_BYTES] = {0};
    const app_error_code_t auth_result = authorize_mutation(request, session_token);
    memset(session_token, 0, sizeof(session_token));
    if (auth_result != APP_ERROR_NONE) {
        return send_error(request, "401 Unauthorized", auth_result, "authentication required");
    }

    factory_reset_state_t reset_state = FACTORY_RESET_STATE_NONE;
    const app_error_code_t reset_result = factory_reset_state_read(&reset_state);
    if (reset_result != APP_ERROR_NONE || reset_state == FACTORY_RESET_STATE_PENDING) {
        return send_error(request, "503 Service Unavailable",
                          reset_result == APP_ERROR_NONE ? APP_ERROR_RESET_RECOVERY_REQUIRED
                                                         : reset_result,
                          "factory reset recovery in progress");
    }

    app_v2_device_settings_t settings = {0};
    if (device_settings_read(&settings) != APP_ERROR_NONE) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_STORAGE_UNAVAILABLE,
                          "device settings unavailable");
    }

    char build_id[STATUS_BUILD_ID_MAX_BYTES] = {0};
    (void)esp_app_get_elf_sha256(build_id, sizeof(build_id));
    char firmware_version[STATUS_VERSION_MAX_BYTES] = {0};
    (void)snprintf(firmware_version, sizeof(firmware_version), "%s",
                   esp_app_get_description()->version);

    const wifi_ap_status_t wifi = wifi_ap_get_status();
    const macro_execution_status_t execution = macro_executor_get_status();
    const char *send_state =
        execution.state == EXECUTION_IDLE ? NULL : execution_state_string(execution.state);

    size_t storage_total_bytes = 0U;
    size_t storage_used_bytes = 0U;
    const storage_mount_state_t mount_state = storage_mount_state();
    const app_error_code_t capacity_result = storage_partition_capacity(
        STORAGE_DATA_PARTITION, &storage_total_bytes, &storage_used_bytes);
    if (capacity_result != APP_ERROR_NONE || storage_used_bytes > storage_total_bytes) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_STORAGE_UNAVAILABLE,
                          "storage capacity unavailable");
    }
    uint32_t blob_count_value = 0U;
    if (blob_count(&blob_count_value) != APP_ERROR_NONE) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_STORAGE_UNAVAILABLE,
                          "blob listing unavailable");
    }

    const web_status_snapshot_t snapshot = {
        .provisioned = settings.provisioned,
        .device_name = settings.device_name,
        .firmware_version = firmware_version,
        .build_id = build_id,
        .uptime_ms = (uint64_t)(esp_timer_get_time() / MICROSECONDS_PER_MILLISECOND),
        .usb_state = usb_state_string(usb_keyboard_get_state()),
        .access_point_state = access_point_state_string(wifi.state),
        .access_point_ssid = settings.ap_ssid,
        .access_point_client_count = (uint32_t)wifi.client_count,
        .station_configured = settings.station_configured,
        .station_state = station_state_string(settings.station_configured),
        .station_ssid = settings.station_configured ? settings.station_ssid : NULL,
        .station_ipv4 = NULL,
        .storage_state = mount_state.data_mounted ? "ready" : "unavailable",
        .storage_total_bytes = (uint64_t)storage_total_bytes,
        .storage_used_bytes = (uint64_t)storage_used_bytes,
        .storage_remaining_bytes = (uint64_t)(storage_total_bytes - storage_used_bytes),
        .storage_blob_count = blob_count_value,
        .send_present = execution.state != EXECUTION_IDLE,
        .send_state = send_state,
    };

    char *json = NULL;
    if (web_status_response_json(&snapshot, &json) != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "status response encoding failed");
    }
    const esp_err_t result = send_json(request, json, "200 OK");
    free(json);
    return result;
}

esp_err_t limits_handler(httpd_req_t *request) {
    char session_token[AUTH_TOKEN_HEX_BYTES] = {0};
    const app_error_code_t auth_result = authorize_mutation(request, session_token);
    memset(session_token, 0, sizeof(session_token));
    if (auth_result != APP_ERROR_NONE) {
        return send_error(request, "401 Unauthorized", auth_result, "authentication required");
    }
    char response[512U];
    const app_error_code_t result = web_adapter_build_limits_json(response, sizeof(response));
    if (result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", result, "response encoding failed");
    }
    return send_json(request, response, "200 OK");
}
