#ifndef STORAGE_ATOMIC_RECOVERY_H
#define STORAGE_ATOMIC_RECOVERY_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"

/* Leftover artifacts from an interrupted atomic write. `storage_atomic_write`
 * stages `<destination>.tmp.<uuid>` and, while swapping, `<destination>.bak.<uuid>`;
 * a crash between steps can leave either behind. Recovery parses those names back
 * into the destination they belong to and the operation that created them. */
typedef enum {
    STORAGE_ATOMIC_ARTIFACT_TEMPORARY = 0,
    STORAGE_ATOMIC_ARTIFACT_BACKUP,
} storage_atomic_artifact_kind_t;

typedef struct {
    char destination[APP_PATH_MAX_BYTES];
    app_uuid_t operation_id;
    storage_atomic_artifact_kind_t kind;
    char artifact_path[APP_PATH_MAX_BYTES];
} storage_atomic_artifact_t;

#define STORAGE_ATOMIC_RECOVERY_MAX_ARTIFACTS 64U

typedef struct {
    storage_atomic_artifact_t items[STORAGE_ATOMIC_RECOVERY_MAX_ARTIFACTS];
    size_t count;
} storage_atomic_artifact_list_t;

/* Parse one artifact path of the exact form
 *   <destination>.tmp.<lowercase-rfc4122-v4-uuid>
 *   <destination>.bak.<lowercase-rfc4122-v4-uuid>
 * Returns:
 *   APP_ERROR_NONE             - parsed; out_artifact populated.
 *   APP_ERROR_NOT_FOUND        - the name is not an artifact (skip it).
 *   APP_ERROR_INVALID_ARGUMENT - malformed input, or an artifact whose
 *                                reconstructed destination is empty, ends in a
 *                                separator, or contains a `..` path component
 *                                (path traversal / cross-directory escape).
 */
app_error_code_t storage_atomic_recovery_parse(const char *artifact_path,
                                               storage_atomic_artifact_t *out_artifact);

/* Parse `artifact_path` and append it to `list`. Propagates the parse result, and
 * additionally returns APP_ERROR_CONFLICT for a duplicate artifact_path already in
 * the list, or APP_ERROR_STORAGE_FULL when the list is full. Nothing is appended
 * on any error. */
app_error_code_t storage_atomic_recovery_list_add(storage_atomic_artifact_list_t *list,
                                                  const char *artifact_path);

/* The reconciliation action for one destination's leftover artifacts. Recovery is
 * conservative: it rolls interrupted writes BACK (an unfinished temporary means
 * the atomic barrier never committed, so no owning transaction was durably
 * declared) and only rolls a temporary FORWARD when the owning transaction proves
 * it should be activated. Transaction-level completion is handled separately by
 * transaction recovery, which runs after this. */
typedef enum {
    STORAGE_ATOMIC_RECONCILE_NOTHING = 0,        /* no artifacts to reconcile */
    STORAGE_ATOMIC_RECONCILE_KEEP_CANONICAL,     /* canonical is authoritative; delete stragglers */
    STORAGE_ATOMIC_RECONCILE_RESTORE_BACKUP,     /* rename backup -> canonical; delete temporary */
    STORAGE_ATOMIC_RECONCILE_ACTIVATE_TEMPORARY, /* rename temporary -> canonical; delete backup */
    STORAGE_ATOMIC_RECONCILE_DISCARD_TEMPORARY,  /* delete temporary; canonical stays absent */
    STORAGE_ATOMIC_RECONCILE_QUARANTINE, /* malformed or conflicting: quarantine artifacts */
} storage_atomic_reconcile_action_t;

/* The observed on-disk state of one destination and its artifacts. `*_valid` is
 * meaningful only when the corresponding count is exactly 1. `roll_forward_proven`
 * is set by the caller when the owning transaction proves the temporary should be
 * activated (never, at the atomic layer, in the current architecture). */
typedef struct {
    bool canonical_present;
    size_t temporary_count;
    bool temporary_valid;
    size_t backup_count;
    bool backup_valid;
    bool roll_forward_proven;
} storage_atomic_reconcile_state_t;

/* Decide the conservative reconciliation action for one destination. Pure: no I/O. */
storage_atomic_reconcile_action_t
storage_atomic_reconcile_decide(const storage_atomic_reconcile_state_t *state);

/* Enumerate every leftover atomic-write artifact across the storage tree
 * (the mount root, global/, transactions/, and every sets/ and staging/
 * subdirectory), group them by destination, and reconcile each destination with
 * storage_atomic_reconcile_decide -- executing the chosen action with every
 * rename/unlink/parent-sync checked and quarantining malformed or conflicting
 * artifacts. Every destination is attempted even if one fails; the first error is
 * returned. Must run before transaction recovery. `generate_uuid` is used when an
 * artifact must be quarantined. */
app_error_code_t storage_atomic_recover_all_with_ops(const storage_fs_ops_t *operations,
                                                     storage_uuid_generate_fn generate_uuid,
                                                     void *uuid_context);

/* Production entry point: reconcile using the POSIX filesystem backend. */
app_error_code_t storage_atomic_recover_all(void);

#endif
