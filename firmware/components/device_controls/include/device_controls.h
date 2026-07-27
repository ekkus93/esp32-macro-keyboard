#ifndef DEVICE_CONTROLS_H
#define DEVICE_CONTROLS_H

#include <stdbool.h>

#include "app_error.h"

typedef enum {
    DEVICE_INDICATOR_BOOTING = 0,
    DEVICE_INDICATOR_READY,
    DEVICE_INDICATOR_EXECUTING,
    DEVICE_INDICATOR_DEGRADED,
    DEVICE_INDICATOR_FATAL
} device_indicator_state_t;

typedef struct {
    app_error_code_t last_error;
    app_error_code_t cleanup_error;
    app_error_code_t last_confirmation_error;
    app_error_code_t last_cancel_error;
    bool task_running;
    bool indicator_output_failed;
    bool confirmation_signal_failed;
    bool cancel_request_failed;
    bool gpio_read_failed;
    bool gpio_configuration_failed;
    bool task_start_failed;
    bool task_stop_failed;
} device_controls_health_t;

app_error_code_t device_controls_init(void);
app_error_code_t device_controls_deinit(void);
void device_controls_set_indicator(device_indicator_state_t state);
app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms);
device_controls_health_t device_controls_get_health(void);

#endif
