#include "web_server_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_lifecycle_health.h"
#include "auth_health.h"
#include "device_controls.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "executor_health.h"
#include "http_health.h"
#include "macro_executor.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_blob.h"
#include "storage_health.h"
#include "usb_health.h"
#include "web_api_handler_common.h"
#include "web_api_response.h"
#include "web_diagnostics.h"
#include "web_http_status.h"
#include "web_server_adapter.h"
#include "wifi_ap.h"

#define WEB_DIAGNOSTICS_RESPONSE_MAX_BYTES 16384U
#define MICROSECONDS_PER_MILLISECOND 1000U

static const char *reset_reason_string(esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_POWERON:
        return "power-on";
    case ESP_RST_EXT:
        return "external-pin";
    case ESP_RST_SW:
        return "software";
    case ESP_RST_PANIC:
        return "panic";
    case ESP_RST_INT_WDT:
        return "interrupt-watchdog";
    case ESP_RST_TASK_WDT:
        return "task-watchdog";
    case ESP_RST_WDT:
        return "other-watchdog";
    case ESP_RST_DEEPSLEEP:
        return "deep-sleep-wake";
    case ESP_RST_BROWNOUT:
        return "brownout";
    case ESP_RST_SDIO:
        return "sdio";
    case ESP_RST_USB:
        return "usb";
    case ESP_RST_JTAG:
        return "jtag";
    case ESP_RST_EFUSE:
        return "efuse-error";
    case ESP_RST_PWR_GLITCH:
        return "power-glitch";
    case ESP_RST_CPU_LOCKUP:
        return "cpu-lockup";
    case ESP_RST_UNKNOWN:
    default:
        return "unknown";
    }
}

static void fill_capacity(const char *partition_label, web_diagnostics_capacity_t *out_capacity) {
    size_t total_bytes = 0U;
    size_t used_bytes = 0U;
    const app_error_code_t result =
        storage_partition_capacity(partition_label, &total_bytes, &used_bytes);
    *out_capacity = (web_diagnostics_capacity_t){
        .ok = result == APP_ERROR_NONE,
        .total_bytes = result == APP_ERROR_NONE ? total_bytes : 0U,
        .used_bytes = result == APP_ERROR_NONE ? used_bytes : 0U,
    };
}

