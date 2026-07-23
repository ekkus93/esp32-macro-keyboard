#ifndef MACRO_EXECUTOR_H
#define MACRO_EXECUTOR_H

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
    EXECUTION_FAILED
} execution_state_t;

typedef struct {
    app_uuid_t execution_id;
    app_uuid_t set_id;
    app_uuid_t macro_id;
    uint32_t macro_revision;
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
    macro_plan_t plan;
} macro_execution_request_t;

typedef struct {
    execution_state_t state;
    app_error_code_t error;
    app_error_code_t release_error;
    app_uuid_t execution_id;
    size_t action_index;
    size_t action_count;
} macro_execution_status_t;

app_error_code_t macro_executor_init(void);
app_error_code_t macro_executor_submit(macro_execution_request_t *request);
app_error_code_t macro_executor_cancel(void);
macro_execution_status_t macro_executor_get_status(void);

#endif
