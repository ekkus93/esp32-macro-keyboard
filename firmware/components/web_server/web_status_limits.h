#ifndef WEB_STATUS_LIMITS_H
#define WEB_STATUS_LIMITS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"

/* Pure JSON builder for GET /api/v1/status (SPEC_V2 13.6). No
 * esp_http_server dependency, so this module is host-testable; the httpd
 * adapter (web_server_status_limits.c) collects a snapshot from the live
 * subsystems and calls web_status_response_json(). All string fields are
 * borrowed (not copied) -- the caller must keep them alive until this
 * function returns. */
typedef struct {
    bool provisioned;
    const char *device_name;
    const char *firmware_version;
    const char *build_id;
    uint64_t uptime_ms;

    const char *usb_state;

    const char *access_point_state;
    const char *access_point_ssid;
    uint32_t access_point_client_count;

    bool station_configured;
    const char *station_state;
    /* NULL when the station is not configured (emitted as JSON null). */
    const char *station_ssid;
    /* NULL when no IPv4 lease is known (emitted as JSON null). */
    const char *station_ipv4;

    const char *storage_state;
    uint64_t storage_total_bytes;
    uint64_t storage_used_bytes;
    uint64_t storage_remaining_bytes;
    uint32_t storage_blob_count;

    bool send_present;
    /* NULL (emitted as JSON null) unless send_present is true. */
    const char *send_state;
} web_status_snapshot_t;

/* *out_json is heap-allocated (cJSON_free()/web_api_handler_json_free()). */
app_error_code_t web_status_response_json(const web_status_snapshot_t *snapshot, char **out_json);

#endif
