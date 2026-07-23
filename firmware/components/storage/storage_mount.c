#include "storage.h"

#include <errno.h>
#include <sys/stat.h>

#include "esp_littlefs.h"

static bool web_mounted;
static bool data_mounted;

static app_error_code_t mkdir_checked(const char *path)
{
    if (mkdir(path, 0750) == 0 || errno == EEXIST) {
        return APP_ERROR_NONE;
    }
    return errno == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

app_error_code_t storage_mount_all(void)
{
    const esp_vfs_littlefs_conf_t web = {
        .base_path = STORAGE_WEB_MOUNT,
        .partition_label = STORAGE_WEB_PARTITION,
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    const esp_vfs_littlefs_conf_t data = {
        .base_path = STORAGE_DATA_MOUNT,
        .partition_label = STORAGE_DATA_PARTITION,
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    if (esp_vfs_littlefs_register(&web) != ESP_OK) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
    web_mounted = true;
    if (esp_vfs_littlefs_register(&data) != ESP_OK) {
        const esp_err_t cleanup_result = esp_vfs_littlefs_unregister(STORAGE_WEB_PARTITION);
        if (cleanup_result == ESP_OK) {
            web_mounted = false;
            return APP_ERROR_STORAGE_UNAVAILABLE;
        }
        return APP_ERROR_IO;
    }
    data_mounted = true;
    const app_error_code_t prepare_result = storage_prepare_directories();
    if (prepare_result != APP_ERROR_NONE) {
        const app_error_code_t cleanup_result = storage_unmount_all();
        return cleanup_result == APP_ERROR_NONE ? prepare_result : cleanup_result;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_unmount_all(void)
{
    app_error_code_t result = APP_ERROR_NONE;
    if (data_mounted) {
        if (esp_vfs_littlefs_unregister(STORAGE_DATA_PARTITION) != ESP_OK) {
            result = APP_ERROR_IO;
        } else {
            data_mounted = false;
        }
    }
    if (web_mounted) {
        if (esp_vfs_littlefs_unregister(STORAGE_WEB_PARTITION) != ESP_OK) {
            result = APP_ERROR_IO;
        } else {
            web_mounted = false;
        }
    }
    return result;
}

app_error_code_t storage_prepare_directories(void)
{
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/global",
        STORAGE_DATA_MOUNT "/global/macros",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/quarantine",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        const app_error_code_t result = mkdir_checked(paths[index]);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}
