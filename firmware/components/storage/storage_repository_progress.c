#include "storage_repository.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_procedures_internal.h"
#include "storage_repository_progress_internal.h"

static bool identity_valid(const storage_procedure_identity_t *identity) {
    return identity != NULL && app_uuid_is_valid_string(identity->set_id.value) &&
           app_uuid_is_valid_string(identity->procedure_id.value);
}

static bool progress_matches_identity(const procedure_progress_t *progress,
                                      const storage_procedure_identity_t *identity) {
    return progress != NULL && identity_valid(identity) &&
           app_uuid_equal(&progress->set_id, &identity->set_id) &&
           app_uuid_equal(&progress->procedure_id, &identity->procedure_id);
}

static bool procedure_contains_step(const procedure_t *procedure, const app_uuid_t *step_id) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        if (app_uuid_equal(&procedure->steps[index].id, step_id)) {
            return true;
        }
    }
    return false;
}

static bool progress_steps_belong_to_procedure(const procedure_progress_t *progress,
                                               const procedure_t *procedure) {
    if (!procedure_contains_step(procedure, &progress->current_step_id)) {
        return false;
    }
    for (size_t index = 0U; index < progress->completed_step_count; ++index) {
        if (!procedure_contains_step(procedure, &progress->completed_step_ids[index])) {
            return false;
        }
    }
    for (size_t index = 0U; index < progress->skipped_step_count; ++index) {
        if (!procedure_contains_step(procedure, &progress->skipped_step_ids[index])) {
            return false;
        }
    }
    return true;
}

