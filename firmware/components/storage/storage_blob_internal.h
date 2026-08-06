#ifndef STORAGE_BLOB_INTERNAL_H
#define STORAGE_BLOB_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "storage_blob.h"
#include "storage_fs_ops.h"

app_error_code_t storage_blob_prepare_directory_with_ops(const storage_fs_ops_t *operations,
                                                         const char *directory_path);
app_error_code_t storage_blob_recover_with_ops(const storage_fs_ops_t *operations,
                                               const char *directory_path,
                                               storage_blob_recovery_summary_t *out_summary);
app_error_code_t storage_blob_scan_with_ops(const storage_fs_ops_t *operations,
                                            const char *directory_path, uint64_t persisted_next_id,
                                            const storage_blob_scan_observer_t *observer,
                                            storage_blob_scan_summary_t *out_summary);
app_error_code_t storage_blob_reader_open_with_ops(const storage_fs_ops_t *operations,
                                                   const char *directory_path, uint64_t blob_id,
                                                   storage_blob_reader_t *out_reader);
app_error_code_t storage_blob_reader_read_with_ops(storage_blob_reader_t *reader, void *buffer,
                                                   size_t buffer_size, size_t *out_count,
                                                   bool *out_eof);
app_error_code_t storage_blob_reader_close_with_ops(storage_blob_reader_t *reader);
app_error_code_t storage_blob_delete_with_ops(const storage_fs_ops_t *operations,
                                              const char *directory_path, uint64_t blob_id);
void storage_blob_record_committed_entry(const storage_blob_entry_t *entry);
void storage_blob_record_deleted_entry(void);

#endif
