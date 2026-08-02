#ifndef DEVICE_CONTROLS_LOGIC_H
#define DEVICE_CONTROLS_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#include "device_controls.h"

/* This device has no buttons. It never did: the confirm and cancel buttons were
 * specified for hardware that no board here exposes (cancel was GPIO4), and the
 * only thing they ever did is now available as the `confirm` and `cancel`
 * serial-console commands. What remains of this component is the status
 * indicator and the confirmation signal those commands drive. */

/* Inputs to the indicator blink computation: the current indicator state and
 * how long (ms) it has been active. */
typedef struct {
    device_indicator_state_t state;
    uint32_t elapsed_ms;
} device_indicator_phase_t;

typedef enum { DEVICE_CONTROLS_PIN_STATUS = 0 } device_controls_pin_t;

typedef enum {
    DEVICE_CONTROLS_RESOURCE_CONFIRM_SIGNAL = 0,
    DEVICE_CONTROLS_RESOURCE_STOPPED_SIGNAL,
    DEVICE_CONTROLS_RESOURCE_STATUS_PIN
} device_controls_resource_t;

typedef enum {
    DEVICE_CONTROLS_FAILURE_GENERIC = 0,
    DEVICE_CONTROLS_FAILURE_INDICATOR,
    DEVICE_CONTROLS_FAILURE_CONFIRMATION,
    DEVICE_CONTROLS_FAILURE_GPIO_CONFIGURATION,
    DEVICE_CONTROLS_FAILURE_TASK_START,
    DEVICE_CONTROLS_FAILURE_TASK_STOP
} device_controls_failure_t;

typedef struct {
    void *context;
    bool (*lock)(void *context);
    bool (*unlock)(void *context);
    bool (*signal_confirmation)(void *context);
    bool (*signal_stopped)(void *context);
    app_error_code_t (*write_indicator)(void *context, bool enabled);
    void (*request_stop)(void *context);
    void (*clear_stop_request)(void *context);
    app_error_code_t (*wait_stopped)(void *context, uint32_t timeout_ms);
    app_error_code_t (*set_safe_output)(void *context);
    app_error_code_t (*reset_pin)(void *context, device_controls_pin_t pin);
    void (*delete_confirmation_signal)(void *context);
    void (*delete_stopped_signal)(void *context);
} device_controls_ops_t;

typedef struct {
    bool stop_requested;
    uint32_t elapsed_ms;
} device_controls_poll_input_t;

typedef struct {
    app_error_code_t error;
    bool continue_running;
} device_controls_poll_result_t;

typedef struct {
    device_controls_ops_t operations;
    device_indicator_state_t indicator_state;
    device_controls_health_t health;
    bool initialized;
    bool task_stop_confirmed;
    bool confirmation_signal_owned;
    bool stopped_signal_owned;
    bool status_pin_owned;
} device_controls_engine_t;

bool device_controls_indicator_on(device_indicator_phase_t phase);
app_error_code_t device_controls_engine_init(device_controls_engine_t *engine,
                                             const device_controls_ops_t *operations);
app_error_code_t device_controls_engine_acquire_resource(device_controls_engine_t *engine,
                                                         device_controls_resource_t resource);
app_error_code_t device_controls_engine_record_failure(device_controls_engine_t *engine,
                                                       app_error_code_t error,
                                                       device_controls_failure_t failure,
                                                       bool cleanup_failure);
app_error_code_t device_controls_engine_task_started(device_controls_engine_t *engine);
device_controls_poll_result_t device_controls_engine_poll(device_controls_engine_t *engine,
                                                          device_controls_poll_input_t input);
app_error_code_t device_controls_engine_set_indicator(device_controls_engine_t *engine,
                                                      device_indicator_state_t state);
device_controls_health_t device_controls_engine_get_health(device_controls_engine_t *engine);
app_error_code_t device_controls_engine_deinit(device_controls_engine_t *engine,
                                               uint32_t timeout_ms);
bool device_controls_engine_owns_resources(const device_controls_engine_t *engine);
bool device_controls_engine_task_stop_confirmed(const device_controls_engine_t *engine);

#endif
