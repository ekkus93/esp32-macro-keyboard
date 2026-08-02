#ifndef STORAGE_ATOMIC_RECOVERY_H
#define STORAGE_ATOMIC_RECOVERY_H

#include "app_error.h"
#include "storage_fs_ops.h"

/* Boot recovery, in its entirety (SPEC 13.4): unlink every `*.tmp` file under
 * the data mount.
 *
 * `storage_atomic_write` stages `<destination>.tmp` and renames it over the
 * destination. Because `rename()` is atomic, an interruption leaves either the
 * complete old file or the complete new one, and a surviving `*.tmp` is debris
 * from a write that never committed. An interrupted write is indistinguishable
 * from one that never started, which is the correct outcome -- so there is
 * nothing to reconcile, roll forward, or restore, and no decision table.
 *
 * Every directory is walked and every removable artifact is attempted even if
 * one unlink fails; the first error is returned. */
app_error_code_t storage_atomic_recover_all_with_ops(const storage_fs_ops_t *operations);

/* Production entry point (declared in storage.h): recover using the POSIX
 * filesystem backend. */

#endif
