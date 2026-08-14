#include "storage_mount_core.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "storage.h"

static bool mount_ops_valid(const storage_mount_ops_t *ops) {
    return ops != NULL && ops->mount_web != NULL && ops->mount_data != NULL &&
           ops->unmount_web != NULL && ops->unmount_data != NULL &&
           ops->prepare_directories != NULL;
}

static app_operation_result_t primary_error(app_error_code_t error) {
    app_operation_result_t result = app_operation_success();
    app_operation_record_primary(&result, error);
    return result;
}

app_operation_result_t storage_mount_core_unmount_result(const storage_mount_ops_t *ops,
                                                         storage_mount_state_t *state) {
    if (!mount_ops_valid(ops) || state == NULL) {
        return primary_error(APP_ERROR_INVALID_ARGUMENT);
    }

    app_operation_result_t result = app_operation_success();
    if (state->data_mounted) {
        const app_error_code_t unmount_error = ops->unmount_data(ops->context);
        if (unmount_error != APP_ERROR_NONE) {
            app_operation_record_primary(&result, unmount_error);
        } else {
            state->data_mounted = false;
        }
    }
    if (state->web_mounted) {
        const app_error_code_t unmount_error = ops->unmount_web(ops->context);
        if (unmount_error != APP_ERROR_NONE) {
            if (result.primary_error == APP_ERROR_NONE) {
                app_operation_record_primary(&result, unmount_error);
            } else {
                app_operation_record_cleanup(&result, unmount_error);
            }
        } else {
            state->web_mounted = false;
        }
    }
    return result;
}

app_error_code_t storage_mount_core_unmount(const storage_mount_ops_t *ops,
                                            storage_mount_state_t *state) {
    return app_operation_result_error(storage_mount_core_unmount_result(ops, state));
}

app_operation_result_t storage_mount_core_mount_result(const storage_mount_ops_t *ops,
                                                       storage_mount_state_t *state) {
    if (!mount_ops_valid(ops) || state == NULL) {
        return primary_error(APP_ERROR_INVALID_ARGUMENT);
    }

    app_error_code_t operation_error = ops->mount_web(ops->context);
    if (operation_error != APP_ERROR_NONE) {
        return primary_error(operation_error);
    }
    state->web_mounted = true;

    operation_error = ops->mount_data(ops->context);
    if (operation_error != APP_ERROR_NONE) {
        app_operation_result_t result = primary_error(operation_error);
        const app_error_code_t cleanup_error = ops->unmount_web(ops->context);
        if (cleanup_error == APP_ERROR_NONE) {
            state->web_mounted = false;
        } else {
            app_operation_record_cleanup(&result, cleanup_error);
        }
        return result;
    }
    state->data_mounted = true;

    operation_error = ops->prepare_directories(ops->context);
    if (operation_error != APP_ERROR_NONE) {
        app_operation_result_t result = primary_error(operation_error);
        const app_operation_result_t rollback = storage_mount_core_unmount_result(ops, state);
        app_operation_record_cleanup(&result, rollback.primary_error);
        app_operation_record_cleanup(&result, rollback.cleanup_error);
        if (rollback.cleanup_incomplete) {
            result.cleanup_incomplete = true;
        }
        return result;
    }
    return app_operation_success();
}

app_error_code_t storage_mount_core_mount(const storage_mount_ops_t *ops,
                                          storage_mount_state_t *state) {
    return app_operation_result_error(storage_mount_core_mount_result(ops, state));
}
