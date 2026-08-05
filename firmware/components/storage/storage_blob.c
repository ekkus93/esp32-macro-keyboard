#include "storage_blob.h"

#include <stdint.h>

#include "storage_blob_internal.h"
#include "storage_fs_ops.h"

static storage_blob_scan_summary_t scan_state;

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

storage_blob_scan_summary_t storage_blob_scan_state(void) {
    return scan_state;
}
