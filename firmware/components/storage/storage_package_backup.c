#include "storage_package.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_object_json.h"
#include "storage_package_internal.h"
#include "storage_repository.h"

#ifdef ESP_PLATFORM
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"
#include "storage_repository_procedures_internal.h"
#include "storage_repository_progress_internal.h"
#include "storage_repository_sets_internal.h"
#endif

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} backup_writer_t;

typedef struct {
    macro_set_t set;
    storage_macro_list_t local_macros;
    storage_procedure_list_t procedures;
    storage_progress_snapshot_t *progress;
    bool *progress_present;
    /* A procedure whose steps reference a macro that was skipped cannot go in
     * the package: the package validator requires every referenced macro to be
     * present, so such a procedure would make the whole backup invalid. Marked
     * excluded rather than removed, so the lists stay owned by their reader. */
    bool *procedure_included;
} backup_set_snapshot_t;

typedef struct {
    backup_set_snapshot_t *sets;
    size_t set_count;
    storage_macro_list_t global_macros;
} backup_snapshot_t;

#ifndef ESP_PLATFORM
static storage_package_backup_ops_t backup_operations;

void storage_package_set_backup_ops_for_test(const storage_package_backup_ops_t *operations) {
    memset(&backup_operations, 0, sizeof(backup_operations));
    if (operations != NULL) {
        backup_operations = *operations;
    }
}

void storage_package_reset_backup_ops_for_test(void) {
    memset(&backup_operations, 0, sizeof(backup_operations));
}
#else
static app_error_code_t production_lock_take(void *context) {
    (void)context;
    return storage_repository_lock_take();
}

static app_error_code_t production_lock_give(void *context) {
    (void)context;
    return storage_repository_lock_give();
}

static app_error_code_t production_set_list(void *context, storage_set_list_t *out_list,
                                            storage_object_ref_t *out_failed,
                                            storage_skip_record_t *out_skips) {
    (void)context;
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    memset(out_list, 0, sizeof(*out_list));
    for (size_t item = 0U; result == APP_ERROR_NONE && item < index.count; ++item) {
        result = storage_set_read_locked(&index.ids[item], &out_list->items[out_list->count]);
        if (result == APP_ERROR_NONE) {
            ++out_list->count;
            continue;
        }
        if (out_skips != NULL && app_error_is_object_fault(result)) {
            memset(&out_list->items[out_list->count], 0, sizeof(out_list->items[0]));
            ++out_skips->total;
            if (out_skips->items != NULL && out_skips->count < out_skips->capacity) {
                out_skips->items[out_skips->count].has_id = true;
                out_skips->items[out_skips->count].id = index.ids[item];
                ++out_skips->count;
            }
            result = APP_ERROR_NONE;
            continue;
        }
        if (out_failed != NULL) {
            out_failed->has_id = true;
            out_failed->id = index.ids[item];
        }
    }
    return result;
}

static app_error_code_t production_macro_list(void *context,
                                              const storage_macro_location_t *location,
                                              storage_macro_list_t *out_list,
                                              storage_object_ref_t *out_failed,
                                              storage_skip_record_t *out_skips) {
    (void)context;
    return storage_macro_list_detail_locked(location, out_list, out_failed, out_skips);
}

static void production_macro_list_free(void *context, storage_macro_list_t *list) {
    (void)context;
    storage_macro_list_free(list);
}

static app_error_code_t production_procedure_list(void *context, const app_uuid_t *set_id,
                                                  storage_procedure_list_t *out_list,
                                                  storage_object_ref_t *out_failed,
                                                  storage_skip_record_t *out_skips) {
    (void)context;
    return storage_procedure_list_detail_locked(set_id, out_list, out_failed, out_skips);
}

static void production_procedure_list_free(void *context, storage_procedure_list_t *list) {
    (void)context;
    storage_procedure_list_free(list);
}

static app_error_code_t production_progress_read(void *context,
                                                 const storage_procedure_identity_t *identity,
                                                 storage_progress_snapshot_t *out_snapshot) {
    (void)context;
    return storage_progress_read_locked(identity, out_snapshot);
}

