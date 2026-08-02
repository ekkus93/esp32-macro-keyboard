#include "storage_atomic_recovery.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_atomic_validators.h"
#include "storage_fs_ops.h"
#include "storage_repository_lock.h"

/* ".tmp." / ".bak." marker (5 chars) + a 36-char UUID. */
#define ARTIFACT_MARKER_LENGTH 5U
#define ARTIFACT_SUFFIX_LENGTH (ARTIFACT_MARKER_LENGTH + APP_UUID_STRING_LENGTH)

/* True if any `/`-delimited component of the path is exactly "..". */
static bool has_dotdot_component(const char *path) {
    const char *segment = path;
    while (*segment != '\0') {
        const char *end = segment;
        while (*end != '\0' && *end != '/') {
            ++end;
        }
        if ((size_t)(end - segment) == 2U && segment[0] == '.' && segment[1] == '.') {
            return true;
        }
        segment = (*end == '/') ? end + 1 : end;
    }
    return false;
}

static bool match_marker(const char *marker, storage_atomic_artifact_kind_t *out_kind) {
    if (memcmp(marker, ".tmp.", ARTIFACT_MARKER_LENGTH) == 0) {
        *out_kind = STORAGE_ATOMIC_ARTIFACT_TEMPORARY;
        return true;
    }
    if (memcmp(marker, ".bak.", ARTIFACT_MARKER_LENGTH) == 0) {
        *out_kind = STORAGE_ATOMIC_ARTIFACT_BACKUP;
        return true;
    }
    return false;
}

