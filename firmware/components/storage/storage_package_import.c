#include "storage_package.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_sets_internal.h"
#include "storage_set_tree_internal.h"

#define PACKAGE_IMPORT_ARRAY_COUNT 4U
#define PACKAGE_IMPORT_JSON_SUFFIX ".json"

typedef enum {
    PACKAGE_IMPORT_SETS = 0,
    PACKAGE_IMPORT_MACROS,
    PACKAGE_IMPORT_PROCEDURES,
    PACKAGE_IMPORT_PROGRESS,
} package_import_array_t;

typedef struct {
    cJSON *root;
    const cJSON *arrays[PACKAGE_IMPORT_ARRAY_COUNT];
    macro_set_t source_set;
} package_import_document_t;

/* Rewrite target for materializing a parsed package under a brand-new set identity:
 * every macro/procedure/progress node in the package must already declare
 * `source_set_id` (self-consistency with the package's own sets[0].id), and gets
 * `new_set_id` stamped on write, mirroring storage_repository_set_operations.c's
 * write_duplicate_macro pattern applied to package content instead of live objects. */
typedef struct {
    const app_uuid_t *source_set_id;
    const app_uuid_t *new_set_id;
} package_import_rewrite_t;

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

static void close_document(package_import_document_t *document) {
    if (document == NULL) {
        return;
    }
    cJSON_Delete(document->root);
    memset(document, 0, sizeof(*document));
}

static app_error_code_t open_document(const char *data, size_t length,
                                      package_import_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, false);
    if (root == NULL || parse_end != data + length || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const names[PACKAGE_IMPORT_ARRAY_COUNT] = {
        [PACKAGE_IMPORT_SETS] = "sets",
        [PACKAGE_IMPORT_MACROS] = "macros",
        [PACKAGE_IMPORT_PROCEDURES] = "procedures",
        [PACKAGE_IMPORT_PROGRESS] = "progress",
    };
    for (size_t index = 0U; index < PACKAGE_IMPORT_ARRAY_COUNT; ++index) {
        out_document->arrays[index] = cJSON_GetObjectItemCaseSensitive(root, names[index]);
        if (!cJSON_IsArray(out_document->arrays[index])) {
            cJSON_Delete(root);
            memset(out_document, 0, sizeof(*out_document));
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    const cJSON *set_node = cJSON_GetArrayItem(out_document->arrays[PACKAGE_IMPORT_SETS], 0);
    app_error_code_t result = parse_set_node(set_node, &out_document->source_set);
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

static app_error_code_t write_import_set(const char *staging, const macro_set_t *set) {
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

static app_error_code_t write_import_order(const char *staging, const char *name,
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

static app_error_code_t write_import_macro_node(const char *directory, const cJSON *node,
                                                const package_import_rewrite_t *rewrite,
                                                storage_uuid_order_t *order) {
    macro_t macro = {0};
    app_error_code_t result = parse_macro_node(node, &macro);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(&macro.set_id, rewrite->source_set_id) ||
                                     order->count >= APP_MACROS_PER_SET_MAX)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        macro.set_id = *rewrite->new_set_id;
        macro.revision = 1U;
    }
    char *json = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_serialize_macro_json(&macro, &json, &length);
    }
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_IMPORT_JSON_SUFFIX)];
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

static app_error_code_t write_import_macros(const char *staging, const cJSON *array,
                                            const package_import_rewrite_t *rewrite) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "macros", directory, sizeof(directory));
    storage_uuid_order_t order = {0};
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result =
            write_import_macro_node(directory, cJSON_GetArrayItem(array, index), rewrite, &order);
    }
    return result == APP_ERROR_NONE
               ? write_import_order(staging, "macro-order.json", &order, APP_MACROS_PER_SET_MAX)
               : result;
}

static app_error_code_t write_import_procedure_node(const char *directory, const cJSON *node,
                                                    const package_import_rewrite_t *rewrite,
                                                    storage_uuid_order_t *order) {
    procedure_t procedure = {0};
    app_error_code_t result = parse_procedure_node(node, &procedure);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(&procedure.set_id, rewrite->source_set_id) ||
                                     order->count >= APP_PROCEDURES_PER_SET_MAX)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        procedure.set_id = *rewrite->new_set_id;
        procedure.revision = 1U;
    }
    char *json = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_serialize_procedure_json(&procedure, &json, &length);
    }
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_IMPORT_JSON_SUFFIX)];
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

static app_error_code_t write_import_procedures(const char *staging, const cJSON *array,
                                                const package_import_rewrite_t *rewrite) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "procedures", directory, sizeof(directory));
    storage_uuid_order_t order = {0};
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result = write_import_procedure_node(directory, cJSON_GetArrayItem(array, index), rewrite,
                                             &order);
    }
    return result == APP_ERROR_NONE ? write_import_order(staging, "procedure-order.json", &order,
                                                         APP_PROCEDURES_PER_SET_MAX)
                                    : result;
}

/* Progress records carry the procedure's revision at export time; every procedure
 * is rewritten to revision 1 on import (matching the fresh set/macro revisions), so
 * progress.procedure_revision must be rewritten to 1 too or storage_set_tree_validate's
 * cross-check against the staged procedure's actual revision fails closed. */
static app_error_code_t write_import_progress_node(const char *directory, const cJSON *node,
                                                   const package_import_rewrite_t *rewrite) {
    procedure_progress_t progress = {0};
    app_error_code_t result = parse_progress_node(node, &progress);
    if (result == APP_ERROR_NONE && !app_uuid_equal(&progress.set_id, rewrite->source_set_id)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        progress.set_id = *rewrite->new_set_id;
        progress.procedure_revision = 1U;
    }
    char *json = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_repository_serialize_progress_json(&progress, &json, &length);
    }
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_IMPORT_JSON_SUFFIX)];
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

