#include "storage.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "storage_repository_internal.h"
#include "storage_transaction_internal.h"

#define STORAGE_TRANSACTION_MAX_ACTIVE 16U
#define STORAGE_TRANSACTION_SUFFIX ".bin"

static app_error_code_t map_error_number(int error_number)
{
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static app_error_code_t production_uuid_generate(void *context,
                                                 app_uuid_t *out_uuid)
{
    (void)context;
    return app_uuid_generate(out_uuid);
}

static app_error_code_t production_set_index_presence(void *context,
                                                       const app_uuid_t *set_id,
                                                       bool should_be_present)
{
    (void)context;
    return storage_repository_set_index_presence(set_id, should_be_present);
}

static bool operations_valid(
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    storage_transaction_set_index_presence_fn set_index_presence)
{
    return storage_fs_ops_has_directory(operations) && generate_uuid != NULL &&
           set_index_presence != NULL;
}

static bool safe_manifest_path(const char *path)
{
    static const char prefix[] = STORAGE_DATA_MOUNT "/transactions/";
    if (path == NULL || strncmp(path, prefix, sizeof(prefix) - 1U) != 0) {
        return false;
    }
    const char *name = path + sizeof(prefix) - 1U;
    const size_t name_length = strlen(name);
    return name_length == APP_UUID_STRING_LENGTH + sizeof(STORAGE_TRANSACTION_SUFFIX) - 1U &&
           strchr(name, '/') == NULL && strstr(name, "..") == NULL &&
           strcmp(name + APP_UUID_STRING_LENGTH, STORAGE_TRANSACTION_SUFFIX) == 0;
}

static app_error_code_t manifest_id_from_path(const char *path,
                                              app_uuid_t *out_id)
{
    static const char prefix[] = STORAGE_DATA_MOUNT "/transactions/";
    if (out_id != NULL) {
        memset(out_id, 0, sizeof(*out_id));
    }
    if (!safe_manifest_path(path) || out_id == NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char value[APP_UUID_STRING_LENGTH + 1U];
    memcpy(value, path + sizeof(prefix) - 1U, APP_UUID_STRING_LENGTH);
    value[APP_UUID_STRING_LENGTH] = '\0';
    return app_uuid_parse(value, out_id) == APP_ERROR_NONE
               ? APP_ERROR_NONE
               : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t manifest_path(const app_uuid_t *id,
                                      char *path,
                                      size_t path_size)
{
    if (id == NULL || path == NULL || !app_uuid_is_valid_string(id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(path,
                                 path_size,
                                 STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 id->value);
    if (written < 0 || (size_t)written >= path_size || !safe_manifest_path(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static bool manifest_strings_terminated(
    const storage_transaction_manifest_t *manifest)
{
    return memchr(manifest->source, '\0', sizeof(manifest->source)) != NULL &&
           memchr(manifest->staging, '\0', sizeof(manifest->staging)) != NULL &&
           memchr(manifest->destination, '\0', sizeof(manifest->destination)) != NULL &&
           memchr(manifest->backup, '\0', sizeof(manifest->backup)) != NULL;
}

static bool manifest_revisions_valid(
    const storage_transaction_manifest_t *manifest)
{
    switch (manifest->type) {
    case STORAGE_TRANSACTION_IMPORT_SET:
    case STORAGE_TRANSACTION_DUPLICATE_SET:
        return manifest->expected_revision == 0U &&
               manifest->replacement_revision != 0U;
    case STORAGE_TRANSACTION_DELETE_SET:
        return manifest->expected_revision != 0U &&
               manifest->replacement_revision == 0U;
    case STORAGE_TRANSACTION_REPLACE_SET:
    case STORAGE_TRANSACTION_RESTORE:
    case STORAGE_TRANSACTION_MIGRATE:
        return true;
    default:
        return false;
    }
}

static bool manifest_shape_valid(
    const storage_transaction_manifest_t *manifest)
{
    return manifest != NULL && manifest->schema_version == APP_SCHEMA_VERSION &&
           app_uuid_is_valid_string(manifest->id.value) &&
           manifest->type >= STORAGE_TRANSACTION_IMPORT_SET &&
           manifest->type <= STORAGE_TRANSACTION_MIGRATE &&
           manifest->phase >= STORAGE_TRANSACTION_PREPARED &&
           manifest->phase <= STORAGE_TRANSACTION_COMPLETE &&
           manifest_strings_terminated(manifest) &&
           manifest_revisions_valid(manifest);
}

app_error_code_t storage_transaction_write_manifest_with_ops(
    const storage_transaction_manifest_t *manifest,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context)
{
    if (!manifest_shape_valid(manifest) ||
        !storage_fs_ops_is_valid(operations) || generate_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result =
        manifest_path(&manifest->id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return storage_atomic_write_with_ops(path,
                                         manifest,
                                         sizeof(*manifest),
                                         true,
                                         operations,
                                         generate_uuid,
                                         uuid_context);
}

app_error_code_t storage_transaction_write_manifest(
    const storage_transaction_manifest_t *manifest)
{
    return storage_transaction_write_manifest_with_ops(manifest,
                                                       storage_fs_ops_posix(),
                                                       production_uuid_generate,
                                                       NULL);
}

static app_error_code_t read_manifest_with_ops(
    const char *path,
    storage_transaction_manifest_t *out_manifest,
    const storage_fs_ops_t *operations)
{
    if (out_manifest != NULL) {
        memset(out_manifest, 0, sizeof(*out_manifest));
    }
    if (path == NULL || out_manifest == NULL ||
        !storage_fs_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_uuid_t path_id = {0};
    app_error_code_t result = manifest_id_from_path(path, &path_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        const int stat_error = errno;
        return map_error_number(stat_error);
    }
    if (metadata.st_size != (off_t)sizeof(*out_manifest)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const int descriptor =
        operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        const int open_error = errno;
        return map_error_number(open_error);
    }

    size_t offset = 0U;
    result = APP_ERROR_NONE;
    while (offset < sizeof(*out_manifest)) {
        const ssize_t count = operations->read_file(
            operations->context,
            descriptor,
            (uint8_t *)out_manifest + offset,
            sizeof(*out_manifest) - offset);
        if (count < 0) {
            const int read_error = errno;
            if (read_error == EINTR) {
                continue;
            }
            result = map_error_number(read_error);
            break;
        }
        if (count == 0) {
            result = APP_ERROR_IO;
            break;
        }
        offset += (size_t)count;
    }
    if (operations->close_file(operations->context, descriptor) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result != APP_ERROR_NONE) {
        memset(out_manifest, 0, sizeof(*out_manifest));
        return result;
    }
    if (!manifest_shape_valid(out_manifest) ||
        !app_uuid_equal(&path_id, &out_manifest->id)) {
        memset(out_manifest, 0, sizeof(*out_manifest));
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t path_exists_with_ops(const char *path,
                                             const storage_fs_ops_t *operations,
                                             bool *out_exists)
{
    if (out_exists != NULL) {
        *out_exists = false;
    }
    if (path == NULL || out_exists == NULL ||
        !storage_fs_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        *out_exists = true;
        return APP_ERROR_NONE;
    }
    const int stat_error = errno;
    return stat_error == ENOENT ? APP_ERROR_NONE : map_error_number(stat_error);
}

static app_error_code_t parse_set_id_from_path(const char *path,
                                                const char *prefix,
                                                app_uuid_t *out_id)
{
    if (path == NULL || prefix == NULL || out_id == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t prefix_length = strlen(prefix);
    if (strncmp(path, prefix, prefix_length) != 0 ||
        strchr(path + prefix_length, '/') != NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return app_uuid_parse(path + prefix_length, out_id) == APP_ERROR_NONE
               ? APP_ERROR_NONE
               : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t remove_manifest_with_ops(
    const storage_transaction_manifest_t *manifest,
    const storage_fs_ops_t *operations)
{
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result =
        manifest_path(&manifest->id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (operations->unlink_path(operations->context, path) != 0) {
        const int unlink_error = errno;
        return map_error_number(unlink_error);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t write_recovery_manifest(
    const storage_transaction_manifest_t *manifest,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context)
{
    const app_error_code_t result = storage_transaction_write_manifest_with_ops(
        manifest, operations, generate_uuid, uuid_context);
    return result == APP_ERROR_INVALID_ARGUMENT ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t recover_create(
    storage_transaction_manifest_t *manifest,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence,
    void *index_context)
{
    char expected_staging[APP_PATH_MAX_BYTES];
    const int staging_length = snprintf(expected_staging,
                                        sizeof(expected_staging),
                                        STORAGE_DATA_MOUNT "/staging/%s",
                                        manifest->id.value);
    if (staging_length < 0 ||
        (size_t)staging_length >= sizeof(expected_staging) ||
        strcmp(manifest->staging, expected_staging) != 0 ||
        manifest->source[0] != '\0' || manifest->backup[0] != '\0') {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    app_uuid_t set_id = {0};
    app_error_code_t result = parse_set_id_from_path(
        manifest->destination, STORAGE_DATA_MOUNT "/sets/", &set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        bool staging_exists = false;
        bool destination_exists = false;
        result = path_exists_with_ops(manifest->staging,
                                      operations,
                                      &staging_exists);
        if (result == APP_ERROR_NONE) {
            result = path_exists_with_ops(manifest->destination,
                                          operations,
                                          &destination_exists);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (staging_exists == destination_exists) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        if (staging_exists && operations->rename_path(
                                  operations->context,
                                  manifest->staging,
                                  manifest->destination) != 0) {
            const int rename_error = errno;
            return map_error_number(rename_error);
        }
        manifest->phase = STORAGE_TRANSACTION_ACTIVATED;
        result = write_recovery_manifest(manifest,
                                         operations,
                                         generate_uuid,
                                         uuid_context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_ACTIVATED) {
        bool destination_exists = false;
        result = path_exists_with_ops(manifest->destination,
                                      operations,
                                      &destination_exists);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (!destination_exists) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        result = set_index_presence(index_context, &set_id, true);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        manifest->phase = STORAGE_TRANSACTION_INDEXED;
        result = write_recovery_manifest(manifest,
                                         operations,
                                         generate_uuid,
                                         uuid_context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result = set_index_presence(index_context, &set_id, true);
        return result == APP_ERROR_NONE
                   ? remove_manifest_with_ops(manifest, operations)
                   : result;
    }
    return APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t recover_delete(
    storage_transaction_manifest_t *manifest,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence,
    void *index_context)
{
    if (manifest->staging[0] != '\0' || manifest->destination[0] != '\0') {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    app_uuid_t set_id = {0};
    app_error_code_t result = parse_set_id_from_path(
        manifest->source, STORAGE_DATA_MOUNT "/sets/", &set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char expected_backup[APP_PATH_MAX_BYTES];
    const int backup_length = snprintf(expected_backup,
                                       sizeof(expected_backup),
                                       STORAGE_DATA_MOUNT "/trash/%s-%s",
                                       set_id.value,
                                       manifest->id.value);
    if (backup_length < 0 ||
        (size_t)backup_length >= sizeof(expected_backup) ||
        strcmp(manifest->backup, expected_backup) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    if (manifest->phase == STORAGE_TRANSACTION_PREPARED) {
        bool source_exists = false;
        bool backup_exists = false;
        result = path_exists_with_ops(manifest->source,
                                      operations,
                                      &source_exists);
        if (result == APP_ERROR_NONE) {
            result = path_exists_with_ops(manifest->backup,
                                          operations,
                                          &backup_exists);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (source_exists == backup_exists) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        if (source_exists && operations->rename_path(
                                 operations->context,
                                 manifest->source,
                                 manifest->backup) != 0) {
            const int rename_error = errno;
            return map_error_number(rename_error);
        }
        manifest->phase = STORAGE_TRANSACTION_BACKED_UP;
        result = write_recovery_manifest(manifest,
                                         operations,
                                         generate_uuid,
                                         uuid_context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_BACKED_UP) {
        bool backup_exists = false;
        bool source_exists = false;
        result = path_exists_with_ops(manifest->backup,
                                      operations,
                                      &backup_exists);
        if (result == APP_ERROR_NONE) {
            result = path_exists_with_ops(manifest->source,
                                          operations,
                                          &source_exists);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (!backup_exists || source_exists) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        result = set_index_presence(index_context, &set_id, false);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        manifest->phase = STORAGE_TRANSACTION_INDEXED;
        result = write_recovery_manifest(manifest,
                                         operations,
                                         generate_uuid,
                                         uuid_context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result = set_index_presence(index_context, &set_id, false);
        return result == APP_ERROR_NONE
                   ? remove_manifest_with_ops(manifest, operations)
                   : result;
    }
    return APP_ERROR_STORAGE_CORRUPT;
}

static int compare_paths(const void *left, const void *right)
{
    const char *const *left_path = left;
    const char *const *right_path = right;
    return strcmp(*left_path, *right_path);
}

static app_error_code_t collect_manifest_paths_with_ops(
    char paths[][APP_PATH_MAX_BYTES],
    size_t *out_count,
    const storage_fs_ops_t *operations)
{
    if (out_count != NULL) {
        *out_count = 0U;
    }
    if (paths == NULL || out_count == NULL ||
        !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    void *directory = operations->open_directory(
        operations->context, STORAGE_DATA_MOUNT "/transactions");
    if (directory == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_STORAGE_UNAVAILABLE
                                    : map_error_number(open_error);
    }

    size_t count = 0U;
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context,
                                       directory,
                                       name,
                                       sizeof(name),
                                       &end) != 0) {
            const int read_error = errno;
            result = map_error_number(read_error);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 ||
            name[0] == '.') {
            continue;
        }
        if (count >= STORAGE_TRANSACTION_MAX_ACTIVE) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        const int written = snprintf(paths[count],
                                     APP_PATH_MAX_BYTES,
                                     STORAGE_DATA_MOUNT "/transactions/%s",
                                     name);
        if (written < 0 || (size_t)written >= APP_PATH_MAX_BYTES ||
            !safe_manifest_path(paths[count])) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        ++count;
    }
    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result == APP_ERROR_NONE) {
        qsort(paths, count, sizeof(paths[0]), compare_paths);
        *out_count = count;
    }
    return result;
}

static app_error_code_t directory_has_entries_with_ops(
    const char *path,
    const storage_fs_ops_t *operations,
    bool *out_has_entries)
{
    if (out_has_entries != NULL) {
        *out_has_entries = false;
    }
    if (path == NULL || out_has_entries == NULL ||
        !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    void *directory = operations->open_directory(operations->context, path);
    if (directory == NULL) {
        const int open_error = errno;
        return map_error_number(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context,
                                       directory,
                                       name,
                                       sizeof(name),
                                       &end) != 0) {
            const int read_error = errno;
            result = map_error_number(read_error);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 &&
            name[0] != '.') {
            *out_has_entries = true;
            break;
        }
    }
    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result != APP_ERROR_NONE) {
        *out_has_entries = false;
    }
    return result;
}

app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence,
    void *index_context)
{
    if (!operations_valid(operations, generate_uuid, set_index_presence)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char paths[STORAGE_TRANSACTION_MAX_ACTIVE][APP_PATH_MAX_BYTES];
    size_t count = 0U;
    app_error_code_t result =
        collect_manifest_paths_with_ops(paths, &count, operations);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    app_error_code_t first_error = APP_ERROR_NONE;
    for (size_t index = 0U; index < count; ++index) {
        storage_transaction_manifest_t manifest = {0};
        result = read_manifest_with_ops(paths[index], &manifest, operations);
        if (result == APP_ERROR_NONE) {
            switch (manifest.type) {
            case STORAGE_TRANSACTION_IMPORT_SET:
            case STORAGE_TRANSACTION_DUPLICATE_SET:
                result = recover_create(&manifest,
                                        operations,
                                        generate_uuid,
                                        uuid_context,
                                        set_index_presence,
                                        index_context);
                break;
            case STORAGE_TRANSACTION_DELETE_SET:
                result = recover_delete(&manifest,
                                        operations,
                                        generate_uuid,
                                        uuid_context,
                                        set_index_presence,
                                        index_context);
                break;
            case STORAGE_TRANSACTION_REPLACE_SET:
            case STORAGE_TRANSACTION_RESTORE:
            case STORAGE_TRANSACTION_MIGRATE:
            default:
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
        }
        if (result != APP_ERROR_NONE && first_error == APP_ERROR_NONE) {
            first_error = result;
        }
    }

    bool staging_has_entries = false;
    result = directory_has_entries_with_ops(STORAGE_DATA_MOUNT "/staging",
                                            operations,
                                            &staging_has_entries);
    if (result != APP_ERROR_NONE && first_error == APP_ERROR_NONE) {
        first_error = result;
    }
    if (staging_has_entries && first_error == APP_ERROR_NONE) {
        first_error = APP_ERROR_STORAGE_CORRUPT;
    }
    return first_error;
}

app_error_code_t storage_transaction_recover_all(void)
{
    return storage_transaction_recover_all_with_ops(
        storage_fs_ops_posix(),
        production_uuid_generate,
        NULL,
        production_set_index_presence,
        NULL);
}
