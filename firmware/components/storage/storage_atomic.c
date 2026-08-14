#include "storage.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../support/include/app_operation_result.h"
#include "app_error.h"
#include "macro_limits.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"

/* rw------- for storage files we create. */
#define STORAGE_FILE_MODE 0600

static app_error_code_t map_error_number(int error_number) {
    return error_number == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

static app_operation_result_t operation_primary_error(app_error_code_t error) {
    app_operation_result_t result = app_operation_success();
    result.primary_error = error;
    return result;
}

static app_operation_result_t operation_primary_error_with_commit(
    app_error_code_t error, app_operation_commit_state_t commit_state) {
    app_operation_result_t result = operation_primary_error(error);
    result.commit_state = commit_state;
    return result;
}

static void operation_record_cleanup(app_operation_result_t *result,
                                     app_error_code_t cleanup_error) {
    if (cleanup_error != APP_ERROR_NONE) {
        result->cleanup_error = cleanup_error;
        result->cleanup_incomplete = true;
    }
}

static app_error_code_t write_all(const storage_fs_ops_t *operations, int descriptor,
                                  const uint8_t *data, size_t length) {
    size_t written = 0U;
    while (written < length) {
        const size_t requested = length - written;
        const intmax_t count =
            operations->write_file(operations->context, descriptor, data + written, requested);
        if (count < 0) {
            const int write_error = errno;
            if (write_error == EINTR) {
                continue;
            }
            return map_error_number(write_error);
        }
        if (count == 0 || (uintmax_t)count > (uintmax_t)requested) {
            return APP_ERROR_IO;
        }
        written += (size_t)count;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t verify_file(const storage_fs_ops_t *operations, const char *path,
                                    const uint8_t *expected, size_t length) {
    int descriptor = operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        const int open_error = errno;
        return map_error_number(open_error);
    }

    app_error_code_t result = APP_ERROR_NONE;
    uint8_t buffer[256U];
    size_t verified = 0U;
    while (verified < length) {
        const size_t remaining = length - verified;
        const size_t requested = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        const intmax_t count =
            operations->read_file(operations->context, descriptor, buffer, requested);
        if (count < 0) {
            const int read_error = errno;
            if (read_error == EINTR) {
                continue;
            }
            result = map_error_number(read_error);
            break;
        }
        if (count == 0 || (uintmax_t)count > (uintmax_t)requested) {
            result = APP_ERROR_IO;
            break;
        }
        const size_t count_bytes = (size_t)count;
        if (memcmp(buffer, expected + verified, count_bytes) != 0) {
            result = APP_ERROR_IO;
            break;
        }
        verified += count_bytes;
    }

    if (result == APP_ERROR_NONE) {
        uint8_t extra = 0U;
        const intmax_t count = operations->read_file(operations->context, descriptor, &extra, 1U);
        if (count < 0) {
            const int read_error = errno;
            result = map_error_number(read_error);
        } else if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 && result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    return result;
}

static app_error_code_t cleanup_path(const storage_fs_ops_t *operations, const char *path) {
    if (operations->unlink_path(operations->context, path) == 0) {
        return APP_ERROR_NONE;
    }
    const int unlink_error = errno;
    return unlink_error == ENOENT ? APP_ERROR_NONE : map_error_number(unlink_error);
}

static app_error_code_t sync_parent(storage_parent_sync_fn sync_parent_path,
                                    void *parent_sync_context, const char *path) {
    if (sync_parent_path(parent_sync_context, path) == 0) {
        return APP_ERROR_NONE;
    }
    const int sync_error = errno;
    return map_error_number(sync_error);
}

/* SPEC 13.4 in full: write <target>.tmp, flush, sync, close, reopen and verify,
 * then rename() it over the target. POSIX rename is atomic, so an interruption
 * at any point leaves either the complete old file or the complete new one.
 *
 * There is deliberately no .bak file and no rollback ladder. The previous
 * implementation renamed the destination aside before activating, which created
 * a second on-device copy of every object being written (SPEC 22, invariant 16)
 * and a window in which the canonical path did not exist at all. A single
 * rename has neither property.
 *
 * The temporary name is fixed rather than UUID-suffixed because every writer
 * holds the repository mutation lock, so there is never a second writer racing
 * for the same destination -- and boot recovery can then be exactly "unlink
 * every *.tmp under /data" (SPEC 13.4). */
static app_error_code_t temporary_path_for(const char *path, char *temporary,
                                           size_t temporary_size) {
    const int written = snprintf(temporary, temporary_size, "%s.tmp", path);
    return written < 0 || (size_t)written >= temporary_size ? APP_ERROR_INVALID_ARGUMENT
                                                            : APP_ERROR_NONE;
}

static app_operation_result_t stage_temporary_file(const char *temporary, const void *data,
                                                   size_t data_length, bool sync_required,
                                                   const storage_fs_ops_t *operations) {
    /* O_TRUNC, not O_EXCL: a *.tmp left by an interrupted write is debris, and
     * failing here would make the destination permanently unwritable until the
     * next boot cleaned it up. */
    int descriptor = operations->open_file(operations->context, temporary,
                                           O_WRONLY | O_CREAT | O_TRUNC, STORAGE_FILE_MODE);
    if (descriptor < 0) {
        const int open_error = errno;
        return operation_primary_error(map_error_number(open_error));
    }

    app_error_code_t primary_error = write_all(operations, descriptor, data, data_length);
    if (primary_error == APP_ERROR_NONE &&
        operations->sync_file(operations->context, descriptor) != 0) {
        const int sync_error = errno;
        if (sync_required || (sync_error != EINVAL && sync_error != ENOTSUP)) {
            primary_error = map_error_number(sync_error);
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 &&
        primary_error == APP_ERROR_NONE) {
        const int close_error = errno;
        primary_error = map_error_number(close_error);
    }
    if (primary_error == APP_ERROR_NONE) {
        primary_error = verify_file(operations, temporary, data, data_length);
    }
    if (primary_error != APP_ERROR_NONE) {
        app_operation_result_t result = operation_primary_error(primary_error);
        operation_record_cleanup(&result, cleanup_path(operations, temporary));
        return result;
    }
    return app_operation_success();
}

app_operation_result_t storage_atomic_write_with_ops_and_parent_sync_result(
    const char *path, const void *data, size_t data_length, bool sync_required,
    const storage_fs_ops_t *operations, storage_parent_sync_fn sync_parent_path,
    void *parent_sync_context) {
    if (path == NULL || (data == NULL && data_length != 0U) || strlen(path) >= APP_PATH_MAX_BYTES ||
        !storage_fs_ops_is_valid(operations) || sync_parent_path == NULL) {
        return operation_primary_error_with_commit(APP_ERROR_INVALID_ARGUMENT,
                                                   APP_OPERATION_NOT_COMMITTED);
    }

    char temporary[APP_PATH_MAX_BYTES];
    app_error_code_t primary_error = temporary_path_for(path, temporary, sizeof(temporary));
    if (primary_error != APP_ERROR_NONE) {
        return operation_primary_error_with_commit(primary_error, APP_OPERATION_NOT_COMMITTED);
    }

    app_operation_result_t result =
        stage_temporary_file(temporary, data, data_length, sync_required, operations);
    if (!app_operation_result_ok(result)) {
        result.commit_state = APP_OPERATION_NOT_COMMITTED;
        return result;
    }

    if (operations->rename_path(operations->context, temporary, path) != 0) {
        const int activate_error = errno;
        result = operation_primary_error(map_error_number(activate_error));
        operation_record_cleanup(&result, cleanup_path(operations, temporary));
        result.commit_state = APP_OPERATION_NOT_COMMITTED;
        return result;
    }

    primary_error = sync_parent(sync_parent_path, parent_sync_context, path);
    if (primary_error != APP_ERROR_NONE) {
        return operation_primary_error_with_commit(primary_error, APP_OPERATION_COMMIT_UNCERTAIN);
    }

    result = app_operation_success();
    result.commit_state = APP_OPERATION_COMMITTED;
    return result;
}

app_error_code_t storage_atomic_write_with_ops_and_parent_sync(
    const char *path, const void *data, size_t data_length, bool sync_required,
    const storage_fs_ops_t *operations, storage_parent_sync_fn sync_parent_path,
    void *parent_sync_context) {
    return app_operation_result_error(storage_atomic_write_with_ops_and_parent_sync_result(
        path, data, data_length, sync_required, operations, sync_parent_path, parent_sync_context));
}

app_error_code_t storage_atomic_write_with_ops(const char *path, const void *data,
                                               size_t data_length, bool sync_required,
                                               const storage_fs_ops_t *operations) {
    return storage_atomic_write_with_ops_and_parent_sync(
        path, data, data_length, sync_required, operations, storage_fs_sync_parent_path, NULL);
}

app_error_code_t storage_atomic_write(const char *path, const void *data, size_t data_length,
                                      bool sync_required) {
    return storage_atomic_write_with_ops(path, data, data_length, sync_required,
                                         storage_fs_ops_posix());
}
