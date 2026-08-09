#include "fake_storage_blob.h"

#include <stdlib.h>
#include <string.h>

fake_storage_blob_state_t g_fake_storage_blob;

static fake_storage_blob_record_t *find_record(uint64_t id) {
    for (size_t index = 0U; index < FAKE_STORAGE_BLOB_MAX_RECORDS; ++index) {
        if (g_fake_storage_blob.records[index].used &&
            g_fake_storage_blob.records[index].id == id) {
            return &g_fake_storage_blob.records[index];
        }
    }
    return NULL;
}

static fake_storage_blob_record_t *find_free_slot(void) {
    for (size_t index = 0U; index < FAKE_STORAGE_BLOB_MAX_RECORDS; ++index) {
        if (!g_fake_storage_blob.records[index].used) {
            return &g_fake_storage_blob.records[index];
        }
    }
    return NULL;
}

void fake_storage_blob_reset(void) {
    for (size_t index = 0U; index < FAKE_STORAGE_BLOB_MAX_RECORDS; ++index) {
        if (g_fake_storage_blob.records[index].used) {
            free(g_fake_storage_blob.records[index].data);
        }
    }
    memset(&g_fake_storage_blob, 0, sizeof(g_fake_storage_blob));
    g_fake_storage_blob.partition_total_bytes = 1048576U;
    g_fake_storage_blob.partition_used_bytes = 0U;
}

void fake_storage_blob_seed(uint64_t id, const void *data, size_t size) {
    if (id == 0U || (data == NULL && size != 0U)) {
        abort();
    }
    fake_storage_blob_record_t *slot = find_free_slot();
    if (slot == NULL) {
        abort();
    }
    slot->data = NULL;
    if (size > 0U) {
        slot->data = malloc(size);
        if (slot->data == NULL) {
            abort();
        }
        memcpy(slot->data, data, size);
    }
    slot->used = true;
    slot->id = id;
    slot->size = size;
    if (id > g_fake_storage_blob.next_id) {
        g_fake_storage_blob.next_id = id;
    }
}

const fake_storage_blob_record_t *fake_storage_blob_find(uint64_t id) {
    return find_record(id);
}

