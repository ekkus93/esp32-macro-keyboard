#ifndef APP_CORE_H
#define APP_CORE_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* App-lifecycle health for Phase 19 diagnostics (FIX1 handoff §7.1). state is
 * always derived from the other fields (see app_core_health.c), never stored
 * independently, so a cleanup failure or incomplete cleanup always reports
 * FAILED rather than HEALTHY. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} app_lifecycle_health_t;

app_error_code_t app_core_start(void);
app_lifecycle_health_t app_core_get_health(void);

#endif
