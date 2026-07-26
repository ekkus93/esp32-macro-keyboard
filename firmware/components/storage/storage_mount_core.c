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

app_error_code_t storage_mount_core_unmount(const storage_mount_ops_t *ops,
                                            storage_mount_state_t *state) {
    if (!mount_ops_valid(ops) || state == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = APP_ERROR_NONE;
    if (state->data_mounted) {
        const app_error_code_t cleanup = ops->unmount_data(ops->context);
        if (cleanup != APP_ERROR_NONE) {
            result = cleanup;
        } else {
            state->data_mounted = false;
        }
    }
    if (state->web_mounted) {
        const app_error_code_t cleanup = ops->unmount_web(ops->context);
        if (cleanup != APP_ERROR_NONE) {
            if (result == APP_ERROR_NONE) {
                result = cleanup;
            }
        } else {
            state->web_mounted = false;
        }
    }
    return result;
}

app_error_code_t storage_mount_core_mount(const storage_mount_ops_t *ops,
                                          storage_mount_state_t *state) {
    if (!mount_ops_valid(ops) || state == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_error_code_t result = ops->mount_web(ops->context);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    state->web_mounted = true;

    result = ops->mount_data(ops->context);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = ops->unmount_web(ops->context);
        if (cleanup != APP_ERROR_NONE) {
            /* The web partition is still mounted; report the cleanup failure and
             * keep web_mounted set so the residual mount stays visible. */
            return cleanup;
        }
        state->web_mounted = false;
        return result;
    }
    state->data_mounted = true;

    result = ops->prepare_directories(ops->context);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = storage_mount_core_unmount(ops, state);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    return APP_ERROR_NONE;
}
