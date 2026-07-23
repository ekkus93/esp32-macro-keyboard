#include "storage_repository_internal.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "storage.h"

app_error_code_t storage_repository_map_file_error(void)
{
    if (errno == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (errno == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

app_error_code_t storage_repository_read_bounded_file(const char *path,
                                          size_t maximum,
                                          char **out_data,
                                          size_t *out_length)
{
    if (path == NULL || out_data == NULL || out_length == NULL || maximum == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_data = NULL;
    *out_length = 0U;

    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return storage_repository_map_file_error();
    }
    if (metadata.st_size < 0 || (uint64_t)metadata.st_size > maximum) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    if (data == NULL) {
        return APP_ERROR_INTERNAL;
    }
    int descriptor = open(path, O_RDONLY);
    if (descriptor < 0) {
        free(data);
        return storage_repository_map_file_error();
    }

    app_error_code_t result = APP_ERROR_NONE;
    size_t offset = 0U;
    while (offset < length) {
        const ssize_t count = read(descriptor, data + offset, length - offset);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            result = storage_repository_map_file_error();
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
        const ssize_t count = read(descriptor, &extra, 1U);
        if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (close(descriptor) != 0 && result == APP_ERROR_NONE) {
        result = storage_repository_map_file_error();
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

bool storage_repository_directory_has_entries(const char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return false;
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

app_error_code_t storage_repository_ensure_initial_file(const char *path, const char *contents)
{
    struct stat metadata;
    if (stat(path, &metadata) == 0) {
        return APP_ERROR_NONE;
    }
    if (errno != ENOENT) {
        return storage_repository_map_file_error();
    }
    return storage_atomic_write(path, contents, strlen(contents), true);
}

app_error_code_t storage_repository_set_file_path(const app_uuid_t *set_id,
                                      char *buffer,
                                      size_t buffer_size)
{
    char directory[APP_PATH_MAX_BYTES];
    const app_error_code_t result = storage_make_set_path(set_id, directory, sizeof(directory));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const int written = snprintf(buffer, buffer_size, "%s/set.json", directory);
    return written >= 0 && (size_t)written < buffer_size ? APP_ERROR_NONE
                                                         : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t storage_repository_transaction_path(const app_uuid_t *transaction_id,
                                         char *buffer,
                                         size_t buffer_size)
{
    const int written = snprintf(buffer, buffer_size,
                                 STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 transaction_id->value);
    return written >= 0 && (size_t)written < buffer_size ? APP_ERROR_NONE
                                                         : APP_ERROR_INVALID_ARGUMENT;
}

app_error_code_t storage_repository_remove_manifest(const app_uuid_t *transaction_id)
{
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = storage_repository_transaction_path(transaction_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return unlink(path) == 0 ? APP_ERROR_NONE : storage_repository_map_file_error();
}

app_error_code_t storage_repository_make_directory(const char *path)
{
    if (mkdir(path, 0750) == 0) {
        return APP_ERROR_NONE;
    }
    return errno == EEXIST ? APP_ERROR_CONFLICT : storage_repository_map_file_error();
}

app_error_code_t storage_repository_remove_tree(const char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char child[APP_PATH_MAX_BYTES];
        const int written = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(child)) {
            result = APP_ERROR_INVALID_ARGUMENT;
            break;
        }
        struct stat metadata;
        if (stat(child, &metadata) != 0) {
            result = storage_repository_map_file_error();
            break;
        }
        if (S_ISDIR(metadata.st_mode)) {
            result = storage_repository_remove_tree(child);
        } else if (unlink(child) != 0) {
            result = storage_repository_map_file_error();
        }
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = storage_repository_map_file_error();
    }
    if (result == APP_ERROR_NONE && rmdir(path) != 0) {
        result = storage_repository_map_file_error();
    }
    return result;
}
