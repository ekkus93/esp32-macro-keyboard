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
/* Deletes every valid blob found by a scan of directory_path. Continues past
 * an individual delete failure so one stuck file can never strand the rest
 * (mirrors the wifi_ap/device_controls "continue past failure, report the
 * first error" cleanup pattern); out_deleted_count always reports how many
 * deletes actually succeeded, even when the return value is an error. Used by
 * SPEC_V2.md §11.4 "factory reset" ("erases ... all repository blobs"). */
app_error_code_t storage_blob_delete_all_with_ops(const storage_fs_ops_t *operations,
                                                  const char *directory_path,
                                                  size_t *out_deleted_count);
void storage_blob_record_committed_entry(const storage_blob_entry_t *entry);
void storage_blob_record_deleted_entry(void);

#endif