static app_error_code_t write_import_progress(const char *staging, const cJSON *array,
                                              const package_import_rewrite_t *rewrite) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "progress", directory, sizeof(directory));
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result = write_import_progress_node(directory, cJSON_GetArrayItem(array, index), rewrite);
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
    app_error_code_t result = storage_repository_make_directory(staging);
    static const char *const children[] = {"macros", "procedures", "progress"};
    for (size_t index = 0U;
         result == APP_ERROR_NONE && index < sizeof(children) / sizeof(children[0]); ++index) {
        char path[APP_PATH_MAX_BYTES];
        result = join_path(staging, children[index], path, sizeof(path));
        if (result == APP_ERROR_NONE) {
            result = storage_repository_make_directory(path);
        }
    }
    return result;
}

static app_error_code_t materialize_staging(const package_import_document_t *document,
                                            const macro_set_t *new_set,
                                            const package_import_rewrite_t *rewrite,
                                            const char *staging) {
    app_error_code_t result = write_import_set(staging, new_set);
    if (result == APP_ERROR_NONE) {
        result = write_import_macros(staging, document->arrays[PACKAGE_IMPORT_MACROS], rewrite);
    }
    if (result == APP_ERROR_NONE) {
        result =
            write_import_procedures(staging, document->arrays[PACKAGE_IMPORT_PROCEDURES], rewrite);
    }
    if (result == APP_ERROR_NONE) {
        result = write_import_progress(staging, document->arrays[PACKAGE_IMPORT_PROGRESS], rewrite);
    }
    return result;
}

static app_error_code_t import_set_accepts_new_id(const storage_set_index_t *index,
                                                  const app_uuid_t *new_set_id) {
    if (index->count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t position = 0U; position < index->count; ++position) {
        if (app_uuid_equal(&index->ids[position], new_set_id)) {
            return APP_ERROR_CONFLICT;
        }
    }
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(new_set_id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(destination, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    return errno == ENOENT ? APP_ERROR_NONE : map_error_number(errno);
}

static app_error_code_t activate_import(const macro_set_t *new_set, storage_set_index_t *index,
                                        const app_uuid_t *transaction_id, const char *staging) {
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(&new_set->id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = *transaction_id,
        .type = STORAGE_TRANSACTION_IMPORT_PACKAGE_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .expected_revision = 0U,
        .replacement_revision = new_set->revision,
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
        return map_error_number(errno);
    }
    manifest.phase = STORAGE_TRANSACTION_ACTIVATED;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    index->ids[index->count++] = new_set->id;
    result = storage_repository_write_index(index);
    if (result != APP_ERROR_NONE) {
        if (rename(destination, staging) != 0) {
            return APP_ERROR_IO;
        }
        return result;
    }
    manifest.phase = STORAGE_TRANSACTION_INDEXED;
    result = storage_transaction_write_manifest(&manifest);
    return result == APP_ERROR_NONE ? storage_repository_remove_manifest(transaction_id) : result;
}

static app_error_code_t import_locked(const app_uuid_t *new_set_id,
                                      package_import_document_t *document, macro_set_t *out_set) {
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result == APP_ERROR_NONE) {
        result = import_set_accepts_new_id(&index, new_set_id);
    }
    macro_set_t new_set = document->source_set;
    if (result == APP_ERROR_NONE) {
        new_set.id = *new_set_id;
        new_set.revision = 1U;
        new_set.sort_order = (int32_t)index.count;
    }
    const package_import_rewrite_t rewrite = {
        .source_set_id = &document->source_set.id,
        .new_set_id = new_set_id,
    };
    app_uuid_t transaction_id = {0};
    if (result == APP_ERROR_NONE) {
        result = app_uuid_generate(&transaction_id);
    }
    char staging[APP_PATH_MAX_BYTES] = {0};
    if (result == APP_ERROR_NONE) {
        result = create_staging(&transaction_id, staging, sizeof(staging));
    }
    if (result == APP_ERROR_NONE) {
        result = materialize_staging(document, &new_set, &rewrite, staging);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_set_tree_validate(staging, new_set_id, 1U);
    }
    if (result == APP_ERROR_NONE) {
        result = activate_import(&new_set, &index, &transaction_id, staging);
    } else if (staging[0] != '\0') {
        const app_error_code_t cleanup = storage_repository_remove_tree(staging);
        if (cleanup != APP_ERROR_NONE) {
            result = cleanup;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = storage_set_read_locked(new_set_id, out_set);
    }
    if (result == APP_ERROR_NONE && out_set->revision != 1U) {
        memset(out_set, 0, sizeof(*out_set));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

app_error_code_t storage_package_import_set(const app_uuid_t *new_set_id, const char *data,
                                            size_t length, macro_set_t *out_set) {
    if (out_set != NULL) {
        memset(out_set, 0, sizeof(*out_set));
    }
    if (new_set_id == NULL || data == NULL || length == 0U || out_set == NULL ||
        !app_uuid_is_valid_string(new_set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {0};
    app_error_code_t result =
        storage_package_validate(data, length, STORAGE_PACKAGE_KIND_SET, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    package_import_document_t document = {0};
    result = open_document(data, length, &document);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_lock_take();
    }
    if (result == APP_ERROR_NONE) {
        result = import_locked(new_set_id, &document, out_set);
        const app_error_code_t unlock = storage_repository_lock_give();
        if (unlock != APP_ERROR_NONE) {
            memset(out_set, 0, sizeof(*out_set));
            result = APP_ERROR_INTERNAL;
        }
    }
    close_document(&document);
    return result;
}
