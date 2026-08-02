#ifndef WEB_DIAGNOSTICS_H
#define WEB_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "subsystem_health.h"

/* Redacted diagnostics snapshot for the `/api/v1/diagnostics` route (FIX1 TODO
 * §19.2). Every field here is a plain, already-resolved value - never a secret,
 * token, credential, or macro source - so the shape itself enforces the
 * redaction requirement; nothing in this struct needs scrubbing before it is
 * JSON-encoded. Populated in web_server_diagnostics.c (ESP-IDF-coupled) and
 * consumed by web_adapter_build_diagnostics_json (portable, host-testable). */

#define WEB_DIAGNOSTICS_BUILD_ID_MAX_BYTES 40U
#define WEB_DIAGNOSTICS_VERSION_MAX_BYTES 32U
#define WEB_DIAGNOSTICS_SUBSYSTEM_COUNT 9U

typedef struct {
    const char *name;
    subsystem_health_state_t state;
} web_diagnostics_subsystem_t;

/* `ok` is false when the underlying query failed (e.g. the LittleFS partition
 * info call returned an error); the numeric fields are then left at 0 rather
 * than omitted, so the JSON shape - and its worst-case length - never changes
 * based on success or failure. */
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
    const char *execution_state;
    web_diagnostics_subsystem_t subsystems[WEB_DIAGNOSTICS_SUBSYSTEM_COUNT];
} web_diagnostics_snapshot_t;

#endif
