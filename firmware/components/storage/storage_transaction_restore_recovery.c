#include "storage.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage_fs_ops.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_tree_internal.h"
#include "storage_transaction_internal.h"

#define STORAGE_RESTORE_MAX_MANIFESTS 16U
#define STORAGE_TRANSACTION_SUFFIX ".bin"

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static app_error_code_t production_uuid_generate(void *context, app_uuid_t *out_uuid) {
    (void)context;
    return app_uuid_generate(out_uuid);
}

static app_error_code_t production_validate_repository(void *context, const char *root) {
    (void)context;
    return storage_repository_tree_validate(root);
}

static app_error_code_t production_remove_tree(void *context, const char *path) {
    (void)context;
    return storage_repository_remove_tree(path);
}

static bool manifest_name_valid(const char *name) {
    if (name == NULL) {
        return false;
    }
    const size_t suffix_length = sizeof(STORAGE_TRANSACTION_SUFFIX) - 1U;
    const size_t length = strlen(name);
    return length == APP_UUID_STRING_LENGTH + suffix_length && strchr(name, '/') == NULL &&
           strstr(name, "..") == NULL &&
           strcmp(name + APP_UUID_STRING_LENGTH, STORAGE_TRANSACTION_SUFFIX) == 0;
}

static app_error_code_t manifest_id_from_name(const char *name, app_uuid_t *out_id) {
    if (out_id != NULL) {
        memset(out_id, 0, sizeof(*out_id));
    }
    if (!manifest_name_valid(name) || out_id == NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char value[APP_UUID_STRING_LENGTH + 1U];
    memcpy(value, name, APP_UUID_STRING_LENGTH);
    value[APP_UUID_STRING_LENGTH] = '\0';
    return app_uuid_parse(value, out_id) == APP_ERROR_NONE ? APP_ERROR_NONE
                                                           : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t manifest_path_from_name(const char *name, char *out_path,
                                                size_t path_size) {
    if (!manifest_name_valid(name) || out_path == NULL || path_size == 0U) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int written = snprintf(out_path, path_size, STORAGE_DATA_MOUNT "/transactions/%s", name);
    return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE
                                                       : APP_ERROR_STORAGE_CORRUPT;
}

app_error_code_t storage_transaction_recover_restores_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_repository_fn validate_repository, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    if (!storage_fs_ops_has_directory(operations) || generate_uuid == NULL ||
        validate_repository == NULL || remove_tree == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    void *directory =
        operations->open_directory(operations->context, STORAGE_DATA_MOUNT "/transactions");
    if (directory == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_STORAGE_UNAVAILABLE : map_error_number(open_error);
    }

    size_t manifest_count = 0U;
    bool restore_found = false;
    storage_transaction_manifest_t restore_manifest = {0};
    app_error_code_t result = APP_ERROR_NONE;
    while (result == APP_ERROR_NONE) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context, directory, name, sizeof(name), &end) !=
            0) {
            result = map_error_number(errno);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 || name[0] == '.') {
            continue;
        }
        if (manifest_count >= STORAGE_RESTORE_MAX_MANIFESTS) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }

        app_uuid_t manifest_id = {0};
        char path[APP_PATH_MAX_BYTES];
        result = manifest_id_from_name(name, &manifest_id);
        if (result == APP_ERROR_NONE) {
            result = manifest_path_from_name(name, path, sizeof(path));
        }
        storage_transaction_manifest_t manifest = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_transaction_read_manifest_with_ops(path, &manifest_id, &manifest,
                                                                operations);
        }
        if (result != APP_ERROR_NONE) {
            break;
        }
        ++manifest_count;
        if (manifest.type == STORAGE_TRANSACTION_RESTORE) {
            if (restore_found) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
            restore_manifest = manifest;
            restore_found = true;
        }
    }

    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    if (result != APP_ERROR_NONE || !restore_found) {
        return result;
    }
    if (manifest_count != 1U) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return storage_transaction_recover_restore_with_ops(
        &restore_manifest, operations, generate_uuid, uuid_context, validate_repository,
        validation_context, remove_tree, remove_context);
}

app_error_code_t storage_transaction_recover_restores(void) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_transaction_recover_restores_with_ops(
        storage_fs_ops_posix(), production_uuid_generate, NULL, production_validate_repository,
        NULL, production_remove_tree, NULL);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
