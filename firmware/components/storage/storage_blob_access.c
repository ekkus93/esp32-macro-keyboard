#include "storage_blob.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"

app_error_code_t storage_blob_reader_open(uint64_t blob_id, storage_blob_reader_t *out_reader) {
    return storage_blob_reader_open_with_ops(storage_fs_ops_posix(), STORAGE_BLOB_DIRECTORY, blob_id,
                                             out_reader);
}

app_error_code_t storage_blob_reader_read(storage_blob_reader_t *reader, void *buffer,
                                          size_t buffer_size, size_t *out_count, bool *out_eof) {
    return storage_blob_reader_read_with_ops(reader, buffer, buffer_size, out_count, out_eof);
}

app_error_code_t storage_blob_reader_close(storage_blob_reader_t *reader) {
    return storage_blob_reader_close_with_ops(reader);
}

app_error_code_t storage_blob_delete(uint64_t blob_id) {
    char filename[STORAGE_BLOB_FILENAME_CAPACITY];
    app_error_code_t result =
        storage_blob_format_filename(blob_id, filename, sizeof(filename));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char path[STORAGE_BLOB_UPLOAD_PATH_CAPACITY];
    const int written = snprintf(path, sizeof(path), "%s/%s", STORAGE_BLOB_DIRECTORY, filename);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_INTERNAL;
    }

    result = storage_blob_delete_with_ops(storage_fs_ops_posix(), STORAGE_BLOB_DIRECTORY, blob_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_blob_record_deleted_entry();
    return storage_fs_sync_parent_path(NULL, path) == 0 ? APP_ERROR_NONE : APP_ERROR_IO;
}
