#include "storage_blob_upload_internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "storage_blob.h"

static app_error_code_t map_io_error(int error_number) {
    return error_number == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

static bool upload_ops_valid(const storage_blob_upload_ops_t *operations) {
    return operations != NULL && operations->open_temporary != NULL &&
           operations->write_stream != NULL && operations->flush_stream != NULL &&
           operations->sync_stream != NULL && operations->close_stream != NULL &&
           operations->stat_path != NULL && operations->rename_path != NULL &&
           operations->unlink_path != NULL && operations->sync_parent != NULL &&
           operations->persist_next_id != NULL;
}

static app_error_code_t build_upload_paths(const char *directory_path, uint64_t blob_id,
                                           storage_blob_upload_t *upload) {
    char filename[STORAGE_BLOB_FILENAME_CAPACITY];
    app_error_code_t result = storage_blob_format_filename(blob_id, filename, sizeof(filename));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    int written = snprintf(upload->final_path, sizeof(upload->final_path), "%s/%s", directory_path,
                           filename);
    if (written < 0 || (size_t)written >= sizeof(upload->final_path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    written = snprintf(upload->temporary_path, sizeof(upload->temporary_path), "%s.tmp",
                       upload->final_path);
    return written < 0 || (size_t)written >= sizeof(upload->temporary_path)
               ? APP_ERROR_INVALID_ARGUMENT
               : APP_ERROR_NONE;
}

static app_error_code_t require_path_absent(const storage_blob_upload_ops_t *operations,
                                            const char *path) {
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    const int stat_error = errno;
    return stat_error == ENOENT ? APP_ERROR_NONE : map_io_error(stat_error);
}

static app_error_code_t unlink_temporary(const storage_blob_upload_ops_t *operations,
                                         const char *path) {
    if (operations->unlink_path(operations->context, path) == 0) {
        return APP_ERROR_NONE;
    }
    const int unlink_error = errno;
    return unlink_error == ENOENT ? APP_ERROR_NONE : map_io_error(unlink_error);
}

app_error_code_t storage_blob_upload_begin_with_ops(
    const char *directory_path, uint64_t blob_id, size_t expected_bytes, size_t maximum_bytes,
    const storage_blob_upload_ops_t *operations, storage_blob_upload_t *out_upload) {
    if (out_upload != NULL) {
        *out_upload = (storage_blob_upload_t){0};
    }
    if (directory_path == NULL || directory_path[0] == '\0' || blob_id == 0U ||
        expected_bytes == 0U || expected_bytes > maximum_bytes || maximum_bytes == 0U ||
        !upload_ops_valid(operations) || out_upload == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    out_upload->operations = operations;
    out_upload->id = blob_id;
    out_upload->expected_bytes = expected_bytes;
    app_error_code_t result = build_upload_paths(directory_path, blob_id, out_upload);
    if (result != APP_ERROR_NONE) {
        *out_upload = (storage_blob_upload_t){0};
        return result;
    }
    result = require_path_absent(operations, out_upload->final_path);
    if (result != APP_ERROR_NONE) {
        *out_upload = (storage_blob_upload_t){0};
        return result;
    }
    result = require_path_absent(operations, out_upload->temporary_path);
    if (result != APP_ERROR_NONE) {
        *out_upload = (storage_blob_upload_t){0};
        return result;
    }

    out_upload->stream =
        operations->open_temporary(operations->context, out_upload->temporary_path);
    if (out_upload->stream == NULL) {
        const int open_error = errno;
        *out_upload = (storage_blob_upload_t){0};
        return map_io_error(open_error);
    }
    out_upload->active = true;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_upload_write_with_ops(storage_blob_upload_t *upload, const void *data,
                                                    size_t data_length) {
    if (upload == NULL || data == NULL || data_length == 0U || !upload->active ||
        upload->committed || upload->operations == NULL || upload->stream == NULL ||
        data_length > upload->expected_bytes - upload->stored_bytes) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const storage_blob_upload_ops_t *operations = upload->operations;
    errno = 0;
    const size_t written =
        operations->write_stream(operations->context, upload->stream, data, data_length);
    if (written != data_length) {
        const int write_error = errno == 0 ? EIO : errno;
        return map_io_error(write_error);
    }
    upload->stored_bytes += written;
    return APP_ERROR_NONE;
}

static app_error_code_t close_staged_stream(storage_blob_upload_t *upload) {
    const storage_blob_upload_ops_t *operations = upload->operations;
    app_error_code_t result = APP_ERROR_NONE;
    if (operations->flush_stream(operations->context, upload->stream) != 0) {
        result = map_io_error(errno);
    }
    if (result == APP_ERROR_NONE &&
        operations->sync_stream(operations->context, upload->stream) != 0) {
        result = map_io_error(errno);
    }
    if (operations->close_stream(operations->context, upload->stream) != 0 &&
        result == APP_ERROR_NONE) {
        result = map_io_error(errno);
    }
    upload->stream = NULL;
    upload->active = false;
    return result;
}

app_error_code_t storage_blob_upload_commit_with_ops(storage_blob_upload_t *upload,
                                                     storage_blob_entry_t *out_entry) {
    if (out_entry != NULL) {
        *out_entry = (storage_blob_entry_t){0};
    }
    if (upload == NULL || out_entry == NULL || !upload->active || upload->committed ||
        upload->operations == NULL || upload->stream == NULL ||
        upload->stored_bytes != upload->expected_bytes || upload->id == UINT64_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const storage_blob_upload_ops_t *operations = upload->operations;
    app_error_code_t result = close_staged_stream(upload);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = unlink_temporary(operations, upload->temporary_path);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    result = require_path_absent(operations, upload->final_path);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = unlink_temporary(operations, upload->temporary_path);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    if (operations->rename_path(operations->context, upload->temporary_path,
                                upload->final_path) != 0) {
        const app_error_code_t rename_result = map_io_error(errno);
        const app_error_code_t cleanup = unlink_temporary(operations, upload->temporary_path);
        return cleanup == APP_ERROR_NONE ? rename_result : cleanup;
    }

    upload->committed = true;
    if (operations->sync_parent(operations->context, upload->final_path) != 0) {
        return map_io_error(errno);
    }
    result = operations->persist_next_id(operations->context, upload->id + UINT64_C(1));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    *out_entry = (storage_blob_entry_t){
        .id = upload->id,
        .stored_bytes = upload->stored_bytes,
    };
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_upload_abort_with_ops(storage_blob_upload_t *upload) {
    if (upload == NULL || upload->operations == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (upload->committed) {
        return APP_ERROR_NONE;
    }

    const storage_blob_upload_ops_t *operations = upload->operations;
    app_error_code_t result = APP_ERROR_NONE;
    if (upload->active && upload->stream != NULL) {
        if (operations->close_stream(operations->context, upload->stream) != 0) {
            result = map_io_error(errno);
        }
        upload->stream = NULL;
        upload->active = false;
    }
    const app_error_code_t cleanup = unlink_temporary(operations, upload->temporary_path);
    return result == APP_ERROR_NONE ? cleanup : result;
}
