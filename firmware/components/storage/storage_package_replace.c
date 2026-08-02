#include "storage_package.h"

#include <errno.h>
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
#include "storage.h"
#include "storage_fs_ops.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_sets_internal.h"
#include "storage_set_tree_internal.h"
#include "storage_transaction_internal.h"

#define PACKAGE_REPLACE_ARRAY_COUNT 4U
#define PACKAGE_JSON_SUFFIX ".json"

typedef enum {
    PACKAGE_REPLACE_SETS = 0,
    PACKAGE_REPLACE_MACROS,
    PACKAGE_REPLACE_PROCEDURES,
    PACKAGE_REPLACE_PROGRESS,
} package_replace_array_t;

typedef struct {
    cJSON *root;
    const cJSON *arrays[PACKAGE_REPLACE_ARRAY_COUNT];
    macro_set_t replacement;
} package_replace_document_t;

static app_error_code_t package_uuid_generate(void *context, app_uuid_t *out_uuid) {
    (void)context;
    return app_uuid_generate(out_uuid);
}

static app_error_code_t package_index_presence(void *context, const app_uuid_t *set_id,
                                               bool should_be_present) {
    (void)context;
    return storage_repository_set_index_presence(set_id, should_be_present);
}

static app_error_code_t package_validate_tree(void *context, const char *path,
                                              const app_uuid_t *set_id,
                                              uint32_t expected_revision) {
    (void)context;
    return storage_set_tree_validate(path, set_id, expected_revision);
}

static app_error_code_t package_remove_tree(void *context, const char *path) {
    (void)context;
    return storage_repository_remove_tree(path);
}

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    return APP_ERROR_IO;
}

static app_error_code_t join_path(const char *parent, const char *name, char *output,
                                  size_t output_size) {
    if (parent == NULL || name == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(output, output_size, "%s/%s", parent, name);
    if (written < 0 || (size_t)written >= output_size) {
        output[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t sync_parent(const char *path) {
    return storage_fs_sync_parent_path(NULL, path) == 0 ? APP_ERROR_NONE : map_error_number(errno);
}

static app_error_code_t make_directory(const char *path) {
    app_error_code_t result = storage_repository_make_directory(path);
    if (result == APP_ERROR_NONE) {
        result = sync_parent(path);
    }
    return result;
}

static app_error_code_t node_json(const cJSON *node, char **out_json, size_t *out_length) {
    if (node == NULL || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_json = NULL;
    *out_length = 0U;
    char *json = cJSON_PrintUnformatted(node);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_length = strlen(json);
    *out_json = json;
    return APP_ERROR_NONE;
}

static app_error_code_t parse_set_node(const cJSON *node, macro_set_t *out_set) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = node_json(node, &json, &length);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_set_json(json, length, out_set);
    }
    cJSON_free(json);
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t parse_macro_node(const cJSON *node, macro_t *out_macro) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = node_json(node, &json, &length);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_macro_json(json, length, out_macro);
    }
    cJSON_free(json);
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t parse_procedure_node(const cJSON *node, procedure_t *out_procedure) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = node_json(node, &json, &length);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_procedure_json(json, length, out_procedure);
    }
    cJSON_free(json);
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t parse_progress_node(const cJSON *node, procedure_progress_t *out_progress) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = node_json(node, &json, &length);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_progress_json(json, length, out_progress);
    }
    cJSON_free(json);
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static void close_document(package_replace_document_t *document) {
    if (document == NULL) {
        return;
    }
    cJSON_Delete(document->root);
    memset(document, 0, sizeof(*document));
}

static app_error_code_t open_document(const char *data, size_t length,
                                      package_replace_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, false);
    if (root == NULL || parse_end != data + length || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const names[PACKAGE_REPLACE_ARRAY_COUNT] = {
        [PACKAGE_REPLACE_SETS] = "sets",
        [PACKAGE_REPLACE_MACROS] = "macros",
        [PACKAGE_REPLACE_PROCEDURES] = "procedures",
        [PACKAGE_REPLACE_PROGRESS] = "progress",
    };
    for (size_t index = 0U; index < PACKAGE_REPLACE_ARRAY_COUNT; ++index) {
        out_document->arrays[index] = cJSON_GetObjectItemCaseSensitive(root, names[index]);
        if (!cJSON_IsArray(out_document->arrays[index])) {
            cJSON_Delete(root);
            memset(out_document, 0, sizeof(*out_document));
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    const cJSON *set_node = cJSON_GetArrayItem(out_document->arrays[PACKAGE_REPLACE_SETS], 0);
    app_error_code_t result = parse_set_node(set_node, &out_document->replacement);
    if (result != APP_ERROR_NONE) {
        cJSON_Delete(root);
        memset(out_document, 0, sizeof(*out_document));
        return result;
    }
    out_document->root = root;
    return APP_ERROR_NONE;
}

static app_error_code_t write_json_file(const char *path, const char *json, size_t length) {
    return storage_atomic_write(path, json, length, true);
}

static app_error_code_t write_set(const char *staging, const macro_set_t *set) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_serialize_set_json(set, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(staging, "set.json", path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t write_order(const char *staging, const char *name,
                                    const storage_uuid_order_t *order, size_t maximum) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_serialize_order_json(order, maximum, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(staging, name, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t write_macro_node(const char *directory, const cJSON *node,
                                         const app_uuid_t *set_id, storage_uuid_order_t *order) {
    macro_t macro = {0};
    app_error_code_t result = parse_macro_node(node, &macro);
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&macro.set_id, set_id) || order->count >= APP_MACROS_PER_SET_MAX)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    char *json = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_serialize_macro_json(&macro, &json, &length);
    }
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_JSON_SUFFIX)];
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(name, sizeof(name), "%s.json", macro.id.value);
        result = written >= 0 && (size_t)written < sizeof(name)
                     ? join_path(directory, name, path, sizeof(path))
                     : APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    if (result == APP_ERROR_NONE) {
        order->ids[order->count++] = macro.id;
    }
    cJSON_free(json);
    macro_model_free_macro(&macro);
    return result;
}

static app_error_code_t write_macros(const char *staging, const cJSON *array,
                                     const app_uuid_t *set_id) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "macros", directory, sizeof(directory));
    storage_uuid_order_t order = {0};
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result = write_macro_node(directory, cJSON_GetArrayItem(array, index), set_id, &order);
    }
    return result == APP_ERROR_NONE
               ? write_order(staging, "macro-order.json", &order, APP_MACROS_PER_SET_MAX)
               : result;
}

