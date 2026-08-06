#include "storage_blob_internal.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_fs_ops.h"

#define STORAGE_BLOB_ACCESS_PATH_CAPACITY (STORAGE_FS_ENTRY_NAME_MAX * 2U)

static bool access_ops_valid(const storage_fs_ops_t *operations) {
    return operations != NULL && operations->open_file != NULL && operations->read_file != NULL &&
           operations->close_file != NULL && operations->stat_path != NULL &&
           operations->unlink_path != NULL;
}

static app_error_code_t path_error(int error_number) {
    return error_number == ENOENT ? APP_ERROR_NOT_FOUND : APP_ERROR_IO;
}

static app_error_code_t build_blob_path(const char *directory_path, uint64_t blob_id,
                                        char *out_path, size_t path_size) {
    if (directory_path == NULL || directory_path[0] == '\0' || out_path == NULL ||
        path_size == 0U || blob_id == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char filename[STORAGE_BLOB_FILENAME_CAPACITY];
    app_error_code_t result = storage_blob_format_filename(blob_id, filename, sizeof(filename));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const int written = snprintf(out_path, path_size, "%s/%s", directory_path, filename);
    return written < 0 || (size_t)written >= path_size ? APP_ERROR_INVALID_ARGUMENT
                                                       : APP_ERROR_NONE;
}

static app_error_code_t stat_regular_blob(const storage_fs_ops_t *operations, const char *path,
                                          size_t *out_size) {
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        return path_error(errno);
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_size < 0 ||
        (uintmax_t)metadata.st_size > (uintmax_t)SIZE_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_size = (size_t)metadata.st_size;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_reader_open_with_ops(const storage_fs_ops_t *operations,
                                                   const char *directory_path, uint64_t blob_id,
                                                   storage_blob_reader_t *out_reader) {
    if (out_reader != NULL) {
        *out_reader = (storage_blob_reader_t){.descriptor = -1};
    }
    if (!access_ops_valid(operations) || out_reader == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char path[STORAGE_BLOB_ACCESS_PATH_CAPACITY];
    app_error_code_t result = build_blob_path(directory_path, blob_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    size_t stored_bytes = 0U;
    result = stat_regular_blob(operations, path, &stored_bytes);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    const int descriptor = operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        return path_error(errno);
    }
    *out_reader = (storage_blob_reader_t){
        .descriptor = descriptor,
        .operations = operations,
        .stored_bytes = stored_bytes,
        .bytes_read = 0U,
        .active = true,
    };
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_reader_read_with_ops(storage_blob_reader_t *reader, void *buffer,
                                                   size_t buffer_size, size_t *out_count,
                                                   bool *out_eof) {
    if (out_count != NULL) {
        *out_count = 0U;
    }
    if (out_eof != NULL) {
        *out_eof = false;
    }
    if (reader == NULL || buffer == NULL || buffer_size == 0U || out_count == NULL ||
        out_eof == NULL || !reader->active || reader->descriptor < 0 ||
        reader->operations == NULL || reader->bytes_read > reader->stored_bytes) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (reader->bytes_read == reader->stored_bytes) {
        *out_eof = true;
        return APP_ERROR_NONE;
    }

    const storage_fs_ops_t *operations = reader->operations;
    size_t requested = reader->stored_bytes - reader->bytes_read;
    if (requested > buffer_size) {
        requested = buffer_size;
    }
    const ssize_t count =
        operations->read_file(operations->context, reader->descriptor, buffer, requested);
    if (count < 0) {
        return APP_ERROR_IO;
    }
    if (count == 0 || (size_t)count > requested) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    reader->bytes_read += (size_t)count;
    *out_count = (size_t)count;
    *out_eof = reader->bytes_read == reader->stored_bytes;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_reader_close_with_ops(storage_blob_reader_t *reader) {
    if (reader == NULL || !reader->active || reader->descriptor < 0 || reader->operations == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const storage_fs_ops_t *operations = reader->operations;
    const int descriptor = reader->descriptor;
    reader->descriptor = -1;
    reader->operations = NULL;
    reader->active = false;
    return operations->close_file(operations->context, descriptor) == 0 ? APP_ERROR_NONE
                                                                        : APP_ERROR_IO;
}

app_error_code_t storage_blob_delete_with_ops(const storage_fs_ops_t *operations,
                                              const char *directory_path, uint64_t blob_id) {
    if (!access_ops_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[STORAGE_BLOB_ACCESS_PATH_CAPACITY];
    app_error_code_t result = build_blob_path(directory_path, blob_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    size_t stored_bytes = 0U;
    result = stat_regular_blob(operations, path, &stored_bytes);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    (void)stored_bytes;
    if (operations->unlink_path(operations->context, path) != 0) {
        return path_error(errno);
    }
    return APP_ERROR_NONE;
}