static app_error_code_t progress_file_path(const storage_procedure_identity_t *identity, char *path,
                                           size_t path_size) {
    if (!identity_valid(identity)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_make_progress_path(&identity->set_id, &identity->procedure_id, path, path_size);
}

/* The specific reason is not carried out of here: the caller learns only that
 * the object was unreadable and was deleted (SPEC 13.6). */
static app_error_code_t discard_progress(const char *path) {
    const app_error_code_t discard = storage_repository_discard_corrupt_file(path);
    return discard == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : discard;
}

static app_error_code_t read_progress_object_locked(const storage_procedure_identity_t *identity,
                                                    procedure_progress_t *out_progress, char *path,
                                                    size_t path_size) {
    if (!identity_valid(identity) || out_progress == NULL || path == NULL || path_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_progress, 0, sizeof(*out_progress));
    app_error_code_t result = progress_file_path(identity, path, path_size);
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_read_bounded_file(path, STORAGE_PROGRESS_FILE_MAX_BYTES, &data,
                                                      &length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_progress_json(data, length, out_progress);
    }
    free(data);
    if (result == APP_ERROR_NONE && !progress_matches_identity(out_progress, identity)) {
        memset(out_progress, 0, sizeof(*out_progress));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        memset(out_progress, 0, sizeof(*out_progress));
        return discard_progress(path);
    }
    return result;
}

/* The progress buffer is passed in rather than declared here: procedure_progress_t
 * is ~16 KB and this function has several early returns, so a thin wrapper owns
 * the single allocation and the single free. That keeps the control flow below
 * untouched, which is what makes the cleanup auditable. */
static app_error_code_t
storage_progress_read_locked_with_buffer(const storage_procedure_identity_t *identity,
                                         storage_progress_snapshot_t *out_snapshot,
                                         procedure_progress_t *progress_buffer) {
    if (out_snapshot != NULL) {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
    }
    if (!identity_valid(identity) || out_snapshot == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char path[APP_PATH_MAX_BYTES];
    procedure_progress_t *const progress = progress_buffer;
    app_error_code_t result = read_progress_object_locked(identity, progress, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }

    procedure_t procedure = {0};
    result = storage_procedure_read_locked(identity, &procedure);
    if (result == APP_ERROR_NOT_FOUND) {
        return discard_progress(path);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }

    out_snapshot->current_procedure_revision = procedure.revision;
    if (progress->procedure_revision != procedure.revision) {
        out_snapshot->status = STORAGE_PROGRESS_STATUS_STALE;
        out_snapshot->progress = *progress;
        macro_model_free_procedure(&procedure);
        return APP_ERROR_NONE;
    }
    if (!progress_steps_belong_to_procedure(progress, &procedure)) {
        macro_model_free_procedure(&procedure);
        return discard_progress(path);
    }

    out_snapshot->status = STORAGE_PROGRESS_STATUS_CURRENT;
    out_snapshot->progress = *progress;
    macro_model_free_procedure(&procedure);
    return APP_ERROR_NONE;
}

app_error_code_t storage_progress_read_locked(const storage_procedure_identity_t *identity,
                                              storage_progress_snapshot_t *out_snapshot) {
    procedure_progress_t *progress = calloc(1U, sizeof(*progress));
    if (progress == NULL) {
        if (out_snapshot != NULL) {
            memset(out_snapshot, 0, sizeof(*out_snapshot));
        }
        return APP_ERROR_INTERNAL;
    }
    const app_error_code_t result =
        storage_progress_read_locked_with_buffer(identity, out_snapshot, progress);
    free(progress);
    return result;
}

static app_error_code_t write_progress_locked(const storage_procedure_identity_t *identity,
                                              const procedure_progress_t *replacement,
                                              storage_progress_snapshot_t *out_snapshot) {
    /* The caller is allowed to alias `replacement` into `out_snapshot` - see
     * test_storage_progress.c, which passes &snapshot.progress alongside
     * &snapshot - so the input MUST be copied before out_snapshot is cleared.
     * The copy is heap-allocated rather than a stack local because
     * procedure_progress_t is ~16 KB (two app_uuid_t[APP_STEPS_PER_PROCEDURE_MAX]
     * arrays), which does not fit comfortably on a FreeRTOS task stack. */
    procedure_progress_t *candidate = calloc(1U, sizeof(*candidate));
    if (candidate == NULL) {
        if (out_snapshot != NULL) {
            memset(out_snapshot, 0, sizeof(*out_snapshot));
        }
        return APP_ERROR_INTERNAL;
    }
    if (replacement != NULL) {
        *candidate = *replacement;
    }
    if (out_snapshot != NULL) {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
    }
    if (!identity_valid(identity) || !progress_matches_identity(candidate, identity) ||
        out_snapshot == NULL) {
        free(candidate);
        return APP_ERROR_INVALID_ARGUMENT;
    }

    procedure_t procedure = {0};
    app_error_code_t result = storage_procedure_read_locked(identity, &procedure);
    if (result != APP_ERROR_NONE) {
        free(candidate);
        return result;
    }
    if (candidate->procedure_revision != procedure.revision) {
        macro_model_free_procedure(&procedure);
        free(candidate);
        return APP_ERROR_CONFLICT;
    }
    if (!progress_steps_belong_to_procedure(candidate, &procedure)) {
        macro_model_free_procedure(&procedure);
        free(candidate);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_model_free_procedure(&procedure);

    char *json = NULL;
    size_t length = 0U;
    result = storage_repository_serialize_progress_json(candidate, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = progress_file_path(identity, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, length, true);
    }
    cJSON_free(json);
    free(candidate);
    if (result == APP_ERROR_NONE) {
        result = storage_progress_read_locked(identity, out_snapshot);
    }
    return result;
}

static app_error_code_t progress_reset_locked(const storage_procedure_identity_t *identity,
                                              uint32_t expected_procedure_revision,
                                              storage_progress_snapshot_t *out_snapshot) {
    if (out_snapshot != NULL) {
        memset(out_snapshot, 0, sizeof(*out_snapshot));
    }
    if (!identity_valid(identity) || expected_procedure_revision == 0U || out_snapshot == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    procedure_t procedure = {0};
    app_error_code_t result = storage_procedure_read_locked(identity, &procedure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (procedure.revision != expected_procedure_revision) {
        macro_model_free_procedure(&procedure);
        return APP_ERROR_CONFLICT;
    }
    /* Heap-allocated: procedure_progress_t is ~16 KB and this is the last
     * thing between here and write_progress_locked, which takes it by
     * pointer anyway. */
    procedure_progress_t *reset = calloc(1U, sizeof(*reset));
    if (reset == NULL) {
        macro_model_free_procedure(&procedure);
        return APP_ERROR_INTERNAL;
    }
    reset->schema_version = APP_SCHEMA_VERSION;
    reset->set_id = identity->set_id;
    reset->procedure_id = identity->procedure_id;
    reset->procedure_revision = procedure.revision;
    reset->current_step_id = procedure.steps[0].id;
    macro_model_free_procedure(&procedure);
    const app_error_code_t result_write = write_progress_locked(identity, reset, out_snapshot);
    free(reset);
    return result_write;
}

app_error_code_t storage_progress_read(const storage_procedure_identity_t *identity,
                                       storage_progress_snapshot_t *out_snapshot) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_progress_read_locked(identity, out_snapshot);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_progress_update(const storage_procedure_identity_t *identity,
                                         const procedure_progress_t *replacement,
                                         storage_progress_snapshot_t *out_snapshot) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = write_progress_locked(identity, replacement, out_snapshot);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_progress_reset(const storage_procedure_identity_t *identity,
                                        uint32_t expected_procedure_revision,
                                        storage_progress_snapshot_t *out_snapshot) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        progress_reset_locked(identity, expected_procedure_revision, out_snapshot);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