static storage_package_backup_ops_t backup_operations = {
    .context = NULL,
    .lock_take = production_lock_take,
    .lock_give = production_lock_give,
    .set_list = production_set_list,
    .macro_list = production_macro_list,
    .macro_list_free = production_macro_list_free,
    .procedure_list = production_procedure_list,
    .procedure_list_free = production_procedure_list_free,
    .progress_read = production_progress_read,
};
#endif

static bool backup_operations_valid(void) {
    return backup_operations.lock_take != NULL && backup_operations.lock_give != NULL &&
           backup_operations.set_list != NULL && backup_operations.macro_list != NULL &&
           backup_operations.macro_list_free != NULL && backup_operations.procedure_list != NULL &&
           backup_operations.procedure_list_free != NULL && backup_operations.progress_read != NULL;
}

static app_error_code_t writer_reserve(backup_writer_t *writer, size_t additional) {
    if (writer == NULL || additional > APP_IMPORT_PACKAGE_MAX_BYTES - writer->length) {
        return APP_ERROR_MACRO_LIMIT;
    }
    const size_t required = writer->length + additional + 1U;
    if (required <= writer->capacity) {
        return APP_ERROR_NONE;
    }
    const size_t maximum = APP_IMPORT_PACKAGE_MAX_BYTES + 1U;
    size_t capacity = writer->capacity == 0U ? 1024U : writer->capacity;
    while (capacity < required) {
        if (capacity > maximum / 2U) {
            capacity = maximum;
            break;
        }
        capacity *= 2U;
    }
    if (capacity < required || capacity > maximum) {
        return APP_ERROR_MACRO_LIMIT;
    }
    char *replacement = realloc(writer->data, capacity);
    if (replacement == NULL) {
        return APP_ERROR_INTERNAL;
    }
    writer->data = replacement;
    writer->capacity = capacity;
    return APP_ERROR_NONE;
}

