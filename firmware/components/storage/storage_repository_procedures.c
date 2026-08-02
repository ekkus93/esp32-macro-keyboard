#include "storage_repository.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_quarantine_internal.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"
#include "storage_repository_order.h"
#include "storage_repository_procedures_internal.h"

static bool procedure_matches_set(const procedure_t *procedure, const app_uuid_t *set_id) {
    return procedure != NULL && set_id != NULL && app_uuid_is_valid_string(set_id->value) &&
           app_uuid_equal(&procedure->set_id, set_id);
}

static app_error_code_t verify_set_exists(const app_uuid_t *set_id) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(set_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t length = strlen(path);
    static const char suffix[] = "/set.json";
    if (length + sizeof(suffix) > sizeof(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(path + length, suffix, sizeof(suffix));
    struct stat metadata;
    if (stat(path, &metadata) == 0) {
        return APP_ERROR_NONE;
    }
    return errno == ENOENT ? APP_ERROR_NOT_FOUND : storage_repository_map_file_error();
}

static app_error_code_t procedure_file_path(const app_uuid_t *set_id,
                                            const app_uuid_t *procedure_id, char *path,
                                            size_t path_size) {
    return storage_make_procedure_path(set_id, procedure_id, path, path_size);
}

static app_error_code_t procedure_order_path(const app_uuid_t *set_id, char *path,
                                             size_t path_size) {
    return storage_make_procedure_order_path(set_id, path, path_size);
}

static app_error_code_t load_procedure_order(const app_uuid_t *set_id,
                                             storage_uuid_order_t *out_order) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = procedure_order_path(set_id, path, sizeof(path));
    return result == APP_ERROR_NONE
               ? storage_repository_load_order_locked(path, APP_PROCEDURES_PER_SET_MAX, out_order)
               : result;
}

static app_error_code_t write_procedure_order(const app_uuid_t *set_id,
                                              const storage_uuid_order_t *order) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = procedure_order_path(set_id, path, sizeof(path));
    return result == APP_ERROR_NONE
               ? storage_repository_write_order_locked(path, APP_PROCEDURES_PER_SET_MAX, order)
               : result;
}

static app_error_code_t read_macro_candidate(const storage_macro_location_t *location,
                                             const app_uuid_t *macro_id, bool *out_exists) {
    macro_t macro = {0};
    const app_error_code_t result = storage_macro_read_locked(location, macro_id, &macro);
    if (result == APP_ERROR_NONE) {
        *out_exists = true;
        macro_model_free_macro(&macro);
        return APP_ERROR_NONE;
    }
    *out_exists = false;
    return result == APP_ERROR_NOT_FOUND ? APP_ERROR_NONE : result;
}

