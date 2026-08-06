#include "storage_blob_internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_fs_ops.h"

static bool is_dot_entry(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

bool storage_blob_parse_temporary_filename(const char *name, uint64_t *out_id) {
    if (out_id != NULL) {
        *out_id = 0U;
    }
    if (name == NULL || out_id == NULL || strlen(name) != STORAGE_BLOB_TEMP_FILENAME_LENGTH ||
        strcmp(name + STORAGE_BLOB_ID_DIGITS, ".gz.tmp") != 0) {
        return false;
    }

    uint64_t value = 0U;
    for (size_t index = 0U; index < STORAGE_BLOB_ID_DIGITS; ++index) {
        const unsigned char character = (unsigned char)name[index];
        if (character < (unsigned char)'0' || character > (unsigned char)'9') {
            return false;
        }
        const uint64_t digit = (uint64_t)(character - (unsigned char)'0');
        if (value > (UINT64_MAX - digit) / UINT64_C(10)) {
            return false;
        }
        value = (value * UINT64_C(10)) + digit;
    }
    if (value == 0U) {
        return false;
    }
    *out_id = value;
    return true;
}

static app_error_code_t join_path(const char *directory_path, const char *name, char *out_path,
                                  size_t path_size) {
    const int written = snprintf(out_path, path_size, "%s/%s", directory_path, name);
    if (written < 0 || (size_t)written >= path_size) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_recover_with_ops(const storage_fs_ops_t *operations,
                                               const char *directory_path,
                                               storage_blob_recovery_summary_t *out_summary) {
    if (!storage_fs_ops_has_directory(operations) || directory_path == NULL ||
        directory_path[0] == '\0' || out_summary == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_summary = (storage_blob_recovery_summary_t){0};

    void *directory = operations->open_directory(operations->context, directory_path);
    if (directory == NULL) {
        return APP_ERROR_IO;
    }

    app_error_code_t result = APP_ERROR_NONE;
    while (result == APP_ERROR_NONE) {
        char name[STORAGE_FS_ENTRY_NAME_MAX] = {0};
        bool end = false;
        if (operations->read_directory(operations->context, directory, name, sizeof(name), &end) !=
            0) {
            result = APP_ERROR_IO;
            break;
        }
        if (end) {
            break;
        }
        if (is_dot_entry(name)) {
            continue;
        }

        uint64_t temporary_id = 0U;
        if (!storage_blob_parse_temporary_filename(name, &temporary_id)) {
            continue;
        }

        char path[STORAGE_FS_ENTRY_NAME_MAX * 2U] = {0};
        result = join_path(directory_path, name, path, sizeof(path));
        if (result != APP_ERROR_NONE) {
            break;
        }

        struct stat metadata;
        if (operations->stat_path(operations->context, path, &metadata) != 0) {
            result = APP_ERROR_IO;
            break;
        }
        if (!S_ISREG(metadata.st_mode)) {
            continue;
        }
        if (operations->unlink_path(operations->context, path) != 0) {
            result = APP_ERROR_IO;
            break;
        }
        ++out_summary->removed_temporary_file_count;
    }

    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    return result;
}

app_error_code_t storage_blob_recover_startup(void) {
    storage_blob_recovery_summary_t summary = {0};
    const app_error_code_t result =
        storage_blob_recover_with_ops(storage_fs_ops_posix(), STORAGE_BLOB_DIRECTORY, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (summary.removed_temporary_file_count > 0U &&
        storage_fs_sync_parent_path(NULL, STORAGE_BLOB_DIRECTORY "/recovery") != 0) {
        return APP_ERROR_IO;
    }
    return APP_ERROR_NONE;
}
