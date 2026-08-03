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
#include "storage_package_internal.h"
#include "storage_package_writer.h"
#include "storage_repository.h"

#ifdef ESP_PLATFORM
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"
#include "storage_repository_packages_internal.h"
#endif

typedef struct {
    macro_package_t set;
    storage_macro_list_t local_macros;
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

static app_error_code_t production_package_read(void *context, const app_uuid_t *set_id,
                                                macro_package_t *out_package) {
    (void)context;
    return storage_package_read_locked(set_id, out_package);
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

static storage_package_export_ops_t export_operations = {
    .context = NULL,
    .lock_take = production_lock_take,
    .lock_give = production_lock_give,
    .set_read = production_package_read,
    .macro_list = production_macro_list,
    .macro_list_free = production_macro_list_free,
};
#endif

static bool export_operations_valid(void) {
    return export_operations.lock_take != NULL && export_operations.lock_give != NULL &&
           export_operations.set_read != NULL && export_operations.macro_list != NULL &&
           export_operations.macro_list_free != NULL;
}

static void snapshot_free(set_export_snapshot_t *snapshot) {
    if (snapshot == NULL) {
        return;
    }
    export_operations.macro_list_free(export_operations.context, &snapshot->local_macros);
    memset(snapshot, 0, sizeof(*snapshot));
}

static app_error_code_t snapshot_load_locked(const app_uuid_t *set_id,
                                             set_export_snapshot_t *out_snapshot) {
    app_error_code_t result =
        export_operations.set_read(export_operations.context, set_id, &out_snapshot->set);
    if (result == APP_ERROR_NONE) {
        result = export_operations.macro_list(export_operations.context, set_id,
                                              &out_snapshot->local_macros);
    }
    return result;
}

static app_error_code_t snapshot_load(const app_uuid_t *set_id,
                                      set_export_snapshot_t *out_snapshot) {
    memset(out_snapshot, 0, sizeof(*out_snapshot));
    const app_error_code_t lock = export_operations.lock_take(export_operations.context);
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    app_error_code_t result = snapshot_load_locked(set_id, out_snapshot);
    const app_error_code_t unlock = export_operations.lock_give(export_operations.context);
    if (result == APP_ERROR_NONE && unlock != APP_ERROR_NONE) {
        result = APP_ERROR_INTERNAL;
    }
    return result;
}

static app_error_code_t validate_snapshot(const set_export_snapshot_t *snapshot) {
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

static app_error_code_t append_macro_array(package_writer_t *writer,
                                           const storage_macro_list_t *list, const bool *included) {
    app_error_code_t result = package_writer_append_text(writer, "[");
    bool wrote_item = false;
    for (size_t index = 0U; result == APP_ERROR_NONE && index < list->count; ++index) {
        if (included != NULL && !included[index]) {
            continue;
        }
        if (wrote_item) {
            result = package_writer_append_text(writer, ",");
        }
        if (result == APP_ERROR_NONE) {
            result = package_writer_append_macro(writer, &list->items[index]);
        }
        wrote_item = result == APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        result = package_writer_append_text(writer, "]");
    }
    return result;
}

static app_error_code_t serialize_snapshot(const set_export_snapshot_t *snapshot, char **out_data,
                                           size_t *out_length) {
    package_writer_t writer = {0};
    app_error_code_t result = package_writer_append_text(
        &writer, "{\"schema_version\":1,\"package_type\":\"package\",\"packages\":[");
    if (result == APP_ERROR_NONE) {
        result = package_writer_append_metadata(&writer, &snapshot->set);
    }
    if (result == APP_ERROR_NONE) {
        result = package_writer_append_text(&writer, "],\"macros\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_macro_array(&writer, &snapshot->local_macros, NULL);
    }
    if (result == APP_ERROR_NONE) {
        result = package_writer_append_text(&writer, "}");
    }

    storage_package_summary_t summary = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_validate(writer.data, writer.length, STORAGE_DOCUMENT_KIND_PACKAGE,
                                          &summary);
    }
    if (result == APP_ERROR_NONE &&
        (summary.set_count != 1U || summary.local_macro_count != snapshot->local_macros.count)) {
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

app_error_code_t storage_package_export(const app_uuid_t *set_id, char **out_data,
                                        size_t *out_length) {
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
    app_error_code_t result = snapshot_load(set_id, &snapshot);
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
