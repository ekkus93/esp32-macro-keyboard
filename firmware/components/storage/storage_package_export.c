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
} package_writer_t;

typedef struct {
    macro_set_t set;
    storage_macro_list_t local_macros;
    storage_procedure_list_t procedures;
    storage_progress_snapshot_t *progress;
    bool *progress_present;
} set_export_snapshot_t;

#ifndef ESP_PLATFORM
static storage_package_export_ops_t export_operations;

void storage_package_set_export_ops_for_test(const storage_package_export_ops_t *operations) {
    memset(&export_operations, 0, sizeof(export_operations));
    if (operations != NULL) {
        export_operations = *operations;
    }
}

void storage_package_reset_export_ops_for_test(void) {
    memset(&export_operations, 0, sizeof(export_operations));
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

static app_error_code_t production_set_read(void *context, const app_uuid_t *set_id,
                                            macro_set_t *out_set) {
    (void)context;
    return storage_set_read_locked(set_id, out_set);
}

static app_error_code_t production_macro_list(void *context, const app_uuid_t *set_id,
                                              storage_macro_list_t *out_list) {
    (void)context;
    return storage_macro_list_locked(set_id, out_list);
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

static storage_package_export_ops_t export_operations = {
    .context = NULL,
    .lock_take = production_lock_take,
    .lock_give = production_lock_give,
    .set_read = production_set_read,
    .macro_list = production_macro_list,
    .macro_list_free = production_macro_list_free,
    .procedure_list = production_procedure_list,
    .procedure_list_free = production_procedure_list_free,
    .progress_read = production_progress_read,
};
#endif

static bool export_operations_valid(void) {
    return export_operations.lock_take != NULL && export_operations.lock_give != NULL &&
           export_operations.set_read != NULL && export_operations.macro_list != NULL &&
           export_operations.macro_list_free != NULL && export_operations.procedure_list != NULL &&
           export_operations.procedure_list_free != NULL && export_operations.progress_read != NULL;
}

static app_error_code_t writer_reserve(package_writer_t *writer, size_t additional) {
    if (writer == NULL || additional > APP_IMPORT_PACKAGE_MAX_BYTES - writer->length) {
        return APP_ERROR_MACRO_LIMIT;
    }
    const size_t required = writer->length + additional + 1U;
    if (required <= writer->capacity) {
        return APP_ERROR_NONE;
    }

    const size_t maximum_capacity = APP_IMPORT_PACKAGE_MAX_BYTES + 1U;
    size_t capacity = writer->capacity == 0U ? 1024U : writer->capacity;
    while (capacity < required) {
        if (capacity > maximum_capacity / 2U) {
            capacity = maximum_capacity;
            break;
        }
        capacity *= 2U;
    }
    if (capacity < required || capacity > maximum_capacity) {
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

static app_error_code_t writer_append_bytes(package_writer_t *writer, const char *data,
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

static app_error_code_t writer_append_text(package_writer_t *writer, const char *text) {
    return text == NULL ? APP_ERROR_INVALID_ARGUMENT
                        : writer_append_bytes(writer, text, strlen(text));
}

static app_error_code_t writer_append_serialized(package_writer_t *writer,
                                                 app_error_code_t serialization_result, char *json,
                                                 size_t length) {
    app_error_code_t result = serialization_result;
    if (result == APP_ERROR_NONE) {
        result = writer_append_bytes(writer, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t writer_append_set(package_writer_t *writer, const macro_set_t *set) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result = storage_repository_serialize_set_json(set, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static app_error_code_t writer_append_macro(package_writer_t *writer, const macro_t *macro) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result = storage_repository_serialize_macro_json(macro, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static app_error_code_t writer_append_procedure(package_writer_t *writer,
                                                const procedure_t *procedure) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result =
        storage_repository_serialize_procedure_json(procedure, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static app_error_code_t writer_append_progress(package_writer_t *writer,
                                               const procedure_progress_t *progress) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result =
        storage_repository_serialize_progress_json(progress, &json, &length);
    return writer_append_serialized(writer, result, json, length);
}

static void snapshot_free(set_export_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    export_operations.macro_list_free(export_operations.context, &snapshot->local_macros);
    export_operations.procedure_list_free(export_operations.context, &snapshot->procedures);
    free(snapshot->progress);
    free(snapshot->progress_present);
    memset(snapshot, 0, sizeof(*snapshot));
}

static app_error_code_t snapshot_load_progress(set_export_snapshot_t *snapshot) {
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
        const procedure_t *procedure = &snapshot->procedures.items[index];
        const storage_procedure_identity_t identity = {
            .set_id = snapshot->set.id,
            .procedure_id = procedure->id,
        };
        storage_progress_snapshot_t progress = {0};
        const app_error_code_t result =
            export_operations.progress_read(export_operations.context, &identity, &progress);
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

static app_error_code_t snapshot_load_locked(const app_uuid_t *set_id, bool include_progress,
                                             set_export_snapshot_t *out_snapshot) {
    app_error_code_t result =
        export_operations.set_read(export_operations.context, set_id, &out_snapshot->set);
    if (result == APP_ERROR_NONE) {
        result = export_operations.macro_list(export_operations.context, set_id,
                                              &out_snapshot->local_macros);
    }
    if (result == APP_ERROR_NONE) {
        result = export_operations.procedure_list(export_operations.context, set_id,
                                                  &out_snapshot->procedures);
    }
    if (result == APP_ERROR_NONE && include_progress) {
        result = snapshot_load_progress(out_snapshot);
    }
    return result;
}

static app_error_code_t snapshot_load(const app_uuid_t *set_id, bool include_progress,
                                      set_export_snapshot_t *out_snapshot) {
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    const app_error_code_t lock = export_operations.lock_take(export_operations.context);
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    app_error_code_t result = snapshot_load_locked(set_id, include_progress, out_snapshot);
    const app_error_code_t unlock = export_operations.lock_give(export_operations.context);
    if (result == APP_ERROR_NONE && unlock != APP_ERROR_NONE) {
        result = APP_ERROR_INTERNAL;
    }
    return result;
}

static size_t macro_index(const storage_macro_list_t *list, const app_uuid_t *macro_id) {
    for (size_t index = 0U; index < list->count; ++index) {
        if (app_uuid_equal(&list->items[index].id, macro_id)) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t validate_snapshot(const set_export_snapshot_t *snapshot) {
    if (snapshot->local_macros.count > APP_MACROS_PER_SET_MAX ||
        snapshot->procedures.count > APP_PROCEDURES_PER_SET_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < snapshot->local_macros.count; ++index) {
        const macro_t *macro = &snapshot->local_macros.items[index];
        if (!app_uuid_equal(&macro->set_id, &snapshot->set.id)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    for (size_t index = 0U; index < snapshot->procedures.count; ++index) {
        const procedure_t *procedure = &snapshot->procedures.items[index];
        if (!app_uuid_equal(&procedure->set_id, &snapshot->set.id)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        for (size_t step_index = 0U; step_index < procedure->step_count; ++step_index) {
            const procedure_step_t *step = &procedure->steps[step_index];
            if (!step->has_macro_id) {
                continue;
            }
            if (macro_index(&snapshot->local_macros, &step->macro_id) == SIZE_MAX) {
                return APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t append_macro_array(package_writer_t *writer,
                                           const storage_macro_list_t *list, const bool *included) {
    app_error_code_t result = writer_append_text(writer, "[");
    bool wrote_item = false;
    for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
        if (included != NULL && !included[index]) {
            continue;
        }
        if (wrote_item) {
            result = writer_append_text(writer, ",");
        }
        if (result == APP_ERROR_NONE) {
            result = writer_append_macro(writer, &list->items[index]);
        }
        wrote_item = result == APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(writer, "]");
    }
    return result;
}

static app_error_code_t append_procedure_array(package_writer_t *writer,
                                               const storage_procedure_list_t *list) {
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
        if (index != 0U) {
            result = writer_append_text(writer, ",");
        }
        if (result == APP_ERROR_NONE) {
            result = writer_append_procedure(writer, &list->items[index]);
        }
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(writer, "]");
    }
    return result;
}

static app_error_code_t append_progress_array(package_writer_t *writer,
                                              const set_export_snapshot_t *snapshot,
                                              size_t *out_count) {
    *out_count = 0U;
    app_error_code_t result = writer_append_text(writer, "[");
    for (size_t index = 0U; result == APP_ERROR_NONE && index < snapshot->procedures.count;
         ++index) {
        if (snapshot->progress_present == NULL || !snapshot->progress_present[index]) {
            continue;
        }
        if (*out_count != 0U) {
            result = writer_append_text(writer, ",");
        }
        if (result == APP_ERROR_NONE) {
            result = writer_append_progress(writer, &snapshot->progress[index].progress);
        }
        if (result == APP_ERROR_NONE) {
            ++*out_count;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(writer, "]");
    }
    return result;
}

static app_error_code_t serialize_snapshot(const set_export_snapshot_t *snapshot, char **out_data,
                                           size_t *out_length) {
    package_writer_t writer = {0};
    app_error_code_t result =
        writer_append_text(&writer, "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[");
    if (result == APP_ERROR_NONE) {
        result = writer_append_set(&writer, &snapshot->set);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, "],\"macros\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_macro_array(&writer, &snapshot->local_macros, NULL);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, ",\"procedures\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_procedure_array(&writer, &snapshot->procedures);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, ",\"progress\":");
    }
    size_t progress_count = 0U;
    if (result == APP_ERROR_NONE) {
        result = append_progress_array(&writer, snapshot, &progress_count);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append_text(&writer, "}");
    }

    storage_package_summary_t summary = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_validate(writer.data, writer.length, STORAGE_PACKAGE_KIND_SET,
                                          &summary);
    }
    if (result == APP_ERROR_NONE &&
        (summary.set_count != 1U || summary.local_macro_count != snapshot->local_macros.count ||
         summary.procedure_count != snapshot->procedures.count ||
         summary.progress_count != progress_count)) {
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

app_error_code_t storage_package_export_set(const app_uuid_t *set_id, bool include_progress,
                                            char **out_data, size_t *out_length) {
    if (out_data != NULL) {
        *out_data = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (set_id == NULL || !app_uuid_is_valid_string(set_id->value) || out_data == NULL ||
        out_length == NULL || !export_operations_valid()) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    set_export_snapshot_t snapshot = {0};
    app_error_code_t result = snapshot_load(set_id, include_progress, &snapshot);
    if (result == APP_ERROR_NONE) {
        result = validate_snapshot(&snapshot);
    }
    if (result == APP_ERROR_NONE) {
        result = serialize_snapshot(&snapshot, out_data, out_length);
    }
    snapshot_free(&snapshot);
    return result;
}

void storage_package_free(char *data) {
    free(data);
}
