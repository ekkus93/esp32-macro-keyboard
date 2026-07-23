#include "macro_executor_engine.h"

#include <stdint.h>
#include <string.h>

#include "macro_limits.h"

#define EXECUTION_WATCHDOG_MARGIN_MS 1000U
#define CANCELLATION_SLICE_MS 10U

static bool operations_valid(const macro_executor_ops_t *ops)
{
    return ops != NULL && ops->lock != NULL && ops->unlock != NULL &&
           ops->queue_send != NULL && ops->notify_executor != NULL &&
           ops->now_ms != NULL && ops->wait_ms != NULL && ops->usb_ready != NULL &&
           ops->usb_press != NULL && ops->usb_release_all != NULL &&
           ops->plan_free != NULL;
}

static app_error_code_t lock_engine(macro_executor_engine_t *engine)
{
    return engine->ops.lock(engine->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t unlock_engine(macro_executor_engine_t *engine)
{
    return engine->ops.unlock(engine->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t publish_status(macro_executor_engine_t *engine,
                                       macro_execution_status_t status)
{
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->status = status;
    return unlock_engine(engine);
}

static app_error_code_t read_cancellation(macro_executor_engine_t *engine, bool *out_cancelled)
{
    if (out_cancelled == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    *out_cancelled = engine->cancellation_requested;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        *out_cancelled = true;
    }
    return result;
}

static app_error_code_t reset_terminal_flags(macro_executor_engine_t *engine)
{
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->busy = false;
    engine->cancellation_requested = false;
    return unlock_engine(engine);
}

static bool deadline_expired(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

static app_error_code_t validate_request(const macro_execution_request_t *request)
{
    if (request == NULL || request->plan.actions == NULL || request->plan.action_count == 0U ||
        request->plan.action_count > APP_COMPILED_ACTION_MAX ||
        request->plan.estimated_duration_ms > APP_ESTIMATED_DURATION_MAX_MS ||
        request->key_press_ms == 0U || request->key_press_ms > APP_DELAY_MAX_MS ||
        request->inter_key_ms > APP_DELAY_MAX_MS ||
        !app_uuid_is_valid_string(request->execution_id.value) ||
        !app_uuid_is_valid_string(request->set_id.value) ||
        !app_uuid_is_valid_string(request->macro_id.value) || request->macro_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t cancellable_delay(macro_executor_engine_t *engine,
                                          uint32_t delay_ms,
                                          uint32_t deadline)
{
    uint32_t remaining = delay_ms;
    while (remaining > 0U) {
        bool cancelled = false;
        app_error_code_t result = read_cancellation(engine, &cancelled);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (cancelled) {
            return APP_ERROR_EXECUTION_CANCELLED;
        }
        if (deadline_expired(engine->ops.now_ms(engine->ops.context), deadline)) {
            return APP_ERROR_TIMEOUT;
        }
        const uint32_t slice = remaining > CANCELLATION_SLICE_MS
                                   ? CANCELLATION_SLICE_MS
                                   : remaining;
        result = engine->ops.wait_ms(engine->ops.context, slice);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        remaining -= slice;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t finish_execution(macro_executor_engine_t *engine,
                                         macro_execution_status_t status,
                                         execution_state_t state,
                                         app_error_code_t primary_error)
{
    status.release_error = engine->ops.usb_release_all(engine->ops.context);
    status.state = state;
    status.error = primary_error;
    const app_error_code_t publish_result = publish_status(engine, status);
    const app_error_code_t reset_result = reset_terminal_flags(engine);
    if (publish_result != APP_ERROR_NONE) {
        return publish_result;
    }
    return reset_result;
}

app_error_code_t macro_executor_engine_init(macro_executor_engine_t *engine,
                                            const macro_executor_ops_t *ops)
{
    if (engine == NULL || !operations_valid(ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(engine, 0, sizeof(*engine));
    engine->ops = *ops;
    engine->status.state = EXECUTION_IDLE;
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_engine_submit(macro_executor_engine_t *engine,
                                              macro_execution_request_t *request)
{
    if (engine == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = validate_request(request);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!engine->ops.usb_ready(engine->ops.context)) {
        return APP_ERROR_USB_NOT_READY;
    }
    result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (engine->busy) {
        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_EXECUTOR_BUSY
                                                        : APP_ERROR_INTERNAL;
    }
    engine->busy = true;
    engine->cancellation_requested = false;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = reset_terminal_flags(engine);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    if (!engine->ops.queue_send(engine->ops.context, request)) {
        const app_error_code_t cleanup = reset_terminal_flags(engine);
        return cleanup == APP_ERROR_NONE ? APP_ERROR_INTERNAL : cleanup;
    }
    request->plan.actions = NULL;
    request->plan.action_count = 0U;
    request->plan.estimated_duration_ms = 0U;
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_engine_cancel(macro_executor_engine_t *engine)
{
    if (engine == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!engine->busy) {
        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_NOT_FOUND
                                                        : APP_ERROR_INTERNAL;
    }
    engine->cancellation_requested = true;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->ops.notify_executor(engine->ops.context);
    return APP_ERROR_NONE;
}

macro_execution_status_t macro_executor_engine_get_status(macro_executor_engine_t *engine)
{
    macro_execution_status_t result = {
        .state = EXECUTION_FAILED,
        .error = APP_ERROR_INTERNAL,
    };
    if (engine == NULL || lock_engine(engine) != APP_ERROR_NONE) {
        return result;
    }
    result = engine->status;
    if (unlock_engine(engine) != APP_ERROR_NONE) {
        result.state = EXECUTION_FAILED;
        result.error = APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t macro_executor_engine_execute(macro_executor_engine_t *engine,
                                               macro_execution_request_t *request)
{
    if (engine == NULL || validate_request(request) != APP_ERROR_NONE) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_execution_status_t status = {
        .state = EXECUTION_RUNNING,
        .error = APP_ERROR_NONE,
        .release_error = APP_ERROR_NONE,
        .execution_id = request->execution_id,
        .action_index = 0U,
        .action_count = request->plan.action_count,
    };
    app_error_code_t result = publish_status(engine, status);
    if (result != APP_ERROR_NONE) {
        engine->ops.plan_free(&request->plan);
        (void)finish_execution(engine, status, EXECUTION_FAILED, result);
        return result;
    }

    const uint32_t started = engine->ops.now_ms(engine->ops.context);
    const uint32_t watchdog_ms = request->plan.estimated_duration_ms +
                                 EXECUTION_WATCHDOG_MARGIN_MS;
    const uint32_t deadline = started + watchdog_ms;

    for (size_t index = 0U; index < request->plan.action_count; ++index) {
        status.action_index = index;
        result = publish_status(engine, status);
        if (result != APP_ERROR_NONE) {
            break;
        }
        bool cancelled = false;
        result = read_cancellation(engine, &cancelled);
        if (result != APP_ERROR_NONE) {
            break;
        }
        if (cancelled) {
            result = APP_ERROR_EXECUTION_CANCELLED;
            break;
        }
        if (deadline_expired(engine->ops.now_ms(engine->ops.context), deadline)) {
            result = APP_ERROR_TIMEOUT;
            break;
        }

        const macro_action_t action = request->plan.actions[index];
        if (action.type == MACRO_ACTION_DELAY) {
            result = cancellable_delay(engine, action.delay_ms, deadline);
        } else if (action.type == MACRO_ACTION_KEY || action.type == MACRO_ACTION_CHORD) {
            result = engine->ops.usb_press(engine->ops.context,
                                           action.modifiers,
                                           action.usage);
            if (result == APP_ERROR_NONE) {
                result = cancellable_delay(engine, request->key_press_ms, deadline);
            }
            const app_error_code_t release_result =
                engine->ops.usb_release_all(engine->ops.context);
            if (result == APP_ERROR_NONE) {
                result = release_result;
            }
            if (result == APP_ERROR_NONE) {
                result = cancellable_delay(engine, request->inter_key_ms, deadline);
            }
        } else {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        if (result != APP_ERROR_NONE) {
            break;
        }
    }

    engine->ops.plan_free(&request->plan);
    execution_state_t terminal = EXECUTION_FAILED;
    if (result == APP_ERROR_NONE) {
        terminal = EXECUTION_COMPLETED;
    } else if (result == APP_ERROR_EXECUTION_CANCELLED) {
        terminal = EXECUTION_CANCELLED;
    }
    const app_error_code_t finish_result = finish_execution(engine, status, terminal, result);
    return finish_result == APP_ERROR_NONE ? result : finish_result;
}
