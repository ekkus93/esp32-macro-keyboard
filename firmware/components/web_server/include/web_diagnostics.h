#ifndef WEB_DIAGNOSTICS_H
#define WEB_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "storage_blob.h"
#include "subsystem_health.h"

#define WEB_DIAGNOSTICS_BUILD_ID_MAX_BYTES 40U
#define WEB_DIAGNOSTICS_VERSION_MAX_BYTES 32U
#define WEB_DIAGNOSTICS_SUBSYSTEM_COUNT 8U

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
    char build_id[WEB_DIAGNOSTICS_BUILD_ID_MAX_BYTES];
    char firmware_version[WEB_DIAGNOSTICS_VERSION_MAX_BYTES];
    uint32_t schema_version;
    const char *reset_reason;
    uint64_t uptime_ms;
    uint32_t free_heap_bytes;
    uint32_t min_free_heap_bytes;
    size_t controls_stack_high_water_mark;
    size_t executor_stack_high_water_mark;
    web_diagnostics_capacity_t webfs;
    web_diagnostics_capacity_t userdata;
    storage_blob_diagnostics_t blob_scan;
    const char *execution_state;
    web_diagnostics_subsystem_t subsystems[WEB_DIAGNOSTICS_SUBSYSTEM_COUNT];
} web_diagnostics_snapshot_t;

#endif
