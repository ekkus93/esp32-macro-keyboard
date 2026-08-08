#ifndef MACRO_EXECUTOR_ENGINE_H
#define MACRO_EXECUTOR_ENGINE_H

#include <stdbool.h>
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
    /* Latched true when a terminal state could not be published or the busy flag
     * could not be cleared (FIX1 §12.3): the engine's observable state is then
     * unreliable, so it rejects new submissions until re-initialized rather than
     * appearing falsely idle. Set only on such a failure, cleared only by init. */
    bool unavailable;
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
