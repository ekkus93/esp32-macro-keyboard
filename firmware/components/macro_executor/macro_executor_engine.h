#ifndef MACRO_EXECUTOR_ENGINE_H
#define MACRO_EXECUTOR_ENGINE_H

#include <stdbool.h>

#include "macro_executor.h"
#include "macro_executor_ops.h"

typedef struct {
    macro_executor_ops_t ops;
    macro_execution_status_t status;
    bool busy;
    bool cancellation_requested;
} macro_executor_engine_t;

app_error_code_t macro_executor_engine_init(macro_executor_engine_t *engine,
                                            const macro_executor_ops_t *ops);
app_error_code_t macro_executor_engine_submit(macro_executor_engine_t *engine,
                                              macro_execution_request_t *request);
app_error_code_t macro_executor_engine_cancel(macro_executor_engine_t *engine);
macro_execution_status_t macro_executor_engine_get_status(macro_executor_engine_t *engine);
app_error_code_t macro_executor_engine_execute(macro_executor_engine_t *engine,
                                               macro_execution_request_t *request);

#endif
