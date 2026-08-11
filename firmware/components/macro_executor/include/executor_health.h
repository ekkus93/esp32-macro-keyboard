#ifndef EXECUTOR_HEALTH_H
#define EXECUTOR_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* Executor lifecycle health for Phase 19 diagnostics (FIX1 handoff §7.1),
 * distinct from a single execution's own result (macro_execution_status_t,
 * already surfaced separately as the current execution state). Updates and
 * snapshots share one FreeRTOS critical-section lock because an HTTP-stop
 * failure can leave diagnostics readers live while startup rollback continues
 * into executor teardown. Host tests map that lock to pthreads. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} executor_health_t;

/* Portable shutdown policy used by macro_executor.c. A worker stop that is not
 * confirmed is a fail-safe latch: submissions remain disabled until a later
 * deinit attempt confirms the worker has stopped. macro_executor_init() may
 * reset this state only after verifying no executor resources are still owned. */
typedef struct {
    volatile bool shutting_down;
    volatile bool stop_unconfirmed;
} executor_shutdown_state_t;

void executor_health_reset(void);
void executor_health_record_primary(app_error_code_t error);
void executor_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
executor_health_t executor_health_snapshot(void);

void executor_shutdown_state_reset(executor_shutdown_state_t *state);
void executor_shutdown_state_begin(executor_shutdown_state_t *state);
void executor_shutdown_state_complete(executor_shutdown_state_t *state, bool worker_stopped);
bool executor_shutdown_state_accepts_submissions(const executor_shutdown_state_t *state);
bool executor_shutdown_state_fault_latched(const executor_shutdown_state_t *state);

#endif
