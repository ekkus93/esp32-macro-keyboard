#ifndef DEVICE_CONTROLS_H
#define DEVICE_CONTROLS_H

#include "app_error.h"

typedef enum {
    DEVICE_INDICATOR_BOOTING = 0,
    DEVICE_INDICATOR_READY,
    DEVICE_INDICATOR_EXECUTING,
    DEVICE_INDICATOR_DEGRADED,
    DEVICE_INDICATOR_FATAL
} device_indicator_state_t;

app_error_code_t device_controls_init(void);
void device_controls_set_indicator(device_indicator_state_t state);
app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms);

#endif
