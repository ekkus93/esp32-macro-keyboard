#ifndef STORAGE_BLOB_UPLOAD_INTERNAL_H
#define STORAGE_BLOB_UPLOAD_INTERNAL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

#include "app_error.h"
#include "storage_blob.h"

typedef struct {
    void *context;
    void *(*open_temporary)(void *context, const char *path);
    size_t (*write_stream)(void *context, void *stream, const void *data, size_t length);
    int (*flush_stream)(void *context, void *stream);
    int (*sync_stream)(void *context, void *stream);
    int (*close_stream)(void *context, void *stream);
    int (*stat_path)(void *context, const char *path, struct stat *metadata);
    int (*rename_path)(void *context, const char *source, const char *destination);
    int (*unlink_path)(void *context, const char *path);
    int (*sync_parent)(void *context, const char *path);
    app_error_code_t (*persist_next_id)(void *context, uint64_t next_id);
} storage_blob_upload_ops_t;

app_error_code_t storage_blob_upload_begin_with_ops(
    const char *directory_path, uint64_t blob_id, size_t expected_bytes, size_t maximum_bytes,
    const storage_blob_upload_ops_t *operations, storage_blob_upload_t *out_upload);
app_error_code_t storage_blob_upload_write_with_ops(storage_blob_upload_t *upload, const void *data,
                                                    size_t data_length);
app_error_code_t storage_blob_upload_commit_with_ops(storage_blob_upload_t *upload,
                                                     storage_blob_entry_t *out_entry);
app_error_code_t storage_blob_upload_abort_with_ops(storage_blob_upload_t *upload);

#endif
