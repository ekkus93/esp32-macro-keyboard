#include "factory_reset_recovery.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "factory_reset_recovery_engine.h"
#include "factory_reset_state.h"
#include "storage.h"
#include "storage_blob.h"

static void secure_zero_local(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

static app_error_code_t adapter_read_state(void *context, factory_reset_state_t *out_state) {
    (void)context;
    return factory_reset_state_read(out_state);
}

static app_error_code_t adapter_settings_init(void *context) {
    (void)context;
    return device_settings_init();
}

static app_error_code_t adapter_erase_settings(void *context) {
    (void)context;
    app_v2_device_settings_t settings = {0};
    bool changed = false;
    const app_error_code_t result = device_settings_factory_reset(&settings, &changed);
    secure_zero_local(&settings, sizeof(settings));
    return result;
}

static app_error_code_t adapter_settings_deinit(void *context) {
    (void)context;
    return device_settings_deinit();
}

static app_error_code_t adapter_storage_mount(void *context) {
    (void)context;
    return storage_mount_all();
}

static app_error_code_t adapter_delete_blobs(void *context) {
    (void)context;
    size_t deleted_count = 0U;
    return storage_blob_delete_all(&deleted_count);
}

static app_error_code_t adapter_cleanup_temporary_files(void *context) {
    (void)context;
    return storage_blob_recover_startup();
}

static app_error_code_t adapter_storage_unmount(void *context) {
    (void)context;
    return storage_unmount_all();
}

static app_error_code_t adapter_clear_pending(void *context) {
    (void)context;
    return factory_reset_state_clear();
}

app_error_code_t factory_reset_recovery_run_if_pending(bool *out_recovered) {
    const factory_reset_recovery_ops_t operations = {
        .context = NULL,
        .read_state = adapter_read_state,
        .settings_init = adapter_settings_init,
        .erase_settings = adapter_erase_settings,
        .settings_deinit = adapter_settings_deinit,
        .storage_mount = adapter_storage_mount,
        .delete_blobs = adapter_delete_blobs,
        .cleanup_temporary_files = adapter_cleanup_temporary_files,
        .storage_unmount = adapter_storage_unmount,
        .clear_pending = adapter_clear_pending,
    };
    return factory_reset_recovery_engine_run(&operations, out_recovered);
}
