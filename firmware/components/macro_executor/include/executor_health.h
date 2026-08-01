#ifndef EXECUTOR_HEALTH_H
#define EXECUTOR_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* Executor lifecycle health for Phase 19 diagnostics (FIX1 handoff §7.1),
 * distinct from a single execution's own result (macro_execution_status_t,
 * already surfaced separately as the current execution state). Portable C
 * with no ESP-IDF dependency, so it is host-testable directly. Recorded from
 * app_core.c's existing executor_init/executor_deinit call sites. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} executor_health_t;

void executor_health_reset(void);
void executor_health_record_primary(app_error_code_t error);
void executor_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
executor_health_t executor_health_snapshot(void);

#endif
