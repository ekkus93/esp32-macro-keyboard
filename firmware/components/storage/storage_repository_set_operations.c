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
#include "storage_repository_procedures_internal.h"
#include "storage_repository_sets_internal.h"

static app_error_code_t copy_text(char *destination, size_t destination_size, const char *source,
                                  size_t maximum_length) {
    if (destination == NULL || destination_size == 0U || source == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t length = strlen(source);
    if (length == 0U || length > maximum_length || length >= destination_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(destination, 0, destination_size);
    memcpy(destination, source, length + 1U);
    return APP_ERROR_NONE;
}

static app_error_code_t write_json_file(const char *path, char *json, size_t json_length) {
    if (path == NULL || json == NULL || json_length == 0U) {
        cJSON_free(json);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t result = storage_atomic_write(path, json, json_length, true);
    cJSON_free(json);
    return result;
}

static app_error_code_t make_child_directory(const char *parent, const char *name) {
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), "%s/%s", parent, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_repository_make_directory(path);
}

static app_error_code_t write_duplicate_metadata(const char *staging,
                                                 const macro_set_t *duplicate) {
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result = storage_repository_serialize_set_json(duplicate, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(path, sizeof(path), "%s/set.json", staging);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t write_duplicate_macro(const char *staging, const macro_t *source,
                                              const app_uuid_t *duplicate_set_id) {
    macro_t duplicate = *source;
    duplicate.revision = 1U;
    duplicate.set_id = *duplicate_set_id;
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_macro_json(&duplicate, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written =
            snprintf(path, sizeof(path), "%s/macros/%s.json", staging, duplicate.id.value);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t write_duplicate_procedure(const char *staging, const procedure_t *source,
                                                  const app_uuid_t *duplicate_set_id) {
    procedure_t duplicate = *source;
    duplicate.revision = 1U;
    duplicate.set_id = *duplicate_set_id;
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_procedure_json(&duplicate, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written =
            snprintf(path, sizeof(path), "%s/procedures/%s.json", staging, duplicate.id.value);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t write_duplicate_order(const char *staging, const char *filename,
                                              size_t maximum_count, const app_uuid_t *ids,
                                              size_t count) {
    storage_uuid_order_t order = {.count = count};
    if (count > 0U) {
        memcpy(order.ids, ids, count * sizeof(*ids));
    }
    char *json = NULL;
    size_t json_length = 0U;
    app_error_code_t result =
        storage_repository_serialize_order_json(&order, maximum_count, &json, &json_length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(path, sizeof(path), "%s/%s", staging, filename);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_free(json);
        return result;
    }
    return write_json_file(path, json, json_length);
}

static app_error_code_t create_duplicate_staging(const macro_set_t *duplicate,
                                                 const storage_macro_list_t *macros,
                                                 const storage_procedure_list_t *procedures,
                                                 const app_uuid_t *transaction_id, char *staging,
                                                 size_t staging_size) {
    const int staging_length =
        snprintf(staging, staging_size, STORAGE_DATA_MOUNT "/staging/%s", transaction_id->value);
    if (staging_length < 0 || (size_t)staging_length >= staging_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = storage_repository_make_directory(staging);
    static const char *const child_names[] = {"macros", "procedures", "progress"};
    for (size_t index = 0U;
         result == APP_ERROR_NONE && index < sizeof(child_names) / sizeof(child_names[0]);
         ++index) {
        result = make_child_directory(staging, child_names[index]);
    }
    if (result == APP_ERROR_NONE) {
        result = write_duplicate_metadata(staging, duplicate);
    }

    app_uuid_t *ordered_ids =
        macros->count == 0U ? NULL : calloc(macros->count, sizeof(*ordered_ids));
    if (macros->count > 0U && ordered_ids == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < macros->count; ++index) {
        ordered_ids[index] = macros->items[index].id;
        result = write_duplicate_macro(staging, &macros->items[index], &duplicate->id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_duplicate_order(staging, "macro-order.json", APP_MACROS_PER_SET_MAX,
                                       ordered_ids, macros->count);
    }
    free(ordered_ids);

    ordered_ids = procedures->count == 0U ? NULL : calloc(procedures->count, sizeof(*ordered_ids));
    if (result == APP_ERROR_NONE && procedures->count > 0U && ordered_ids == NULL) {
        result = APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < procedures->count; ++index) {
        ordered_ids[index] = procedures->items[index].id;
        result = write_duplicate_procedure(staging, &procedures->items[index], &duplicate->id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_duplicate_order(staging, "procedure-order.json", APP_PROCEDURES_PER_SET_MAX,
                                       ordered_ids, procedures->count);
    }
    free(ordered_ids);
    return result;
}

static bool index_contains_set(const storage_set_index_t *index, const app_uuid_t *set_id) {
    for (size_t position = 0U; position < index->count; ++position) {
        if (app_uuid_equal(&index->ids[position], set_id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t index_accepts_duplicate(const storage_set_index_t *index,
                                                const app_uuid_t *duplicate_id) {
    if (index->count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t index_position = 0U; index_position < index->count; ++index_position) {
        if (app_uuid_equal(&index->ids[index_position], duplicate_id)) {
            return APP_ERROR_CONFLICT;
        }
    }
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(duplicate_id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(destination, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
}

static app_error_code_t activate_duplicate(const macro_set_t *duplicate, storage_set_index_t *index,
                                           const app_uuid_t *transaction_id, const char *staging) {
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        storage_make_set_path(&duplicate->id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = *transaction_id,
        .type = STORAGE_TRANSACTION_DUPLICATE_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .expected_revision = 0U,
        .replacement_revision = duplicate->revision,
    };
    const int staging_copy = snprintf(manifest.staging, sizeof(manifest.staging), "%s", staging);
    const int destination_copy =
        snprintf(manifest.destination, sizeof(manifest.destination), "%s", destination);
    if (staging_copy < 0 || (size_t)staging_copy >= sizeof(manifest.staging) ||
        destination_copy < 0 || (size_t)destination_copy >= sizeof(manifest.destination)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = storage_repository_remove_tree(staging);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    if (rename(staging, destination) != 0) {
        return storage_repository_map_file_error();
    }
    manifest.phase = STORAGE_TRANSACTION_ACTIVATED;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    index->ids[index->count++] = duplicate->id;
    result = storage_repository_write_index(index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    manifest.phase = STORAGE_TRANSACTION_INDEXED;
    result = storage_transaction_write_manifest(&manifest);
    return result == APP_ERROR_NONE ? storage_repository_remove_manifest(transaction_id) : result;
}

static app_error_code_t storage_set_duplicate_locked(const app_uuid_t *source_id,
                                                     uint32_t expected_revision,
                                                     const app_uuid_t *duplicate_id,
                                                     const char *duplicate_name,
                                                     macro_set_t *out_duplicate) {
    if (out_duplicate != NULL) {
        memset(out_duplicate, 0, sizeof(*out_duplicate));
    }
    if (source_id == NULL || expected_revision == 0U || duplicate_id == NULL ||
        duplicate_name == NULL || out_duplicate == NULL ||
        !app_uuid_is_valid_string(source_id->value) ||
        !app_uuid_is_valid_string(duplicate_id->value) || app_uuid_equal(source_id, duplicate_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result == APP_ERROR_NONE && !index_contains_set(&index, source_id)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = index_accepts_duplicate(&index, duplicate_id);
    }
    macro_set_t source = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_set_read_locked(source_id, &source);
    }
    if (result == APP_ERROR_NONE && source.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    macro_set_t duplicate = source;
    if (result == APP_ERROR_NONE) {
        duplicate.id = *duplicate_id;
        duplicate.revision = 1U;
        duplicate.sort_order = (int32_t)index.count;
        result =
            copy_text(duplicate.name, sizeof(duplicate.name), duplicate_name, APP_NAME_MAX_BYTES);
    }

    storage_macro_list_t macros = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_macro_list_locked(source_id, &macros);
    }
    storage_procedure_list_t procedures = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_list_locked(source_id, &procedures);
    }

    app_uuid_t transaction_id = {0};
    if (result == APP_ERROR_NONE) {
        result = app_uuid_generate(&transaction_id);
    }
    char staging[APP_PATH_MAX_BYTES] = {0};
    if (result == APP_ERROR_NONE) {
        result = create_duplicate_staging(&duplicate, &macros, &procedures, &transaction_id,
                                          staging, sizeof(staging));
    }
    if (result == APP_ERROR_NONE) {
        result = activate_duplicate(&duplicate, &index, &transaction_id, staging);
    } else if (staging[0] != '\0') {
        const app_error_code_t cleanup = storage_repository_remove_tree(staging);
        if (cleanup != APP_ERROR_NONE) {
            result = cleanup;
        }
    }

    storage_macro_list_free(&macros);
    storage_procedure_list_free(&procedures);
    if (result == APP_ERROR_NONE) {
        *out_duplicate = duplicate;
    }
    return result;
}

app_error_code_t storage_set_duplicate(const app_uuid_t *source_id, uint32_t expected_revision,
                                       const app_uuid_t *duplicate_id, const char *duplicate_name,
                                       macro_set_t *out_duplicate) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_duplicate_locked(
        source_id, expected_revision, duplicate_id, duplicate_name, out_duplicate);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

static bool order_has_exact_members(const storage_set_index_t *current,
                                    const app_uuid_t *ordered_ids, size_t count) {
    if (current->count != count) {
        return false;
    }
    for (size_t replacement_index = 0U; replacement_index < count; ++replacement_index) {
        if (!app_uuid_is_valid_string(ordered_ids[replacement_index].value)) {
            return false;
        }
        bool found = false;
        for (size_t current_index = 0U; current_index < current->count; ++current_index) {
            if (app_uuid_equal(&ordered_ids[replacement_index], &current->ids[current_index])) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        for (size_t earlier = 0U; earlier < replacement_index; ++earlier) {
            if (app_uuid_equal(&ordered_ids[replacement_index], &ordered_ids[earlier])) {
                return false;
            }
        }
    }
    return true;
}

static app_error_code_t storage_set_reorder_locked(const app_uuid_t *ordered_ids, size_t count) {
    if ((ordered_ids == NULL && count != 0U) || count > APP_MACRO_SETS_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_index_t current = {0};
    app_error_code_t result = storage_repository_load_index(&current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!order_has_exact_members(&current, ordered_ids, count)) {
        return APP_ERROR_CONFLICT;
    }
    storage_set_index_t replacement = {.count = count};
    if (count > 0U) {
        memcpy(replacement.ids, ordered_ids, count * sizeof(*ordered_ids));
    }
    return storage_repository_write_index(&replacement);
}

app_error_code_t storage_set_reorder(const app_uuid_t *ordered_ids, size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_reorder_locked(ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
