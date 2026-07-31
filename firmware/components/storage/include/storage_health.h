#ifndef STORAGE_HEALTH_H
#define STORAGE_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* Storage mount/recovery health for Phase 19 diagnostics (FIX1 handoff §7.1).
 * Portable C with no ESP-IDF dependency, so it is host-testable directly.
 * Recorded by the mount/recovery/unmount call sites in app_core.c (today the
 * only orchestrator of these calls) rather than duplicating that sequencing
 * here. state is always derived fresh from the recorded fields: never
 * HEALTHY while cleanup is incomplete or a cleanup error is present. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} storage_health_t;

void storage_health_reset(void);
void storage_health_record_primary(app_error_code_t error);
void storage_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
storage_health_t storage_health_snapshot(void);

#endif
