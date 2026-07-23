#include "storage.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "storage_repository_internal.h"

#define STORAGE_TRANSACTION_MAX_ACTIVE 16U

static app_error_code_t map_file_error(void)
{
    if (errno == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (errno == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static bool safe_manifest_path(const char *path)
{
    const size_t prefix_length = strlen(STORAGE_DATA_MOUNT "/transactions/");
    return path != NULL && strncmp(path, STORAGE_DATA_MOUNT "/transactions/", prefix_length) == 0 &&
           strstr(path, "..") == NULL && strchr(path + prefix_length, '/') == NULL;
}

static app_error_code_t manifest_path(const app_uuid_t *id, char *path, size_t path_size)
{
    if (id == NULL || path == NULL || !app_uuid_is_valid_string(id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(path, path_size, STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 id->value);
    if (written < 0 || (size_t)written >= path_size || !safe_manifest_path(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_transaction_write_manifest(const storage_transaction_manifest_t *manifest)
{
    if (manifest == NULL || manifest->schema_version != APP_SCHEMA_VERSION ||
        !app_uuid_is_valid_string(manifest->id.value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = manifest_path(&manifest->id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return storage_atomic_write(path, manifest, sizeof(*manifest), true);
}

static app_error_code_t read_manifest(const char *path,
                                      storage_transaction_manifest_t *out_manifest)
{
    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return map_file_error();
    }
    if (metadata.st_size != (off_t)sizeof(*out_manifest)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    int descriptor = open(path, O_RDONLY);
    if (descriptor < 0) {
        return map_file_error();
    }
    size_t offset = 0U;
    app_error_code_t result = APP_ERROR_NONE;
    while (offset < sizeof(*out_manifest)) {
        const ssize_t count = read(descriptor, (uint8_t *)out_manifest + offset,
                                   sizeof(*out_manifest) - offset);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            result = map_file_error();
            break;
        }
        if (count == 0) {
            result = APP_ERROR_IO;
            break;
        }
        offset += (size_t)count;
    }
    if (close(descriptor) != 0 && result == APP_ERROR_NONE) {
        result = map_file_error();
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (out_manifest->schema_version != APP_SCHEMA_VERSION ||
        !app_uuid_is_valid_string(out_manifest->id.value) ||
        memchr(out_manifest->source, '\0', sizeof(out_manifest->source)) == NULL ||
        memchr(out_manifest->staging, '\0', sizeof(out_manifest->staging)) == NULL ||
        memchr(out_manifest->destination, '\0', sizeof(out_manifest->destination)) == NULL ||
        memchr(out_manifest->backup, '\0', sizeof(out_manifest->backup)) == NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static bool path_exists(const char *path)
{
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static app_error_code_t parse_set_id_from_path(const char *path,
                                               const char *prefix,
                                               app_uuid_t *out_id)
{
    if (path == NULL || prefix == NULL || out_id == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t prefix_length = strlen(prefix);
    if (strncmp(path, prefix, prefix_length) != 0 || strchr(path + prefix_length, '/') != NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return app_uuid_parse(path + prefix_length, out_id) == APP_ERROR_NONE
               ? APP_ERROR_NONE
               : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t remove_manifest(const storage_transaction_manifest_t *manifest)
{
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = manifest_path(&manifest->id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return unlink(path) == 0 ? APP_ERROR_NONE : map_file_error();
}

static app_error_code_t recover_create(storage_transaction_manifest_t *manifest)
{
    if (manifest->staging[0] == '\0' || manifest->destination[0] == '\0' ||
        strncmp(manifest->staging, STORAGE_DATA_MOUNT "/staging/",
                strlen(STORAGE_DATA_MOUNT "/staging/")) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    app_uuid_t set_id = {0};
    app_error_code_t result = parse_set_id_from_path(
        manifest->destination, STORAGE_DATA_MOUNT "/sets/", &set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        const bool staging_exists = path_exists(manifest->staging);
        const bool destination_exists = path_exists(manifest->destination);
        if (staging_exists == destination_exists) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        if (staging_exists && rename(manifest->staging, manifest->destination) != 0) {
            return map_file_error();
        }
        manifest->phase = STORAGE_TRANSACTION_ACTIVATED;
        result = storage_transaction_write_manifest(manifest);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_ACTIVATED) {
        if (!path_exists(manifest->destination)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        result = storage_repository_set_index_presence(&set_id, true);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        manifest->phase = STORAGE_TRANSACTION_INDEXED;
        result = storage_transaction_write_manifest(manifest);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result = storage_repository_set_index_presence(&set_id, true);
        return result == APP_ERROR_NONE ? remove_manifest(manifest) : result;
    }
    return APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t recover_delete(storage_transaction_manifest_t *manifest)
{
    if (manifest->source[0] == '\0' || manifest->backup[0] == '\0' ||
        strncmp(manifest->backup, STORAGE_DATA_MOUNT "/trash/",
                strlen(STORAGE_DATA_MOUNT "/trash/")) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    app_uuid_t set_id = {0};
    app_error_code_t result = parse_set_id_from_path(
        manifest->source, STORAGE_DATA_MOUNT "/sets/", &set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    if (manifest->phase == STORAGE_TRANSACTION_PREPARED) {
        const bool source_exists = path_exists(manifest->source);
        const bool backup_exists = path_exists(manifest->backup);
        if (source_exists == backup_exists) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        if (source_exists && rename(manifest->source, manifest->backup) != 0) {
            return map_file_error();
        }
        manifest->phase = STORAGE_TRANSACTION_BACKED_UP;
        result = storage_transaction_write_manifest(manifest);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_BACKED_UP) {
        if (!path_exists(manifest->backup) || path_exists(manifest->source)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        result = storage_repository_set_index_presence(&set_id, false);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        manifest->phase = STORAGE_TRANSACTION_INDEXED;
        result = storage_transaction_write_manifest(manifest);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    if (manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result = storage_repository_set_index_presence(&set_id, false);
        return result == APP_ERROR_NONE ? remove_manifest(manifest) : result;
    }
    return APP_ERROR_STORAGE_CORRUPT;
}

static int compare_paths(const void *left, const void *right)
{
    const char *const *left_path = left;
    const char *const *right_path = right;
    return strcmp(*left_path, *right_path);
}

static app_error_code_t collect_manifest_paths(char paths[][APP_PATH_MAX_BYTES],
                                               size_t *out_count)
{
    DIR *directory = opendir(STORAGE_DATA_MOUNT "/transactions");
    if (directory == NULL) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
    size_t count = 0U;
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            break;
        }
        if (entry->d_name[0] == '.') {
            continue;
        }
        if (count >= STORAGE_TRANSACTION_MAX_ACTIVE) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        const int written = snprintf(paths[count], APP_PATH_MAX_BYTES,
                                     STORAGE_DATA_MOUNT "/transactions/%s",
                                     entry->d_name);
        if (written < 0 || (size_t)written >= APP_PATH_MAX_BYTES ||
            !safe_manifest_path(paths[count])) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        ++count;
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_file_error();
    }
    if (result == APP_ERROR_NONE) {
        qsort(paths, count, sizeof(paths[0]), compare_paths);
        *out_count = count;
    }
    return result;
}

static bool directory_has_entries(const char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return true;
    }
    bool found = false;
    while (true) {
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            break;
        }
        if (entry->d_name[0] != '.') {
            found = true;
            break;
        }
    }
    if (closedir(directory) != 0) {
        return true;
    }
    return found;
}

app_error_code_t storage_transaction_recover_all(void)
{
    char paths[STORAGE_TRANSACTION_MAX_ACTIVE][APP_PATH_MAX_BYTES];
    size_t count = 0U;
    app_error_code_t result = collect_manifest_paths(paths, &count);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    for (size_t index = 0U; index < count; ++index) {
        storage_transaction_manifest_t manifest = {0};
        result = read_manifest(paths[index], &manifest);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        switch (manifest.type) {
        case STORAGE_TRANSACTION_IMPORT_SET:
        case STORAGE_TRANSACTION_DUPLICATE_SET:
            result = recover_create(&manifest);
            break;
        case STORAGE_TRANSACTION_DELETE_SET:
            result = recover_delete(&manifest);
            break;
        case STORAGE_TRANSACTION_REPLACE_SET:
        case STORAGE_TRANSACTION_RESTORE:
        case STORAGE_TRANSACTION_MIGRATE:
        default:
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return directory_has_entries(STORAGE_DATA_MOUNT "/staging")
               ? APP_ERROR_STORAGE_CORRUPT
               : APP_ERROR_NONE;
}