static app_error_code_t write_procedure_node(const char *directory, const cJSON *node,
                                             const app_uuid_t *set_id,
                                             storage_uuid_order_t *order) {
    procedure_t procedure = {0};
    app_error_code_t result = parse_procedure_node(node, &procedure);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(&procedure.set_id, set_id) ||
                                     order->count >= APP_PROCEDURES_PER_SET_MAX)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    char *json = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_serialize_procedure_json(&procedure, &json, &length);
    }
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_JSON_SUFFIX)];
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(name, sizeof(name), "%s.json", procedure.id.value);
        result = written >= 0 && (size_t)written < sizeof(name)
                     ? join_path(directory, name, path, sizeof(path))
                     : APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    if (result == APP_ERROR_NONE) {
        order->ids[order->count++] = procedure.id;
    }
    cJSON_free(json);
    macro_model_free_procedure(&procedure);
    return result;
}

static app_error_code_t write_procedures(const char *staging, const cJSON *array,
                                         const app_uuid_t *set_id) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "procedures", directory, sizeof(directory));
    storage_uuid_order_t order = {0};
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result = write_procedure_node(directory, cJSON_GetArrayItem(array, index), set_id, &order);
    }
    return result == APP_ERROR_NONE
               ? write_order(staging, "procedure-order.json", &order, APP_PROCEDURES_PER_SET_MAX)
               : result;
}

static app_error_code_t write_progress_node(const char *directory, const cJSON *node,
                                            const app_uuid_t *set_id) {
    procedure_progress_t progress = {0};
    app_error_code_t result = parse_progress_node(node, &progress);
    if (result == APP_ERROR_NONE && !app_uuid_equal(&progress.set_id, set_id)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    char *json = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_serialize_progress_json(&progress, &json, &length);
    }
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_JSON_SUFFIX)];
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(name, sizeof(name), "%s.json", progress.procedure_id.value);
        result = written >= 0 && (size_t)written < sizeof(name)
                     ? join_path(directory, name, path, sizeof(path))
                     : APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t write_progress(const char *staging, const cJSON *array,
                                       const app_uuid_t *set_id) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "progress", directory, sizeof(directory));
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result = write_progress_node(directory, cJSON_GetArrayItem(array, index), set_id);
    }
    return result;
}

