#ifndef STORAGE_BLOB_H
#define STORAGE_BLOB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "storage.h"

#define STORAGE_BLOB_DIRECTORY STORAGE_DATA_MOUNT "/repository"
#define STORAGE_BLOB_ID_DIGITS 20U
#define STORAGE_BLOB_FILENAME_LENGTH (STORAGE_BLOB_ID_DIGITS + 3U)
#define STORAGE_BLOB_FILENAME_CAPACITY (STORAGE_BLOB_FILENAME_LENGTH + 1U)

typedef struct {
    uint64_t id;
    size_t stored_bytes;
} storage_blob_entry_t;

typedef struct {
    size_t valid_count;
    size_t invalid_name_count;
    bool has_max_id;
    uint64_t max_id;
    uint64_t next_id;
} storage_blob_scan_summary_t;

typedef app_error_code_t (*storage_blob_entry_visitor_t)(void *context,
                                                         const storage_blob_entry_t *entry);
typedef app_error_code_t (*storage_blob_invalid_name_visitor_t)(void *context, const char *name);

typedef struct {
    void *context;
    storage_blob_entry_visitor_t visit_entry;
    storage_blob_invalid_name_visitor_t visit_invalid_name;
} storage_blob_scan_observer_t;

bool storage_blob_parse_filename(const char *name, uint64_t *out_id);
app_error_code_t storage_blob_format_filename(uint64_t id, char *out_name, size_t name_size);
app_error_code_t storage_blob_derive_next_id(uint64_t persisted_next_id, bool has_max_id,
                                             uint64_t max_id, uint64_t *out_next_id);
void storage_blob_sort_newest_first(storage_blob_entry_t *entries, size_t entry_count);
app_error_code_t storage_blob_scan_startup(uint64_t persisted_next_id);
app_error_code_t storage_blob_scan(uint64_t persisted_next_id,
                                   const storage_blob_scan_observer_t *observer,
                                   storage_blob_scan_summary_t *out_summary);
storage_blob_scan_summary_t storage_blob_scan_state(void);

#endif
