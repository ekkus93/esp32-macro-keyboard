#ifndef STORAGE_BLOB_INTERNAL_H
#define STORAGE_BLOB_INTERNAL_H
#include "storage_blob.h"
#include "storage_fs_ops.h"
#include <stdint.h>
app_error_code_t storage_blob_prepare_directory_with_ops(const storage_fs_ops_t *operations,
                                                         const char *directory_path);
app_error_code_t storage_blob_scan_with_ops(const storage_fs_ops_t *operations,
                                            const char *directory_path, uint64_t persisted_next_id,
                                            const storage_blob_scan_observer_t *observer,
                                            storage_blob_scan_summary_t *out_summary);
#endif
