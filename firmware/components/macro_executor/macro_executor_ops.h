#ifndef MACRO_EXECUTOR_OPS_H
#define MACRO_EXECUTOR_OPS_H

#include <stdbool.h>
#include <stdint.h>

#include "macro_executor.h"

typedef struct {
    void *context;
    bool (*lock)(void *context);
    bool (*unlock)(void *context);
    bool (*queue_send)(void *context, const macro_execution_request_t *request);
    void (*notify_executor)(void *context);
    uint32_t (*now_ms)(void *context);
    app_error_code_t (*wait_ms)(void *context, uint32_t milliseconds);
    bool (*usb_ready)(void *context);
    app_error_code_t (*usb_press)(void *context, uint8_t modifiers, uint8_t usage);
    app_error_code_t (*usb_release_all)(void *context);
    /* Called exactly when a release-all attempt fails. The engine keeps the
     * failure in status itself; this hook lets the platform persist the same
     * sanitized app_error_code_t into subsystem diagnostics without polling. */
    void (*record_release_failure)(void *context, app_error_code_t error);
    void (*plan_free)(macro_plan_t *plan);
} macro_executor_ops_t;

#endif
