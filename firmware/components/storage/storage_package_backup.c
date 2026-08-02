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
} backup_set_snapshot_t;

typedef struct {
    backup_set_snapshot_t *sets;
    size_t set_count;
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

static app_error_code_t production_macro_list(void *context, const app_uuid_t *set_id,
                                              storage_macro_list_t *out_list,
                                              storage_object_ref_t *out_failed,
                                              storage_skip_record_t *out_skips) {
    (void)context;
    return storage_macro_list_detail_locked(set_id, out_list, out_failed, out_skips);
}

static void production_macro_list_free(void *context, storage_macro_list_t *list) {
    (void)context;
    storage_macro_list_free(list);
}

static storage_package_backup_ops_t backup_operations = {
    .context = NULL,
    .lock_take = production_lock_take,
    .lock_give = production_lock_give,
    .set_list = production_set_list,
    .macro_list = production_macro_list,
    .macro_list_free = production_macro_list_free,
};
#endif

static bool backup_operations_valid(void) {
    return backup_operations.lock_take != NULL && backup_operations.lock_give != NULL &&
           backup_operations.set_list != NULL && backup_operations.macro_list != NULL &&
           backup_operations.macro_list_free != NULL;
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

static void set_snapshot_free(backup_set_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    backup_operations.macro_list_free(backup_operations.context, &snapshot->local_macros);
    memset(snapshot, 0, sizeof(*snapshot));
}

static void backup_snapshot_free(backup_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    for (size_t index = 0U; index < snapshot->set_count; ++index) {
        set_snapshot_free(&snapshot->sets[index]);
    }
    free(snapshot->sets);
    memset(snapshot, 0, sizeof(*snapshot));
}

/* Enough to fold one list read's skips into the report without a heap round
 * trip; the report's own total still counts everything beyond it. */
#define BACKUP_SKIP_SCRATCH 8U

static void fold_skips(storage_package_skip_report_t *report, storage_package_object_kind_t kind,
                       const app_uuid_t *set_id, const storage_skip_record_t *skips) {
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
        if (set_id != NULL) {
            entry->has_set_id = true;
            entry->set_id = *set_id;
        }
        entry->object_id = skips->items[index].id;
        ++report->count;
    }
}

static void record_failure(storage_package_failure_t *out_failure,
                           storage_package_object_kind_t kind, const app_uuid_t *set_id,
                           const storage_object_ref_t *object) {
    if (out_failure == NULL || out_failure->kind != STORAGE_PACKAGE_OBJECT_NONE) {
        return; /* keep the first failure: it is the one that stopped the export */
    }
    out_failure->kind = kind;
    if (set_id != NULL) {
        out_failure->has_set_id = true;
        out_failure->set_id = *set_id;
    }
    if (object != NULL && object->has_id) {
        out_failure->has_object_id = true;
        out_failure->object_id = object->id;
    }
}

static app_error_code_t load_set_snapshot(const macro_set_t *set,
                                          backup_set_snapshot_t *out_snapshot,
                                          storage_package_failure_t *out_failure,
                                          storage_package_skip_report_t *out_skipped) {
    out_snapshot->set = *set;
    storage_object_ref_t failed = {0};
    storage_object_ref_t scratch[BACKUP_SKIP_SCRATCH];
    storage_skip_record_t skips = {
        .items = scratch,
        .capacity = BACKUP_SKIP_SCRATCH,
        .count = 0U,
        .total = 0U,
    };
    app_error_code_t result = backup_operations.macro_list(
        backup_operations.context, &set->id, &out_snapshot->local_macros, &failed, &skips);
    if (result != APP_ERROR_NONE) {
        record_failure(out_failure, STORAGE_PACKAGE_OBJECT_MACRO, &set->id, &failed);
        return result;
    }
    fold_skips(out_skipped, STORAGE_PACKAGE_OBJECT_MACRO, &set->id, &skips);
    return result;
}

static app_error_code_t snapshot_load_locked(backup_snapshot_t *out_snapshot,
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
        record_failure(out_failure, STORAGE_PACKAGE_OBJECT_SET, NULL, &set_failed);
    }
    fold_skips(out_skipped, STORAGE_PACKAGE_OBJECT_SET, NULL, &set_skips);
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
        result = load_set_snapshot(&set_list->items[index], &out_snapshot->sets[index], out_failure,
                                   out_skipped);
    }
    free(set_list);
    return result;
}

static app_error_code_t snapshot_load(backup_snapshot_t *out_snapshot,
                                      storage_package_failure_t *out_failure,
                                      storage_package_skip_report_t *out_skipped) {
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    const app_error_code_t lock = backup_operations.lock_take(backup_operations.context);
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    app_error_code_t result = snapshot_load_locked(out_snapshot, out_failure, out_skipped);
    const app_error_code_t unlock = backup_operations.lock_give(backup_operations.context);
    if (result == APP_ERROR_NONE && unlock != APP_ERROR_NONE) {
        result = APP_ERROR_INTERNAL;
    }
    return result;
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

static app_error_code_t validate_set_snapshot(const backup_set_snapshot_t *snapshot) {
    if (snapshot->local_macros.count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < snapshot->local_macros.count; ++index) {
        const macro_t *macro = &snapshot->local_macros.items[index];
        if (!app_uuid_equal(&macro->set_id, &snapshot->set.id)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t validate_snapshot(const backup_snapshot_t *snapshot,
                                          storage_package_failure_t *out_failure) {
    if (snapshot->set_count > APP_MACRO_SETS_MAX || !set_ids_unique(snapshot)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < snapshot->set_count; ++index) {
        const app_error_code_t result = validate_set_snapshot(&snapshot->sets[index]);
        if (result != APP_ERROR_NONE) {
            record_failure(out_failure, STORAGE_PACKAGE_OBJECT_SET, &snapshot->sets[index].set.id,
                           NULL);
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

/* Two UUIDs, two keys, and the JSON punctuation around them. */
#define BACKUP_SKIPPED_ITEM_BYTES 192U
#define BACKUP_SKIPPED_HEAD_BYTES 64U

static const char *skipped_kind_text(storage_package_object_kind_t kind) {
    switch (kind) {
    case STORAGE_PACKAGE_OBJECT_SET:
        return "set";
    case STORAGE_PACKAGE_OBJECT_MACRO:
        return "macro";
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
            written = snprintf(item, sizeof(item), "%s{\"kind\":\"%s\",\"id\":\"%s\"}",
                               index == 0U ? "" : ",", skipped_kind_text(entry->kind),
                               entry->object_id.value);
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
        (summary.set_count != snapshot->set_count || summary.local_macro_count != local_count)) {
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

app_error_code_t storage_package_export_backup_detail(char **out_data, size_t *out_length,
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
    app_error_code_t result = snapshot_load(&snapshot, out_failure, skipped);
    if (result == APP_ERROR_NONE) {
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

app_error_code_t storage_package_export_backup(char **out_data, size_t *out_length) {
    return storage_package_export_backup_detail(out_data, out_length, NULL, NULL);
}