static app_error_code_t
validate_macro_reference_locked(const storage_macro_location_t *set_location,
                                const app_uuid_t *macro_id) {
    const storage_macro_location_t global_location = {
        .scope = MACRO_SCOPE_GLOBAL,
        .has_set_id = false,
    };
    bool set_exists = false;
    bool global_exists = false;
    app_error_code_t result = read_macro_candidate(set_location, macro_id, &set_exists);
    if (result == APP_ERROR_NONE) {
        result = read_macro_candidate(&global_location, macro_id, &global_exists);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (set_exists && global_exists) {
        return APP_ERROR_CONFLICT;
    }
    return set_exists || global_exists ? APP_ERROR_NONE : APP_ERROR_NOT_FOUND;
}

static app_error_code_t validate_procedure_shape(const procedure_t *procedure) {
    char *probe = NULL;
    size_t probe_length = 0U;
    const app_error_code_t result =
        storage_repository_serialize_procedure_json(procedure, &probe, &probe_length);
    cJSON_free(probe);
    return result;
}

static app_error_code_t validate_procedure_references_locked(const procedure_t *procedure) {
    const storage_macro_location_t set_location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = procedure->set_id,
    };
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        const procedure_step_t *step = &procedure->steps[index];
        if (step->type != PROCEDURE_STEP_MACRO) {
            continue;
        }
        const app_error_code_t result =
            validate_macro_reference_locked(&set_location, &step->macro_id);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t quarantine_procedure(const char *path, const char *reason) {
    storage_quarantine_entry_t entry = {0};
    const app_error_code_t quarantine = storage_quarantine_file_locked(path, reason, &entry);
    return quarantine == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : quarantine;
}

static app_error_code_t procedure_read_object_locked(const app_uuid_t *set_id,
                                                     const app_uuid_t *procedure_id,
                                                     procedure_t *out_procedure) {
    if (set_id == NULL || procedure_id == NULL || out_procedure == NULL ||
        !app_uuid_is_valid_string(set_id->value) ||
        !app_uuid_is_valid_string(procedure_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_procedure, 0, sizeof(*out_procedure));
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = procedure_file_path(set_id, procedure_id, path, sizeof(path));
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_read_bounded_file(path, STORAGE_PROCEDURE_FILE_MAX_BYTES, &data,
                                                      &length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_procedure_json(data, length, out_procedure);
    }
    free(data);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(procedure_id, &out_procedure->id) ||
                                     !procedure_matches_set(out_procedure, set_id))) {
        macro_model_free_procedure(out_procedure);
        memset(out_procedure, 0, sizeof(*out_procedure));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = validate_procedure_references_locked(out_procedure);
        if (result == APP_ERROR_NOT_FOUND || result == APP_ERROR_CONFLICT ||
            result == APP_ERROR_STORAGE_CORRUPT) {
            macro_model_free_procedure(out_procedure);
            memset(out_procedure, 0, sizeof(*out_procedure));
            return quarantine_procedure(path, "invalid procedure macro reference");
        }
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        macro_model_free_procedure(out_procedure);
        memset(out_procedure, 0, sizeof(*out_procedure));
        return quarantine_procedure(path, "invalid procedure object");
    }
    return result;
}

static app_error_code_t procedure_order_index(const app_uuid_t *set_id, size_t *out_index,
                                              const app_uuid_t *procedure_id) {
    storage_uuid_order_t order = {0};
    const app_error_code_t result = load_procedure_order(set_id, &order);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return storage_repository_order_contains(&order, procedure_id, out_index)
               ? APP_ERROR_NONE
               : APP_ERROR_STORAGE_CORRUPT;
}