static app_error_code_t create_staging(const app_uuid_t *transaction_id, char *staging,
                                       size_t staging_size) {
    const int written =
        snprintf(staging, staging_size, STORAGE_DATA_MOUNT "/staging/%s", transaction_id->value);
    if (written < 0 || (size_t)written >= staging_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = make_directory(staging);
    static const char *const children[] = {"macros", "procedures", "progress"};
    for (size_t index = 0U;
         result == APP_ERROR_NONE && index < sizeof(children) / sizeof(children[0]); ++index) {
        char path[APP_PATH_MAX_BYTES];
        result = join_path(staging, children[index], path, sizeof(path));
        if (result == APP_ERROR_NONE) {
            result = make_directory(path);
        }
    }
    return result;
}

static app_error_code_t materialize_staging(const package_replace_document_t *document,
                                            const char *staging) {
    app_error_code_t result = write_set(staging, &document->replacement);
    if (result == APP_ERROR_NONE) {
        result = write_macros(staging, document->arrays[PACKAGE_REPLACE_MACROS],
                              &document->replacement.id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_procedures(staging, document->arrays[PACKAGE_REPLACE_PROCEDURES],
                                  &document->replacement.id);
    }
    if (result == APP_ERROR_NONE) {
        result = write_progress(staging, document->arrays[PACKAGE_REPLACE_PROGRESS],
                                &document->replacement.id);
    }
    return result;
}

static app_error_code_t copy_manifest_path(char *destination, size_t destination_size,
                                           const char *source) {
    const int written = snprintf(destination, destination_size, "%s", source);
    return written >= 0 && (size_t)written < destination_size ? APP_ERROR_NONE
                                                              : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t initialize_manifest(const app_uuid_t *transaction_id,
                                            const macro_set_t *current,
                                            const macro_set_t *replacement, const char *staging,
                                            storage_transaction_manifest_t *out_manifest) {
    memset(out_manifest, 0, sizeof(*out_manifest));
    out_manifest->schema_version = APP_SCHEMA_VERSION;
    out_manifest->id = *transaction_id;
    out_manifest->type = STORAGE_TRANSACTION_REPLACE_SET;
    out_manifest->phase = STORAGE_TRANSACTION_PREPARED;
    out_manifest->expected_revision = current->revision;
    out_manifest->replacement_revision = replacement->revision;
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        storage_make_set_path(&replacement->id, destination, sizeof(destination));
    char backup[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(backup, sizeof(backup), STORAGE_DATA_MOUNT "/trash/%s-%s",
                                     replacement->id.value, transaction_id->value);
        result = written >= 0 && (size_t)written < sizeof(backup) ? APP_ERROR_NONE
                                                                  : APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result =
            copy_manifest_path(out_manifest->source, sizeof(out_manifest->source), destination);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->staging, sizeof(out_manifest->staging), staging);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->destination, sizeof(out_manifest->destination),
                                    destination);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->backup, sizeof(out_manifest->backup), backup);
    }
    return result;
}

static app_error_code_t recover_replace(storage_transaction_manifest_t *manifest) {
    return storage_transaction_recover_replace_with_ops(
        manifest, storage_fs_ops_posix(), package_uuid_generate, NULL, package_index_presence, NULL,
        package_validate_tree, NULL, package_remove_tree, NULL);
}

static app_error_code_t replace_locked(const app_uuid_t *target_set_id, uint32_t expected_revision,
                                       package_replace_document_t *document, macro_set_t *out_set) {
    macro_set_t current = {0};
    app_error_code_t result = storage_set_read_locked(target_set_id, &current);
    if (result == APP_ERROR_NONE && current.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    app_uuid_t transaction_id = {0};
    if (result == APP_ERROR_NONE) {
        result = app_uuid_generate(&transaction_id);
    }
    char staging[APP_PATH_MAX_BYTES] = {0};
    storage_transaction_manifest_t manifest = {0};
    bool manifest_written = false;
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(staging, sizeof(staging), STORAGE_DATA_MOUNT "/staging/%s",
                                     transaction_id.value);
        result = written >= 0 && (size_t)written < sizeof(staging) ? APP_ERROR_NONE
                                                                   : APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = initialize_manifest(&transaction_id, &current, &document->replacement, staging,
                                     &manifest);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_transaction_write_manifest(&manifest);
        manifest_written = result == APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        result = create_staging(&transaction_id, staging, sizeof(staging));
    }
    if (result == APP_ERROR_NONE) {
        result = materialize_staging(document, staging);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_set_tree_validate(staging, target_set_id, document->replacement.revision);
    }
    if (result == APP_ERROR_NONE) {
        manifest.phase = STORAGE_TRANSACTION_STAGED;
        result = storage_transaction_write_manifest(&manifest);
    }
    if (result == APP_ERROR_NONE) {
        result = recover_replace(&manifest);
    } else if (manifest_written) {
        const app_error_code_t rollback = recover_replace(&manifest);
        if (rollback != APP_ERROR_NONE) {
            return result;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = storage_set_read_locked(target_set_id, out_set);
    }
    if (result == APP_ERROR_NONE && out_set->revision != document->replacement.revision) {
        memset(out_set, 0, sizeof(*out_set));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

app_error_code_t storage_package_replace_set(const app_uuid_t *target_set_id,
                                             uint32_t expected_revision, const char *data,
                                             size_t length, macro_set_t *out_set) {
    if (out_set != NULL) {
        memset(out_set, 0, sizeof(*out_set));
    }
    if (target_set_id == NULL || expected_revision == 0U || data == NULL || length == 0U ||
        out_set == NULL || !app_uuid_is_valid_string(target_set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {0};
    app_error_code_t result =
        storage_package_validate(data, length, STORAGE_PACKAGE_KIND_SET, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    package_replace_document_t document = {0};
    result = open_document(data, length, &document);
    if (result == APP_ERROR_NONE && !app_uuid_equal(target_set_id, &document.replacement.id)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_lock_take();
    }
    if (result == APP_ERROR_NONE) {
        result = replace_locked(target_set_id, expected_revision, &document, out_set);
        const app_error_code_t unlock = storage_repository_lock_give();
        if (unlock != APP_ERROR_NONE) {
            memset(out_set, 0, sizeof(*out_set));
            result = APP_ERROR_INTERNAL;
        }
    }
    close_document(&document);
    return result;
}
