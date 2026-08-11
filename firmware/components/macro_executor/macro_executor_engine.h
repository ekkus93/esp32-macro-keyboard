#ifndef MACRO_EXECUTOR_ENGINE_H
#define MACRO_EXECUTOR_ENGINE_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stdint.h>

#include "macro_executor.h"
#include "macro_executor_ops.h"

typedef struct {
    macro_executor_ops_t ops;
    macro_execution_status_t status;
    bool busy;
    bool cancellation_requested;
    /* SPEC_V2 §7.12: set by macro_executor_engine_confirm() while status.state is
     * EXECUTION_AWAITING_CONFIRMATION. Mirrors cancellation_requested's lock
     * discipline and is reset alongside it in reset_terminal_flags() and on
     * submission. */
    bool confirmed_requested;
    /* Fail-closed availability latch. It is set when terminal status/flag
     * publication becomes unreliable OR whenever a release-all attempt fails and
     * HID key state therefore cannot be proven safe (post-v2 H7). Submit/cancel/
     * confirm read it concurrently with the worker, so it is atomic. It clears
     * only during lifecycle re-initialization after the higher-level USB/executor
     * stack has been re-established. */
    atomic_bool unavailable;
    /* First release-all failure for the current fault epoch. Kept separately from
     * status so get_status() can still surface the cleanup failure if a later
     * terminal publish itself fails. Atomic for the same cross-task reason as the
     * availability latch. */
    atomic_int release_fault_error;
    /* Absolute watchdog deadline for the in-flight execution (ms). */
    uint32_t deadline;
} macro_executor_engine_t;

app_error_code_t macro_executor_engine_init(macro_executor_engine_t *engine,
                                            const macro_executor_ops_t *ops);
app_error_code_t macro_executor_engine_submit(macro_executor_engine_t *engine,
                                              macro_execution_request_t *request);
app_error_code_t macro_executor_engine_cancel(macro_executor_engine_t *engine);
app_error_code_t macro_executor_engine_confirm(macro_executor_engine_t *engine);
macro_execution_status_t macro_executor_engine_get_status(macro_executor_engine_t *engine);
app_error_code_t macro_executor_engine_execute(macro_executor_engine_t *engine,
                                               macro_execution_request_t *request);

#endif
