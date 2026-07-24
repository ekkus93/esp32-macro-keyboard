#include "storage_repository_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "storage.h"

app_error_code_t storage_repository_map_error_number(int error_number)
{
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

app_error_code_t storage_repository_map_file_error(void)
{
    return storage_repository_map_error_number(errno);
}

static app_error_code_t production_uuid_generate(void *context, app_uuid_t *out_uuid)
{
    (void)context;
    return app_uuid_generate(out_uuid);
}

app_error_code_t storage_repository_read_bounded_file_with_ops(
    const char *path,
    size_t maximum,
    char **out_data,
    size_t *out_length,
    const storage_fs_ops_t *operations)
{
    if (out_data != NULL) {
        *out_data = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (path == NULL || out_data == NULL || out_length == NULL || maximum == 0U ||
        !storage_fs_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        const int stat_error = errno;
        return storage_repository_map_error_number(stat_error);
    }
    if (metadata.st_size < 0 || (uint64_t)metadata.st_size > maximum ||
        (uint64_t)metadata.st_size > SIZE_MAX - 1U) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    if (data == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int descriptor = operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        const int open_error = errno;
        free(data);
        return storage_repository_map_error_number(open_error);
    }

    app_error_code_t result = APP_ERROR_NONE;
    size_t offset = 0U;
    while (offset < length) {
        const ssize_t count = operations->read_file(
            operations->context, descriptor, data + offset, length - offset);
        if (count < 0) {
            const int read_error = errno;
            if (read_error == EINTR) {
                continue;
            }
            result = storage_repository_map_error_number(read_error);
            break;
        }
        if (count == 0) {
            result = APP_ERROR_IO;
            break;
        }
        offset += (size_t)count;
    }
    if (result == APP_ERROR_NONE) {
        char extra = '\0';
        const ssize_t count = operations->read_file(
            operations->context, descriptor, &extra, 1U);
        if (count < 0) {
            result = storage_repository_map_error_number(errno);
        } else if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = storage_repository_map_error_number(close_error);
    }
    if (result != APP_ERROR_NONE) {
        free(data);
        return result;
    }

    data[length] = '\0';
    *out_data = data;
    *out_length = length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_read_bounded_file(const char *path,
                                                       size_t maximum,
                                                       char **out_data,
                                                       size_t *out_length)
{
    return storage_repository_read_bounded_file_with_ops(
        path, maximum, out_data, out_length, storage_fs_ops_posix());
}

app_error_code_t storage_repository_directory_has_entries_with_ops(
    const char *path,
    const storage_fs_ops_t *operations,
    bool *out_has_entries)
{
    if (out_has_entries != NULL) {
        *out_has_entries = false;
    }
    if (path == NULL || out_has_entries == NULL || !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    void *directory = operations->open_directory(operations->context, path);
    if (directory == NULL) {
        const int open_error = errno;
        return storage_repository_map_error_number(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(
                operations->context, directory, name, sizeof(name), &end) != 0) {
            const int read_error = errno;
            result = storage_repository_map_error_number(read_error);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && name[0] != '.') {
            *out_has_entries = true;
            break;
        }
    }
    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = storage_repository_map_error_number(close_error);
    }
    if (result != APP_ERROR_NONE) {
        *out_has_entries = false;
    }
    return result;
}

app_error_code_t storage_repository_directory_has_entries(const char *path,
                                                           bool *out_has_entries)
{
    return storage_repository_directory_has_entries_with_ops(
        path, storage_fs_ops_posix(), out_has_entries);
}

app_error_code_t storage_repository_ensure_initial_file_with_ops(
    const char *path,
    const char *contents,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context)
{
    if (path == NULL || contents == NULL || !storage_fs_ops_is_valid(operations) ||
        generate_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        return APP_ERROR_NONE;
    }
    const int stat_error = errno;
    if (stat_error != ENOENT) {
        return storage_repository_map_error_number(stat_error);
    }
    return storage_atomic_write_with_ops(path,
                                         contents,
                                         strlen(contents),
                                         true,
                                         operations,
                                         generate_uuid,
                                         uuid_context);
}

app_error_code_t storage_repository_ensure_initial_file(const char *path,
                                                         const char *contents)
{
    return storage_repository_ensure_initial_file_with_ops(path,
                                                            contents,
                                                            storage_fs_ops_posix(),
                                                            production_uuid_generate,
                                                            NULL);
}

app_error_code_t storage_repository_set_file_path(const app_uuid_t *set_id,
                                                   char *buffer,
                                                   size_t buffer_size)
{
    char directory[APP_PATH_MAX_BYTES];
    const app_error_code_t result =
        storage_make_set_path(set_id, directory, sizeof(directory));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const int written = snprintf(buffer, buffer_size, "%s/set.json", directory);
    return written >= 0 && (size_t)written < buffer_size ? APP_ERROR_NONE
                                                         : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t storage_repository_transaction_path(
    const app_uuid_t *transaction_id,
    char *buffer,
    size_t buffer_size)
{
    if (transaction_id == NULL || buffer == NULL ||
        !app_uuid_is_valid_string(transaction_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(buffer,
                                 buffer_size,
                                 STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 transaction_id->value);
    return written >= 0 && (size_t)written < buffer_size ? APP_ERROR_NONE
                                                         : APP_ERROR_INVALID_ARGUMENT;
}

app_error_code_t storage_repository_remove_manifest_with_ops(
    const app_uuid_t *transaction_id,
    const storage_fs_ops_t *operations)
{
    if (!storage_fs_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = storage_repository_transaction_path(
        transaction_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (operations->unlink_path(operations->context, path) == 0) {
        return APP_ERROR_NONE;
    }
    const int unlink_error = errno;
    return storage_repository_map_error_number(unlink_error);
}

app_error_code_t storage_repository_remove_manifest(const app_uuid_t *transaction_id)
{
    return storage_repository_remove_manifest_with_ops(
        transaction_id, storage_fs_ops_posix());
}

app_error_code_t storage_repository_make_directory_with_ops(
    const char *path,
    const storage_fs_ops_t *operations)
{
    if (path == NULL || !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (operations->make_directory(operations->context, path, 0750) == 0) {
        return APP_ERROR_NONE;
    }
    const int mkdir_error = errno;
    return mkdir_error == EEXIST ? APP_ERROR_CONFLICT
                                  : storage_repository_map_error_number(mkdir_error);
}

app_error_code_t storage_repository_make_directory(const char *path)
{
    return storage_repository_make_directory_with_ops(path, storage_fs_ops_posix());
}

app_error_code_t storage_repository_remove_tree_with_ops(
    const char *path,
    const storage_fs_ops_t *operations)
{
    if (path == NULL || !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    void *directory = operations->open_directory(operations->context, path);
    if (directory == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_NONE
                                    : storage_repository_map_error_number(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(
                operations->context, directory, name, sizeof(name), &end) != 0) {
            const int read_error = errno;
            result = storage_repository_map_error_number(read_error);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        char child[APP_PATH_MAX_BYTES];
        const int written = snprintf(child, sizeof(child), "%s/%s", path, name);
        if (written < 0 || (size_t)written >= sizeof(child)) {
            result = APP_ERROR_INVALID_ARGUMENT;
            break;
        }
        struct stat metadata;
        if (operations->stat_path(operations->context, child, &metadata) != 0) {
            const int stat_error = errno;
            result = storage_repository_map_error_number(stat_error);
            break;
        }
        if (S_ISDIR(metadata.st_mode)) {
            result = storage_repository_remove_tree_with_ops(child, operations);
        } else if (operations->unlink_path(operations->context, child) != 0) {
            const int unlink_error = errno;
            result = storage_repository_map_error_number(unlink_error);
        }
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = storage_repository_map_error_number(close_error);
    }
    if (result == APP_ERROR_NONE &&
        operations->remove_directory(operations->context, path) != 0) {
        const int remove_error = errno;
        result = storage_repository_map_error_number(remove_error);
    }
    return result;
}

app_error_code_t storage_repository_remove_tree(const char *path)
{
    return storage_repository_remove_tree_with_ops(path, storage_fs_ops_posix());
}
