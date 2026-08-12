#ifndef FACTORY_RESET_STATE_H
#define FACTORY_RESET_STATE_H

#include "app_error.h"

typedef enum {
    FACTORY_RESET_STATE_NONE = 0,
    FACTORY_RESET_STATE_PENDING = 1
} factory_reset_state_t;

/* H3 durable factory-reset journal. Each public operation opens the dedicated
 * NVS namespace for one bounded transaction and closes it before returning, so
 * the boot gate and runtime factory-reset path do not share a long-lived NVS
 * handle or lifecycle state. */
app_error_code_t factory_reset_state_read(factory_reset_state_t *out_state);
app_error_code_t factory_reset_state_mark_pending(void);
app_error_code_t factory_reset_state_clear(void);

#endif
