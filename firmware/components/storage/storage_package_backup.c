#include "storage_package.h"

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

static app_error_code_t production_set_list(void *context, storage_set_list_t *out_list) {
    (void)context;
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    memset(out_list, 0, sizeof(*out_list));
    for (size_t item = 0U; result == APP_ERROR_NONE && item < index.count; ++item) {
        result = storage_set_read_locked(&index.ids[item], &out_list->items[item]);
        if (result == APP_ERROR_NONE) {
            ++out_list->count;
        }
    }
    return result;
}

static app_error_code_t production_macro_list(void *context,
                                              const storage_macro_location_t *location,
                                              storage_macro_list_t *out_list) {
    (void)context;
    return storage_macro_list_locked(location, out_list);
}

static void production_macro_list_free(void *context, storage_macro_list_t *list) {
    (void)context;
    storage_macro_list_free(list);
}

static app_error_code_t production_procedure_list(void *context, const app_uuid_t *set_id,
                                                  storage_procedure_list_t *out_list) {
    (void)context;
    return storage_procedure_list_locked(set_id, out_list);
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

static app_error_code_t load_set_progress(backup_set_snapshot_t *snapshot) {
    if (snapshot->procedures.count == 0U) {
        return APP_ERROR_NONE;
    }
    snapshot->progress = calloc(snapshot->procedures.count, sizeof(*snapshot->progress));
    snapshot->progress_present =
        calloc(snapshot->procedures.count, sizeof(*snapshot->progress_present));
    if (snapshot->progress == NULL || snapshot->progress_present == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < snapshot->procedures.count; ++index) {
        const storage_procedure_identity_t identity = {
            .set_id = snapshot->set.id,
            .procedure_id = snapshot->procedures.items[index].id,
        };
        storage_progress_snapshot_t progress = {0};
        const app_error_code_t result =
            backup_operations.progress_read(backup_operations.context, &identity, &progress);
        if (result == APP_ERROR_NOT_FOUND) {
            continue;
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (progress.status == STORAGE_PROGRESS_STATUS_CURRENT) {
            snapshot->progress[index] = progress;
            snapshot->progress_present[index] = true;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t load_set_snapshot(const macro_set_t *set, bool include_progress,
                                          backup_set_snapshot_t *out_snapshot) {
    out_snapshot->set = *set;
    const storage_macro_location_t location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = set->id,
    };
    app_error_code_t result = backup_operations.macro_list(backup_operations.context, &location,
                                                           &out_snapshot->local_macros);
    if (result == APP_ERROR_NONE) {
        result = backup_operations.procedure_list(backup_operations.context, &set->id,
                                                  &out_snapshot->procedures);
    }
    if (result == APP_ERROR_NONE && include_progress) {
        result = load_set_progress(out_snapshot);
    }
    return result;
}

static app_error_code_t snapshot_load_locked(bool include_progress,
                                             backup_snapshot_t *out_snapshot) {
    storage_set_list_t set_list = {0};
    app_error_code_t result = backup_operations.set_list(backup_operations.context, &set_list);
    if (result == APP_ERROR_NONE && set_list.count > APP_MACRO_SETS_MAX) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE && set_list.count != 0U) {
        out_snapshot->sets = calloc(set_list.count, sizeof(*out_snapshot->sets));
        if (out_snapshot->sets == NULL) {
            result = APP_ERROR_INTERNAL;
        } else {
            out_snapshot->set_count = set_list.count;
        }
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < set_list.count; ++index) {
        result =
            load_set_snapshot(&set_list.items[index], include_progress, &out_snapshot->sets[index]);
    }
    const storage_macro_location_t global_location = {
        .scope = MACRO_SCOPE_GLOBAL,
        .has_set_id = false,
        .set_id = {{0}},
    };
    if (result == APP_ERROR_NONE) {
        result = backup_operations.macro_list(backup_operations.context, &global_location,
                                              &out_snapshot->global_macros);
    }
    return result;
}

static app_error_code_t snapshot_load(bool include_progress, backup_snapshot_t *out_snapshot) {
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    const app_error_code_t lock = backup_operations.lock_take(backup_operations.context);
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    app_error_code_t result = snapshot_load_locked(include_progress, out_snapshot);
    const app_error_code_t unlock = backup_operations.lock_give(backup_operations.context);
    if (result == APP_ERROR_NONE && unlock != APP_ERROR_NONE) {
        result = APP_ERROR_INTERNAL;
    }
    return result;
}

static size_t macro_index(const storage_macro_list_t *list, const app_uuid_t *id) {
    for (size_t index = 0U; index < list->count; ++index) {
        if (app_uuid_equal(&list->items[index].id, id)) {
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

static app_error_code_t validate_snapshot(const backup_snapshot_t *snapshot) {
    if (snapshot->set_count > APP_MACRO_SETS_MAX ||
        snapshot->global_macros.count > APP_MACROS_PER_SET_MAX || !set_ids_unique(snapshot)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < snapshot->global_macros.count; ++index) {
        const macro_t *macro = &snapshot->global_macros.items[index];
        if (macro->scope != MACRO_SCOPE_GLOBAL || macro->has_set_id) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    for (size_t index = 0U; index < snapshot->set_count; ++index) {
        const app_error_code_t result = validate_set_snapshot(snapshot, &snapshot->sets[index]);
        if (result != APP_ERROR_NONE) {
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
        const storage_procedure_list_t *list = &snapshot->sets[set_index].procedures;
        for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
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
            if (set->progress_present == NULL || !set->progress_present[index]) {
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

static app_error_code_t serialize_snapshot(const backup_snapshot_t *snapshot, char **out_data,
                                           size_t *out_length) {
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

app_error_code_t storage_package_export_backup(bool include_progress, char **out_data,
                                               size_t *out_length) {
    if (out_data != NULL) {
        *out_data = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (out_data == NULL || out_length == NULL || !backup_operations_valid()) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    backup_snapshot_t snapshot = {0};
    app_error_code_t result = snapshot_load(include_progress, &snapshot);
    if (result == APP_ERROR_NONE) {
        result = validate_snapshot(&snapshot);
    }
    if (result == APP_ERROR_NONE) {
        result = serialize_snapshot(&snapshot, out_data, out_length);
    }
    backup_snapshot_free(&snapshot);
    return result;
}