static app_error_code_t copy_blob_scan(web_diagnostics_blob_scan_t *out_blob_scan,
                                       const storage_blob_diagnostics_t *storage_diagnostics) {
    if (storage_diagnostics->invalid_names_truncated ||
        storage_diagnostics->reported_invalid_name_count !=
            storage_diagnostics->summary.invalid_name_count ||
        storage_diagnostics->reported_invalid_name_count > WEB_DIAGNOSTICS_INVALID_NAME_MAX ||
        storage_diagnostics->temporary_files_truncated ||
        storage_diagnostics->reported_temporary_file_count !=
            storage_diagnostics->summary.temporary_file_count ||
        storage_diagnostics->reported_temporary_file_count > WEB_DIAGNOSTICS_INVALID_NAME_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    out_blob_scan->blob_count = storage_diagnostics->summary.valid_count;
    out_blob_scan->invalid_name_count = storage_diagnostics->summary.invalid_name_count;
    out_blob_scan->reported_invalid_name_count = storage_diagnostics->reported_invalid_name_count;
    for (size_t index = 0U; index < storage_diagnostics->reported_invalid_name_count; ++index) {
        const size_t length = strlen(storage_diagnostics->invalid_names[index]);
        if (length >= WEB_DIAGNOSTICS_INVALID_NAME_CAPACITY) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        memcpy(out_blob_scan->invalid_names[index], storage_diagnostics->invalid_names[index],
               length + 1U);
    }
    out_blob_scan->temporary_file_count = storage_diagnostics->summary.temporary_file_count;
    out_blob_scan->reported_temporary_file_count =
        storage_diagnostics->reported_temporary_file_count;
    for (size_t index = 0U; index < storage_diagnostics->reported_temporary_file_count; ++index) {
        const size_t length = strlen(storage_diagnostics->temporary_files[index]);
        if (length >= WEB_DIAGNOSTICS_INVALID_NAME_CAPACITY) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        memcpy(out_blob_scan->temporary_files[index], storage_diagnostics->temporary_files[index],
               length + 1U);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t fill_blob_scan(web_diagnostics_blob_scan_t *out_blob_scan) {
    if (out_blob_scan == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_blob_scan = (web_diagnostics_blob_scan_t){0};

    storage_blob_diagnostics_t *storage_diagnostics = malloc(sizeof(*storage_diagnostics));
    if (storage_diagnostics == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = storage_blob_collect_diagnostics(storage_diagnostics);
    if (result == APP_ERROR_NONE) {
        result = copy_blob_scan(out_blob_scan, storage_diagnostics);
    }
    free(storage_diagnostics);
    return result;
}

static void fill_subsystems(web_diagnostics_subsystem_t *out_subsystems) {
    size_t index = 0U;
    out_subsystems[index++] = (web_diagnostics_subsystem_t){
        .name = "app_core", .state = app_lifecycle_health_snapshot().state};
    out_subsystems[index++] =
        (web_diagnostics_subsystem_t){.name = "storage", .state = storage_health_snapshot().state};
    out_subsystems[index++] =
        (web_diagnostics_subsystem_t){.name = "auth", .state = auth_health_snapshot().state};
    out_subsystems[index++] =
        (web_diagnostics_subsystem_t){.name = "usb", .state = usb_health_snapshot().state};
    out_subsystems[index++] = (web_diagnostics_subsystem_t){
        .name = "executor", .state = executor_health_snapshot().state};
    out_subsystems[index++] = (web_diagnostics_subsystem_t){
        .name = "controls",
        .state = device_controls_health_derive_state(device_controls_get_health())};
    out_subsystems[index++] = (web_diagnostics_subsystem_t){
        .name = "wifi", .state = wifi_ap_health_derive_state(wifi_ap_get_status())};
    out_subsystems[index++] =
        (web_diagnostics_subsystem_t){.name = "http", .state = http_health_snapshot().state};
}

static app_error_code_t collect_diagnostics(web_diagnostics_snapshot_t *out_snapshot) {
    if (out_snapshot == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_snapshot = (web_diagnostics_snapshot_t){0};
    (void)esp_app_get_elf_sha256(out_snapshot->build_id, sizeof(out_snapshot->build_id));
    const esp_app_desc_t *description = esp_app_get_description();
    (void)snprintf(out_snapshot->firmware_version, sizeof(out_snapshot->firmware_version), "%s",
                   description->version);
    out_snapshot->schema_version = APP_SCHEMA_VERSION;
    out_snapshot->reset_reason = reset_reason_string(esp_reset_reason());
    out_snapshot->uptime_ms = (uint64_t)(esp_timer_get_time() / MICROSECONDS_PER_MILLISECOND);
    out_snapshot->free_heap_bytes = esp_get_free_heap_size();
    out_snapshot->min_free_heap_bytes = esp_get_minimum_free_heap_size();
    out_snapshot->controls_stack_high_water_mark = device_controls_stack_high_water_mark();
    out_snapshot->executor_stack_high_water_mark = macro_executor_stack_high_water_mark();
    fill_capacity(STORAGE_WEB_PARTITION, &out_snapshot->webfs);
    fill_capacity(STORAGE_DATA_PARTITION, &out_snapshot->userdata);
    const app_error_code_t blob_result = fill_blob_scan(&out_snapshot->blob_scan);
    if (blob_result != APP_ERROR_NONE) {
        return blob_result;
    }
    out_snapshot->execution_state = execution_state_string(macro_executor_get_status().state);
    fill_subsystems(out_snapshot->subsystems);
    return APP_ERROR_NONE;
}

app_error_code_t web_diagnostics_handle(web_api_response_t *response) {
    web_diagnostics_snapshot_t *snapshot = malloc(sizeof(*snapshot));
    if (snapshot == NULL) {
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "diagnostics unavailable", NULL);
    }
    const app_error_code_t collect_result = collect_diagnostics(snapshot);
    if (collect_result != APP_ERROR_NONE) {
        free(snapshot);
        return web_api_handler_error(response, collect_result, "diagnostics unavailable", NULL);
    }

    char *body = malloc(WEB_DIAGNOSTICS_RESPONSE_MAX_BYTES);
    if (body == NULL) {
        free(snapshot);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "diagnostics unavailable", NULL);
    }
    const app_error_code_t build_result =
        web_adapter_build_diagnostics_json(snapshot, body, WEB_DIAGNOSTICS_RESPONSE_MAX_BYTES);
    free(snapshot);
    if (build_result != APP_ERROR_NONE) {
        free(body);
        return web_api_handler_error(response, build_result, "diagnostics unavailable", NULL);
    }
    const app_error_code_t response_result =
        web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, body);
    free(body);
    return response_result;
}
