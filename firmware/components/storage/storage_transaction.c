#include "storage.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>

static bool safe_manifest_path(const char *path)
{
    const size_t prefix_length = strlen(STORAGE_DATA_MOUNT "/transactions/");
    return path != NULL && strncmp(path, STORAGE_DATA_MOUNT "/transactions/", prefix_length) == 0 &&
           strstr(path, "..") == NULL;
}

app_error_code_t storage_transaction_write_manifest(const storage_transaction_manifest_t *manifest)
{
    if (manifest == NULL || !app_uuid_is_valid_string(manifest->id.value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char path[APP_PATH_MAX_BYTES];
    const int path_length = snprintf(path, sizeof(path), STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                     manifest->id.value);
    if (path_length < 0 || (size_t)path_length >= sizeof(path) || !safe_manifest_path(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_atomic_write(path, manifest, sizeof(*manifest), true);
}

app_error_code_t storage_transaction_recover_all(void)
{
    DIR *directory = opendir(STORAGE_DATA_MOUNT "/transactions");
    if (directory == NULL) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }

    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            break;
        }
        if (entry->d_name[0] == '.') {
            continue;
        }
        /* Recovery is intentionally conservative: unknown or incomplete binary
         * manifests are preserved for diagnostics instead of guessed at. */
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    return result;
}
