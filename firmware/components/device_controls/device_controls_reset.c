#include "device_controls_reset.h"

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"

static void record_first_error(app_error_code_t candidate, app_error_code_t *first_error) {
    if (candidate != APP_ERROR_NONE && *first_error == APP_ERROR_NONE) {
        *first_error = candidate;
    }
}

bool device_controls_reset_ops_is_valid(const device_controls_reset_ops_t *operations) {
    return operations != NULL && operations->reset_settings_noncredential != NULL &&
           operations->mark_factory_reset_pending != NULL &&
           operations->erase_all_settings != NULL && operations->invalidate_all_sessions != NULL &&
           operations->delete_all_blobs != NULL && operations->cleanup_temporary_files != NULL &&
           operations->clear_factory_reset_pending != NULL && operations->schedule_restart != NULL;
}

app_error_code_t device_controls_reset_engine_restart(const device_controls_reset_ops_t *operations,
                                                      uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return operations->schedule_restart(operations->context, delay_ms);
}

device_controls_reset_settings_outcome_t
device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,
                                            uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return (device_controls_reset_settings_outcome_t){
            .settings_applied = false,
            .sessions_invalidated = false,
            .restart_owned = false,
            .primary_error = APP_ERROR_INVALID_ARGUMENT,
            .restart_error = APP_ERROR_NONE,
        };
    }

    const app_error_code_t settings_result =
        operations->reset_settings_noncredential(operations->context);
    if (settings_result != APP_ERROR_NONE) {
        return (device_controls_reset_settings_outcome_t){
            .settings_applied = false,
            .sessions_invalidated = false,
            .restart_owned = false,
            .primary_error = settings_result,
            .restart_error = APP_ERROR_NONE,
        };
    }

    const app_error_code_t session_result =
        operations->invalidate_all_sessions(operations->context);
    const app_error_code_t restart_result =
        operations->schedule_restart(operations->context, delay_ms);
    return (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = session_result == APP_ERROR_NONE,
        .restart_owned = restart_result == APP_ERROR_NONE,
        .primary_error = session_result,
        .restart_error = restart_result,
    };
}

device_controls_factory_reset_outcome_t
device_controls_reset_engine_factory_reset(const device_controls_reset_ops_t *operations,
                                           uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return (device_controls_factory_reset_outcome_t){
            .durably_accepted = false,
            .recovery_required = false,
            .primary_error = APP_ERROR_INVALID_ARGUMENT,
        };
    }

    const app_error_code_t marker_result =
        operations->mark_factory_reset_pending(operations->context);
    if (marker_result != APP_ERROR_NONE) {
        return (device_controls_factory_reset_outcome_t){
            .durably_accepted = false,
            .recovery_required = false,
            .primary_error = marker_result,
        };
    }

    app_error_code_t first_error = APP_ERROR_NONE;
    record_first_error(operations->erase_all_settings(operations->context), &first_error);
    if (first_error == APP_ERROR_NONE) {
        record_first_error(operations->invalidate_all_sessions(operations->context), &first_error);
        record_first_error(operations->delete_all_blobs(operations->context), &first_error);
        record_first_error(operations->cleanup_temporary_files(operations->context), &first_error);
    }
    /* Establish restart ownership before clearing PENDING. If restart cannot
     * be owned, keeping the durable marker is safer than exposing ordinary
     * operation after a reset that has already been accepted. If an immediate
     * restart happens before the clear below, boot recovery sees PENDING and
     * safely replays the idempotent cleanup. */
    record_first_error(operations->schedule_restart(operations->context, delay_ms), &first_error);
    if (first_error == APP_ERROR_NONE) {
        record_first_error(operations->clear_factory_reset_pending(operations->context),
                           &first_error);
    }

    return (device_controls_factory_reset_outcome_t){
        .durably_accepted = true,
        .recovery_required = first_error != APP_ERROR_NONE,
        .primary_error = first_error,
    };
}
