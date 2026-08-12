#include "macro_executor_engine.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "app_uuid.h"
#include "macro_executor.h"
#include "macro_executor_ops.h"
#include "macro_limits.h"
#include "macro_parser.h"

#define CANCELLATION_SLICE_MS 10U
#define CURRENT_ACTION_NONE "none"
/* SPEC_V2 §7.12: "for at most 60 seconds". */
#define CONFIRMATION_TIMEOUT_MS (APP_V2_SERIAL_CONFIRMATION_TIMEOUT_SECONDS * 1000U)

static const char *action_type_string(macro_action_type_t type) {
    switch (type) {
    case MACRO_ACTION_KEY:
        return "key";
    case MACRO_ACTION_CHORD:
        return "chord";
    case MACRO_ACTION_DELAY:
        return "delay";
    default:
        return CURRENT_ACTION_NONE;
    }
}

static bool operations_valid(const macro_executor_ops_t *ops) {
    return ops != NULL && ops->lock != NULL && ops->unlock != NULL && ops->queue_send != NULL &&
           ops->notify_executor != NULL && ops->now_ms != NULL && ops->wait_ms != NULL &&
           ops->usb_ready != NULL && ops->usb_press != NULL && ops->usb_release_all != NULL &&
           ops->record_release_failure != NULL && ops->plan_free != NULL;
}

static bool engine_unavailable(const macro_executor_engine_t *engine) {
    return atomic_load_explicit(&engine->unavailable, memory_order_acquire);
}

static void latch_engine_unavailable(macro_executor_engine_t *engine) {
    atomic_store_explicit(&engine->unavailable, true, memory_order_release);
}

static app_error_code_t engine_release_fault(const macro_executor_engine_t *engine) {
    return (app_error_code_t)atomic_load_explicit(&engine->release_fault_error,
                                                  memory_order_acquire);
}

/* Release-all is a safety boundary, not ordinary cleanup. The first failure is
 * retained independently of status publication, the engine is immediately
 * latched unavailable so another request cannot be accepted, and the platform
 * health hook receives the same fixed-vocabulary error for diagnostics. */
static void record_release_failure(macro_executor_engine_t *engine,
                                   macro_execution_status_t *status,
                                   app_error_code_t release_error) {
    if (release_error == APP_ERROR_NONE) {
        return;
    }
    int expected = APP_ERROR_NONE;
    (void)atomic_compare_exchange_strong_explicit(&engine->release_fault_error, &expected,
                                                  (int)release_error, memory_order_acq_rel,
                                                  memory_order_acquire);
    if (status != NULL && status->release_error == APP_ERROR_NONE) {
        status->release_error = release_error;
    }
    latch_engine_unavailable(engine);
    engine->ops.record_release_failure(engine->ops.context, release_error);
}

