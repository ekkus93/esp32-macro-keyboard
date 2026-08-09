#ifndef FAKE_STORAGE_BLOB_H
#define FAKE_STORAGE_BLOB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "storage.h"
#include "storage_blob.h"

/* Link-time test double for the storage_blob.h / storage.h functions
 * web_server_blob.c calls directly (storage_blob_list, storage_blob_upload_*,
 * storage_blob_reader_*, storage_blob_delete, storage_partition_capacity).
 * There is no dependency-injection seam
 * at that boundary (unlike e.g. web_send_ops_t), and the real implementations
 * are not host-linkable: storage_blob_upload_commit()'s persist_next_id()
 * unconditionally returns APP_ERROR_STORAGE_UNAVAILABLE off-device (no NVS),
 * and storage_partition_capacity() lives in storage_mount.c, which includes
 * ESP-IDF's esp_littlefs.h and does not compile on the host at all. Real
 * on-device storage_blob behavior (filenames, next-id derivation, atomic
 * commit, capacity accounting) is exercised by storage_blob_tests /
 * storage_blob_upload_tests / storage_blob_access_tests (V2-034); this fake
 * only needs to honor the storage_blob.h contract closely enough to prove
 * web_server_blob.c's handlers wire status codes, content, and bytes
 * correctly against it. Provides real symbols with the real names, so this
 * file must never be linked into the same binary as the production
 * storage_blob*.c sources. */

#define FAKE_STORAGE_BLOB_MAX_RECORDS 8U

typedef struct {
    bool used;
    uint64_t id;
    uint8_t *data;
    size_t size;
} fake_storage_blob_record_t;

typedef struct {
    fake_storage_blob_record_t records[FAKE_STORAGE_BLOB_MAX_RECORDS];
    uint64_t next_id;

    app_error_code_t force_list_error;
    app_error_code_t force_upload_begin_error;
    app_error_code_t force_upload_write_error;
    app_error_code_t force_upload_commit_error;
    app_error_code_t force_reader_open_error;
    app_error_code_t force_delete_error;

    size_t partition_total_bytes;
    size_t partition_used_bytes;
    app_error_code_t force_partition_capacity_error;
} fake_storage_blob_state_t;

extern fake_storage_blob_state_t g_fake_storage_blob;

/* Frees every seeded/committed record and resets all injected errors and
 * counters. Call before every test. */
void fake_storage_blob_reset(void);
/* Preloads a blob as if it had already been committed, for load/delete
 * tests that do not go through the create path first. */
void fake_storage_blob_seed(uint64_t id, const void *data, size_t size);
/* NULL if no record with that id exists. */
const fake_storage_blob_record_t *fake_storage_blob_find(uint64_t id);

#endif