app_error_code_t storage_atomic_recovery_parse(const char *artifact_path,
                                               storage_atomic_artifact_t *out_artifact) {
    if (artifact_path == NULL || out_artifact == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t length = strlen(artifact_path);
    if (length >= APP_PATH_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* Not long enough to even contain the suffix -- not an artifact. A name that
     * does contain the suffix but leaves an empty destination is a malformed
     * artifact (rejected below), not merely a non-artifact. */
    if (length < ARTIFACT_SUFFIX_LENGTH) {
        return APP_ERROR_NOT_FOUND;
    }

    const size_t marker_offset = length - ARTIFACT_SUFFIX_LENGTH;
    storage_atomic_artifact_kind_t kind;
    if (!match_marker(artifact_path + marker_offset, &kind)) {
        return APP_ERROR_NOT_FOUND;
    }

    /* The trailing 36 characters must be a valid UUID (this also guarantees the
     * suffix, and therefore the reconstructed destination, contains no separator
     * so it cannot escape the artifact's own directory). */
    char uuid_text[APP_UUID_BUFFER_LENGTH];
    memcpy(uuid_text, artifact_path + (length - APP_UUID_STRING_LENGTH), APP_UUID_STRING_LENGTH);
    uuid_text[APP_UUID_STRING_LENGTH] = '\0';
    if (!app_uuid_is_valid_string(uuid_text)) {
        return APP_ERROR_NOT_FOUND;
    }

    const size_t destination_length = marker_offset;
    /* Reject an empty destination, a destination that ends in a separator (no
     * filename to reconstruct), and any `..` traversal component. */
    if (destination_length == 0U || artifact_path[destination_length - 1U] == '/') {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    memset(out_artifact, 0, sizeof(*out_artifact));
    memcpy(out_artifact->destination, artifact_path, destination_length);
    out_artifact->destination[destination_length] = '\0';
    if (has_dotdot_component(out_artifact->destination)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const app_error_code_t uuid_result = app_uuid_parse(uuid_text, &out_artifact->operation_id);
    if (uuid_result != APP_ERROR_NONE) {
        return uuid_result;
    }
    out_artifact->kind = kind;
    memcpy(out_artifact->artifact_path, artifact_path, length);
    out_artifact->artifact_path[length] = '\0';
    return APP_ERROR_NONE;
}

app_error_code_t storage_atomic_recovery_list_add(storage_atomic_artifact_list_t *list,
                                                  const char *artifact_path) {
    if (list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_atomic_artifact_t artifact;
    const app_error_code_t result = storage_atomic_recovery_parse(artifact_path, &artifact);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        if (strcmp(list->items[index].artifact_path, artifact.artifact_path) == 0) {
            return APP_ERROR_CONFLICT;
        }
    }
    if (list->count >= STORAGE_ATOMIC_RECOVERY_MAX_ARTIFACTS) {
        return APP_ERROR_STORAGE_FULL;
    }
    list->items[list->count++] = artifact;
    return APP_ERROR_NONE;
}

storage_atomic_reconcile_action_t
storage_atomic_reconcile_decide(const storage_atomic_reconcile_state_t *state) {
    if (state == NULL) {
        return STORAGE_ATOMIC_RECONCILE_NOTHING;
    }
    /* More than one temporary or backup for a single destination is two
     * interrupted writes racing: conflicting, discard the artifacts. */
    if (state->temporary_count > 1U || state->backup_count > 1U) {
        return STORAGE_ATOMIC_RECONCILE_QUARANTINE;
    }
    if (state->canonical_present) {
        /* The canonical file is fully written (the atomic barrier only publishes
         * it once complete), so it is authoritative; any leftover temporary or
         * backup is a straggler to remove. */
        if (state->temporary_count == 0U && state->backup_count == 0U) {
            return STORAGE_ATOMIC_RECONCILE_NOTHING;
        }
        return STORAGE_ATOMIC_RECONCILE_KEEP_CANONICAL;
    }
    /* Canonical is absent. A backup is the previous committed state: restore it
     * when it is valid, otherwise there is nothing safe to activate -- discard
     * the corrupt backup rather than resurrect bad state. */
    if (state->backup_count == 1U) {
        return state->backup_valid ? STORAGE_ATOMIC_RECONCILE_RESTORE_BACKUP
                                   : STORAGE_ATOMIC_RECONCILE_QUARANTINE;
    }
    /* Only a temporary remains. A temporary is by definition an incomplete write
     * (the barrier would have published the canonical name otherwise), so when the
     * owning transaction does not prove roll-forward it is simply discarded and its
     * content-validity is irrelevant. It is activated only when roll-forward is
     * proven AND the bytes are valid; a proven-but-corrupt temporary is
     * discarded. */
    if (state->temporary_count == 1U) {
        if (state->roll_forward_proven) {
            return state->temporary_valid ? STORAGE_ATOMIC_RECONCILE_ACTIVATE_TEMPORARY
                                          : STORAGE_ATOMIC_RECONCILE_QUARANTINE;
        }
        return STORAGE_ATOMIC_RECONCILE_DISCARD_TEMPORARY;
    }
    return STORAGE_ATOMIC_RECONCILE_NOTHING;
}

/* ---- Executor: enumerate, group by destination, decide, and apply. ---- */

typedef struct {
    const char *temporary_path;
    const char *backup_path;
} destination_artifacts_t;

static app_error_code_t recovery_map_errno(int error_number) {
    return error_number == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

static app_error_code_t sync_parent(const storage_fs_ops_t *operations, const char *path) {
    if (storage_fs_sync_parent_path(operations->context, path) != 0) {
        return recovery_map_errno(errno);
    }
    return APP_ERROR_NONE;
}

/* Add every artifact file in one directory to the list. Non-artifact and
 * unparseable names are skipped; a missing directory is not an error. */
static app_error_code_t scan_directory(const storage_fs_ops_t *operations, const char *directory,
                                       storage_atomic_artifact_list_t *list) {
    void *handle = operations->open_directory(operations->context, directory);
    if (handle == NULL) {
        const int open_error = errno;
        return (open_error == ENOENT || open_error == ENOTDIR) ? APP_ERROR_NONE
                                                               : recovery_map_errno(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context, handle, name, sizeof(name), &end) !=
            0) {
            result = recovery_map_errno(errno);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        char path[APP_PATH_MAX_BYTES];
        const int written = snprintf(path, sizeof(path), "%s/%s", directory, name);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_IO;
            break;
        }
        const app_error_code_t add = storage_atomic_recovery_list_add(list, path);
        if (add == APP_ERROR_STORAGE_FULL) {
            result = add;
            break;
        }
        /* NOT_FOUND (not an artifact), INVALID_ARGUMENT (malformed name), and
         * CONFLICT (duplicate) are all simply not added. */
    }
    if (operations->close_directory(operations->context, handle) != 0 && result == APP_ERROR_NONE) {
        result = recovery_map_errno(errno);
    }
    return result;
}

/* Scan every immediate subdirectory of `parent` for artifacts (used for sets/ and
 * staging/, whose children are per-set / per-transaction directories). */
static app_error_code_t scan_subdirectories(const storage_fs_ops_t *operations, const char *parent,
                                            storage_atomic_artifact_list_t *list) {
    void *handle = operations->open_directory(operations->context, parent);
    if (handle == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_NONE : recovery_map_errno(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context, handle, name, sizeof(name), &end) !=
            0) {
            result = recovery_map_errno(errno);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }
        char path[APP_PATH_MAX_BYTES];
        const int written = snprintf(path, sizeof(path), "%s/%s", parent, name);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_IO;
            break;
        }
        result = scan_directory(operations, path, list);
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    if (operations->close_directory(operations->context, handle) != 0 && result == APP_ERROR_NONE) {
        result = recovery_map_errno(errno);
    }
    return result;
}

static app_error_code_t collect_all_artifacts(const storage_fs_ops_t *operations,
                                              storage_atomic_artifact_list_t *list) {
    app_error_code_t result = scan_directory(operations, STORAGE_DATA_MOUNT, list);
    if (result == APP_ERROR_NONE) {
        result = scan_directory(operations, STORAGE_DATA_MOUNT "/transactions", list);
    }
    if (result == APP_ERROR_NONE) {
        result = scan_subdirectories(operations, STORAGE_DATA_MOUNT "/sets", list);
    }
    if (result == APP_ERROR_NONE) {
        result = scan_subdirectories(operations, STORAGE_DATA_MOUNT "/staging", list);
    }
    return result;
}

static void gather_destination(const storage_atomic_artifact_list_t *list, const char *destination,
                               storage_atomic_reconcile_state_t *state,
                               destination_artifacts_t *artifacts) {
    memset(state, 0, sizeof(*state));
    artifacts->temporary_path = NULL;
    artifacts->backup_path = NULL;
    for (size_t index = 0U; index < list->count; ++index) {
        const storage_atomic_artifact_t *artifact = &list->items[index];
        if (strcmp(artifact->destination, destination) != 0) {
            continue;
        }
        if (artifact->kind == STORAGE_ATOMIC_ARTIFACT_TEMPORARY) {
            ++state->temporary_count;
            artifacts->temporary_path = artifact->artifact_path;
        } else {
            ++state->backup_count;
            artifacts->backup_path = artifact->artifact_path;
        }
    }
}

/* Unlink the destination's artifacts (all, or only temporaries). A missing file is
 * not an error; the first genuine failure is returned so the artifacts are left in
 * place for a retry rather than silently lost. */
static app_error_code_t remove_destination_artifacts(const storage_fs_ops_t *operations,
                                                     const storage_atomic_artifact_list_t *list,
                                                     const char *destination,
                                                     bool temporaries_only) {
    app_error_code_t result = APP_ERROR_NONE;
    for (size_t index = 0U; index < list->count; ++index) {
        const storage_atomic_artifact_t *artifact = &list->items[index];
        if (strcmp(artifact->destination, destination) != 0) {
            continue;
        }
        if (temporaries_only && artifact->kind != STORAGE_ATOMIC_ARTIFACT_TEMPORARY) {
            continue;
        }
        if (operations->unlink_path(operations->context, artifact->artifact_path) != 0) {
            const int unlink_error = errno;
            if (unlink_error != ENOENT && result == APP_ERROR_NONE) {
                result = recovery_map_errno(unlink_error);
            }
        }
    }
    return result;
}

static app_error_code_t discard_destination_artifacts(const storage_fs_ops_t *operations,
                                                      const storage_atomic_artifact_list_t *list,
                                                      const char *destination) {
    app_error_code_t result = APP_ERROR_NONE;
    for (size_t index = 0U; index < list->count; ++index) {
        const storage_atomic_artifact_t *artifact = &list->items[index];
        if (strcmp(artifact->destination, destination) != 0) {
            continue;
        }
        /* Discard the artifact. Keeping it archived spent storage on data
         * nothing read; if the unlink itself fails the error is reported so
         * recovery is retried rather than silently skipped. */
        if (operations->unlink_path(operations->context, artifact->artifact_path) != 0 &&
            result == APP_ERROR_NONE) {
            result = APP_ERROR_IO;
        }
    }
    return result;
}

static app_error_code_t restore_backup(const storage_fs_ops_t *operations,
                                       const storage_atomic_artifact_list_t *list,
                                       const char *destination,
                                       const destination_artifacts_t *artifacts) {
    if (operations->rename_path(operations->context, artifacts->backup_path, destination) != 0) {
        return recovery_map_errno(errno);
    }
    app_error_code_t result = sync_parent(operations, destination);
    const app_error_code_t remove_result =
        remove_destination_artifacts(operations, list, destination, true);
    if (result == APP_ERROR_NONE) {
        result = remove_result;
    }
    const app_error_code_t sync_result = sync_parent(operations, destination);
    return result == APP_ERROR_NONE ? sync_result : result;
}

static app_error_code_t activate_temporary(const storage_fs_ops_t *operations,
                                           const storage_atomic_artifact_list_t *list,
                                           const char *destination,
                                           const destination_artifacts_t *artifacts) {
    if (operations->rename_path(operations->context, artifacts->temporary_path, destination) != 0) {
        return recovery_map_errno(errno);
    }
    app_error_code_t result = sync_parent(operations, destination);
    /* The temporary is now the canonical file; remove any leftover backup. */
    const app_error_code_t remove_result =
        remove_destination_artifacts(operations, list, destination, false);
    if (result == APP_ERROR_NONE) {
        result = remove_result;
    }
    const app_error_code_t sync_result = sync_parent(operations, destination);
    return result == APP_ERROR_NONE ? sync_result : result;
}

static app_error_code_t execute_reconcile_action(const storage_fs_ops_t *operations,
                                                 const storage_atomic_artifact_list_t *list,
                                                 const char *destination,
                                                 storage_atomic_reconcile_action_t action,
                                                 const destination_artifacts_t *artifacts) {
    switch (action) {
    case STORAGE_ATOMIC_RECONCILE_NOTHING:
        return APP_ERROR_NONE;
    case STORAGE_ATOMIC_RECONCILE_KEEP_CANONICAL: {
        const app_error_code_t result =
            remove_destination_artifacts(operations, list, destination, false);
        return result == APP_ERROR_NONE ? sync_parent(operations, destination) : result;
    }
    case STORAGE_ATOMIC_RECONCILE_RESTORE_BACKUP:
        return restore_backup(operations, list, destination, artifacts);
    case STORAGE_ATOMIC_RECONCILE_ACTIVATE_TEMPORARY:
        return activate_temporary(operations, list, destination, artifacts);
    case STORAGE_ATOMIC_RECONCILE_DISCARD_TEMPORARY: {
        const app_error_code_t result =
            remove_destination_artifacts(operations, list, destination, true);
        return result == APP_ERROR_NONE ? sync_parent(operations, destination) : result;
    }
    case STORAGE_ATOMIC_RECONCILE_QUARANTINE:
        return discard_destination_artifacts(operations, list, destination);
    default:
        return APP_ERROR_INTERNAL;
    }
}

static app_error_code_t reconcile_destination(const storage_fs_ops_t *operations,
                                              const storage_atomic_artifact_list_t *list,
                                              const char *destination) {
    storage_atomic_reconcile_state_t state;
    destination_artifacts_t artifacts;
    gather_destination(list, destination, &state, &artifacts);

    struct stat metadata;
    state.canonical_present =
        operations->stat_path(operations->context, destination, &metadata) == 0;
    /* Validate the backup only when it is the restore source (canonical absent).
     * A temporary is never validated: it is discarded unless roll-forward is
     * proven, which never happens at the atomic layer. */
    if (!state.canonical_present && state.backup_count == 1U) {
        state.backup_valid = storage_atomic_validate_candidate(
                                 operations, destination, artifacts.backup_path) == APP_ERROR_NONE;
    }
    state.roll_forward_proven = false;

    const storage_atomic_reconcile_action_t action = storage_atomic_reconcile_decide(&state);
    return execute_reconcile_action(operations, list, destination, action, &artifacts);
}

static bool destination_already_seen(const storage_atomic_artifact_list_t *list, size_t upto) {
    for (size_t prior = 0U; prior < upto; ++prior) {
        if (strcmp(list->items[prior].destination, list->items[upto].destination) == 0) {
            return true;
        }
    }
    return false;
}

app_error_code_t storage_atomic_recover_all_with_ops(const storage_fs_ops_t *operations) {
    if (!storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* The artifact list is large; allocate it rather than place it on the task
     * stack. Recovery is single-threaded at startup. */
    storage_atomic_artifact_list_t *list = calloc(1U, sizeof(*list));
    if (list == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = collect_all_artifacts(operations, list);
    /* Reconcile each distinct destination once. Every destination is attempted
     * even if one fails; the first error is kept. Skipped when enumeration failed
     * (the list may be incomplete). */
    if (result == APP_ERROR_NONE) {
        for (size_t index = 0U; index < list->count; ++index) {
            if (destination_already_seen(list, index)) {
                continue;
            }
            const app_error_code_t reconcile_result =
                reconcile_destination(operations, list, list->items[index].destination);
            if (reconcile_result != APP_ERROR_NONE && result == APP_ERROR_NONE) {
                result = reconcile_result;
            }
        }
    }
    free(list);
    return result;
}

app_error_code_t storage_atomic_recover_all(void) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_atomic_recover_all_with_ops(storage_fs_ops_posix());
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
