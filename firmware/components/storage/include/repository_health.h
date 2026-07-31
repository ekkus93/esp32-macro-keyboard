#ifndef REPOSITORY_HEALTH_H
#define REPOSITORY_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* Repository (set-index/schema) health for Phase 19 diagnostics (FIX1 handoff
 * §7.1), distinct from storage_health.h's mount/recovery scope. Portable C
 * with no ESP-IDF dependency, so it is host-testable directly. Recorded from
 * app_core.c's existing repository_init/repository_deinit call sites. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} repository_health_t;

void repository_health_reset(void);
void repository_health_record_primary(app_error_code_t error);
void repository_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
repository_health_t repository_health_snapshot(void);

#endif
