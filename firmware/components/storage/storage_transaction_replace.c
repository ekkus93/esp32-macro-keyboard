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

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static app_error_code_t path_is_directory(const char *path, const storage_fs_ops_t *operations,
                                          bool *out_exists) {
    *out_exists = false;
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        if (!S_ISDIR(metadata.st_mode)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
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
    if (storage_fs_sync_parent_path(operations->context, destination) != 0) {
        return map_error_number(errno);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t rename_and_sync(const char *source, const char *destination,
                                        const storage_fs_ops_t *operations) {
    if (operations->rename_path(operations->context, source, destination) != 0) {
        return map_error_number(errno);
    }
    return sync_rename_parents(source, destination, operations);
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
    const int written =
        snprintf(path, sizeof(path), STORAGE_DATA_MOUNT "/transactions/%s.bin", manifest->id.value);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (operations->unlink_path(operations->context, path) != 0) {
        return map_error_number(errno);
    }
    return storage_fs_sync_parent_path(operations->context, path) == 0 ? APP_ERROR_NONE
                                                                       : map_error_number(errno);
}

static app_error_code_t parse_set_path(const char *path, app_uuid_t *out_set_id) {
    static const char prefix[] = STORAGE_DATA_MOUNT "/sets/";
    const size_t prefix_length = sizeof(prefix) - 1U;
    if (path == NULL || out_set_id == NULL || strncmp(path, prefix, prefix_length) != 0 ||
        strchr(path + prefix_length, '/') != NULL ||
        app_uuid_parse(path + prefix_length, out_set_id) != APP_ERROR_NONE) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static bool staging_path_matches(const char *actual, const app_uuid_t *transaction_id) {
    char expected[APP_PATH_MAX_BYTES];
    const int written = snprintf(expected, sizeof(expected), STORAGE_DATA_MOUNT "/staging/%s",
                                 transaction_id->value);
    return written >= 0 && (size_t)written < sizeof(expected) && strcmp(actual, expected) == 0;
}

static bool backup_path_matches(const char *actual, const app_uuid_t *set_id,
                                const app_uuid_t *transaction_id) {
    char expected[APP_PATH_MAX_BYTES];
    const int written = snprintf(expected, sizeof(expected), STORAGE_DATA_MOUNT "/trash/%s-%s",
                                 set_id->value, transaction_id->value);
    return written >= 0 && (size_t)written < sizeof(expected) && strcmp(actual, expected) == 0;
}

static app_error_code_t validate_manifest_paths(const storage_transaction_manifest_t *manifest,
                                                app_uuid_t *out_set_id) {
    app_error_code_t result = parse_set_path(manifest->destination, out_set_id);
    if (result != APP_ERROR_NONE || strcmp(manifest->source, manifest->destination) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (!staging_path_matches(manifest->staging, &manifest->id) ||
        !backup_path_matches(manifest->backup, out_set_id, &manifest->id)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t validate_tree(storage_transaction_validate_set_fn validate_set,
                                      void *validation_context, const char *path,
                                      const app_uuid_t *set_id, uint32_t revision) {
    const app_error_code_t result = validate_set(validation_context, path, set_id, revision);
    return result == APP_ERROR_INVALID_ARGUMENT ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t
recover_prepared(const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
                 storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
                 storage_transaction_validate_set_fn validate_set, void *validation_context,
                 storage_transaction_remove_tree_fn remove_tree, void *remove_context,
                 const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || !destination_exists || backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                           manifest->expected_revision);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    if (result == APP_ERROR_NONE && staging_exists) {
        result = remove_tree(remove_context, manifest->staging);
        if (result == APP_ERROR_NONE &&
            storage_fs_sync_parent_path(operations->context, manifest->staging) != 0) {
            result = map_error_number(errno);
        }
    }
    return result == APP_ERROR_NONE ? remove_manifest(manifest, operations) : result;
}

static app_error_code_t recover_staged(storage_transaction_manifest_t *manifest,
                                       const storage_fs_ops_t *operations,
                                       storage_uuid_generate_fn generate_uuid, void *uuid_context,
                                       storage_transaction_validate_set_fn validate_set,
                                       void *validation_context, const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || !staging_exists || destination_exists == backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->staging, set_id,
                           manifest->replacement_revision);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (destination_exists) {
        result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                               manifest->expected_revision);
        if (result == APP_ERROR_NONE) {
            result = rename_and_sync(manifest->destination, manifest->backup, operations);
        }
    } else {
        result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                               manifest->expected_revision);
        if (result == APP_ERROR_NONE) {
            result = sync_rename_parents(manifest->destination, manifest->backup, operations);
        }
    }
    return result == APP_ERROR_NONE ? write_phase(manifest, STORAGE_TRANSACTION_BACKED_UP,
                                                  operations, generate_uuid, uuid_context)
                                    : result;
}

static app_error_code_t recover_backed_up(storage_transaction_manifest_t *manifest,
                                          const storage_fs_ops_t *operations,
                                          storage_uuid_generate_fn generate_uuid,
                                          void *uuid_context,
                                          storage_transaction_validate_set_fn validate_set,
                                          void *validation_context, const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || !backup_exists || staging_exists == destination_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                           manifest->expected_revision);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (staging_exists) {
        result = validate_tree(validate_set, validation_context, manifest->staging, set_id,
                               manifest->replacement_revision);
        if (result == APP_ERROR_NONE) {
            result = rename_and_sync(manifest->staging, manifest->destination, operations);
        }
    } else {
        result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                               manifest->replacement_revision);
        if (result == APP_ERROR_NONE) {
            result = sync_rename_parents(manifest->staging, manifest->destination, operations);
        }
    }
    return result == APP_ERROR_NONE ? write_phase(manifest, STORAGE_TRANSACTION_ACTIVATED,
                                                  operations, generate_uuid, uuid_context)
                                    : result;
}

static app_error_code_t validate_activated_paths(const storage_transaction_manifest_t *manifest,
                                                 const storage_fs_ops_t *operations,
                                                 storage_transaction_validate_set_fn validate_set,
                                                 void *validation_context,
                                                 const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || staging_exists || !destination_exists || !backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                           manifest->replacement_revision);
    if (result == APP_ERROR_NONE) {
        result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                               manifest->expected_revision);
    }
    return result;
}

