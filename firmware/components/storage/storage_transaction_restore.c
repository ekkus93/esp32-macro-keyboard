#include "storage_transaction_internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_fs_ops.h"

static const char *const RESTORE_ITEMS[] = {"set-index.json", "sets", "global"};

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static bool expected_staging_path(char *output, size_t output_size,
                                  const app_uuid_t *transaction_id) {
    const int written = snprintf(output, output_size, STORAGE_DATA_MOUNT "/staging/%s",
                                 transaction_id->value);
    return written >= 0 && (size_t)written < output_size;
}

static bool expected_backup_path(char *output, size_t output_size,
                                 const app_uuid_t *transaction_id) {
    const int written = snprintf(output, output_size, STORAGE_DATA_MOUNT "/trash/restore-%s",
                                 transaction_id->value);
    return written >= 0 && (size_t)written < output_size;
}

static bool restore_manifest_paths_valid(const storage_transaction_manifest_t *manifest) {
    char staging[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    return manifest != NULL && manifest->type == STORAGE_TRANSACTION_RESTORE &&
           manifest->expected_revision == 0U && manifest->replacement_revision == 0U &&
           strcmp(manifest->source, STORAGE_DATA_MOUNT) == 0 &&
           strcmp(manifest->destination, STORAGE_DATA_MOUNT) == 0 &&
           expected_staging_path(staging, sizeof(staging), &manifest->id) &&
           expected_backup_path(backup, sizeof(backup), &manifest->id) &&
           strcmp(manifest->staging, staging) == 0 && strcmp(manifest->backup, backup) == 0;
}

static app_error_code_t join_path(char *output, size_t output_size, const char *root,
                                  const char *name) {
    const int written = snprintf(output, output_size, "%s/%s", root, name);
    return written >= 0 && (size_t)written < output_size ? APP_ERROR_NONE
                                                         : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t path_exists(const char *path, const storage_fs_ops_t *operations,
                                    bool *out_exists) {
    *out_exists = false;
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        *out_exists = true;
        return APP_ERROR_NONE;
    }
    const int stat_error = errno;
    return stat_error == ENOENT ? APP_ERROR_NONE : map_error_number(stat_error);
}

static app_error_code_t sync_rename_parents(const char *source, const char *destination,
                                            const storage_fs_ops_t *operations) {
    if (storage_fs_sync_parent_path(operations->context, source) != 0) {
        return map_error_number(errno);
    }
    return storage_fs_sync_parent_path(operations->context, destination) == 0
               ? APP_ERROR_NONE
               : map_error_number(errno);
}

static app_error_code_t move_once(const char *source, const char *destination,
                                  const storage_fs_ops_t *operations) {
    bool source_exists = false;
    bool destination_exists = false;
    app_error_code_t result = path_exists(source, operations, &source_exists);
    if (result == APP_ERROR_NONE) {
        result = path_exists(destination, operations, &destination_exists);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (source_exists == destination_exists) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (source_exists && operations->rename_path(operations->context, source, destination) != 0) {
        return map_error_number(errno);
    }
    return sync_rename_parents(source, destination, operations);
}

static app_error_code_t move_repository_items(const char *source_root,
                                              const char *destination_root,
                                              const storage_fs_ops_t *operations) {
    for (size_t index = 0U; index < sizeof(RESTORE_ITEMS) / sizeof(RESTORE_ITEMS[0]); ++index) {
        char source[APP_PATH_MAX_BYTES];
        char destination[APP_PATH_MAX_BYTES];
        app_error_code_t result =
            join_path(source, sizeof(source), source_root, RESTORE_ITEMS[index]);
        if (result == APP_ERROR_NONE) {
            result = join_path(destination, sizeof(destination), destination_root,
                               RESTORE_ITEMS[index]);
        }
        if (result == APP_ERROR_NONE) {
            result = move_once(source, destination, operations);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t validate_repository(
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context,
    const char *root) {
    const app_error_code_t result = validate_repository_fn(validation_context, root);
    return result == APP_ERROR_INVALID_ARGUMENT ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t write_phase(storage_transaction_manifest_t *manifest,
                                    storage_transaction_phase_t phase,
                                    const storage_fs_ops_t *operations,
                                    storage_uuid_generate_fn generate_uuid, void *uuid_context) {
    manifest->phase = phase;
    const app_error_code_t result = storage_transaction_write_manifest_with_ops(
        manifest, operations, generate_uuid, uuid_context);
    return result == APP_ERROR_INVALID_ARGUMENT ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t remove_manifest(const storage_transaction_manifest_t *manifest,
                                        const storage_fs_ops_t *operations) {
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 manifest->id.value);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (operations->unlink_path(operations->context, path) != 0) {
        const int unlink_error = errno;
        if (unlink_error != ENOENT) {
            return map_error_number(unlink_error);
        }
    }
    return storage_fs_sync_parent_path(operations->context, path) == 0
               ? APP_ERROR_NONE
               : map_error_number(errno);
}

static app_error_code_t remove_tree_if_present(
    const char *path, const storage_fs_ops_t *operations,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    bool exists = false;
    app_error_code_t result = path_exists(path, operations, &exists);
    if (result == APP_ERROR_NONE && exists) {
        result = remove_tree(remove_context, path);
        if (result == APP_ERROR_NONE &&
            storage_fs_sync_parent_path(operations->context, path) != 0) {
            result = map_error_number(errno);
        }
    }
    return result;
}

static app_error_code_t recover_prepared(
    const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    bool backup_exists = false;
    app_error_code_t result = path_exists(manifest->backup, operations, &backup_exists);
    if (result != APP_ERROR_NONE || backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_repository(validate_repository_fn, validation_context, STORAGE_DATA_MOUNT);
    if (result == APP_ERROR_NONE) {
        result = remove_tree_if_present(manifest->staging, operations, remove_tree, remove_context);
    }
    return result == APP_ERROR_NONE ? remove_manifest(manifest, operations) : result;
}

static app_error_code_t recover_staged(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context) {
    app_error_code_t result =
        validate_repository(validate_repository_fn, validation_context, manifest->staging);
    if (result == APP_ERROR_NONE) {
        result = move_repository_items(STORAGE_DATA_MOUNT, manifest->backup, operations);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_repository(validate_repository_fn, validation_context, manifest->backup);
    }
    return result == APP_ERROR_NONE
               ? write_phase(manifest, STORAGE_TRANSACTION_BACKED_UP, operations, generate_uuid,
                             uuid_context)
               : result;
}

static app_error_code_t recover_backed_up(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context) {
    app_error_code_t result =
        validate_repository(validate_repository_fn, validation_context, manifest->backup);
    if (result == APP_ERROR_NONE) {
        result = move_repository_items(manifest->staging, STORAGE_DATA_MOUNT, operations);
    }
    if (result == APP_ERROR_NONE) {
        result =
            validate_repository(validate_repository_fn, validation_context, STORAGE_DATA_MOUNT);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_repository(validate_repository_fn, validation_context, manifest->backup);
    }
    return result == APP_ERROR_NONE
               ? write_phase(manifest, STORAGE_TRANSACTION_ACTIVATED, operations, generate_uuid,
                             uuid_context)
               : result;
}

static app_error_code_t validate_activated(
    const storage_transaction_manifest_t *manifest,
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context) {
    app_error_code_t result =
        validate_repository(validate_repository_fn, validation_context, STORAGE_DATA_MOUNT);
    if (result == APP_ERROR_NONE) {
        result = validate_repository(validate_repository_fn, validation_context, manifest->backup);
    }
    return result;
}

static app_error_code_t recover_complete(
    const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    app_error_code_t result =
        validate_repository(validate_repository_fn, validation_context, STORAGE_DATA_MOUNT);
    if (result == APP_ERROR_NONE) {
        result = remove_tree_if_present(manifest->backup, operations, remove_tree, remove_context);
    }
    if (result == APP_ERROR_NONE) {
        result = remove_tree_if_present(manifest->staging, operations, remove_tree, remove_context);
    }
    return result == APP_ERROR_NONE ? remove_manifest(manifest, operations) : result;
}

app_error_code_t storage_transaction_recover_restore_with_ops(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_repository_fn validate_repository_fn, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    if (manifest == NULL || !storage_fs_ops_has_directory(operations) || generate_uuid == NULL ||
        validate_repository_fn == NULL || remove_tree == NULL ||
        !restore_manifest_paths_valid(manifest)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (manifest->phase == STORAGE_TRANSACTION_PREPARED) {
        return recover_prepared(manifest, operations, validate_repository_fn, validation_context,
                                remove_tree, remove_context);
    }
    app_error_code_t result = APP_ERROR_NONE;
    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        result = recover_staged(manifest, operations, generate_uuid, uuid_context,
                                validate_repository_fn, validation_context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_BACKED_UP) {
        result = recover_backed_up(manifest, operations, generate_uuid, uuid_context,
                                   validate_repository_fn, validation_context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_ACTIVATED) {
        result = validate_activated(manifest, validate_repository_fn, validation_context);
        if (result == APP_ERROR_NONE) {
            result = write_phase(manifest, STORAGE_TRANSACTION_INDEXED, operations, generate_uuid,
                                 uuid_context);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result = validate_activated(manifest, validate_repository_fn, validation_context);
        if (result == APP_ERROR_NONE) {
            result = write_phase(manifest, STORAGE_TRANSACTION_COMPLETE, operations, generate_uuid,
                                 uuid_context);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return manifest->phase == STORAGE_TRANSACTION_COMPLETE
               ? recover_complete(manifest, operations, validate_repository_fn,
                                  validation_context, remove_tree, remove_context)
               : APP_ERROR_STORAGE_CORRUPT;
}
