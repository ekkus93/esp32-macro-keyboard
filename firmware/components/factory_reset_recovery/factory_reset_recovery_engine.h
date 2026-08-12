#ifndef FACTORY_RESET_RECOVERY_ENGINE_H
#define FACTORY_RESET_RECOVERY_ENGINE_H

#include <stdbool.h>

#include "app_error.h"
#include "factory_reset_state.h"

typedef struct {
    void *context;
    app_error_code_t (*read_state)(void *context, factory_reset_state_t *out_state);
    app_error_code_t (*settings_init)(void *context);
    app_error_code_t (*erase_settings)(void *context);
    app_error_code_t (*settings_deinit)(void *context);
    app_error_code_t (*storage_mount)(void *context);
    app_error_code_t (*delete_blobs)(void *context);
    app_error_code_t (*cleanup_temporary_files)(void *context);
    app_error_code_t (*storage_unmount)(void *context);
    app_error_code_t (*clear_pending)(void *context);
} factory_reset_recovery_ops_t;

bool factory_reset_recovery_ops_is_valid(const factory_reset_recovery_ops_t *operations);

app_error_code_t factory_reset_recovery_engine_run(const factory_reset_recovery_ops_t *operations,
                                                   bool *out_recovered);

#endif
