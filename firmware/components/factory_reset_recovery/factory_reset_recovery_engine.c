#include "factory_reset_recovery_engine.h"

#include <stddef.h>

static void record_first_error(app_error_code_t candidate, app_error_code_t *first_error) {
    if (candidate != APP_ERROR_NONE && *first_error == APP_ERROR_NONE) {
        *first_error = candidate;
    }
}

bool factory_reset_recovery_ops_is_valid(const factory_reset_recovery_ops_t *operations) {
    return operations != NULL && operations->read_state != NULL &&
           operations->settings_init != NULL && operations->erase_settings != NULL &&
           operations->settings_deinit != NULL && operations->storage_mount != NULL &&
           operations->delete_blobs != NULL && operations->cleanup_temporary_files != NULL &&
           operations->storage_unmount != NULL && operations->clear_pending != NULL;
}

app_error_code_t factory_reset_recovery_engine_run(const factory_reset_recovery_ops_t *operations,
                                                   bool *out_recovered) {
    if (out_recovered != NULL) {
        *out_recovered = false;
    }
    if (!factory_reset_recovery_ops_is_valid(operations) || out_recovered == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    factory_reset_state_t state = FACTORY_RESET_STATE_NONE;
    app_error_code_t result = operations->read_state(operations->context, &state);
    if (result != APP_ERROR_NONE || state == FACTORY_RESET_STATE_NONE) {
        return result;
    }
    if (state != FACTORY_RESET_STATE_PENDING) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    result = operations->settings_init(operations->context);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    app_error_code_t first_error = operations->erase_settings(operations->context);
    record_first_error(operations->settings_deinit(operations->context), &first_error);
    if (first_error != APP_ERROR_NONE) {
        return first_error;
    }

    result = operations->storage_mount(operations->context);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    first_error = APP_ERROR_NONE;
    record_first_error(operations->delete_blobs(operations->context), &first_error);
    record_first_error(operations->cleanup_temporary_files(operations->context), &first_error);
    record_first_error(operations->storage_unmount(operations->context), &first_error);
    if (first_error != APP_ERROR_NONE) {
        return first_error;
    }

    result = operations->clear_pending(operations->context);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    *out_recovered = true;
    return APP_ERROR_NONE;
}
