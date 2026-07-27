#include "device_controls_logic.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "device_controls.h"

bool device_controls_level_is_pressed(int level, int active_level) {
    return (active_level == 0 || active_level == 1) && level == active_level;
}

bool device_controls_debounce_update(device_controls_debounce_t *button, bool sample) {
    if (button == NULL) {
        return false;
    }
    if (sample != button->candidate) {
        button->candidate = sample;
        button->candidate_count = 1U;
        return false;
    }
    if (button->candidate_count < DEVICE_CONTROLS_DEBOUNCE_SAMPLES) {
        ++button->candidate_count;
    }
    if (button->candidate_count == DEVICE_CONTROLS_DEBOUNCE_SAMPLES && button->stable != sample) {
        button->stable = sample;
        return sample;
    }
    return false;
}

/* Status-LED blink patterns as (period, on-time) in milliseconds. */
#define INDICATOR_BOOTING_PERIOD_MS 1000U
#define INDICATOR_BOOTING_ON_MS 250U
#define INDICATOR_EXECUTING_PERIOD_MS 200U
#define INDICATOR_EXECUTING_ON_MS 100U
#define INDICATOR_DEGRADED_PERIOD_MS 2000U
#define INDICATOR_DEGRADED_FIRST_ON_MS 250U
#define INDICATOR_DEGRADED_SECOND_ON_START_MS 500U
#define INDICATOR_DEGRADED_SECOND_ON_END_MS 750U
#define INDICATOR_FATAL_PERIOD_MS 500U
#define INDICATOR_FATAL_ON_MS 250U

bool device_controls_indicator_on(device_indicator_phase_t phase) {
    const device_indicator_state_t state = phase.state;
    const uint32_t elapsed_ms = phase.elapsed_ms;
    switch (state) {
    case DEVICE_INDICATOR_READY:
        return true;
    case DEVICE_INDICATOR_BOOTING:
        return (elapsed_ms % INDICATOR_BOOTING_PERIOD_MS) < INDICATOR_BOOTING_ON_MS;
    case DEVICE_INDICATOR_EXECUTING:
        return (elapsed_ms % INDICATOR_EXECUTING_PERIOD_MS) < INDICATOR_EXECUTING_ON_MS;
    case DEVICE_INDICATOR_DEGRADED: {
        const uint32_t position = elapsed_ms % INDICATOR_DEGRADED_PERIOD_MS;
        return position < INDICATOR_DEGRADED_FIRST_ON_MS ||
               (position >= INDICATOR_DEGRADED_SECOND_ON_START_MS &&
                position < INDICATOR_DEGRADED_SECOND_ON_END_MS);
    }
    case DEVICE_INDICATOR_FATAL:
        return (elapsed_ms % INDICATOR_FATAL_PERIOD_MS) < INDICATOR_FATAL_ON_MS;
    default:
        return false;
    }
}

static bool operations_valid(const device_controls_ops_t *operations) {
    return operations != NULL && operations->lock != NULL && operations->unlock != NULL &&
           operations->signal_confirmation != NULL && operations->signal_stopped != NULL &&
           operations->cancel_execution != NULL && operations->write_indicator != NULL &&
           operations->request_stop != NULL && operations->clear_stop_request != NULL &&
           operations->wait_stopped != NULL && operations->set_safe_output != NULL &&
           operations->reset_pin != NULL && operations->delete_confirmation_signal != NULL &&
           operations->delete_stopped_signal != NULL;
}

static void record_first_error(app_error_code_t error, app_error_code_t *first_error) {
    if (error != APP_ERROR_NONE && *first_error == APP_ERROR_NONE) {
        *first_error = error;
    }
}

static void set_failure_flag(device_controls_health_t *health, device_controls_failure_t failure,
                             app_error_code_t error) {
    switch (failure) {
    case DEVICE_CONTROLS_FAILURE_INDICATOR:
        health->indicator_output_failed = true;
        break;
    case DEVICE_CONTROLS_FAILURE_CONFIRMATION:
        health->confirmation_signal_failed = true;
        health->last_confirmation_error = error;
        break;
    case DEVICE_CONTROLS_FAILURE_CANCEL:
        health->cancel_request_failed = true;
        health->last_cancel_error = error;
        break;
    case DEVICE_CONTROLS_FAILURE_GPIO_READ:
        health->gpio_read_failed = true;
        break;
    case DEVICE_CONTROLS_FAILURE_GPIO_CONFIGURATION:
        health->gpio_configuration_failed = true;
        break;
    case DEVICE_CONTROLS_FAILURE_TASK_START:
        health->task_start_failed = true;
        break;
    case DEVICE_CONTROLS_FAILURE_TASK_STOP:
        health->task_stop_failed = true;
        break;
    case DEVICE_CONTROLS_FAILURE_GENERIC:
    default:
        break;
    }
}