static app_error_code_t writer_append_bytes(backup_writer_t *writer, const char *data,
                                            size_t length) {
    if (writer == NULL || (data == NULL && length != 0U)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = writer_reserve(writer, length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (length != 0U) {
        memcpy(writer->data + writer->length, data, length);
        writer->length += length;
    }
    writer->data[writer->length] = '\0';
    return APP_ERROR_NONE;
}

static app_error_code_t writer_append_text(backup_writer_t *writer, const char *text) {
    return text == NULL ? APP_ERROR_INVALID_ARGUMENT
                        : writer_append_bytes(writer, text, strlen(text));
}

static app_error_code_t writer_append_serialized(backup_writer_t *writer,
                                                 app_error_code_t serialization_result, char *json,
                                                 size_t length) {
    app_error_code_t result = serialization_result;
    if (result == APP_ERROR_NONE) {
        result = writer_append_bytes(writer, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t writer_append_set(backup_writer_t *writer, const macro_set_t *set) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result = storage_repository_serialize_set_json(set, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static app_error_code_t writer_append_macro(backup_writer_t *writer, const macro_t *macro) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result = storage_repository_serialize_macro_json(macro, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static app_error_code_t writer_append_procedure(backup_writer_t *writer,
                                                const procedure_t *procedure) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result =
        storage_repository_serialize_procedure_json(procedure, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static app_error_code_t writer_append_progress(backup_writer_t *writer,
                                               const procedure_progress_t *progress) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result =
        storage_repository_serialize_progress_json(progress, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static void set_snapshot_free(backup_set_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    backup_operations.macro_list_free(backup_operations.context, &snapshot->local_macros);
    backup_operations.procedure_list_free(backup_operations.context, &snapshot->procedures);
    free(snapshot->progress);
    free(snapshot->progress_present);
    free(snapshot->procedure_included);
    memset(snapshot, 0, sizeof(*snapshot));
}

static void backup_snapshot_free(backup_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    for (size_t index = 0U; index < snapshot->set_count; ++index) {
        set_snapshot_free(&snapshot->sets[index]);
    }
    backup_operations.macro_list_free(backup_operations.context, &snapshot->global_macros);
    free(snapshot->sets);
    memset(snapshot, 0, sizeof(*snapshot));
}

/* Enough to fold one list read's skips into the report without a heap round
 * trip; the report's own total still counts everything beyond it. */
#define BACKUP_SKIP_SCRATCH 8U

static void fold_skips(storage_package_skip_report_t *report, storage_package_object_kind_t kind,
                       bool global_scope, const app_uuid_t *set_id,
                       const storage_skip_record_t *skips) {
    if (report == NULL || skips == NULL || skips->total == 0U) {
        return;
    }
    report->total += skips->total;
    for (size_t index = 0U; index < skips->count; ++index) {
        if (report->count >= STORAGE_PACKAGE_SKIP_REPORT_MAX) {
            return;
        }
        storage_package_skipped_object_t *entry = &report->items[report->count];
        memset(entry, 0, sizeof(*entry));
        entry->kind = kind;
        entry->global_scope = global_scope;
        if (set_id != NULL) {
            entry->has_set_id = true;
            entry->set_id = *set_id;
        }
        entry->object_id = skips->items[index].id;
        ++report->count;
    }
}

static void note_skipped_object(storage_package_skip_report_t *report,
                                storage_package_object_kind_t kind, const app_uuid_t *set_id,
                                const storage_object_ref_t *object) {
    if (report == NULL) {
        return;
    }
    ++report->total;
    if (report->count >= STORAGE_PACKAGE_SKIP_REPORT_MAX) {
        return;
    }
    storage_package_skipped_object_t *entry = &report->items[report->count];
    memset(entry, 0, sizeof(*entry));
    entry->kind = kind;
    if (set_id != NULL) {
        entry->has_set_id = true;
        entry->set_id = *set_id;
    }
    if (object != NULL && object->has_id) {
        entry->object_id = object->id;
    }
    ++report->count;
}

static void record_failure(storage_package_failure_t *out_failure,
                           storage_package_object_kind_t kind, bool global_scope,
                           const app_uuid_t *set_id, const storage_object_ref_t *object) {
    if (out_failure == NULL || out_failure->kind != STORAGE_PACKAGE_OBJECT_NONE) {
        return; /* keep the first failure: it is the one that stopped the export */
    }
    out_failure->kind = kind;
    out_failure->global_scope = global_scope;
    if (set_id != NULL) {
        out_failure->has_set_id = true;
        out_failure->set_id = *set_id;
    }
    if (object != NULL && object->has_id) {
        out_failure->has_object_id = true;
        out_failure->object_id = object->id;
    }
}

static app_error_code_t load_set_progress(backup_set_snapshot_t *snapshot,
                                          storage_package_failure_t *out_failure) {
    if (snapshot->procedures.count == 0U) {
        return APP_ERROR_NONE;
    }
    snapshot->progress = calloc(snapshot->procedures.count, sizeof(*snapshot->progress));
    snapshot->progress_present =
        calloc(snapshot->procedures.count, sizeof(*snapshot->progress_present));
    if (snapshot->progress == NULL || snapshot->progress_present == NULL) {
        return APP_ERROR_INTERNAL;
    }
    /* storage_progress_snapshot_t is ~16 KB (two app_uuid_t
     * [APP_STEPS_PER_PROCEDURE_MAX] arrays). Declaring it inside the loop put
     * this frame at ~15 KB. One reused heap buffer, cleared per iteration,
     * with a single free on a single exit. */
    storage_progress_snapshot_t *progress = calloc(1U, sizeof(*progress));
    if (progress == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t outcome = APP_ERROR_NONE;
    for (size_t index = 0U; index < snapshot->procedures.count; ++index) {
        const storage_procedure_identity_t identity = {
            .set_id = snapshot->set.id,
            .procedure_id = snapshot->procedures.items[index].id,
        };
        memset(progress, 0, sizeof(*progress));
        const app_error_code_t result =
            backup_operations.progress_read(backup_operations.context, &identity, progress);
        if (result == APP_ERROR_NOT_FOUND) {
            continue;
        }
        if (result != APP_ERROR_NONE) {
            const storage_object_ref_t failed = {.has_id = true, .id = identity.procedure_id};
            record_failure(out_failure, STORAGE_PACKAGE_OBJECT_PROGRESS, false, &identity.set_id,
                           &failed);
            outcome = result;
            break;
        }
        if (progress->status == STORAGE_PROGRESS_STATUS_CURRENT) {
            snapshot->progress[index] = *progress;
            snapshot->progress_present[index] = true;
        }
    }
    free(progress);
    return outcome;
}

static app_error_code_t load_set_snapshot(const macro_set_t *set, bool include_progress,
                                          backup_set_snapshot_t *out_snapshot,
                                          storage_package_failure_t *out_failure,
                                          storage_package_skip_report_t *out_skipped) {
    out_snapshot->set = *set;
    const storage_macro_location_t location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = set->id,
    };
    storage_object_ref_t failed = {0};
    storage_object_ref_t scratch[BACKUP_SKIP_SCRATCH];
    storage_skip_record_t skips = {
        .items = scratch,
        .capacity = BACKUP_SKIP_SCRATCH,
        .count = 0U,
        .total = 0U,
    };
    app_error_code_t result = backup_operations.macro_list(
        backup_operations.context, &location, &out_snapshot->local_macros, &failed, &skips);
    if (result != APP_ERROR_NONE) {
        record_failure(out_failure, STORAGE_PACKAGE_OBJECT_MACRO, false, &set->id, &failed);
        return result;
    }
    fold_skips(out_skipped, STORAGE_PACKAGE_OBJECT_MACRO, false, &set->id, &skips);

    skips.count = 0U;
    skips.total = 0U;
    result = backup_operations.procedure_list(backup_operations.context, &set->id,
                                              &out_snapshot->procedures, &failed, &skips);
    if (result != APP_ERROR_NONE) {
        record_failure(out_failure, STORAGE_PACKAGE_OBJECT_PROCEDURE, false, &set->id, &failed);
        return result;
    }
    fold_skips(out_skipped, STORAGE_PACKAGE_OBJECT_PROCEDURE, false, &set->id, &skips);

    if (out_snapshot->procedures.count != 0U) {
        out_snapshot->procedure_included =
            calloc(out_snapshot->procedures.count, sizeof(*out_snapshot->procedure_included));
        if (out_snapshot->procedure_included == NULL) {
            return APP_ERROR_INTERNAL;
        }
        for (size_t index = 0U; index < out_snapshot->procedures.count; ++index) {
            out_snapshot->procedure_included[index] = true;
        }
    }
    if (include_progress) {
        result = load_set_progress(out_snapshot, out_failure);
    }
    return result;
}

static app_error_code_t snapshot_load_locked(bool include_progress, backup_snapshot_t *out_snapshot,
                                             storage_package_failure_t *out_failure,
                                             storage_package_skip_report_t *out_skipped) {
    /* storage_set_list_t inlines macro_set_t[APP_MACRO_SETS_MAX] and so is
     * ~29 KB. As a stack local it put this frame at ~42 KB, far past the httpd
     * task stack that serves GET /api/v1/backup, panicking the device. One
     * allocation, one free, single exit. */
    storage_set_list_t *set_list = calloc(1U, sizeof(*set_list));
    if (set_list == NULL) {
        return APP_ERROR_INTERNAL;
    }
    storage_object_ref_t set_failed = {0};
    storage_object_ref_t set_scratch[BACKUP_SKIP_SCRATCH];
    storage_skip_record_t set_skips = {
        .items = set_scratch,
        .capacity = BACKUP_SKIP_SCRATCH,
        .count = 0U,
        .total = 0U,
    };
    app_error_code_t result =
        backup_operations.set_list(backup_operations.context, set_list, &set_failed, &set_skips);
    if (result != APP_ERROR_NONE) {
        record_failure(out_failure, STORAGE_PACKAGE_OBJECT_SET, false, NULL, &set_failed);
    }
    fold_skips(out_skipped, STORAGE_PACKAGE_OBJECT_SET, false, NULL, &set_skips);
    if (result == APP_ERROR_NONE && set_list->count > APP_MACRO_SETS_MAX) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE && set_list->count != 0U) {
        out_snapshot->sets = calloc(set_list->count, sizeof(*out_snapshot->sets));
        if (out_snapshot->sets == NULL) {
            result = APP_ERROR_INTERNAL;
        } else {
            out_snapshot->set_count = set_list->count;
        }
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < set_list->count; ++index) {
        result = load_set_snapshot(&set_list->items[index], include_progress,
                                   &out_snapshot->sets[index], out_failure, out_skipped);
    }
    const storage_macro_location_t global_location = {
        .scope = MACRO_SCOPE_GLOBAL,
        .has_set_id = false,
        .set_id = {{0}},
    };
    if (result == APP_ERROR_NONE) {
        storage_object_ref_t failed = {0};
        storage_object_ref_t scratch[BACKUP_SKIP_SCRATCH];
        storage_skip_record_t skips = {
            .items = scratch,
            .capacity = BACKUP_SKIP_SCRATCH,
            .count = 0U,
            .total = 0U,
        };
        result = backup_operations.macro_list(backup_operations.context, &global_location,
                                              &out_snapshot->global_macros, &failed, &skips);
        if (result != APP_ERROR_NONE) {
            record_failure(out_failure, STORAGE_PACKAGE_OBJECT_MACRO, true, NULL, &failed);
        }
        fold_skips(out_skipped, STORAGE_PACKAGE_OBJECT_MACRO, true, NULL, &skips);
    }
    free(set_list);
    return result;
}

static app_error_code_t snapshot_load(bool include_progress, backup_snapshot_t *out_snapshot,
                                      storage_package_failure_t *out_failure,
                                      storage_package_skip_report_t *out_skipped) {
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    const app_error_code_t lock = backup_operations.lock_take(backup_operations.context);
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    app_error_code_t result =
        snapshot_load_locked(include_progress, out_snapshot, out_failure, out_skipped);
    const app_error_code_t unlock = backup_operations.lock_give(backup_operations.context);
    if (result == APP_ERROR_NONE && unlock != APP_ERROR_NONE) {
        result = APP_ERROR_INTERNAL;
    }
    return result;
}

static size_t macro_index(const storage_macro_list_t *list, const app_uuid_t *uuid) {
    for (size_t index = 0U; index < list->count; ++index) {
        if (app_uuid_equal(&list->items[index].id, uuid)) {
            return index;
        }
    }
    return SIZE_MAX;
}

static bool set_ids_unique(const backup_snapshot_t *snapshot) {
    for (size_t index = 0U; index < snapshot->set_count; ++index) {
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&snapshot->sets[index].set.id, &snapshot->sets[prior].set.id)) {
                return false;
            }
        }
    }
    return true;
}

static app_error_code_t validate_set_snapshot(const backup_snapshot_t *backup,
                                              const backup_set_snapshot_t *snapshot) {
    if (snapshot->local_macros.count > APP_MACROS_PER_SET_MAX ||
        snapshot->procedures.count > APP_PROCEDURES_PER_SET_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < snapshot->local_macros.count; ++index) {
        const macro_t *macro = &snapshot->local_macros.items[index];
        if (macro->scope != MACRO_SCOPE_SET || !macro->has_set_id ||
            !app_uuid_equal(&macro->set_id, &snapshot->set.id)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    for (size_t index = 0U; index < snapshot->procedures.count; ++index) {
        if (snapshot->procedure_included != NULL && !snapshot->procedure_included[index]) {
            continue;
        }
        const procedure_t *procedure = &snapshot->procedures.items[index];
        if (!app_uuid_equal(&procedure->set_id, &snapshot->set.id)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        for (size_t step = 0U; step < procedure->step_count; ++step) {
            if (!procedure->steps[step].has_macro_id) {
                continue;
            }
            const bool local =
                macro_index(&snapshot->local_macros, &procedure->steps[step].macro_id) != SIZE_MAX;
            const bool global =
                macro_index(&backup->global_macros, &procedure->steps[step].macro_id) != SIZE_MAX;
            if (local == global) {
                return APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    return APP_ERROR_NONE;
}

/* A skipped macro orphans every procedure step that referenced it. The package
 * validator requires each referenced macro to be present, so those procedures
 * have to leave the package too - otherwise one skipped macro would make the
 * whole backup fail validation, which is exactly what skipping is meant to
 * avoid. */
static void prune_dangling_procedures(backup_snapshot_t *snapshot,
                                      storage_package_skip_report_t *out_skipped) {
    for (size_t set_index = 0U; set_index < snapshot->set_count; ++set_index) {
        backup_set_snapshot_t *set = &snapshot->sets[set_index];
        if (set->procedure_included == NULL) {
            continue;
        }
        for (size_t index = 0U; index < set->procedures.count; ++index) {
            if (!set->procedure_included[index]) {
                continue;
            }
            const procedure_t *procedure = &set->procedures.items[index];
            bool intact = true;
            for (size_t step = 0U; intact && step < procedure->step_count; ++step) {
                if (!procedure->steps[step].has_macro_id) {
                    continue;
                }
                const bool local =
                    macro_index(&set->local_macros, &procedure->steps[step].macro_id) != SIZE_MAX;
                const bool global = macro_index(&snapshot->global_macros,
                                                &procedure->steps[step].macro_id) != SIZE_MAX;
                intact = local != global;
            }
            if (!intact) {
                set->procedure_included[index] = false;
                if (set->progress_present != NULL) {
                    set->progress_present[index] = false;
                }
                const storage_object_ref_t orphan = {.has_id = true, .id = procedure->id};
                note_skipped_object(out_skipped, STORAGE_PACKAGE_OBJECT_PROCEDURE, &set->set.id,
                                    &orphan);
            }
        }
    }
}

static app_error_code_t validate_snapshot(const backup_snapshot_t *snapshot,
                                          storage_package_failure_t *out_failure) {
    if (snapshot->set_count > APP_MACRO_SETS_MAX ||
        snapshot->global_macros.count > APP_MACROS_PER_SET_MAX || !set_ids_unique(snapshot)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < snapshot->global_macros.count; ++index) {
        const macro_t *macro = &snapshot->global_macros.items[index];
        if (macro->scope != MACRO_SCOPE_GLOBAL || macro->has_set_id) {
            const storage_object_ref_t failed = {.has_id = true, .id = macro->id};
            record_failure(out_failure, STORAGE_PACKAGE_OBJECT_MACRO, true, NULL, &failed);
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    for (size_t index = 0U; index < snapshot->set_count; ++index) {
        const app_error_code_t result = validate_set_snapshot(snapshot, &snapshot->sets[index]);
        if (result != APP_ERROR_NONE) {
            record_failure(out_failure, STORAGE_PACKAGE_OBJECT_SET, false,
                           &snapshot->sets[index].set.id, NULL);
            return result;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t append_separator(backup_writer_t *writer, bool *in_out_first) {
    if (*in_out_first) {
        *in_out_first = false;
        return APP_ERROR_NONE;
    }
    return writer_append_text(writer, ",");
}

static app_error_code_t append_sets(backup_writer_t *writer, const backup_snapshot_t *snapshot) {
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t index = 0U; result == APP_ERROR_NONE && index < snapshot->set_count; ++index) {
        if (index != 0U) {
            result = writer_append_text(writer, ",");
        }
        if (result == APP_ERROR_NONE) {
            result = writer_append_set(writer, &snapshot->sets[index].set);
        }
    }
    return result == APP_ERROR_NONE ? writer_append_text(writer, "]") : result;
}

static app_error_code_t append_local_macros(backup_writer_t *writer,
                                            const backup_snapshot_t *snapshot, size_t *out_count) {
    *out_count = 0U;
    bool first = true;
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t set_index = 0U; result == APP_ERROR_NONE && set_index < snapshot->set_count;
         ++set_index) {
        const storage_macro_list_t *list = &snapshot->sets[set_index].local_macros;
        for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
            result = append_separator(writer, &first);
            if (result == APP_ERROR_NONE) {
                result = writer_append_macro(writer, &list->items[index]);
            }
            if (result == APP_ERROR_NONE) {
                ++*out_count;
            }
        }
    }
    return result == APP_ERROR_NONE ? writer_append_text(writer, "]") : result;
}

static app_error_code_t append_global_macros(backup_writer_t *writer,
                                             const storage_macro_list_t *list) {
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
        if (index != 0U) {
            result = writer_append_text(writer, ",");
        }
        if (result == APP_ERROR_NONE) {
            result = writer_append_macro(writer, &list->items[index]);
        }
    }
    return result == APP_ERROR_NONE ? writer_append_text(writer, "]") : result;
}

static app_error_code_t append_procedures(backup_writer_t *writer,
                                          const backup_snapshot_t *snapshot, size_t *out_count) {
    *out_count = 0U;
    bool first = true;
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t set_index = 0U; result == APP_ERROR_NONE && set_index < snapshot->set_count;
         ++set_index) {
        const backup_set_snapshot_t *set = &snapshot->sets[set_index];
        const storage_procedure_list_t *list = &set->procedures;
        for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
            if (set->procedure_included != NULL && !set->procedure_included[index]) {
                continue;
            }
            result = append_separator(writer, &first);
            if (result == APP_ERROR_NONE) {
                result = writer_append_procedure(writer, &list->items[index]);
            }
            if (result == APP_ERROR_NONE) {
                ++*out_count;
            }
        }
    }
    return result == APP_ERROR_NONE ? writer_append_text(writer, "]") : result;
}

static app_error_code_t append_progress(backup_writer_t *writer, const backup_snapshot_t *snapshot,
                                        size_t *out_count) {
    *out_count = 0U;
    bool first = true;
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t set_index = 0U; result == APP_ERROR_NONE && set_index < snapshot->set_count;
         ++set_index) {
        const backup_set_snapshot_t *set = &snapshot->sets[set_index];
        for (size_t index = 0U; result == APP_ERROR_NONE && index < set->procedures.count;
             ++index) {
            if (set->progress_present == NULL || !set->progress_present[index] ||
                (set->procedure_included != NULL && !set->procedure_included[index])) {
                continue;
            }
            result = append_separator(writer, &first);
            if (result == APP_ERROR_NONE) {
                result = writer_append_progress(writer, &set->progress[index].progress);
            }
            if (result == APP_ERROR_NONE) {
                ++*out_count;
            }
        }
    }
    return result == APP_ERROR_NONE ? writer_append_text(writer, "]") : result;
}

/* Two UUIDs, two keys, and the JSON punctuation around them. */
#define BACKUP_SKIPPED_ITEM_BYTES 192U
#define BACKUP_SKIPPED_HEAD_BYTES 64U

static const char *skipped_kind_text(storage_package_object_kind_t kind) {
    switch (kind) {
    case STORAGE_PACKAGE_OBJECT_SET:
        return "set";
    case STORAGE_PACKAGE_OBJECT_MACRO:
        return "macro";
    case STORAGE_PACKAGE_OBJECT_PROCEDURE:
        return "procedure";
    case STORAGE_PACKAGE_OBJECT_PROGRESS:
        return "progress";
    case STORAGE_PACKAGE_OBJECT_NONE:
    default:
        return "object";
    }
}

/* Written only when something was actually skipped, so a complete backup is
 * byte-identical to one produced before partial backups existed. A partial
 * backup must never be mistaken for a complete one, so the record travels
 * inside the package rather than alongside it. */
static app_error_code_t append_skipped(backup_writer_t *writer,
                                       const storage_package_skip_report_t *skipped) {
    if (skipped == NULL || skipped->total == 0U) {
        return APP_ERROR_NONE;
    }
    char head[BACKUP_SKIPPED_HEAD_BYTES];
    const int head_written = snprintf(head, sizeof(head), ",\"skipped\":{\"total\":%lu,\"items\":[",
                                      (unsigned long)skipped->total);
    if (head_written < 0 || (size_t)head_written >= sizeof(head)) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = writer_append_text(writer, head);
    for (size_t index = 0U; result == APP_ERROR_NONE && index < skipped->count; ++index) {
        const storage_package_skipped_object_t *entry = &skipped->items[index];
        char item[BACKUP_SKIPPED_ITEM_BYTES];
        int written = 0;
        if (entry->has_set_id) {
            written =
                snprintf(item, sizeof(item), "%s{\"kind\":\"%s\",\"id\":\"%s\",\"set_id\":\"%s\"}",
                         index == 0U ? "" : ",", skipped_kind_text(entry->kind),
                         entry->object_id.value, entry->set_id.value);
        } else {
            written =
                snprintf(item, sizeof(item), "%s{\"kind\":\"%s\",\"id\":\"%s\",\"scope\":\"%s\"}",
                         index == 0U ? "" : ",", skipped_kind_text(entry->kind),
                         entry->object_id.value, entry->global_scope ? "global" : "device");
        }
        if (written < 0 || (size_t)written >= sizeof(item)) {
            return APP_ERROR_INTERNAL;
        }
        result = writer_append_text(writer, item);
    }
    return result == APP_ERROR_NONE ? writer_append_text(writer, "]}") : result;
}

static app_error_code_t serialize_snapshot(const backup_snapshot_t *snapshot, char **out_data,
                                           size_t *out_length,
                                           const storage_package_skip_report_t *skipped) {
    backup_writer_t writer = {0};
    app_error_code_t result =
        writer_append_text(&writer, "{\"schema_version\":1,\"package_type\":\"backup\",\"sets\":");
    if (result == APP_ERROR_NONE) {
        result = append_sets(&writer, snapshot);
    }
    size_t local_count = 0U;
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, ",\"macros\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_local_macros(&writer, snapshot, &local_count);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, ",\"global_macros\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_global_macros(&writer, &snapshot->global_macros);
    }
    size_t procedure_count = 0U;
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, ",\"procedures\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_procedures(&writer, snapshot, &procedure_count);
    }
    size_t progress_count = 0U;
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, ",\"progress\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_progress(&writer, snapshot, &progress_count);
    }
    if (result == APP_ERROR_NONE) {
        result = append_skipped(&writer, skipped);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, "}");
    }

    storage_package_summary_t summary = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_validate(writer.data, writer.length, STORAGE_PACKAGE_KIND_BACKUP,
                                          &summary);
    }
    if (result == APP_ERROR_NONE &&
        (summary.set_count != snapshot->set_count || summary.local_macro_count != local_count ||
         summary.global_macro_count != snapshot->global_macros.count ||
         summary.procedure_count != procedure_count || summary.progress_count != progress_count)) {
        result = APP_ERROR_INTERNAL;
    }
    if (result != APP_ERROR_NONE) {
        free(writer.data);
        return result;
    }
    *out_data = writer.data;
    *out_length = writer.length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_package_export_backup_detail(bool include_progress, char **out_data,
                                                      size_t *out_length,
                                                      storage_package_failure_t *out_failure,
                                                      storage_package_skip_report_t *out_skipped) {
    if (out_data != NULL) {
        *out_data = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (out_failure != NULL) {
        memset(out_failure, 0, sizeof(*out_failure));
    }
    if (out_data == NULL || out_length == NULL || !backup_operations_valid()) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* ~1.4 KB; the export already runs deep on the httpd task stack, so it does
     * not go on the frame. Allocated even when the caller passed NULL, because
     * the package itself must record what was skipped either way. */
    storage_package_skip_report_t *skipped = calloc(1U, sizeof(*skipped));
    if (skipped == NULL) {
        return APP_ERROR_INTERNAL;
    }
    backup_snapshot_t snapshot = {0};
    app_error_code_t result = snapshot_load(include_progress, &snapshot, out_failure, skipped);
    if (result == APP_ERROR_NONE) {
        prune_dangling_procedures(&snapshot, skipped);
        result = validate_snapshot(&snapshot, out_failure);
    }
    if (result == APP_ERROR_NONE) {
        result = serialize_snapshot(&snapshot, out_data, out_length, skipped);
    }
    if (result == APP_ERROR_NONE && out_skipped != NULL) {
        *out_skipped = *skipped;
    }
    backup_snapshot_free(&snapshot);
    free(skipped);
    return result;
}

app_error_code_t storage_package_export_backup(bool include_progress, char **out_data,
                                               size_t *out_length) {
    return storage_package_export_backup_detail(include_progress, out_data, out_length, NULL, NULL);
}
