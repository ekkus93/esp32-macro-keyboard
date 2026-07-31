#ifndef AUTH_HEALTH_H
#define AUTH_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* Authentication health for Phase 19 diagnostics (FIX1 handoff §7.1). Portable
 * C with no ESP-IDF dependency, so it is host-testable directly. Recorded
 * from app_core.c's existing auth_init/auth_deinit call sites. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} auth_health_t;

void auth_health_reset(void);
void auth_health_record_primary(app_error_code_t error);
void auth_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
auth_health_t auth_health_snapshot(void);

#endif