static app_error_code_t lock_engine(macro_executor_engine_t *engine) {
    return engine->ops.lock(engine->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t unlock_engine(macro_executor_engine_t *engine) {
    return engine->ops.unlock(engine->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t publish_status(macro_executor_engine_t *engine,
                                       macro_execution_status_t status) {
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->status = status;
    return unlock_engine(engine);
}

static app_error_code_t read_cancellation(macro_executor_engine_t *engine, bool *out_cancelled) {
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

static app_error_code_t read_confirmation(macro_executor_engine_t *engine, bool *out_confirmed) {
    if (out_confirmed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    *out_confirmed = engine->confirmed_requested;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        *out_confirmed = false;
    }
    return result;
}

static app_error_code_t reset_terminal_flags(macro_executor_engine_t *engine) {
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->busy = false;
    engine->cancellation_requested = false;
    engine->confirmed_requested = false;
    return unlock_engine(engine);
}

static bool deadline_expired(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}

static app_error_code_t validate_request(const macro_execution_request_t *request) {
    /* key_press_ms accepts the complete SPEC_V2 §7.11 range, 0 through 10,000 ms
     * inclusive -- the same range macro_compile_v2() already validates and
     * documents. A lower bound of 1 here was a v1-shaped constraint the v2
     * contract does not share (PHASE_5_EXACT_V2_HTTP_API_2026-08-08.md Known
     * gap #4); a keyPressMs of 0 is a legitimate "no dwell" request. */
    if (request == NULL || request->plan.actions == NULL || request->plan.action_count == 0U ||
        request->plan.action_count > APP_COMPILED_ACTION_MAX ||
        request->plan.estimated_duration_ms > APP_ESTIMATED_DURATION_MAX_MS ||
        request->key_press_ms > APP_DELAY_MAX_MS || request->inter_key_ms > APP_DELAY_MAX_MS ||
        !app_uuid_is_valid_string(request->execution_id.value) ||
        !app_uuid_is_valid_string(request->set_id.value) ||
        !app_uuid_is_valid_string(request->macro_id.value) || request->macro_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t cancellable_delay(macro_executor_engine_t *engine, uint32_t delay_ms) {
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
        if (deadline_expired(engine->ops.now_ms(engine->ops.context), engine->deadline)) {
            return APP_ERROR_TIMEOUT;
        }
        const uint32_t slice =
            remaining > CANCELLATION_SLICE_MS ? CANCELLATION_SLICE_MS : remaining;
        result = engine->ops.wait_ms(engine->ops.context, slice);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        remaining -= slice;
    }
    return APP_ERROR_NONE;
}

/* SPEC_V2 §7.12: waits up to CONFIRMATION_TIMEOUT_MS for macro_executor_confirm()
 * (or a cancellation) while status.state is EXECUTION_AWAITING_CONFIRMATION.
 * Returns APP_ERROR_NONE once confirmed, APP_ERROR_EXECUTION_CANCELLED if
 * cancelled first, or APP_ERROR_TIMEOUT ("Expiry produces timed_out and types
 * nothing") if neither happens before the deadline. No key or chord action runs
 * before this returns APP_ERROR_NONE. Polls in CANCELLATION_SLICE_MS slices
 * against a wall-clock deadline, the same idiom cancellable_delay() uses for
 * the execution-phase watchdog, rather than counting down a fixed number of
 * slices -- so it stays correct even if a wait_ms() call takes longer than
 * requested. */
static app_error_code_t await_confirmation(macro_executor_engine_t *engine) {
    const uint32_t deadline = engine->ops.now_ms(engine->ops.context) + CONFIRMATION_TIMEOUT_MS;
    while (true) {
        bool cancelled = false;
        app_error_code_t result = read_cancellation(engine, &cancelled);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (cancelled) {
            return APP_ERROR_EXECUTION_CANCELLED;
        }
        bool confirmed = false;
        result = read_confirmation(engine, &confirmed);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (confirmed) {
            return APP_ERROR_NONE;
        }
        if (deadline_expired(engine->ops.now_ms(engine->ops.context), deadline)) {
            return APP_ERROR_TIMEOUT;
        }
        result = engine->ops.wait_ms(engine->ops.context, CANCELLATION_SLICE_MS);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
}

static app_error_code_t finish_execution(macro_executor_engine_t *engine,
                                         macro_execution_status_t status, execution_state_t state,
                                         app_error_code_t primary_error) {
    const app_error_code_t release_result = engine->ops.usb_release_all(engine->ops.context);
    record_release_failure(engine, &status, release_result);
    status.state = state;
    status.error = primary_error;
    status.available = !engine_unavailable(engine);
    status.completed_ms = engine->ops.now_ms(engine->ops.context);
    status.current_action = CURRENT_ACTION_NONE;
    const app_error_code_t publish_result = publish_status(engine, status);
    const app_error_code_t reset_result = reset_terminal_flags(engine);
    if (publish_result != APP_ERROR_NONE || reset_result != APP_ERROR_NONE) {
        /* The terminal state could not be published or the busy flag could not be
         * cleared: latch the engine unavailable so it rejects new work until
         * re-initialized rather than appearing falsely idle (FIX1 §12.3). */
        latch_engine_unavailable(engine);
    }
    if (publish_result != APP_ERROR_NONE) {
        return publish_result;
    }
    return reset_result;
}

/* Publishes status; on failure, frees the request's plan and drives the engine to
 * a EXECUTION_FAILED terminal state through the same finish_execution() path
 * every other failure uses. Returns true (with *out_result untouched) when the
 * caller should keep going; false when the caller must return *out_result
 * immediately -- the request is already finished. */
static bool publish_step(macro_executor_engine_t *engine, macro_execution_request_t *request,
                         macro_execution_status_t status, app_error_code_t *out_result) {
    const app_error_code_t result = publish_status(engine, status);
    if (result == APP_ERROR_NONE) {
        return true;
    }
    engine->ops.plan_free(&request->plan);
    const app_error_code_t finish_result =
        finish_execution(engine, status, EXECUTION_FAILED, result);
    *out_result = finish_result != APP_ERROR_NONE ? finish_result : result;
    return false;
}

/* A submission can fail after the executor has claimed busy but before the
 * request reaches the queue. Those paths still issue the defensive release-all
 * required by SPEC_V2 §7.3. Preserve the submit failure as the primary error
 * while publishing the release result separately so status consumers can tell
 * whether HID cleanup also failed (post-v2 F-009 / Round 2 F-025). */
static app_error_code_t finish_submission_failure(macro_executor_engine_t *engine,
                                                  const macro_execution_request_t *request,
                                                  app_error_code_t primary_error) {
    macro_execution_status_t status = {
        .state = EXECUTION_FAILED,
        .error = primary_error,
        .release_error = APP_ERROR_NONE,
        .execution_id = request->execution_id,
        .set_id = request->set_id,
        .macro_id = request->macro_id,
        .macro_revision = request->macro_revision,
        .action_index = 0U,
        .action_count = request->plan.action_count,
        .available = true,
        .accepted_ms = request->accepted_ms,
        .completed_ms = engine->ops.now_ms(engine->ops.context),
        .current_action = CURRENT_ACTION_NONE,
    };
    const app_error_code_t release_result = engine->ops.usb_release_all(engine->ops.context);
    record_release_failure(engine, &status, release_result);
    status.available = !engine_unavailable(engine);
    const app_error_code_t publish_result = publish_status(engine, status);
    const app_error_code_t reset_result = reset_terminal_flags(engine);
    if (publish_result != APP_ERROR_NONE || reset_result != APP_ERROR_NONE) {
        /* Match finish_execution(): if status publication or flag cleanup is
         * itself unreliable, fail closed until the engine is reinitialized. */
        latch_engine_unavailable(engine);
    }
    return primary_error;
}

app_error_code_t macro_executor_engine_init(macro_executor_engine_t *engine,
                                            const macro_executor_ops_t *ops) {
    if (engine == NULL || !operations_valid(ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(engine, 0, sizeof(*engine));
    engine->ops = *ops;
    atomic_init(&engine->unavailable, false);
    atomic_init(&engine->release_fault_error, APP_ERROR_NONE);
    engine->status.state = EXECUTION_IDLE;
    engine->status.available = true;
    engine->status.current_action = CURRENT_ACTION_NONE;
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_engine_submit(macro_executor_engine_t *engine,
                                              macro_execution_request_t *request) {
    if (engine == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* A prior terminal-publish/reset or HID-release failure latched the engine
     * unavailable: refuse new work until lifecycle re-initialization. The atomic
     * latch is read without the status lock so even a lock failure stays fail-closed. */
    if (engine_unavailable(engine)) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = validate_request(request);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    request->accepted_ms = engine->ops.now_ms(engine->ops.context);
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
    engine->confirmed_requested = false;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        /* SPEC_V2 §7.3 names "internal error" explicitly among the release-all
         * triggers. No action for this request ever ran, but the attempt costs
         * nothing and guards against a stuck key from whatever ran immediately
         * before this submission. */
        return finish_submission_failure(engine, request, result);
    }
    if (!engine->ops.queue_send(engine->ops.context, request)) {
        /* SPEC_V2 §7.3 names "queue failure" explicitly. */
        return finish_submission_failure(engine, request, APP_ERROR_INTERNAL);
    }
    request->plan.actions = NULL;
    request->plan.action_count = 0U;
    request->plan.estimated_duration_ms = 0U;
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_engine_cancel(macro_executor_engine_t *engine) {
    if (engine == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (engine_unavailable(engine)) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!engine->busy) {
        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_NOT_FOUND : APP_ERROR_INTERNAL;
    }
    if (engine->cancellation_requested) {
        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_CONFLICT : APP_ERROR_INTERNAL;
    }
    engine->cancellation_requested = true;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->ops.notify_executor(engine->ops.context);
    return APP_ERROR_NONE;
}

app_error_code_t macro_executor_engine_confirm(macro_executor_engine_t *engine) {
    if (engine == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (engine_unavailable(engine)) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = lock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!engine->busy || engine->status.state != EXECUTION_AWAITING_CONFIRMATION) {
        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_NOT_FOUND : APP_ERROR_INTERNAL;
    }
    if (engine->confirmed_requested) {
        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_CONFLICT : APP_ERROR_INTERNAL;
    }
    engine->confirmed_requested = true;
    result = unlock_engine(engine);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    engine->ops.notify_executor(engine->ops.context);
    return APP_ERROR_NONE;
}

macro_execution_status_t macro_executor_engine_get_status(macro_executor_engine_t *engine) {
    macro_execution_status_t result = {
        .state = EXECUTION_FAILED,
        .error = APP_ERROR_INTERNAL,
        .current_action = CURRENT_ACTION_NONE,
    };
    if (engine == NULL) {
        return result;
    }
    const app_error_code_t release_fault = engine_release_fault(engine);
    result.available = !engine_unavailable(engine);
    result.release_error = release_fault;
    if (lock_engine(engine) != APP_ERROR_NONE) {
        return result;
    }
    result = engine->status;
    result.available = !engine_unavailable(engine);
    if (result.release_error == APP_ERROR_NONE) {
        result.release_error = release_fault;
    }
    result.cancellation_requested = engine->cancellation_requested;
    if (unlock_engine(engine) != APP_ERROR_NONE) {
        result.state = EXECUTION_FAILED;
        result.error = APP_ERROR_INTERNAL;
    }
    return result;
}

static app_error_code_t execute_action(macro_executor_engine_t *engine,
                                       macro_execution_status_t *status, macro_action_t action,
                                       uint32_t key_press_ms, uint32_t inter_key_ms) {
    if (action.type == MACRO_ACTION_DELAY) {
        return cancellable_delay(engine, action.delay_ms);
    }
    if (action.type != MACRO_ACTION_KEY && action.type != MACRO_ACTION_CHORD) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result =
        engine->ops.usb_press(engine->ops.context, action.modifiers, action.usage);
    if (result == APP_ERROR_NONE) {
        result = cancellable_delay(engine, key_press_ms);
    }
    const app_error_code_t release_result = engine->ops.usb_release_all(engine->ops.context);
    record_release_failure(engine, status, release_result);
    if (result == APP_ERROR_NONE) {
        result = release_result;
    }
    if (result == APP_ERROR_NONE) {
        result = cancellable_delay(engine, inter_key_ms);
    }
    return result;
}

/* Runs the require_confirmation branch of macro_executor_engine_execute(): waits
 * for confirmation (or cancellation/expiry), and republishes *status as RUNNING
 * on success. Returns true when the caller should proceed to execute the
 * plan's actions; false when the send already reached a terminal state and the
 * caller must return *out_result immediately. Split out of
 * macro_executor_engine_execute() to keep its cognitive complexity bounded. */
static bool run_confirmation_phase(macro_executor_engine_t *engine,
                                   macro_execution_request_t *request,
                                   macro_execution_status_t *status, app_error_code_t *out_result) {
    /* SPEC_V2 §7.12: no action executes until macro_executor_confirm() is
     * called, up to the 60-second timeout; a cancellation or expiry here ends
     * the send having typed nothing. */
    const app_error_code_t confirm_result = await_confirmation(engine);
    if (confirm_result == APP_ERROR_NONE) {
        status->state = EXECUTION_RUNNING;
        return publish_step(engine, request, *status, out_result);
    }
    engine->ops.plan_free(&request->plan);
    execution_state_t confirm_terminal = EXECUTION_FAILED;
    if (confirm_result == APP_ERROR_EXECUTION_CANCELLED) {
        confirm_terminal = EXECUTION_CANCELLED;
    } else if (confirm_result == APP_ERROR_TIMEOUT) {
        confirm_terminal = EXECUTION_TIMED_OUT;
    }
    const app_error_code_t finish_result =
        finish_execution(engine, *status, confirm_terminal, confirm_result);
    *out_result = finish_result == APP_ERROR_NONE ? confirm_result : finish_result;
    return false;
}

app_error_code_t macro_executor_engine_execute(macro_executor_engine_t *engine,
                                               macro_execution_request_t *request) {
    if (engine == NULL || validate_request(request) != APP_ERROR_NONE) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_execution_status_t status = {
        .state =
            request->require_confirmation ? EXECUTION_AWAITING_CONFIRMATION : EXECUTION_RUNNING,
        .error = APP_ERROR_NONE,
        .release_error = APP_ERROR_NONE,
        .execution_id = request->execution_id,
        .set_id = request->set_id,
        .macro_id = request->macro_id,
        .macro_revision = request->macro_revision,
        .available = true,
        .action_index = 0U,
        .action_count = request->plan.action_count,
        .accepted_ms = request->accepted_ms,
        .current_action = CURRENT_ACTION_NONE,
    };
    app_error_code_t result = APP_ERROR_NONE;
    if (!publish_step(engine, request, status, &result)) {
        return result;
    }

    if (request->require_confirmation &&
        !run_confirmation_phase(engine, request, &status, &result)) {
        return result;
    }

    const uint32_t started = engine->ops.now_ms(engine->ops.context);
    status.started_ms = started;
    /* SPEC_V2 §7.11: "absolute executor deadline 310,000 ms", "a 10-second
     * safety margin beyond the maximum accepted estimate" (300,000 ms) -- a
     * fixed ceiling from the moment execution actually starts, independent of
     * this particular request's own (possibly much smaller) estimated
     * duration. */
    engine->deadline = started + APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS;

    for (size_t index = 0U; index < request->plan.action_count; ++index) {
        status.action_index = index;
        status.current_action = action_type_string(request->plan.actions[index].type);
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
        if (deadline_expired(engine->ops.now_ms(engine->ops.context), engine->deadline)) {
            result = APP_ERROR_TIMEOUT;
            break;
        }

        result = execute_action(engine, &status, request->plan.actions[index],
                                request->key_press_ms, request->inter_key_ms);
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
    } else if (result == APP_ERROR_TIMEOUT) {
        terminal = EXECUTION_TIMED_OUT;
    }
    const app_error_code_t finish_result = finish_execution(engine, status, terminal, result);
    return finish_result == APP_ERROR_NONE ? result : finish_result;
}