app_error_code_t storage_blob_list(const storage_blob_scan_observer_t *observer,
                                   storage_blob_scan_summary_t *out_summary) {
    if (out_summary != NULL) {
        *out_summary = (storage_blob_scan_summary_t){0};
    }
    if (observer == NULL || observer->visit_entry == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_fake_storage_blob.force_list_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_list_error;
    }
    size_t valid_count = 0U;
    for (size_t index = 0U; index < FAKE_STORAGE_BLOB_MAX_RECORDS; ++index) {
        const fake_storage_blob_record_t *record = &g_fake_storage_blob.records[index];
        if (!record->used) {
            continue;
        }
        const storage_blob_entry_t entry = {.id = record->id, .stored_bytes = record->size};
        const app_error_code_t result = observer->visit_entry(observer->context, &entry);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        ++valid_count;
    }
    if (out_summary != NULL) {
        out_summary->valid_count = valid_count;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_upload_begin(size_t expected_bytes,
                                           storage_blob_upload_t *out_upload) {
    if (out_upload != NULL) {
        *out_upload = (storage_blob_upload_t){0};
    }
    if (out_upload == NULL || expected_bytes == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_fake_storage_blob.force_upload_begin_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_upload_begin_error;
    }
    uint8_t *buffer = malloc(expected_bytes);
    if (buffer == NULL) {
        return APP_ERROR_INTERNAL;
    }
    out_upload->stream = buffer;
    out_upload->id = ++g_fake_storage_blob.next_id;
    out_upload->expected_bytes = expected_bytes;
    out_upload->stored_bytes = 0U;
    out_upload->active = true;
    out_upload->committed = false;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_upload_write(storage_blob_upload_t *upload, const void *data,
                                           size_t data_length) {
    if (upload == NULL || data == NULL || data_length == 0U || !upload->active ||
        upload->committed || upload->stream == NULL ||
        data_length > upload->expected_bytes - upload->stored_bytes) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_fake_storage_blob.force_upload_write_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_upload_write_error;
    }
    memcpy((uint8_t *)upload->stream + upload->stored_bytes, data, data_length);
    upload->stored_bytes += data_length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_upload_commit(storage_blob_upload_t *upload,
                                            storage_blob_entry_t *out_entry) {
    if (out_entry != NULL) {
        *out_entry = (storage_blob_entry_t){0};
    }
    if (upload == NULL || out_entry == NULL || !upload->active || upload->committed ||
        upload->stored_bytes != upload->expected_bytes) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_fake_storage_blob.force_upload_commit_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_upload_commit_error;
    }
    fake_storage_blob_record_t *slot = find_free_slot();
    if (slot == NULL) {
        return APP_ERROR_STORAGE_FULL;
    }
    slot->used = true;
    slot->id = upload->id;
    slot->data = upload->stream;
    slot->size = upload->stored_bytes;
    upload->stream = NULL;
    upload->committed = true;
    upload->active = false;
    out_entry->id = upload->id;
    out_entry->stored_bytes = upload->stored_bytes;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_upload_abort(storage_blob_upload_t *upload) {
    if (upload == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (upload->committed) {
        return APP_ERROR_NONE;
    }
    if (upload->stream != NULL) {
        free(upload->stream);
        upload->stream = NULL;
    }
    upload->active = false;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_reader_open(uint64_t blob_id, storage_blob_reader_t *out_reader) {
    if (out_reader != NULL) {
        *out_reader = (storage_blob_reader_t){.descriptor = -1};
    }
    if (out_reader == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_fake_storage_blob.force_reader_open_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_reader_open_error;
    }
    for (size_t index = 0U; index < FAKE_STORAGE_BLOB_MAX_RECORDS; ++index) {
        if (g_fake_storage_blob.records[index].used &&
            g_fake_storage_blob.records[index].id == blob_id) {
            out_reader->descriptor = (int)index;
            out_reader->stored_bytes = g_fake_storage_blob.records[index].size;
            out_reader->bytes_read = 0U;
            out_reader->active = true;
            return APP_ERROR_NONE;
        }
    }
    return APP_ERROR_NOT_FOUND;
}

app_error_code_t storage_blob_reader_read(storage_blob_reader_t *reader, void *buffer,
                                          size_t buffer_size, size_t *out_count, bool *out_eof) {
    if (out_count != NULL) {
        *out_count = 0U;
    }
    if (out_eof != NULL) {
        *out_eof = false;
    }
    if (reader == NULL || buffer == NULL || buffer_size == 0U || out_count == NULL ||
        out_eof == NULL || !reader->active || reader->descriptor < 0 ||
        reader->descriptor >= (int)FAKE_STORAGE_BLOB_MAX_RECORDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const fake_storage_blob_record_t *record = &g_fake_storage_blob.records[reader->descriptor];
    if (!record->used || reader->bytes_read > record->size) {
        return APP_ERROR_INTERNAL;
    }
    size_t remaining = record->size - reader->bytes_read;
    size_t count = remaining < buffer_size ? remaining : buffer_size;
    if (count > 0U) {
        memcpy(buffer, record->data + reader->bytes_read, count);
    }
    reader->bytes_read += count;
    *out_count = count;
    *out_eof = reader->bytes_read >= record->size;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_reader_close(storage_blob_reader_t *reader) {
    if (reader == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    reader->active = false;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_delete(uint64_t blob_id) {
    if (g_fake_storage_blob.force_delete_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_delete_error;
    }
    fake_storage_blob_record_t *record = find_record(blob_id);
    if (record == NULL) {
        return APP_ERROR_NOT_FOUND;
    }
    free(record->data);
    record->data = NULL;
    record->size = 0U;
    record->used = false;
    record->id = 0U;
    return APP_ERROR_NONE;
}

app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes) {
    if (partition_label == NULL || out_total_bytes == NULL || out_used_bytes == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_fake_storage_blob.force_partition_capacity_error != APP_ERROR_NONE) {
        return g_fake_storage_blob.force_partition_capacity_error;
    }
    *out_total_bytes = g_fake_storage_blob.partition_total_bytes;
    *out_used_bytes = g_fake_storage_blob.partition_used_bytes;
    return APP_ERROR_NONE;
}
