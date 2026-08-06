#include "storage_blob.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"

static storage_blob_scan_summary_t scan_state;

static app_error_code_t capture_invalid_name(void *context, const char *name) {
    storage_blob_diagnostics_t *diagnostics = context;
    if (diagnostics == NULL || name == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (diagnostics->reported_invalid_name_count >= STORAGE_BLOB_DIAGNOSTIC_INVALID_NAME_MAX) {
        diagnostics->invalid_names_truncated = true;
        return APP_ERROR_NONE;
    }
    const size_t length = strlen(name);
    if (length >= STORAGE_BLOB_DIAGNOSTIC_NAME_CAPACITY) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(diagnostics->invalid_names[diagnostics->reported_invalid_name_count], name, length + 1U);
    ++diagnostics->reported_invalid_name_count;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_scan(uint64_t persisted_next_id,
                                   const storage_blob_scan_observer_t *observer,
                                   storage_blob_scan_summary_t *out_summary) {
    return storage_blob_scan_with_ops(storage_fs_ops_posix(), STORAGE_BLOB_DIRECTORY,
                                      persisted_next_id, observer, out_summary);
}

app_error_code_t storage_blob_scan_startup(uint64_t persisted_next_id) {
    storage_blob_scan_summary_t summary = {0};
    const app_error_code_t result = storage_blob_scan(persisted_next_id, NULL, &summary);
    if (result != APP_ERROR_NONE) {
        scan_state = (storage_blob_scan_summary_t){0};
        return result;
    }
    scan_state = summary;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_list(const storage_blob_scan_observer_t *observer,
                                   storage_blob_scan_summary_t *out_summary) {
    if (out_summary == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_blob_scan_summary_t summary = {0};
    const app_error_code_t result = storage_blob_scan(scan_state.next_id, observer, &summary);
    if (result != APP_ERROR_NONE) {
        *out_summary = (storage_blob_scan_summary_t){0};
        return result;
    }
    scan_state = summary;
    *out_summary = summary;
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_collect_diagnostics(storage_blob_diagnostics_t *out_diagnostics) {
    if (out_diagnostics == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_diagnostics = (storage_blob_diagnostics_t){0};
    const storage_blob_scan_observer_t observer = {
        .context = out_diagnostics,
        .visit_entry = NULL,
        .visit_invalid_name = capture_invalid_name,
    };
    return storage_blob_scan(scan_state.next_id, &observer, &out_diagnostics->summary);
}

storage_blob_scan_summary_t storage_blob_scan_state(void) {
    return scan_state;
}

void storage_blob_record_committed_entry(const storage_blob_entry_t *entry) {
    if (entry == NULL || entry->id == 0U || entry->id == UINT64_MAX) {
        return;
    }
    ++scan_state.valid_count;
    if (!scan_state.has_max_id || entry->id > scan_state.max_id) {
        scan_state.has_max_id = true;
        scan_state.max_id = entry->id;
    }
    scan_state.next_id = entry->id + UINT64_C(1);
}

void storage_blob_record_deleted_entry(void) {
    if (scan_state.valid_count > 0U) {
        --scan_state.valid_count;
    }
}
