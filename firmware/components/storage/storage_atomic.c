#include "storage.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static app_error_code_t map_errno(void)
{
    return errno == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

static app_error_code_t write_all(int descriptor, const uint8_t *data, size_t length)
{
    size_t written = 0U;
    while (written < length) {
        const ssize_t count = write(descriptor, data + written, length - written);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            return map_errno();
        }
        if (count == 0) {
            return APP_ERROR_IO;
        }
        written += (size_t)count;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t verify_file(const char *path, const uint8_t *expected, size_t length)
{
    int descriptor = open(path, O_RDONLY);
    if (descriptor < 0) {
        return map_errno();
    }

    app_error_code_t result = APP_ERROR_NONE;
    uint8_t buffer[256U];
    size_t verified = 0U;
    while (verified < length) {
        const size_t remaining = length - verified;
        const size_t requested = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const ssize_t count = read(descriptor, buffer, requested);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            result = map_errno();
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
        const ssize_t count = read(descriptor, &extra, 1U);
        if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (close(descriptor) != 0 && result == APP_ERROR_NONE) {
        result = map_errno();
    }
    return result;
}

static app_error_code_t cleanup_path(const char *path)
{
    if (unlink(path) == 0 || errno == ENOENT) {
        return APP_ERROR_NONE;
    }
    return map_errno();
}

app_error_code_t storage_atomic_write(const char *path,
                                      const void *data,
                                      size_t data_length,
                                      bool sync_required)
{
    if (path == NULL || (data == NULL && data_length != 0U) || strlen(path) >= APP_PATH_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_uuid_t operation_id = {0};
    app_error_code_t result = app_uuid_generate(&operation_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    char temporary[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    const int temporary_length =
        snprintf(temporary, sizeof(temporary), "%s.tmp.%s", path, operation_id.value);
    const int backup_length =
        snprintf(backup, sizeof(backup), "%s.bak.%s", path, operation_id.value);
    if (temporary_length < 0 || (size_t)temporary_length >= sizeof(temporary) ||
        backup_length < 0 || (size_t)backup_length >= sizeof(backup)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    int descriptor = open(temporary, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (descriptor < 0) {
        return map_errno();
    }

    result = write_all(descriptor, data, data_length);
    if (result == APP_ERROR_NONE && fsync(descriptor) != 0 &&
        (sync_required || (errno != EINVAL && errno != ENOTSUP))) {
        result = map_errno();
    }
    if (close(descriptor) != 0 && result == APP_ERROR_NONE) {
        result = map_errno();
    }
    if (result == APP_ERROR_NONE) {
        result = verify_file(temporary, data, data_length);
    }
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup_result = cleanup_path(temporary);
        return cleanup_result == APP_ERROR_NONE ? result : cleanup_result;
    }

    struct stat existing;
    const bool destination_exists = stat(path, &existing) == 0;
    if (!destination_exists && errno != ENOENT) {
        const app_error_code_t stat_result = map_errno();
        const app_error_code_t cleanup_result = cleanup_path(temporary);
        return cleanup_result == APP_ERROR_NONE ? stat_result : cleanup_result;
    }

    if (destination_exists && rename(path, backup) != 0) {
        const app_error_code_t rename_result = map_errno();
        const app_error_code_t cleanup_result = cleanup_path(temporary);
        return cleanup_result == APP_ERROR_NONE ? rename_result : cleanup_result;
    }

    if (rename(temporary, path) != 0) {
        const app_error_code_t activate_result = map_errno();
        app_error_code_t rollback_result = APP_ERROR_NONE;
        if (destination_exists && rename(backup, path) != 0) {
            rollback_result = map_errno();
        }
        const app_error_code_t cleanup_result = cleanup_path(temporary);
        if (rollback_result != APP_ERROR_NONE) {
            return rollback_result;
        }
        return cleanup_result == APP_ERROR_NONE ? activate_result : cleanup_result;
    }

    if (destination_exists) {
        result = cleanup_path(backup);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}
