#include "web_server_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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
#include "storage_health.h"
#include "usb_health.h"
#include "web_api_handler_common.h"
#include "web_api_response.h"
#include "web_diagnostics.h"
#include "web_http_status.h"
#include "web_server_adapter.h"
#include "wifi_ap.h"

#define WEB_DIAGNOSTICS_RESPONSE_MAX_BYTES 1536U
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

static web_diagnostics_snapshot_t collect_diagnostics(void) {
    web_diagnostics_snapshot_t snapshot = {0};
    (void)esp_app_get_elf_sha256(snapshot.build_id, sizeof(snapshot.build_id));
    const esp_app_desc_t *description = esp_app_get_description();
    (void)snprintf(snapshot.firmware_version, sizeof(snapshot.firmware_version), "%s",
                   description->version);
    snapshot.schema_version = APP_SCHEMA_VERSION;
    snapshot.reset_reason = reset_reason_string(esp_reset_reason());
    snapshot.uptime_ms = (uint64_t)(esp_timer_get_time() / MICROSECONDS_PER_MILLISECOND);
    snapshot.free_heap_bytes = esp_get_free_heap_size();
    snapshot.min_free_heap_bytes = esp_get_minimum_free_heap_size();
    snapshot.controls_stack_high_water_mark = device_controls_stack_high_water_mark();
    snapshot.executor_stack_high_water_mark = macro_executor_stack_high_water_mark();
    fill_capacity(STORAGE_WEB_PARTITION, &snapshot.webfs);
    fill_capacity(STORAGE_DATA_PARTITION, &snapshot.userdata);
    snapshot.execution_state = execution_state_string(macro_executor_get_status().state);
    fill_subsystems(snapshot.subsystems);
    return snapshot;
}

app_error_code_t web_diagnostics_handle(web_api_response_t *response) {
    const web_diagnostics_snapshot_t snapshot = collect_diagnostics();
    char body[WEB_DIAGNOSTICS_RESPONSE_MAX_BYTES];
    const app_error_code_t result =
        web_adapter_build_diagnostics_json(&snapshot, body, sizeof(body));
    if (result != APP_ERROR_NONE) {
        return web_api_handler_error(response, result, "diagnostics unavailable", NULL);
    }
    return web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, body);
}
