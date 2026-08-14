#include "storage_blob.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "app_operation_result.h"
#include "storage_blob_internal.h"
#include "storage_blob_upload_internal.h"
#include "storage_fs_ops.h"

#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "nvs.h"
#endif

#define STORAGE_BLOB_NVS_NAMESPACE "storage"
#define STORAGE_BLOB_NVS_NEXT_ID_KEY "next_blob_id"

static void *open_temporary(void *context, const char *path) {
    (void)context;
    return fopen(path, "wbx");
}

static size_t write_stream(void *context, void *stream, const void *data, size_t length) {
    (void)context;
    return fwrite(data, 1U, length, stream);
}

static int flush_stream(void *context, void *stream) {
    (void)context;
    return fflush(stream);
}

static int sync_stream(void *context, void *stream) {
    (void)context;
    const int descriptor = fileno(stream);
    return descriptor < 0 ? -1 : fsync(descriptor);
}

static int close_stream(void *context, void *stream) {
    (void)context;
    return fclose(stream);
}

static int stat_path(void *context, const char *path, struct stat *metadata) {
    (void)context;
    return stat(path, metadata);
}

static int rename_path(void *context, const char *source, const char *destination) {
    (void)context;
    return rename(source, destination);
}

static int unlink_path(void *context, const char *path) {
    (void)context;
    return unlink(path);
}

static int sync_parent(void *context, const char *path) {
    return storage_fs_sync_parent_path(context, path);
}

static app_error_code_t persist_next_id(void *context, uint64_t next_id) {
    (void)context;
#ifdef ESP_PLATFORM
    nvs_handle_t handle = 0U;
    esp_err_t result = nvs_open(STORAGE_BLOB_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (result != ESP_OK) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
    result = nvs_set_u64(handle, STORAGE_BLOB_NVS_NEXT_ID_KEY, next_id);
    if (result == ESP_OK) {
        result = nvs_commit(handle);
    }
    nvs_close(handle);
    if (result == ESP_ERR_NVS_NOT_ENOUGH_SPACE) {
        return APP_ERROR_STORAGE_FULL;
    }
    return result == ESP_OK ? APP_ERROR_NONE : APP_ERROR_STORAGE_UNAVAILABLE;
#else
    (void)next_id;
    return APP_ERROR_STORAGE_UNAVAILABLE;
#endif
}

static const storage_blob_upload_ops_t *upload_operations(void) {
    static const storage_blob_upload_ops_t operations = {
        .context = NULL,
        .open_temporary = open_temporary,
        .write_stream = write_stream,
        .flush_stream = flush_stream,
        .sync_stream = sync_stream,
        .close_stream = close_stream,
        .stat_path = stat_path,
        .rename_path = rename_path,
        .unlink_path = unlink_path,
        .sync_parent = sync_parent,
        .persist_next_id = persist_next_id,
    };
    return &operations;
}

app_error_code_t storage_blob_upload_begin(size_t expected_bytes,
                                           storage_blob_upload_t *out_upload) {
    const storage_blob_scan_summary_t state = storage_blob_scan_state();
    return storage_blob_upload_begin_with_ops(STORAGE_BLOB_DIRECTORY, state.next_id, expected_bytes,
                                              (size_t)APP_V2_BLOB_MAX_BYTES, upload_operations(),
                                              out_upload);
}

app_error_code_t storage_blob_upload_write(storage_blob_upload_t *upload, const void *data,
                                           size_t data_length) {
    return storage_blob_upload_write_with_ops(upload, data, data_length);
}

app_error_code_t storage_blob_upload_commit(storage_blob_upload_t *upload,
                                            storage_blob_entry_t *out_entry) {
    const bool was_committed = upload != NULL && upload->committed;
    const app_operation_result_t detailed =
        storage_blob_upload_commit_with_ops_result(upload, out_entry);
    if (upload != NULL && !was_committed && upload->committed) {
        const storage_blob_entry_t committed = {
            .id = upload->id,
            .stored_bytes = upload->stored_bytes,
        };
        storage_blob_record_committed_entry(&committed);
    }
    return app_operation_result_error(detailed);
}

app_error_code_t storage_blob_upload_abort(storage_blob_upload_t *upload) {
    return app_operation_result_error(storage_blob_upload_abort_with_ops_result(upload));
}
