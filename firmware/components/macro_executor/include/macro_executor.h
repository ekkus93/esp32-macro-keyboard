#ifndef MACRO_EXECUTOR_H
#define MACRO_EXECUTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_parser.h"

typedef enum {
    EXECUTION_IDLE = 0,
    EXECUTION_RUNNING,
    EXECUTION_COMPLETED,
    EXECUTION_CANCELLED,
    EXECUTION_FAILED,
    /* A run that exceeded its watchdog deadline. Distinct from EXECUTION_FAILED so
     * the API and frontend can report a timeout specifically (FIX1 §12.4). */
    EXECUTION_TIMED_OUT
} execution_state_t;

typedef struct {
    app_uuid_t execution_id;
    app_uuid_t set_id;
    app_uuid_t macro_id;
    uint32_t macro_revision;
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
    macro_plan_t plan;
    /* Set by macro_executor_engine_submit() from the injected monotonic clock
     * (the same ops.now_ms() the watchdog already uses), before the request is
     * queued - the "accepted" instant for FIX1 §12.4's observability metadata.
     * Carried through the request queue so macro_executor_engine_execute() can
     * copy it into the published status alongside its own started/completed
     * stamps. */
    uint32_t accepted_ms;
} macro_execution_request_t;

typedef struct {
    execution_state_t state;
    app_error_code_t error;
    app_error_code_t release_error;
    app_uuid_t execution_id;
    app_uuid_t set_id;
    app_uuid_t macro_id;
    uint32_t macro_revision;
    size_t action_index;
    size_t action_count;
    bool available;
    bool cancellation_requested;
    /* Monotonic milliseconds (ops.now_ms(), never wall-clock/RTC - this
     * device has no synchronized time source) for FIX1 §12.4's observability
     * metadata: when the request was accepted (queued), when execution
     * actually began (dequeued and the watchdog armed), and when it reached
     * any terminal state (success, cancellation, failure, or timeout alike).
     * All three are 0 until set; completed_ms - started_ms is a meaningful
     * duration for every terminal outcome, not just EXECUTION_COMPLETED. */
    uint32_t accepted_ms;
    uint32_t started_ms;
    uint32_t completed_ms;
    /* Redacted-by-construction summary of what the executor is doing right
     * now: the compiled action's type ("key"/"chord"/"delay"), never the key
     * usage code, modifiers, or any macro content. "none" outside of an
     * in-flight action (idle, or any terminal state). Always one of this
     * fixed vocabulary, matching execution_state_string()'s convention -
     * never NULL. */
    const char *current_action;
} macro_execution_status_t;

app_error_code_t macro_executor_init(void);
app_error_code_t macro_executor_deinit(void);
app_error_code_t macro_executor_submit(macro_execution_request_t *request);
app_error_code_t macro_executor_cancel(void);
macro_execution_status_t macro_executor_get_status(void);

/* Executor task stack high-water mark in words for Phase 19 diagnostics (FIX1
 * handoff §7.1), or 0 when the task isn't running. Not host-testable (reads
 * FreeRTOS state directly); the diagnostics aggregator reaches it through an
 * injected ops seam. */
size_t macro_executor_stack_high_water_mark(void);

#endif
