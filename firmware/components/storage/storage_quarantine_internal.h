#ifndef STORAGE_QUARANTINE_INTERNAL_H
#define STORAGE_QUARANTINE_INTERNAL_H

#include "storage.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"

/* Staging directories for in-progress quarantine entries live under
 * /data/staging with this name prefix (FIX1 §8.1). Transaction recovery skips
 * them when asserting that the transaction staging root is otherwise empty. */
#define STORAGE_QUARANTINE_STAGING_PREFIX "quarantine-"

app_error_code_t storage_quarantine_file_with_ops(const char *source_path, const char *reason,
                                                  storage_quarantine_entry_t *out_entry,
                                                  const storage_fs_ops_t *operations,
                                                  storage_uuid_generate_fn generate_uuid,
                                                  void *uuid_context);

app_error_code_t storage_quarantine_list_with_ops(storage_quarantine_list_t *out_list,
                                                  const storage_fs_ops_t *operations);

/* Quarantine a source file without acquiring the repository mutation lock, for
 * callers that already hold it (the repository read/load and recovery paths).
 * The public storage_quarantine_file wraps this with lock acquisition. */
app_error_code_t storage_quarantine_file_locked(const char *source_path, const char *reason,
                                                storage_quarantine_entry_t *out_entry);

/* Reconcile interrupted staged quarantine entries under /data/staging (FIX1
 * §8.3): finish provably complete entries into the quarantine root, discard
 * staging whose source was never durably moved, and preserve ambiguous staging
 * as evidence (never deleting an unmatched evidence file). Returns the first
 * health error encountered (ambiguous/damaged staging) after processing every
 * entry, or a filesystem error. */
app_error_code_t storage_quarantine_recover_all_with_ops(const storage_fs_ops_t *operations);

#endif
