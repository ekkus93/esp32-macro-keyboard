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
    /* usages/usage_count carry a MACRO_ACTION_KEY or MACRO_ACTION_CHORD
     * action's simultaneous non-modifier keys (0 for a standalone modifier
     * tap, up to MACRO_ACTION_USAGES_MAX for a [...] group) -- one HID report,
     * pressed together. */
    app_error_code_t (*usb_press)(void *context, uint8_t modifiers, const uint8_t *usages,
                                  uint8_t usage_count);
    app_error_code_t (*usb_release_all)(void *context);
    /* Called exactly when a release-all attempt fails. The engine keeps the
     * failure in status itself; this hook lets the platform persist the same
     * sanitized app_error_code_t into subsystem diagnostics without polling. */
    void (*record_release_failure)(void *context, app_error_code_t error);
    void (*plan_free)(macro_plan_t *plan);
} macro_executor_ops_t;

#endif
