#ifndef STORAGE_QUARANTINE_INTERNAL_H
#define STORAGE_QUARANTINE_INTERNAL_H

#include "storage.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"

app_error_code_t storage_quarantine_file_with_ops(const char *source_path, const char *reason,
                                                  storage_quarantine_entry_t *out_entry,
                                                  const storage_fs_ops_t *operations,
                                                  storage_uuid_generate_fn generate_uuid,
                                                  void *uuid_context);

app_error_code_t storage_quarantine_list_with_ops(storage_quarantine_list_t *out_list,
                                                  const storage_fs_ops_t *operations);

/* Read and validate a single quarantine record at `path`, requiring its JSON id
 * to equal `expected_id`. Reads via `operations` and does not move or modify any
 * file. */
app_error_code_t storage_quarantine_read_record_with_ops(const char *path,
                                                         const app_uuid_t *expected_id,
                                                         storage_quarantine_entry_t *out_entry,
                                                         const storage_fs_ops_t *operations);

#endif
