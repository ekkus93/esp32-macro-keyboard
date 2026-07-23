#include "storage.h"

#include <errno.h>
#include <fcntl.h>
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

app_error_code_t storage_atomic_write(const char *path,
                                      const void *data,
                                      size_t data_length,
                                      bool sync_required)
{
    if (path == NULL || (data == NULL && data_length != 0U) || strlen(path) >= APP_PATH_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_uuid_t temporary_id = {0};
    app_error_code_t result = app_uuid_generate(&temporary_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    char temporary[APP_PATH_MAX_BYTES];
    const int temporary_length =
        snprintf(temporary, sizeof(temporary), "%s.tmp.%s", path, temporary_id.value);
    if (temporary_length < 0 || (size_t)temporary_length >= sizeof(temporary)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    int descriptor = open(temporary, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (descriptor < 0) {
        return map_errno();
    }

    result = write_all(descriptor, data, data_length);
    if (result == APP_ERROR_NONE && sync_required && fsync(descriptor) != 0) {
        result = map_errno();
    }
    if (close(descriptor) != 0 && result == APP_ERROR_NONE) {
        result = map_errno();
    }
    if (result == APP_ERROR_NONE) {
        result = verify_file(temporary, data, data_length);
    }
    if (result != APP_ERROR_NONE) {
        if (unlink(temporary) != 0 && errno != ENOENT) {
            return APP_ERROR_IO;
        }
        return result;
    }

    if (rename(temporary, path) != 0) {
        const app_error_code_t rename_result = map_errno();
        if (unlink(temporary) != 0 && errno != ENOENT) {
            return APP_ERROR_IO;
        }
        return rename_result;
    }
    return APP_ERROR_NONE;
}
