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
           operations->delete_all_blobs != NULL &&
           operations->clear_factory_reset_pending != NULL && operations->schedule_restart != NULL;
}

app_error_code_t device_controls_reset_engine_restart(const device_controls_reset_ops_t *operations,
                                                      uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    operations->schedule_restart(operations->context, delay_ms);
    return APP_ERROR_NONE;
}

app_error_code_t
device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,
                                            uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const app_error_code_t settings_result =
        operations->reset_settings_noncredential(operations->context);
    if (settings_result != APP_ERROR_NONE) {
        return settings_result;
    }

    app_error_code_t first_error = APP_ERROR_NONE;
    record_first_error(operations->invalidate_all_sessions(operations->context), &first_error);
    operations->schedule_restart(operations->context, delay_ms);
    return first_error;
}

app_error_code_t
device_controls_reset_engine_factory_reset(const device_controls_reset_ops_t *operations,
                                           uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const app_error_code_t marker_result =
        operations->mark_factory_reset_pending(operations->context);
    if (marker_result != APP_ERROR_NONE) {
        return marker_result;
    }

    app_error_code_t first_error = APP_ERROR_NONE;
    record_first_error(operations->erase_all_settings(operations->context), &first_error);
    if (first_error == APP_ERROR_NONE) {
        record_first_error(operations->invalidate_all_sessions(operations->context), &first_error);
        record_first_error(operations->delete_all_blobs(operations->context), &first_error);
    }
    if (first_error == APP_ERROR_NONE) {
        record_first_error(operations->clear_factory_reset_pending(operations->context),
                           &first_error);
    }

    /* Once PENDING committed, every failure path reboots. If cleanup or marker
     * clear failed, the durable marker makes the next boot stop before settings,
     * storage, USB, Wi-Fi, or HTTP startup. */
    operations->schedule_restart(operations->context, delay_ms);
    return first_error;
}
