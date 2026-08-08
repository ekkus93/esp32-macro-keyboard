#ifndef WEB_DIAGNOSTICS_H
#define WEB_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "subsystem_health.h"

#define WEB_DIAGNOSTICS_BUILD_ID_MAX_BYTES 40U
#define WEB_DIAGNOSTICS_VERSION_MAX_BYTES 32U
#define WEB_DIAGNOSTICS_SUBSYSTEM_COUNT 8U
#define WEB_DIAGNOSTICS_INVALID_NAME_MAX 8U
#define WEB_DIAGNOSTICS_INVALID_NAME_CAPACITY 256U

typedef struct {
    const char *name;
    subsystem_health_state_t state;
} web_diagnostics_subsystem_t;

typedef struct {
    bool ok;
    size_t total_bytes;
    size_t used_bytes;
} web_diagnostics_capacity_t;

typedef struct {
    size_t blob_count;
    size_t invalid_name_count;
    size_t reported_invalid_name_count;
    bool invalid_names_truncated;
    char invalid_names[WEB_DIAGNOSTICS_INVALID_NAME_MAX][WEB_DIAGNOSTICS_INVALID_NAME_CAPACITY];
    size_t temporary_file_count;
    size_t reported_temporary_file_count;
    bool temporary_files_truncated;
    char temporary_files[WEB_DIAGNOSTICS_INVALID_NAME_MAX][WEB_DIAGNOSTICS_INVALID_NAME_CAPACITY];
} web_diagnostics_blob_scan_t;

/* Matches the fixed schema in SPEC_V2 13.13 / contracts/v2/api/examples.json's
 * "diagnostics" example exactly. No "schemaVersion" or "stack" field: neither
 * appears in that checked-in contract, and CLAUDE.md/SPEC_V2 forbid adding
 * response fields the frozen contract does not define. */
typedef struct {
    char build_id[WEB_DIAGNOSTICS_BUILD_ID_MAX_BYTES];
    char firmware_version[WEB_DIAGNOSTICS_VERSION_MAX_BYTES];
    const char *reset_reason;
    uint64_t uptime_ms;
    uint64_t free_heap_bytes;
    uint64_t minimum_free_heap_bytes;
    uint64_t largest_free_block_bytes;
    const char *usb_state;
    const char *access_point_state;
    const char *station_state;
    const char *storage_state;
    web_diagnostics_capacity_t webfs;
    web_diagnostics_capacity_t userdata;
    web_diagnostics_blob_scan_t blob_scan;
    bool send_present;
    /* NULL (emitted as JSON null) unless send_present is true. */
    const char *send_state;
    web_diagnostics_subsystem_t subsystems[WEB_DIAGNOSTICS_SUBSYSTEM_COUNT];
} web_diagnostics_snapshot_t;

#endif
