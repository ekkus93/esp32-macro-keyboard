#include "storage_blob.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"

static storage_blob_scan_summary_t scan_state;

static app_error_code_t capture_temporary_file(void *context, const char *name) {
    storage_blob_diagnostics_t *diagnostics = context;
    if (diagnostics == NULL || name == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (diagnostics->reported_temporary_file_count >= STORAGE_BLOB_DIAGNOSTIC_INVALID_NAME_MAX) {
        diagnostics->temporary_files_truncated = true;
        return APP_ERROR_NONE;
    }
    const size_t length = strlen(name);
    if (length >= STORAGE_BLOB_DIAGNOSTIC_NAME_CAPACITY) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(diagnostics->temporary_files[diagnostics->reported_temporary_file_count], name,
           length + 1U);
    ++diagnostics->reported_temporary_file_count;
    return APP_ERROR_NONE;
}

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
        .visit_temporary_file = capture_temporary_file,
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

typedef struct {
    const storage_fs_ops_t *operations;
    const char *directory_path;
    app_error_code_t first_error;
    size_t deleted_count;
} storage_blob_delete_all_state_t;

static app_error_code_t delete_all_visit_entry(void *context, const storage_blob_entry_t *entry) {
    storage_blob_delete_all_state_t *state = context;
    if (state == NULL || entry == NULL) {
        return APP_ERROR_NONE;
    }
    const app_error_code_t result =
        storage_blob_delete_with_ops(state->operations, state->directory_path, entry->id);
    if (result == APP_ERROR_NONE) {
        ++state->deleted_count;
    } else if (state->first_error == APP_ERROR_NONE) {
        state->first_error = result;
    }
    /* Always continue: one blob that resists deletion must never strand the
     * rest (SPEC_V2.md §11.4 "factory reset ... erases ... all repository
     * blobs"). The accumulated first_error is returned by the caller once the
     * scan finishes. */
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_delete_all_with_ops(const storage_fs_ops_t *operations,
                                                  const char *directory_path,
                                                  size_t *out_deleted_count) {
    if (out_deleted_count != NULL) {
        *out_deleted_count = 0U;
    }
    storage_blob_delete_all_state_t state = {
        .operations = operations,
        .directory_path = directory_path,
        .first_error = APP_ERROR_NONE,
        .deleted_count = 0U,
    };
    const storage_blob_scan_observer_t observer = {
        .context = &state,
        .visit_entry = delete_all_visit_entry,
        .visit_invalid_name = NULL,
        .visit_temporary_file = NULL,
    };
    storage_blob_scan_summary_t summary = {0};
    const app_error_code_t scan_result =
        storage_blob_scan_with_ops(operations, directory_path, 0U, &observer, &summary);
    if (out_deleted_count != NULL) {
        *out_deleted_count = state.deleted_count;
    }
    return scan_result != APP_ERROR_NONE ? scan_result : state.first_error;
}

app_error_code_t storage_blob_delete_all(size_t *out_deleted_count) {
    size_t deleted_count = 0U;
    app_error_code_t result = storage_blob_delete_all_with_ops(
        storage_fs_ops_posix(), STORAGE_BLOB_DIRECTORY, &deleted_count);
    for (size_t index = 0U; index < deleted_count; ++index) {
        storage_blob_record_deleted_entry();
    }
    if (deleted_count > 0U && storage_fs_sync_parent_path(NULL, STORAGE_BLOB_DIRECTORY "/.") != 0 &&
        result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    if (out_deleted_count != NULL) {
        *out_deleted_count = deleted_count;
    }
    return result;
}