app_error_code_t storage_procedure_read_locked(const storage_procedure_identity_t *identity,
                                               procedure_t *out_procedure) {
    if (identity == NULL || out_procedure == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result =
        procedure_read_object_locked(&identity->set_id, &identity->procedure_id, out_procedure);
    size_t index = 0U;
    if (result == APP_ERROR_NONE) {
        result = procedure_order_index(&identity->set_id, &index, &identity->procedure_id);
    }
    if (result == APP_ERROR_NONE) {
        out_procedure->sort_order = (int32_t)index;
    } else {
        macro_model_free_procedure(out_procedure);
        memset(out_procedure, 0, sizeof(*out_procedure));
    }
    return result;
}

static app_error_code_t procedure_read_locked(const app_uuid_t *set_id,
                                              const app_uuid_t *procedure_id,
                                              procedure_t *out_procedure) {
    if (set_id == NULL || procedure_id == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const storage_procedure_identity_t identity = {
        .set_id = *set_id,
        .procedure_id = *procedure_id,
    };
    return storage_procedure_read_locked(&identity, out_procedure);
}

static void record_procedure_skip(storage_skip_record_t *skips, const app_uuid_t *object_id) {
    if (skips == NULL) {
        return;
    }
    ++skips->total;
    if (skips->items != NULL && skips->count < skips->capacity) {
        skips->items[skips->count].has_id = true;
        skips->items[skips->count].id = *object_id;
        ++skips->count;
    }
}

app_error_code_t storage_procedure_list_detail_locked(const app_uuid_t *set_id,
                                                      storage_procedure_list_t *out_list,
                                                      storage_object_ref_t *out_failed,
                                                      storage_skip_record_t *out_skips) {
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
    if (set_id == NULL || out_list == NULL || !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    app_error_code_t result = verify_set_exists(set_id);
    storage_uuid_order_t order = {0};
    if (result == APP_ERROR_NONE) {
        result = load_procedure_order(set_id, &order);
    }
    if (result != APP_ERROR_NONE || order.count == 0U) {
        return result;
    }
    procedure_t *items = calloc(order.count, sizeof(*items));
    if (items == NULL) {
        return APP_ERROR_INTERNAL;
    }
    size_t loaded = 0U;
    for (size_t index = 0U; index < order.count; ++index) {
        result = procedure_read_object_locked(set_id, &order.ids[index], &items[loaded]);
        if (result == APP_ERROR_NONE) {
            items[loaded].sort_order = (int32_t)loaded;
            ++loaded;
            continue;
        }
        if (out_skips != NULL && app_error_is_object_fault(result)) {
            memset(&items[loaded], 0, sizeof(items[loaded]));
            record_procedure_skip(out_skips, &order.ids[index]);
            result = APP_ERROR_NONE;
            continue;
        }
        /* Captured before unwinding: this is the only point that knows which
         * procedure failed, and the caller needs it to name it. */
        if (out_failed != NULL) {
            out_failed->has_id = true;
            out_failed->id = order.ids[index];
        }
        break;
    }
    if (result != APP_ERROR_NONE) {
        for (size_t index = 0U; index < loaded; ++index) {
            macro_model_free_procedure(&items[index]);
        }
        free(items);
        return result;
    }
    out_list->items = items;
    out_list->count = loaded;
    return APP_ERROR_NONE;
}

app_error_code_t storage_procedure_list_locked(const app_uuid_t *set_id,
                                               storage_procedure_list_t *out_list) {
    return storage_procedure_list_detail_locked(set_id, out_list, NULL, NULL);
}

static app_error_code_t write_procedure_object(const app_uuid_t *set_id,
                                               const procedure_t *procedure) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_serialize_procedure_json(procedure, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = procedure_file_path(set_id, &procedure->id, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, length, true);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t procedure_create_locked(const app_uuid_t *set_id,
                                                const procedure_t *procedure) {
    if (!procedure_matches_set(procedure, set_id) || procedure->revision != 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = validate_procedure_shape(procedure);
    if (result == APP_ERROR_NONE) {
        result = verify_set_exists(set_id);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_procedure_references_locked(procedure);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_uuid_order_t order = {0};
    result = load_procedure_order(set_id, &order);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (storage_repository_order_contains(&order, &procedure->id, NULL)) {
        return APP_ERROR_CONFLICT;
    }
    char path[APP_PATH_MAX_BYTES];
    result = procedure_file_path(set_id, &procedure->id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(path, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    if (errno != ENOENT) {
        return storage_repository_map_file_error();
    }
    result = storage_repository_order_append(&order, APP_PROCEDURES_PER_SET_MAX, &procedure->id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    procedure_t stored = *procedure;
    stored.sort_order = (int32_t)(order.count - 1U);
    result = write_procedure_object(set_id, &stored);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = write_procedure_order(set_id, &order);
    if (result != APP_ERROR_NONE && unlink(path) != 0) {
        return APP_ERROR_IO;
    }
    return result;
}

static app_error_code_t procedure_update_locked(const app_uuid_t *set_id,
                                                const procedure_t *replacement,
                                                uint32_t expected_revision,
                                                procedure_t *out_updated) {
    if (!procedure_matches_set(replacement, set_id) || out_updated == NULL ||
        expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_updated, 0, sizeof(*out_updated));
    procedure_t current = {0};
    app_error_code_t result = procedure_read_locked(set_id, &replacement->id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision || replacement->revision != expected_revision ||
        expected_revision == UINT32_MAX) {
        macro_model_free_procedure(&current);
        return APP_ERROR_CONFLICT;
    }
    result = validate_procedure_shape(replacement);
    if (result == APP_ERROR_NONE) {
        result = validate_procedure_references_locked(replacement);
    }
    if (result != APP_ERROR_NONE) {
        macro_model_free_procedure(&current);
        return result;
    }
    procedure_t updated = *replacement;
    updated.revision = expected_revision + 1U;
    updated.sort_order = current.sort_order;
    result = write_procedure_object(set_id, &updated);
    macro_model_free_procedure(&current);
    if (result == APP_ERROR_NONE) {
        result = procedure_read_locked(set_id, &updated.id, out_updated);
    }
    return result;
}

static app_error_code_t remove_progress_if_present(const app_uuid_t *set_id,
                                                   const app_uuid_t *procedure_id) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result =
        storage_make_progress_path(set_id, procedure_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (unlink(path) == 0 || errno == ENOENT) {
        return APP_ERROR_NONE;
    }
    return storage_repository_map_file_error();
}

static app_error_code_t procedure_delete_locked(const app_uuid_t *set_id,
                                                const app_uuid_t *procedure_id,
                                                uint32_t expected_revision) {
    if (set_id == NULL || procedure_id == NULL || expected_revision == 0U ||
        !app_uuid_is_valid_string(set_id->value) ||
        !app_uuid_is_valid_string(procedure_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    procedure_t current = {0};
    app_error_code_t result = procedure_read_locked(set_id, procedure_id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision) {
        macro_model_free_procedure(&current);
        return APP_ERROR_CONFLICT;
    }
    macro_model_free_procedure(&current);
    result = remove_progress_if_present(set_id, procedure_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_uuid_order_t order = {0};
    result = load_procedure_order(set_id, &order);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_order_remove(&order, procedure_id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_procedure_order(set_id, &order);
    }
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = procedure_file_path(set_id, procedure_id, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE && unlink(path) != 0) {
        result = storage_repository_map_file_error();
    }
    return result;
}

static app_error_code_t procedure_reorder_locked(const app_uuid_t *set_id,
                                                 const app_uuid_t *ordered_ids, size_t count) {
    if (set_id == NULL || !app_uuid_is_valid_string(set_id->value) ||
        (ordered_ids == NULL && count != 0U) || count > APP_PROCEDURES_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_uuid_order_t current = {0};
    app_error_code_t result = load_procedure_order(set_id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_uuid_order_t replacement = {.count = count};
    if (count > 0U) {
        memcpy(replacement.ids, ordered_ids, count * sizeof(*ordered_ids));
    }
    char *probe = NULL;
    size_t probe_length = 0U;
    result = storage_repository_serialize_order_json(&replacement, APP_PROCEDURES_PER_SET_MAX,
                                                     &probe, &probe_length);
    cJSON_free(probe);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!storage_repository_order_same_members(&current, &replacement)) {
        return APP_ERROR_CONFLICT;
    }
    return write_procedure_order(set_id, &replacement);
}

app_error_code_t storage_procedure_list(const app_uuid_t *set_id,
                                        storage_procedure_list_t *out_list) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_procedure_list_locked(set_id, out_list);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

void storage_procedure_list_free(storage_procedure_list_t *list) {
    if (list == NULL) {
        return;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        macro_model_free_procedure(&list->items[index]);
    }
    free(list->items);
    memset(list, 0, sizeof(*list));
}

app_error_code_t storage_procedure_create(const app_uuid_t *set_id, const procedure_t *procedure) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = procedure_create_locked(set_id, procedure);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_procedure_read(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
                                        procedure_t *out_procedure) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = procedure_read_locked(set_id, procedure_id, out_procedure);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_procedure_update(const app_uuid_t *set_id, const procedure_t *replacement,
                                          uint32_t expected_revision, procedure_t *out_updated) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        procedure_update_locked(set_id, replacement, expected_revision, out_updated);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_procedure_delete(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
                                          uint32_t expected_revision) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        procedure_delete_locked(set_id, procedure_id, expected_revision);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_procedure_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,
                                           size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = procedure_reorder_locked(set_id, ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
