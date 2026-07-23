#include "storage.h"
#include "storage_atomic_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define STORAGE_ATOMIC_NAME_ATTEMPTS 4U

static app_error_code_t map_error_number(int error_number)
{
    return error_number == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

static app_error_code_t write_all(const storage_fs_ops_t *operations,
                                  int descriptor,
                                  const uint8_t *data,
                                  size_t length)
{
    size_t written = 0U;
    while (written < length) {
        const ssize_t count = operations->write_file(
            operations->context, descriptor, data + written, length - written);
        if (count < 0) {
            const int write_error = errno;
            if (write_error == EINTR) {
                continue;
            }
            return map_error_number(write_error);
        }
        if (count == 0) {
            return APP_ERROR_IO;
        }
        written += (size_t)count;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t verify_file(const storage_fs_ops_t *operations,
                                    const char *path,
                                    const uint8_t *expected,
                                    size_t length)
{
    int descriptor = operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        return map_error_number(errno);
    }

    app_error_code_t result = APP_ERROR_NONE;
    uint8_t buffer[256U];
    size_t verified = 0U;
    while (verified < length) {
        const size_t remaining = length - verified;
        const size_t requested = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const ssize_t count = operations->read_file(
            operations->context, descriptor, buffer, requested);
        if (count < 0) {
            const int read_error = errno;
            if (read_error == EINTR) {
                continue;
            }
            result = map_error_number(read_error);
            break;
        }
        if (count == 0 || memcmp(buffer, expected + verified, (size_t)count) != 0) {
            result = APP_ERROR_IO;
            break;
        }
        verified += (size_t)count;
    }

    if (result == APP_ERROR_NONE) {
        uint8_t extra = 0U;
        const ssize_t count = operations->read_file(
            operations->context, descriptor, &extra, 1U);
        if (count < 0) {
            result = map_error_number(errno);
        } else if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 &&
        result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    return result;
}

static app_error_code_t cleanup_path(const storage_fs_ops_t *operations,
                                     const char *path)
{
    if (operations->unlink_path(operations->context, path) == 0) {
        return APP_ERROR_NONE;
    }
    const int unlink_error = errno;
    return unlink_error == ENOENT ? APP_ERROR_NONE : map_error_number(unlink_error);
}

static app_error_code_t production_uuid_generate(void *context, app_uuid_t *out_uuid)
{
    (void)context;
    return app_uuid_generate(out_uuid);
}

static app_error_code_t create_temporary_file(const char *path,
                                              const storage_fs_ops_t *operations,
                                              storage_uuid_generate_fn generate_uuid,
                                              void *uuid_context,
                                              char *temporary,
                                              size_t temporary_size,
                                              char *backup,
                                              size_t backup_size,
                                              int *out_descriptor)
{
    for (size_t attempt = 0U; attempt < STORAGE_ATOMIC_NAME_ATTEMPTS; ++attempt) {
        app_uuid_t operation_id = {0};
        app_error_code_t result = generate_uuid(uuid_context, &operation_id);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        const int temporary_length = snprintf(
            temporary, temporary_size, "%s.tmp.%s", path, operation_id.value);
        const int backup_length = snprintf(
            backup, backup_size, "%s.bak.%s", path, operation_id.value);
        if (temporary_length < 0 || (size_t)temporary_length >= temporary_size ||
            backup_length < 0 || (size_t)backup_length >= backup_size) {
            return APP_ERROR_INVALID_ARGUMENT;
        }

        int descriptor = operations->open_file(
            operations->context, temporary, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (descriptor < 0) {
            const int open_error = errno;
            if (open_error == EEXIST) {
                continue;
            }
            return map_error_number(open_error);
        }

        struct stat backup_metadata;
        if (operations->stat_path(operations->context, backup, &backup_metadata) == 0) {
            const app_error_code_t cleanup_result = cleanup_path(operations, temporary);
            if (cleanup_result != APP_ERROR_NONE) {
                return cleanup_result;
            }
            continue;
        }
        const int stat_error = errno;
        if (stat_error != ENOENT) {
            const app_error_code_t result_for_stat = map_error_number(stat_error);
            const app_error_code_t cleanup_result = cleanup_path(operations, temporary);
            return cleanup_result == APP_ERROR_NONE ? result_for_stat : cleanup_result;
        }
        *out_descriptor = descriptor;
        return APP_ERROR_NONE;
    }
    return APP_ERROR_CONFLICT;
}

app_error_code_t storage_atomic_write_with_ops(const char *path,
                                               const void *data,
                                               size_t data_length,
                                               bool sync_required,
                                               const storage_fs_ops_t *operations,
                                               storage_uuid_generate_fn generate_uuid,
                                               void *uuid_context)
{
    if (path == NULL || (data == NULL && data_length != 0U) ||
        strlen(path) >= APP_PATH_MAX_BYTES || !storage_fs_ops_is_valid(operations) ||
        generate_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char temporary[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    int descriptor = -1;
    app_error_code_t result = create_temporary_file(
        path, operations, generate_uuid, uuid_context, temporary, sizeof(temporary),
        backup, sizeof(backup), &descriptor);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    result = write_all(operations, descriptor, data, data_length);
    if (result == APP_ERROR_NONE &&
        operations->sync_file(operations->context, descriptor) != 0) {
        const int sync_error = errno;
        if (sync_required || (sync_error != EINVAL && sync_error != ENOTSUP)) {
            result = map_error_number(sync_error);
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 &&
        result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    if (result == APP_ERROR_NONE) {
        result = verify_file(operations, temporary, data, data_length);
    }
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup_result = cleanup_path(operations, temporary);
        return cleanup_result == APP_ERROR_NONE ? result : cleanup_result;
    }

    struct stat existing;
    const bool destination_exists =
        operations->stat_path(operations->context, path, &existing) == 0;
    if (!destination_exists) {
        const int stat_error = errno;
        if (stat_error != ENOENT) {
            const app_error_code_t stat_result = map_error_number(stat_error);
            const app_error_code_t cleanup_result = cleanup_path(operations, temporary);
            return cleanup_result == APP_ERROR_NONE ? stat_result : cleanup_result;
        }
    }

    if (destination_exists &&
        operations->rename_path(operations->context, path, backup) != 0) {
        const app_error_code_t rename_result = map_error_number(errno);
        const app_error_code_t cleanup_result = cleanup_path(operations, temporary);
        return cleanup_result == APP_ERROR_NONE ? rename_result : cleanup_result;
    }

    if (operations->rename_path(operations->context, temporary, path) != 0) {
        const app_error_code_t activate_result = map_error_number(errno);
        app_error_code_t rollback_result = APP_ERROR_NONE;
        if (destination_exists &&
            operations->rename_path(operations->context, backup, path) != 0) {
            rollback_result = map_error_number(errno);
        }
        const app_error_code_t cleanup_result = cleanup_path(operations, temporary);
        if (rollback_result != APP_ERROR_NONE) {
            return rollback_result;
        }
        return cleanup_result == APP_ERROR_NONE ? activate_result : cleanup_result;
    }

    if (destination_exists) {
        result = cleanup_path(operations, backup);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_atomic_write(const char *path,
                                      const void *data,
                                      size_t data_length,
                                      bool sync_required)
{
    return storage_atomic_write_with_ops(path, data, data_length, sync_required,
                                         storage_fs_ops_posix(),
                                         production_uuid_generate, NULL);
}