static app_error_code_t
recover_activated(storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
                  storage_uuid_generate_fn generate_uuid, void *uuid_context,
                  storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
                  storage_transaction_validate_set_fn validate_set, void *validation_context,
                  const app_uuid_t *set_id) {
    app_error_code_t result =
        validate_activated_paths(manifest, operations, validate_set, validation_context, set_id);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    return result == APP_ERROR_NONE ? write_phase(manifest, STORAGE_TRANSACTION_INDEXED, operations,
                                                  generate_uuid, uuid_context)
                                    : result;
}

static app_error_code_t
recover_indexed(storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
                storage_uuid_generate_fn generate_uuid, void *uuid_context,
                storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
                storage_transaction_validate_set_fn validate_set, void *validation_context,
                const app_uuid_t *set_id) {
    app_error_code_t result =
        validate_activated_paths(manifest, operations, validate_set, validation_context, set_id);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    return result == APP_ERROR_NONE ? write_phase(manifest, STORAGE_TRANSACTION_COMPLETE,
                                                  operations, generate_uuid, uuid_context)
                                    : result;
}

static app_error_code_t
recover_complete(const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
                 storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
                 storage_transaction_validate_set_fn validate_set, void *validation_context,
                 storage_transaction_remove_tree_fn remove_tree, void *remove_context,
                 const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || staging_exists || !destination_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                           manifest->replacement_revision);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    if (result == APP_ERROR_NONE && backup_exists) {
        result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                               manifest->expected_revision);
    }
    if (result == APP_ERROR_NONE && backup_exists) {
        result = remove_tree(remove_context, manifest->backup);
        if (result == APP_ERROR_NONE &&
            storage_fs_sync_parent_path(operations->context, manifest->backup) != 0) {
            result = map_error_number(errno);
        }
    }
    return result == APP_ERROR_NONE ? remove_manifest(manifest, operations) : result;
}

app_error_code_t storage_transaction_recover_replace_with_ops(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    if (manifest == NULL || !storage_fs_ops_has_directory(operations) || generate_uuid == NULL ||
        set_index_presence == NULL || validate_set == NULL || remove_tree == NULL ||
        manifest->type != STORAGE_TRANSACTION_REPLACE_SET || manifest->expected_revision == 0U ||
        manifest->replacement_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_uuid_t set_id = {0};
    app_error_code_t result = validate_manifest_paths(manifest, &set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (manifest->phase == STORAGE_TRANSACTION_PREPARED) {
        return recover_prepared(manifest, operations, set_index_presence, index_context,
                                validate_set, validation_context, remove_tree, remove_context,
                                &set_id);
    }
    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        result = recover_staged(manifest, operations, generate_uuid, uuid_context, validate_set,
                                validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_BACKED_UP) {
        result = recover_backed_up(manifest, operations, generate_uuid, uuid_context, validate_set,
                                   validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_ACTIVATED) {
        result =
            recover_activated(manifest, operations, generate_uuid, uuid_context, set_index_presence,
                              index_context, validate_set, validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result =
            recover_indexed(manifest, operations, generate_uuid, uuid_context, set_index_presence,
                            index_context, validate_set, validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_COMPLETE) {
        result =
            recover_complete(manifest, operations, set_index_presence, index_context, validate_set,
                             validation_context, remove_tree, remove_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase < STORAGE_TRANSACTION_STAGED) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}
