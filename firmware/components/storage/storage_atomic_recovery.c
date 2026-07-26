#include "storage_atomic_recovery.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"

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
     * interrupted writes racing: conflicting, quarantine the evidence. */
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
     * when it is valid, otherwise there is nothing safe to activate -- quarantine
     * the corrupt backup rather than resurrect bad state. */
    if (state->backup_count == 1U) {
        return state->backup_valid ? STORAGE_ATOMIC_RECONCILE_RESTORE_BACKUP
                                   : STORAGE_ATOMIC_RECONCILE_QUARANTINE;
    }
    /* Only a temporary remains. A malformed temporary is quarantined; a valid one
     * is activated only when the owning transaction proves roll-forward, and
     * otherwise discarded (the interrupted write is rolled back). */
    if (state->temporary_count == 1U) {
        if (!state->temporary_valid) {
            return STORAGE_ATOMIC_RECONCILE_QUARANTINE;
        }
        return state->roll_forward_proven ? STORAGE_ATOMIC_RECONCILE_ACTIVATE_TEMPORARY
                                          : STORAGE_ATOMIC_RECONCILE_DISCARD_TEMPORARY;
    }
    return STORAGE_ATOMIC_RECONCILE_NOTHING;
}
