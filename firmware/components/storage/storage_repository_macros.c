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
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"
#include "storage_repository_order.h"

static bool location_valid(const app_uuid_t *set_id) {
    return set_id != NULL && app_uuid_is_valid_string(set_id->value);
}

static bool macro_matches_location(const macro_t *macro, const app_uuid_t *set_id) {
    if (macro == NULL || !location_valid(set_id)) {
        return false;
    }
    return app_uuid_equal(&macro->set_id, set_id);
}

static app_error_code_t macro_order_path(const app_uuid_t *set_id, char *path, size_t path_size) {
    if (!location_valid(set_id) || path == NULL || path_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_make_set_macro_order_path(set_id, path, path_size);
}

static app_error_code_t macro_file_path(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                        char *path, size_t path_size) {
    if (!location_valid(set_id) || macro_id == NULL || path == NULL || path_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_make_macro_path(set_id, macro_id, path, path_size);
}

static app_error_code_t verify_set_location(const app_uuid_t *set_id) {
    char set_path[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(set_id, set_path, sizeof(set_path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char metadata_path[APP_PATH_MAX_BYTES];
    const int written = snprintf(metadata_path, sizeof(metadata_path), "%s/set.json", set_path);
    if (written < 0 || (size_t)written >= sizeof(metadata_path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    struct stat metadata;
    if (stat(metadata_path, &metadata) == 0) {
        return APP_ERROR_NONE;
    }
    return errno == ENOENT ? APP_ERROR_NOT_FOUND : storage_repository_map_file_error();
}

app_error_code_t storage_macro_read_locked(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                           macro_t *out_macro) {
    if (!location_valid(set_id) || macro_id == NULL || out_macro == NULL ||
        !app_uuid_is_valid_string(macro_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_macro, 0, sizeof(*out_macro));
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = macro_file_path(set_id, macro_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char *data = NULL;
    size_t length = 0U;
    result =
        storage_repository_read_bounded_file(path, STORAGE_MACRO_FILE_MAX_BYTES, &data, &length);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_macro_json(data, length, out_macro);
    }
    free(data);
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(macro_id, &out_macro->id) || !macro_matches_location(out_macro, set_id))) {
        macro_model_free_macro(out_macro);
        memset(out_macro, 0, sizeof(*out_macro));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        const app_error_code_t discard = storage_repository_discard_corrupt_file(path);
        return discard == APP_ERROR_NONE ? result : discard;
    }
    return result;
}

static app_error_code_t load_macro_order(const app_uuid_t *set_id,
                                         storage_uuid_order_t *out_order) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = macro_order_path(set_id, path, sizeof(path));
    return result == APP_ERROR_NONE
               ? storage_repository_load_order_locked(path, APP_MACROS_PER_SET_MAX, out_order)
               : result;
}

static app_error_code_t write_macro_order(const app_uuid_t *set_id,
                                          const storage_uuid_order_t *order) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = macro_order_path(set_id, path, sizeof(path));
    return result == APP_ERROR_NONE
               ? storage_repository_write_order_locked(path, APP_MACROS_PER_SET_MAX, order)
               : result;
}

static void record_skip(storage_skip_record_t *skips, const app_uuid_t *object_id) {
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

app_error_code_t storage_macro_list_detail_locked(const app_uuid_t *set_id,
                                                  storage_macro_list_t *out_list,
                                                  storage_object_ref_t *out_failed,
                                                  storage_skip_record_t *out_skips) {
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
    if (!location_valid(set_id) || out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    storage_uuid_order_t order = {0};
    app_error_code_t result = load_macro_order(set_id, &order);
    if (result != APP_ERROR_NONE || order.count == 0U) {
        return result;
    }
    macro_t *items = calloc(order.count, sizeof(*items));
    if (items == NULL) {
        return APP_ERROR_INTERNAL;
    }
    size_t loaded = 0U;
    for (size_t index = 0U; index < order.count; ++index) {
        result = storage_macro_read_locked(set_id, &order.ids[index], &items[loaded]);
        if (result == APP_ERROR_NONE) {
            ++loaded;
            continue;
        }
        if (out_skips != NULL && app_error_is_object_fault(result)) {
            /* Step over this one object and keep the rest: a single unreadable
             * macro must not make the whole repository unreadable. */
            memset(&items[loaded], 0, sizeof(items[loaded]));
            record_skip(out_skips, &order.ids[index]);
            result = APP_ERROR_NONE;
            continue;
        }
        /* Captured before unwinding: this is the only point that knows which
         * macro failed, and the caller needs it to name the object. */
        if (out_failed != NULL) {
            out_failed->has_id = true;
            out_failed->id = order.ids[index];
        }
        break;
    }
    if (result != APP_ERROR_NONE) {
        for (size_t index = 0U; index < loaded; ++index) {
            macro_model_free_macro(&items[index]);
        }
        free(items);
        return result;
    }
    out_list->items = items;
    out_list->count = loaded;
    return APP_ERROR_NONE;
}

app_error_code_t storage_macro_list_locked(const app_uuid_t *set_id,
                                           storage_macro_list_t *out_list) {
    return storage_macro_list_detail_locked(set_id, out_list, NULL, NULL);
}

static app_error_code_t write_macro_object(const app_uuid_t *set_id, const macro_t *macro) {
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result = storage_repository_serialize_macro_json(macro, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = macro_file_path(set_id, &macro->id, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, json_length, true);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t macro_create_locked(const app_uuid_t *set_id, const macro_t *macro) {
    if (!macro_matches_location(macro, set_id) || macro->revision != 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = verify_set_location(set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_uuid_order_t order = {0};
    result = load_macro_order(set_id, &order);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (storage_repository_order_contains(&order, &macro->id, NULL)) {
        return APP_ERROR_CONFLICT;
    }
    char path[APP_PATH_MAX_BYTES];
    result = macro_file_path(set_id, &macro->id, path, sizeof(path));
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
    result = storage_repository_order_append(&order, APP_MACROS_PER_SET_MAX, &macro->id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = write_macro_object(set_id, macro);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = write_macro_order(set_id, &order);
    if (result != APP_ERROR_NONE && unlink(path) != 0) {
        return APP_ERROR_IO;
    }
    return result;
}

static app_error_code_t macro_update_locked(const app_uuid_t *set_id, const macro_t *replacement,
                                            uint32_t expected_revision, macro_t *out_updated) {
    if (!macro_matches_location(replacement, set_id) || out_updated == NULL ||
        expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_updated, 0, sizeof(*out_updated));
    macro_t current = {0};
    app_error_code_t result = storage_macro_read_locked(set_id, &replacement->id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision || replacement->revision != expected_revision ||
        expected_revision == UINT32_MAX) {
        macro_model_free_macro(&current);
        return APP_ERROR_CONFLICT;
    }
    macro_t updated = *replacement;
    updated.revision = expected_revision + 1U;
    result = write_macro_object(set_id, &updated);
    macro_model_free_macro(&current);
    if (result == APP_ERROR_NONE) {
        result = storage_macro_read_locked(set_id, &updated.id, out_updated);
    }
    return result;
}

static app_error_code_t macro_delete_locked(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                            uint32_t expected_revision) {
    if (!location_valid(set_id) || macro_id == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_t current = {0};
    app_error_code_t result = storage_macro_read_locked(set_id, macro_id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision) {
        macro_model_free_macro(&current);
        return APP_ERROR_CONFLICT;
    }
    macro_model_free_macro(&current);
    /* storage_uuid_order_t is ~4 KB; keep it off the task stack. */
    storage_uuid_order_t *order = calloc(1U, sizeof(*order));
    if (order == NULL) {
        return APP_ERROR_INTERNAL;
    }
    result = load_macro_order(set_id, order);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_order_remove(order, macro_id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_macro_order(set_id, order);
    }
    free(order);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char path[APP_PATH_MAX_BYTES];
    result = macro_file_path(set_id, macro_id, path, sizeof(path));
    if (result == APP_ERROR_NONE && unlink(path) != 0) {
        result = storage_repository_map_file_error();
    }
    return result;
}

static app_error_code_t macro_duplicate_locked(const app_uuid_t *set_id,
                                               const app_uuid_t *source_id,
                                               const app_uuid_t *duplicate_id,
                                               const char *duplicate_name, macro_t *out_duplicate) {
    if (!location_valid(set_id) || source_id == NULL || duplicate_id == NULL ||
        duplicate_name == NULL || out_duplicate == NULL ||
        !app_uuid_is_valid_string(duplicate_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_duplicate, 0, sizeof(*out_duplicate));
    macro_t source = {0};
    app_error_code_t result = storage_macro_read_locked(set_id, source_id, &source);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t name_length = strlen(duplicate_name);
    if (name_length == 0U || name_length > APP_MACRO_NAME_MAX_BYTES) {
        macro_model_free_macro(&source);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_t duplicate = source;
    duplicate.id = *duplicate_id;
    duplicate.revision = 1U;
    memset(duplicate.name, 0, sizeof(duplicate.name));
    memcpy(duplicate.name, duplicate_name, name_length + 1U);
    duplicate.source = malloc(source.source_length + 1U);
    if (duplicate.source == NULL) {
        macro_model_free_macro(&source);
        return APP_ERROR_INTERNAL;
    }
    memcpy(duplicate.source, source.source, source.source_length + 1U);
    result = macro_create_locked(set_id, &duplicate);
    macro_model_free_macro(&source);
    if (result == APP_ERROR_NONE) {
        *out_duplicate = duplicate;
    } else {
        macro_model_free_macro(&duplicate);
    }
    return result;
}

static app_error_code_t macro_reorder_locked(const app_uuid_t *set_id,
                                             const app_uuid_t *ordered_ids, size_t count) {
    if (!location_valid(set_id) || (ordered_ids == NULL && count != 0U) ||
        count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_uuid_order_t current = {0};
    app_error_code_t result = load_macro_order(set_id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_uuid_order_t replacement = {.count = count};
    if (count > 0U) {
        memcpy(replacement.ids, ordered_ids, count * sizeof(*ordered_ids));
    }
    char *probe = NULL;
    size_t probe_length = 0U;
    result = storage_repository_serialize_order_json(&replacement, APP_MACROS_PER_SET_MAX, &probe,
                                                     &probe_length);
    cJSON_free(probe);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!storage_repository_order_same_members(&current, &replacement)) {
        return APP_ERROR_CONFLICT;
    }
    return write_macro_order(set_id, &replacement);
}

app_error_code_t storage_macro_list(const app_uuid_t *set_id, storage_macro_list_t *out_list) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_macro_list_locked(set_id, out_list);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

void storage_macro_list_free(storage_macro_list_t *list) {
    if (list == NULL) {
        return;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        macro_model_free_macro(&list->items[index]);
    }
    free(list->items);
    memset(list, 0, sizeof(*list));
}

app_error_code_t storage_macro_create(const app_uuid_t *set_id, const macro_t *macro) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_create_locked(set_id, macro);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_read(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                    macro_t *out_macro) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_macro_read_locked(set_id, macro_id, out_macro);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_update(const app_uuid_t *set_id, const macro_t *replacement,
                                      uint32_t expected_revision, macro_t *out_updated) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_update_locked(set_id, replacement, expected_revision, out_updated);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_delete(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                      uint32_t expected_revision) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_delete_locked(set_id, macro_id, expected_revision);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_duplicate(const app_uuid_t *set_id, const app_uuid_t *source_id,
                                         const app_uuid_t *duplicate_id, const char *duplicate_name,
                                         macro_t *out_duplicate) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_duplicate_locked(set_id, source_id, duplicate_id, duplicate_name, out_duplicate);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,
                                       size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_reorder_locked(set_id, ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