app_error_code_t device_controls_engine_record_failure(device_controls_engine_t *engine,
                                                       app_error_code_t error,
                                                       device_controls_failure_t failure,
                                                       bool cleanup_failure) {
    if (engine == NULL || !engine->initialized || error == APP_ERROR_NONE) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!engine->operations.lock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    engine->health.last_error = error;
    if (cleanup_failure && engine->health.cleanup_error == APP_ERROR_NONE) {
        engine->health.cleanup_error = error;
    }
    set_failure_flag(&engine->health, failure, error);
    if (!engine->operations.unlock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    return error;
}

static app_error_code_t set_task_running(device_controls_engine_t *engine, bool task_running) {
    if (!engine->operations.lock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    engine->health.task_running = task_running;
    if (!engine->operations.unlock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t get_indicator_state(device_controls_engine_t *engine,
                                            device_indicator_state_t *out_state) {
    if (!engine->operations.lock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    *out_state = engine->indicator_state;
    if (!engine->operations.unlock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t record_runtime_failure(device_controls_engine_t *engine,
                                               app_error_code_t error,
                                               device_controls_failure_t failure) {
    const app_error_code_t health_result =
        device_controls_engine_record_failure(engine, error, failure, false);
    const app_error_code_t indicator_result =
        device_controls_engine_set_indicator(engine, DEVICE_INDICATOR_FATAL);
    if (health_result != error) {
        return health_result;
    }
    return indicator_result != APP_ERROR_NONE ? indicator_result : error;
}

static app_error_code_t stop_task(device_controls_engine_t *engine) {
    app_error_code_t result = set_task_running(engine, false);
    if (!engine->operations.signal_stopped(engine->operations.context)) {
        const app_error_code_t health_result = device_controls_engine_record_failure(
            engine, APP_ERROR_INTERNAL, DEVICE_CONTROLS_FAILURE_TASK_STOP, true);
        record_first_error(health_result, &result);
    }
    return result;
}

app_error_code_t device_controls_engine_init(device_controls_engine_t *engine,
                                             const device_controls_ops_t *operations) {
    if (engine == NULL || !operations_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(engine, 0, sizeof(*engine));
    engine->operations = *operations;
    engine->indicator_state = DEVICE_INDICATOR_BOOTING;
    engine->initialized = true;
    engine->task_stop_confirmed = true;
    return APP_ERROR_NONE;
}

app_error_code_t device_controls_engine_acquire_resource(device_controls_engine_t *engine,
                                                         device_controls_resource_t resource) {
    if (engine == NULL || !engine->initialized) {
        return APP_ERROR_CONFLICT;
    }
    switch (resource) {
    case DEVICE_CONTROLS_RESOURCE_CONFIRM_SIGNAL:
        engine->confirmation_signal_owned = true;
        break;
    case DEVICE_CONTROLS_RESOURCE_STOPPED_SIGNAL:
        engine->stopped_signal_owned = true;
        break;
    case DEVICE_CONTROLS_RESOURCE_CONFIRM_PIN:
        engine->confirm_pin_owned = true;
        break;
    case DEVICE_CONTROLS_RESOURCE_CANCEL_PIN:
        engine->cancel_pin_owned = true;
        break;
    case DEVICE_CONTROLS_RESOURCE_STATUS_PIN:
        engine->status_pin_owned = true;
        break;
    default:
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t device_controls_engine_task_started(device_controls_engine_t *engine) {
    if (engine == NULL || !engine->initialized) {
        return APP_ERROR_CONFLICT;
    }
    engine->task_stop_confirmed = false;
    return set_task_running(engine, true);
}

device_controls_poll_result_t device_controls_engine_poll(device_controls_engine_t *engine,
                                                          device_controls_poll_input_t input) {
    device_controls_poll_result_t result = {
        .error = APP_ERROR_NONE,
        .continue_running = false,
    };
    if (engine == NULL || !engine->initialized) {
        result.error = APP_ERROR_CONFLICT;
        return result;
    }
    if (input.stop_requested) {
        result.error = stop_task(engine);
        return result;
    }

    result.continue_running = true;
    if (input.gpio_read_error != APP_ERROR_NONE) {
        const app_error_code_t failure = record_runtime_failure(engine, input.gpio_read_error,
                                                                DEVICE_CONTROLS_FAILURE_GPIO_READ);
        record_first_error(failure, &result.error);
    } else {
        if (device_controls_debounce_update(&engine->confirmation, input.confirmation_pressed) &&
            !engine->operations.signal_confirmation(engine->operations.context)) {
            const app_error_code_t failure = record_runtime_failure(
                engine, APP_ERROR_INTERNAL, DEVICE_CONTROLS_FAILURE_CONFIRMATION);
            record_first_error(failure, &result.error);
        }
        if (device_controls_debounce_update(&engine->cancel, input.cancel_pressed)) {
            const app_error_code_t cancel =
                engine->operations.cancel_execution(engine->operations.context);
            if (cancel != APP_ERROR_NONE && cancel != APP_ERROR_NOT_FOUND) {
                const app_error_code_t failure =
                    record_runtime_failure(engine, cancel, DEVICE_CONTROLS_FAILURE_CANCEL);
                record_first_error(failure, &result.error);
            }
        }
    }

    device_indicator_state_t indicator = DEVICE_INDICATOR_FATAL;
    const app_error_code_t indicator_state_result = get_indicator_state(engine, &indicator);
    if (indicator_state_result != APP_ERROR_NONE) {
        record_first_error(indicator_state_result, &result.error);
        result.continue_running = false;
        return result;
    }
    const bool enabled = device_controls_indicator_on((device_indicator_phase_t){
        .state = indicator,
        .elapsed_ms = input.elapsed_ms,
    });
    const app_error_code_t write =
        engine->operations.write_indicator(engine->operations.context, enabled);
    if (write != APP_ERROR_NONE) {
        const app_error_code_t failure =
            record_runtime_failure(engine, write, DEVICE_CONTROLS_FAILURE_INDICATOR);
        record_first_error(failure, &result.error);
    }
    return result;
}

app_error_code_t device_controls_engine_set_indicator(device_controls_engine_t *engine,
                                                      device_indicator_state_t state) {
    if (engine == NULL || !engine->initialized) {
        return APP_ERROR_CONFLICT;
    }
    if (state < DEVICE_INDICATOR_BOOTING || state > DEVICE_INDICATOR_FATAL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!engine->operations.lock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    engine->indicator_state = state;
    if (!engine->operations.unlock(engine->operations.context)) {
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

device_controls_health_t device_controls_engine_get_health(device_controls_engine_t *engine) {
    device_controls_health_t health = {0};
    if (engine == NULL) {
        health.last_error = APP_ERROR_INVALID_ARGUMENT;
        return health;
    }
    if (!operations_valid(&engine->operations)) {
        return engine->health;
    }
    if (!engine->operations.lock(engine->operations.context)) {
        health.last_error = APP_ERROR_INTERNAL;
        return health;
    }
    health = engine->health;
    if (!engine->operations.unlock(engine->operations.context)) {
        health.last_error = APP_ERROR_INTERNAL;
    }
    return health;
}

bool device_controls_engine_owns_resources(const device_controls_engine_t *engine) {
    return engine != NULL &&
           (engine->confirmation_signal_owned || engine->stopped_signal_owned ||
            engine->confirm_pin_owned || engine->cancel_pin_owned || engine->status_pin_owned);
}

bool device_controls_engine_task_stop_confirmed(const device_controls_engine_t *engine) {
    return engine != NULL && engine->task_stop_confirmed;
}

static app_error_code_t cleanup_pin(device_controls_engine_t *engine, device_controls_pin_t pin,
                                    bool *owned) {
    if (!*owned) {
        return APP_ERROR_NONE;
    }
    const app_error_code_t result = engine->operations.reset_pin(engine->operations.context, pin);
    if (result == APP_ERROR_NONE) {
        *owned = false;
    }
    return result;
}

static void record_cleanup_failure(device_controls_engine_t *engine, app_error_code_t error,
                                   device_controls_failure_t failure,
                                   app_error_code_t *first_error) {
    record_first_error(error, first_error);
    if (error != APP_ERROR_NONE) {
        const app_error_code_t health_result =
            device_controls_engine_record_failure(engine, error, failure, true);
        record_first_error(health_result, first_error);
    }
}

static app_error_code_t cleanup_resources(device_controls_engine_t *engine) {
    app_error_code_t first_error = APP_ERROR_NONE;
    if (engine->status_pin_owned) {
        const app_error_code_t safe =
            engine->operations.set_safe_output(engine->operations.context);
        record_cleanup_failure(engine, safe, DEVICE_CONTROLS_FAILURE_INDICATOR, &first_error);
    }

    app_error_code_t result =
        cleanup_pin(engine, DEVICE_CONTROLS_PIN_CONFIRM, &engine->confirm_pin_owned);
    record_cleanup_failure(engine, result, DEVICE_CONTROLS_FAILURE_GENERIC, &first_error);

    result = cleanup_pin(engine, DEVICE_CONTROLS_PIN_CANCEL, &engine->cancel_pin_owned);
    record_cleanup_failure(engine, result, DEVICE_CONTROLS_FAILURE_GENERIC, &first_error);

    result = cleanup_pin(engine, DEVICE_CONTROLS_PIN_STATUS, &engine->status_pin_owned);
    record_cleanup_failure(engine, result, DEVICE_CONTROLS_FAILURE_GENERIC, &first_error);

    if (engine->confirmation_signal_owned) {
        engine->operations.delete_confirmation_signal(engine->operations.context);
        engine->confirmation_signal_owned = false;
    }
    if (engine->stopped_signal_owned) {
        engine->operations.delete_stopped_signal(engine->operations.context);
        engine->stopped_signal_owned = false;
    }
    return first_error;
}

static app_error_code_t record_stop_wait_failure(device_controls_engine_t *engine,
                                                 app_error_code_t wait_error) {
    const app_error_code_t health_result = device_controls_engine_record_failure(
        engine, wait_error, DEVICE_CONTROLS_FAILURE_TASK_STOP, true);
    if (health_result != wait_error) {
        return health_result;
    }
    const device_controls_health_t health = device_controls_engine_get_health(engine);
    return health.cleanup_error != APP_ERROR_NONE ? health.cleanup_error : wait_error;
}

static app_error_code_t finalize_deinit(device_controls_engine_t *engine,
                                        app_error_code_t cleanup_error) {
    if (device_controls_engine_owns_resources(engine)) {
        return cleanup_error;
    }
    engine->initialized = false;
    engine->operations.clear_stop_request(engine->operations.context);
    if (!engine->operations.lock(engine->operations.context)) {
        return cleanup_error != APP_ERROR_NONE ? cleanup_error : APP_ERROR_INTERNAL;
    }
    engine->indicator_state = DEVICE_INDICATOR_BOOTING;
    if (!engine->operations.unlock(engine->operations.context)) {
        return cleanup_error != APP_ERROR_NONE ? cleanup_error : APP_ERROR_INTERNAL;
    }
    return cleanup_error;
}

app_error_code_t device_controls_engine_deinit(device_controls_engine_t *engine,
                                               uint32_t timeout_ms) {
    if (engine == NULL || timeout_ms == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!engine->initialized && !device_controls_engine_owns_resources(engine)) {
        return APP_ERROR_NONE;
    }

    if (!engine->task_stop_confirmed) {
        engine->operations.request_stop(engine->operations.context);
        const app_error_code_t wait =
            engine->operations.wait_stopped(engine->operations.context, timeout_ms);
        if (wait != APP_ERROR_NONE) {
            return record_stop_wait_failure(engine, wait);
        }
        engine->task_stop_confirmed = true;
        const app_error_code_t running_result = set_task_running(engine, false);
        if (running_result != APP_ERROR_NONE) {
            return device_controls_engine_record_failure(engine, running_result,
                                                         DEVICE_CONTROLS_FAILURE_TASK_STOP, true);
        }
    }

    if (!engine->task_stop_confirmed && device_controls_engine_owns_resources(engine)) {
        const device_controls_health_t failed_health = device_controls_engine_get_health(engine);
        return failed_health.cleanup_error != APP_ERROR_NONE ? failed_health.cleanup_error
                                                             : APP_ERROR_INTERNAL;
    }

    return finalize_deinit(engine, cleanup_resources(engine));
}
