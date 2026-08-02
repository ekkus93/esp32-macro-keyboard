#include "storage_repository.h"

#include <dirent.h>
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

static bool location_valid(const storage_macro_location_t *location) {
    if (location == NULL) {
        return false;
    }
    if (location->scope == MACRO_SCOPE_SET) {
        return location->has_set_id && app_uuid_is_valid_string(location->set_id.value);
    }
    static const app_uuid_t zero = {0};
    return location->scope == MACRO_SCOPE_GLOBAL && !location->has_set_id &&
           memcmp(&location->set_id, &zero, sizeof(zero)) == 0;
}

static bool macro_matches_location(const macro_t *macro, const storage_macro_location_t *location) {
    if (macro == NULL || !location_valid(location) || macro->scope != location->scope ||
        macro->has_set_id != location->has_set_id) {
        return false;
    }
    return !location->has_set_id || app_uuid_equal(&macro->set_id, &location->set_id);
}

static app_error_code_t macro_order_path(const storage_macro_location_t *location, char *path,
                                         size_t path_size) {
    if (!location_valid(location) || path == NULL || path_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (location->scope == MACRO_SCOPE_GLOBAL) {
        const int written = snprintf(path, path_size, "%s", STORAGE_GLOBAL_ORDER_FILE_PATH);
        return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE
                                                           : APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_make_set_macro_order_path(&location->set_id, path, path_size);
}

static app_error_code_t macro_file_path(const storage_macro_location_t *location,
                                        const app_uuid_t *macro_id, char *path, size_t path_size) {
    if (!location_valid(location) || macro_id == NULL || path == NULL || path_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return location->scope == MACRO_SCOPE_GLOBAL
               ? storage_make_global_macro_path(macro_id, path, path_size)
               : storage_make_macro_path(&location->set_id, macro_id, path, path_size);
}

static app_error_code_t verify_set_location(const storage_macro_location_t *location) {
    if (location->scope == MACRO_SCOPE_GLOBAL) {
        return APP_ERROR_NONE;
    }
    char set_path[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(&location->set_id, set_path, sizeof(set_path));
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

app_error_code_t storage_macro_read_locked(const storage_macro_location_t *location,
                                           const app_uuid_t *macro_id, macro_t *out_macro) {
    if (!location_valid(location) || macro_id == NULL || out_macro == NULL ||
        !app_uuid_is_valid_string(macro_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_macro, 0, sizeof(*out_macro));
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = macro_file_path(location, macro_id, path, sizeof(path));
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
    if (result == APP_ERROR_NONE && (!app_uuid_equal(macro_id, &out_macro->id) ||
                                     !macro_matches_location(out_macro, location))) {
        macro_model_free_macro(out_macro);
        memset(out_macro, 0, sizeof(*out_macro));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        storage_quarantine_entry_t entry = {0};
        const app_error_code_t quarantine =
            storage_quarantine_file_locked(path, "invalid macro object", &entry);
        return quarantine == APP_ERROR_NONE ? result : quarantine;
    }
    return result;
}

static app_error_code_t load_macro_order(const storage_macro_location_t *location,
                                         storage_uuid_order_t *out_order) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = macro_order_path(location, path, sizeof(path));
    return result == APP_ERROR_NONE
               ? storage_repository_load_order_locked(path, APP_MACROS_PER_SET_MAX, out_order)
               : result;
}

static app_error_code_t write_macro_order(const storage_macro_location_t *location,
                                          const storage_uuid_order_t *order) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = macro_order_path(location, path, sizeof(path));
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

app_error_code_t storage_macro_list_detail_locked(const storage_macro_location_t *location,
                                                  storage_macro_list_t *out_list,
                                                  storage_object_ref_t *out_failed,
                                                  storage_skip_record_t *out_skips) {
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
    if (!location_valid(location) || out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    storage_uuid_order_t order = {0};
    app_error_code_t result = load_macro_order(location, &order);
    if (result != APP_ERROR_NONE || order.count == 0U) {
        return result;
    }
    macro_t *items = calloc(order.count, sizeof(*items));
    if (items == NULL) {
        return APP_ERROR_INTERNAL;
    }
    size_t loaded = 0U;
    for (size_t index = 0U; index < order.count; ++index) {
        result = storage_macro_read_locked(location, &order.ids[index], &items[loaded]);
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

app_error_code_t storage_macro_list_locked(const storage_macro_location_t *location,
                                           storage_macro_list_t *out_list) {
    return storage_macro_list_detail_locked(location, out_list, NULL, NULL);
}

static app_error_code_t write_macro_object(const storage_macro_location_t *location,
                                           const macro_t *macro) {
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result = storage_repository_serialize_macro_json(macro, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = macro_file_path(location, &macro->id, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, json_length, true);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t macro_create_locked(const storage_macro_location_t *location,
                                            const macro_t *macro) {
    if (!macro_matches_location(macro, location) || macro->revision != 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = verify_set_location(location);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_uuid_order_t order = {0};
    result = load_macro_order(location, &order);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (storage_repository_order_contains(&order, &macro->id, NULL)) {
        return APP_ERROR_CONFLICT;
    }
    char path[APP_PATH_MAX_BYTES];
    result = macro_file_path(location, &macro->id, path, sizeof(path));
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
    result = write_macro_object(location, macro);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = write_macro_order(location, &order);
    if (result != APP_ERROR_NONE && unlink(path) != 0) {
        return APP_ERROR_IO;
    }
    return result;
}

static app_error_code_t macro_update_locked(const storage_macro_location_t *location,
                                            const macro_t *replacement, uint32_t expected_revision,
                                            macro_t *out_updated) {
    if (!macro_matches_location(replacement, location) || out_updated == NULL ||
        expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_updated, 0, sizeof(*out_updated));
    macro_t current = {0};
    app_error_code_t result = storage_macro_read_locked(location, &replacement->id, &current);
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
    result = write_macro_object(location, &updated);
    macro_model_free_macro(&current);
    if (result == APP_ERROR_NONE) {
        result = storage_macro_read_locked(location, &updated.id, out_updated);
    }
    return result;
}

static bool step_references_macro(const procedure_step_t *step, const app_uuid_t *macro_id) {
    return step->type == PROCEDURE_STEP_MACRO && step->has_macro_id &&
           app_uuid_equal(&step->macro_id, macro_id);
}

static void add_reference(storage_reference_list_t *references, const app_uuid_t *procedure_id) {
    if (references->count < STORAGE_REFERENCE_DETAIL_MAX_IDS) {
        references->ids[references->count] = *procedure_id;
        ++references->count;
    } else {
        references->truncated = true;
    }
}

static const char PROCEDURE_JSON_SUFFIX[] = ".json";

typedef struct {
    const app_uuid_t *set_id;
    const app_uuid_t *macro_id;
    storage_reference_list_t *references;
} procedure_reference_scan_t;

static app_error_code_t parse_procedure_filename(const char *filename, bool *out_matches,
                                                 app_uuid_t *out_procedure_id) {
    *out_matches = false;
    const size_t suffix_length = sizeof(PROCEDURE_JSON_SUFFIX) - 1U;
    const size_t name_length = strlen(filename);
    if (name_length != APP_UUID_STRING_LENGTH + suffix_length ||
        strcmp(filename + APP_UUID_STRING_LENGTH, PROCEDURE_JSON_SUFFIX) != 0) {
        return APP_ERROR_NONE;
    }
    char uuid_text[APP_UUID_BUFFER_LENGTH];
    memcpy(uuid_text, filename, APP_UUID_STRING_LENGTH);
    uuid_text[APP_UUID_STRING_LENGTH] = '\0';
    if (app_uuid_parse(uuid_text, out_procedure_id) != APP_ERROR_NONE) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_matches = true;
    return APP_ERROR_NONE;
}

static app_error_code_t read_reference_scan_procedure(const procedure_reference_scan_t *scan,
                                                      const app_uuid_t *procedure_id,
                                                      procedure_t *out_procedure) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        storage_make_procedure_path(scan->set_id, procedure_id, path, sizeof(path));
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
    if (result != APP_ERROR_STORAGE_CORRUPT) {
        return result;
    }
    storage_quarantine_entry_t quarantine_entry = {0};
    const app_error_code_t quarantine = storage_quarantine_file_locked(
        path, "invalid procedure during reference scan", &quarantine_entry);
    return quarantine == APP_ERROR_NONE ? result : quarantine;
}

static bool procedure_references_macro(const procedure_t *procedure, const app_uuid_t *macro_id) {
    for (size_t step_index = 0U; step_index < procedure->step_count; ++step_index) {
        if (step_references_macro(&procedure->steps[step_index], macro_id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t scan_procedure_entry(const procedure_reference_scan_t *scan,
                                             const struct dirent *entry) {
    app_uuid_t procedure_id = {0};
    bool matches = false;
    app_error_code_t result = parse_procedure_filename(entry->d_name, &matches, &procedure_id);
    if (result != APP_ERROR_NONE || !matches) {
        return result;
    }
    procedure_t procedure = {0};
    result = read_reference_scan_procedure(scan, &procedure_id, &procedure);
    if (result == APP_ERROR_NONE && procedure_references_macro(&procedure, scan->macro_id)) {
        add_reference(scan->references, &procedure.id);
    }
    macro_model_free_procedure(&procedure);
    return result;
}

static app_error_code_t scan_set_procedure_references(const procedure_reference_scan_t *scan) {
    char directory_path[APP_PATH_MAX_BYTES];
    const int written = snprintf(directory_path, sizeof(directory_path),
                                 STORAGE_DATA_MOUNT "/sets/%s/procedures", scan->set_id->value);
    if (written < 0 || (size_t)written >= sizeof(directory_path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    DIR *directory = opendir(directory_path);
    if (directory == NULL) {
        return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (struct dirent *entry = readdir(directory); entry != NULL; entry = readdir(directory)) {
        result = scan_procedure_entry(scan, entry);
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    return result;
}

static app_error_code_t find_macro_references(const storage_macro_location_t *location,
                                              const app_uuid_t *macro_id,
                                              storage_reference_list_t *references) {
    memset(references, 0, sizeof(*references));
    if (location->scope == MACRO_SCOPE_SET) {
        const procedure_reference_scan_t scan = {
            .set_id = &location->set_id,
            .macro_id = macro_id,
            .references = references,
        };
        return scan_set_procedure_references(&scan);
    }
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    for (size_t set = 0U; result == APP_ERROR_NONE && set < index.count; ++set) {
        const procedure_reference_scan_t scan = {
            .set_id = &index.ids[set],
            .macro_id = macro_id,
            .references = references,
        };
        result = scan_set_procedure_references(&scan);
    }
    return result;
}

static app_error_code_t macro_delete_locked(const storage_macro_location_t *location,
                                            const app_uuid_t *macro_id, uint32_t expected_revision,
                                            storage_reference_list_t *out_references) {
    if (!location_valid(location) || macro_id == NULL || expected_revision == 0U ||
        out_references == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_references, 0, sizeof(*out_references));
    macro_t current = {0};
    app_error_code_t result = storage_macro_read_locked(location, macro_id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision) {
        macro_model_free_macro(&current);
        return APP_ERROR_CONFLICT;
    }
    macro_model_free_macro(&current);
    result = find_macro_references(location, macro_id, out_references);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (out_references->count > 0U || out_references->truncated) {
        return APP_ERROR_CONFLICT;
    }
    storage_uuid_order_t order = {0};
    result = load_macro_order(location, &order);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_order_remove(&order, macro_id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_macro_order(location, &order);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char path[APP_PATH_MAX_BYTES];
    result = macro_file_path(location, macro_id, path, sizeof(path));
    if (result == APP_ERROR_NONE && unlink(path) != 0) {
        result = storage_repository_map_file_error();
    }
    return result;
}

static app_error_code_t macro_duplicate_locked(const storage_macro_location_t *location,
                                               const app_uuid_t *source_id,
                                               const app_uuid_t *duplicate_id,
                                               const char *duplicate_name, macro_t *out_duplicate) {
    if (!location_valid(location) || source_id == NULL || duplicate_id == NULL ||
        duplicate_name == NULL || out_duplicate == NULL ||
        !app_uuid_is_valid_string(duplicate_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_duplicate, 0, sizeof(*out_duplicate));
    macro_t source = {0};
    app_error_code_t result = storage_macro_read_locked(location, source_id, &source);
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
    result = macro_create_locked(location, &duplicate);
    macro_model_free_macro(&source);
    if (result == APP_ERROR_NONE) {
        *out_duplicate = duplicate;
    } else {
        macro_model_free_macro(&duplicate);
    }
    return result;
}

static app_error_code_t macro_reorder_locked(const storage_macro_location_t *location,
                                             const app_uuid_t *ordered_ids, size_t count) {
    if (!location_valid(location) || (ordered_ids == NULL && count != 0U) ||
        count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_uuid_order_t current = {0};
    app_error_code_t result = load_macro_order(location, &current);
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
    return write_macro_order(location, &replacement);
}

app_error_code_t storage_macro_list(const storage_macro_location_t *location,
                                    storage_macro_list_t *out_list) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_macro_list_locked(location, out_list);
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

app_error_code_t storage_macro_create(const storage_macro_location_t *location,
                                      const macro_t *macro) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_create_locked(location, macro);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_read(const storage_macro_location_t *location,
                                    const app_uuid_t *macro_id, macro_t *out_macro) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_macro_read_locked(location, macro_id, out_macro);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_update(const storage_macro_location_t *location,
                                      const macro_t *replacement, uint32_t expected_revision,
                                      macro_t *out_updated) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_update_locked(location, replacement, expected_revision, out_updated);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_delete(const storage_macro_location_t *location,
                                      const app_uuid_t *macro_id, uint32_t expected_revision,
                                      storage_reference_list_t *out_references) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_delete_locked(location, macro_id, expected_revision, out_references);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_duplicate(const storage_macro_location_t *location,
                                         const app_uuid_t *source_id,
                                         const app_uuid_t *duplicate_id, const char *duplicate_name,
                                         macro_t *out_duplicate) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_duplicate_locked(location, source_id, duplicate_id, duplicate_name, out_duplicate);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_reorder(const storage_macro_location_t *location,
                                       const app_uuid_t *ordered_ids, size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_reorder_locked(location, ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
