#include "storage.h"

#include <stddef.h>

#include "app_error.h"
#include "esp_littlefs.h"
#include "storage_mount_core.h"

static storage_mount_state_t mount_state;

static app_error_code_t register_partition(const char *base_path, const char *partition_label) {
    const esp_vfs_littlefs_conf_t configuration = {
        .base_path = base_path,
        .partition_label = partition_label,
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    return esp_vfs_littlefs_register(&configuration) == ESP_OK ? APP_ERROR_NONE
                                                               : APP_ERROR_STORAGE_UNAVAILABLE;
}

static app_error_code_t adapter_mount_web(void *context) {
    (void)context;
    return register_partition(STORAGE_WEB_MOUNT, STORAGE_WEB_PARTITION);
}

static app_error_code_t adapter_mount_data(void *context) {
    (void)context;
    return register_partition(STORAGE_DATA_MOUNT, STORAGE_DATA_PARTITION);
}

static app_error_code_t adapter_unmount_web(void *context) {
    (void)context;
    return esp_vfs_littlefs_unregister(STORAGE_WEB_PARTITION) == ESP_OK ? APP_ERROR_NONE
                                                                        : APP_ERROR_IO;
}

static app_error_code_t adapter_unmount_data(void *context) {
    (void)context;
    return esp_vfs_littlefs_unregister(STORAGE_DATA_PARTITION) == ESP_OK ? APP_ERROR_NONE
                                                                         : APP_ERROR_IO;
}

static app_error_code_t adapter_prepare_directories(void *context) {
    (void)context;
    return storage_prepare_directories();
}

static storage_mount_ops_t mount_ops(void) {
    return (storage_mount_ops_t){
        .context = NULL,
        .mount_web = adapter_mount_web,
        .mount_data = adapter_mount_data,
        .unmount_web = adapter_unmount_web,
        .unmount_data = adapter_unmount_data,
        .prepare_directories = adapter_prepare_directories,
    };
}

app_error_code_t storage_mount_all(void) {
    const storage_mount_ops_t ops = mount_ops();
    return storage_mount_core_mount(&ops, &mount_state);
}

app_error_code_t storage_unmount_all(void) {
    const storage_mount_ops_t ops = mount_ops();
    return storage_mount_core_unmount(&ops, &mount_state);
}

storage_mount_state_t storage_mount_state(void) {
    return mount_state;
}

app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes) {
    if (partition_label == NULL || out_total_bytes == NULL || out_used_bytes == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return esp_littlefs_info(partition_label, out_total_bytes, out_used_bytes) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_IO;
}
