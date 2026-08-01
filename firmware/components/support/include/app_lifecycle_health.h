#ifndef APP_LIFECYCLE_HEALTH_H
#define APP_LIFECYCLE_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* App-lifecycle health for Phase 19 diagnostics (FIX1 handoff §7.1). Lives in
 * `support` (rather than `app_core`, which records it) so components that
 * cannot depend on `app_core` - the orchestrator that depends on nearly
 * everything else, including web_server - can still read it; web_server's
 * diagnostics route is the reason this seam exists. Portable C with no
 * ESP-IDF dependency, so it is host-testable directly. state is always
 * derived from the other fields, never stored independently, so a cleanup
 * failure or incomplete cleanup always reports FAILED rather than HEALTHY. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} app_lifecycle_health_t;

void app_lifecycle_health_reset(void);
void app_lifecycle_health_record_stage_failure(app_error_code_t error);
void app_lifecycle_health_record_degraded(void);
void app_lifecycle_health_record_cleanup_failed(app_error_code_t cleanup_error,
                                                bool cleanup_incomplete);
app_lifecycle_health_t app_lifecycle_health_snapshot(void);

#endif
