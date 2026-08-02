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
#include "storage_repository_tree_internal.h"
#include "storage_transaction_internal.h"

#define PACKAGE_RESTORE_ARRAY_COUNT 4U
#define PACKAGE_JSON_SUFFIX ".json"

typedef enum {
    PACKAGE_RESTORE_SETS = 0,
    PACKAGE_RESTORE_MACROS,
    PACKAGE_RESTORE_PROCEDURES,
    PACKAGE_RESTORE_PROGRESS,
} package_restore_array_t;

typedef struct {
    cJSON *root;
    const cJSON *arrays[PACKAGE_RESTORE_ARRAY_COUNT];
} package_restore_document_t;

static app_error_code_t restore_uuid_generate(void *context, app_uuid_t *out_uuid) {
    (void)context;
    return app_uuid_generate(out_uuid);
}

static app_error_code_t restore_validate_repository(void *context, const char *root) {
    (void)context;
    return storage_repository_tree_validate(root);
}

static app_error_code_t restore_remove_tree(void *context, const char *path) {
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

static app_error_code_t write_json_file(const char *path, const char *json, size_t length) {
    return storage_atomic_write(path, json, length, true);
}

static app_error_code_t node_json(const cJSON *node, char **out_json, size_t *out_length) {
    if (node == NULL || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_json = cJSON_PrintUnformatted(node);
    if (*out_json == NULL) {
        *out_length = 0U;
        return APP_ERROR_INTERNAL;
    }
    *out_length = strlen(*out_json);
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

static app_error_code_t open_document(const char *data, size_t length,
                                      package_restore_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, false);
    if (root == NULL || parse_end != data + length || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const names[PACKAGE_RESTORE_ARRAY_COUNT] = {
        [PACKAGE_RESTORE_SETS] = "sets",
        [PACKAGE_RESTORE_MACROS] = "macros",
        [PACKAGE_RESTORE_PROCEDURES] = "procedures",
        [PACKAGE_RESTORE_PROGRESS] = "progress",
    };
    for (size_t index = 0U; index < PACKAGE_RESTORE_ARRAY_COUNT; ++index) {
        out_document->arrays[index] = cJSON_GetObjectItemCaseSensitive(root, names[index]);
        if (!cJSON_IsArray(out_document->arrays[index])) {
            cJSON_Delete(root);
            memset(out_document, 0, sizeof(*out_document));
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    out_document->root = root;
    return APP_ERROR_NONE;
}

static void close_document(package_restore_document_t *document) {
    cJSON_Delete(document->root);
    memset(document, 0, sizeof(*document));
}

static app_error_code_t write_order_file(const char *directory, const char *name,
                                         const storage_uuid_order_t *order, size_t maximum) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_serialize_order_json(order, maximum, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(directory, name, path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t write_set_index(const char *staging, const storage_set_index_t *index) {
    cJSON *root = cJSON_CreateObject();
    cJSON *ids = cJSON_CreateArray();
    if (root == NULL || ids == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", 1.0) == NULL ||
        !cJSON_AddItemToObject(root, "ids", ids)) {
        cJSON_Delete(ids);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    for (size_t item = 0U; item < index->count; ++item) {
        cJSON *value = cJSON_CreateString(index->ids[item].value);
        if (value == NULL || !cJSON_AddItemToArray(ids, value)) {
            cJSON_Delete(value);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(json);
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = length <= STORAGE_INDEX_FILE_MAX_BYTES
                                  ? join_path(staging, "set-index.json", path, sizeof(path))
                                  : APP_ERROR_INVALID_ARGUMENT;
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t write_set_metadata(const char *set_root, const macro_set_t *set) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_serialize_set_json(set, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(set_root, "set.json", path, sizeof(path));
    }
    if (result == APP_ERROR_NONE) {
        result = write_json_file(path, json, length);
    }
    cJSON_free(json);
    return result;
}

static app_error_code_t write_macro_object(const char *directory, const macro_t *macro) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_serialize_macro_json(macro, &json, &length);
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_JSON_SUFFIX)];
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(name, sizeof(name), "%s.json", macro->id.value);
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

static app_error_code_t write_procedure_object(const char *directory,
                                               const procedure_t *procedure) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_serialize_procedure_json(procedure, &json, &length);
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_JSON_SUFFIX)];
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(name, sizeof(name), "%s.json", procedure->id.value);
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

static app_error_code_t write_progress_object(const char *directory,
                                              const procedure_progress_t *progress) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_serialize_progress_json(progress, &json, &length);
    char name[APP_UUID_STRING_LENGTH + sizeof(PACKAGE_JSON_SUFFIX)];
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(name, sizeof(name), "%s.json", progress->procedure_id.value);
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

static app_error_code_t create_set_directories(const char *sets_root, const app_uuid_t *set_id,
                                               char *out_set_root, size_t set_root_size) {
    app_error_code_t result = join_path(sets_root, set_id->value, out_set_root, set_root_size);
    if (result == APP_ERROR_NONE) {
        result = make_directory(out_set_root);
    }
    static const char *const children[] = {"macros", "procedures", "progress"};
    for (size_t index = 0U;
         result == APP_ERROR_NONE && index < sizeof(children) / sizeof(children[0]); ++index) {
        char path[APP_PATH_MAX_BYTES];
        result = join_path(out_set_root, children[index], path, sizeof(path));
        if (result == APP_ERROR_NONE) {
            result = make_directory(path);
        }
    }
    return result;
}

static app_error_code_t write_set_macros(const package_restore_document_t *document,
                                         const macro_set_t *set, const char *set_root) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(set_root, "macros", directory, sizeof(directory));
    storage_uuid_order_t order = {0};
    const int count = cJSON_GetArraySize(document->arrays[PACKAGE_RESTORE_MACROS]);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        macro_t macro = {0};
        result = parse_macro_node(
            cJSON_GetArrayItem(document->arrays[PACKAGE_RESTORE_MACROS], index), &macro);
        const bool belongs = result == APP_ERROR_NONE && app_uuid_equal(&macro.set_id, &set->id);
        if (result == APP_ERROR_NONE && belongs) {
            if (order.count >= APP_MACROS_PER_SET_MAX) {
                result = APP_ERROR_INVALID_ARGUMENT;
            } else {
                result = write_macro_object(directory, &macro);
                if (result == APP_ERROR_NONE) {
                    order.ids[order.count++] = macro.id;
                }
            }
        }
        macro_model_free_macro(&macro);
    }
    return result == APP_ERROR_NONE
               ? write_order_file(set_root, "macro-order.json", &order, APP_MACROS_PER_SET_MAX)
               : result;
}

static app_error_code_t write_set_procedures(const package_restore_document_t *document,
                                             const macro_set_t *set, const char *set_root) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(set_root, "procedures", directory, sizeof(directory));
    storage_uuid_order_t order = {0};
    const int count = cJSON_GetArraySize(document->arrays[PACKAGE_RESTORE_PROCEDURES]);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        procedure_t procedure = {0};
        result = parse_procedure_node(
            cJSON_GetArrayItem(document->arrays[PACKAGE_RESTORE_PROCEDURES], index), &procedure);
        const bool belongs =
            result == APP_ERROR_NONE && app_uuid_equal(&procedure.set_id, &set->id);
        if (result == APP_ERROR_NONE && belongs) {
            if (order.count >= APP_PROCEDURES_PER_SET_MAX) {
                result = APP_ERROR_INVALID_ARGUMENT;
            } else {
                result = write_procedure_object(directory, &procedure);
                if (result == APP_ERROR_NONE) {
                    order.ids[order.count++] = procedure.id;
                }
            }
        }
        macro_model_free_procedure(&procedure);
    }
    return result == APP_ERROR_NONE ? write_order_file(set_root, "procedure-order.json", &order,
                                                       APP_PROCEDURES_PER_SET_MAX)
                                    : result;
}

static app_error_code_t write_set_progress(const package_restore_document_t *document,
                                           const macro_set_t *set, const char *set_root) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(set_root, "progress", directory, sizeof(directory));
    const int count = cJSON_GetArraySize(document->arrays[PACKAGE_RESTORE_PROGRESS]);
    if (result != APP_ERROR_NONE || count == 0) {
        return result;
    }
    /* procedure_progress_t is ~16 KB (two app_uuid_t[APP_STEPS_PER_PROCEDURE_MAX]
     * arrays). Restore runs on a task stack of a few KiB, so it lives on the
     * heap: one allocation, one free, one exit. */
    procedure_progress_t *progress = calloc(1U, sizeof(*progress));
    if (progress == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        memset(progress, 0, sizeof(*progress));
        result = parse_progress_node(
            cJSON_GetArrayItem(document->arrays[PACKAGE_RESTORE_PROGRESS], index), progress);
        if (result == APP_ERROR_NONE && app_uuid_equal(&progress->set_id, &set->id)) {
            result = write_progress_object(directory, progress);
        }
    }
    free(progress);
    return result;
}

static app_error_code_t materialize_sets(const package_restore_document_t *document,
                                         const char *staging) {
    char sets_root[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(staging, "sets", sets_root, sizeof(sets_root));
    /* storage_set_index_t is several KiB; keep it off the task stack. */
    storage_set_index_t *index = calloc(1U, sizeof(*index));
    if (index == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int count = cJSON_GetArraySize(document->arrays[PACKAGE_RESTORE_SETS]);
    for (int item = 0; result == APP_ERROR_NONE && item < count; ++item) {
        macro_set_t set = {0};
        result =
            parse_set_node(cJSON_GetArrayItem(document->arrays[PACKAGE_RESTORE_SETS], item), &set);
        if (result == APP_ERROR_NONE && index->count >= APP_MACRO_SETS_MAX) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        char set_root[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = create_set_directories(sets_root, &set.id, set_root, sizeof(set_root));
        }
        if (result == APP_ERROR_NONE) {
            result = write_set_metadata(set_root, &set);
        }
        if (result == APP_ERROR_NONE) {
            result = write_set_macros(document, &set, set_root);
        }
        if (result == APP_ERROR_NONE) {
            result = write_set_procedures(document, &set, set_root);
        }
        if (result == APP_ERROR_NONE) {
            result = write_set_progress(document, &set, set_root);
        }
        if (result == APP_ERROR_NONE) {
            index->ids[index->count++] = set.id;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = write_set_index(staging, index);
    }
    free(index);
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
    if (result == APP_ERROR_NONE) {
        char path[APP_PATH_MAX_BYTES];
        result = join_path(staging, "sets", path, sizeof(path));
        if (result == APP_ERROR_NONE) {
            result = make_directory(path);
        }
    }
    return result;
}

static app_error_code_t materialize_staging(const package_restore_document_t *document,
                                            const char *staging) {
    return materialize_sets(document, staging);
}

static app_error_code_t copy_manifest_path(char *destination, size_t destination_size,
                                           const char *source) {
    const int written = snprintf(destination, destination_size, "%s", source);
    return written >= 0 && (size_t)written < destination_size ? APP_ERROR_NONE
                                                              : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t initialize_manifest(const app_uuid_t *transaction_id, const char *staging,
                                            storage_transaction_manifest_t *out_manifest) {
    memset(out_manifest, 0, sizeof(*out_manifest));
    out_manifest->schema_version = APP_SCHEMA_VERSION;
    out_manifest->id = *transaction_id;
    out_manifest->type = STORAGE_TRANSACTION_RESTORE;
    out_manifest->phase = STORAGE_TRANSACTION_PREPARED;
    char backup[APP_PATH_MAX_BYTES];
    const int written = snprintf(backup, sizeof(backup), STORAGE_DATA_MOUNT "/trash/restore-%s",
                                 transaction_id->value);
    app_error_code_t result = written >= 0 && (size_t)written < sizeof(backup)
                                  ? APP_ERROR_NONE
                                  : APP_ERROR_INVALID_ARGUMENT;
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->source, sizeof(out_manifest->source),
                                    STORAGE_DATA_MOUNT);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->staging, sizeof(out_manifest->staging), staging);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->destination, sizeof(out_manifest->destination),
                                    STORAGE_DATA_MOUNT);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_manifest_path(out_manifest->backup, sizeof(out_manifest->backup), backup);
    }
    return result;
}

static app_error_code_t recover_restore(storage_transaction_manifest_t *manifest) {
    return storage_transaction_recover_restore_with_ops(
        manifest, storage_fs_ops_posix(), restore_uuid_generate, NULL, restore_validate_repository,
        NULL, restore_remove_tree, NULL);
}

static app_error_code_t restore_locked(const package_restore_document_t *document) {
    app_uuid_t transaction_id = {0};
    app_error_code_t result = app_uuid_generate(&transaction_id);
    char staging[APP_PATH_MAX_BYTES] = {0};
    if (result == APP_ERROR_NONE) {
        const int written = snprintf(staging, sizeof(staging), STORAGE_DATA_MOUNT "/staging/%s",
                                     transaction_id.value);
        result = written >= 0 && (size_t)written < sizeof(staging) ? APP_ERROR_NONE
                                                                   : APP_ERROR_INVALID_ARGUMENT;
    }
    /* storage_transaction_manifest_t is ~20 KB. As a stack local it put this
     * frame past the 24 KiB httpd task stack once anything ran beneath it -
     * and everything does: write_manifest descends through littlefs, the flash
     * driver, and esp_partition_find. Verified on hardware before this change:
     * POST /api/v1/restore tripped the stack canary on the `http` task every
     * time ("Stack canary watchpoint triggered (http)"), so restore could never
     * have completed for any user. One allocation, one free, single exit. */
    storage_transaction_manifest_t *manifest = calloc(1U, sizeof(*manifest));
    if (manifest == NULL) {
        return APP_ERROR_INTERNAL;
    }
    if (result == APP_ERROR_NONE) {
        result = initialize_manifest(&transaction_id, staging, manifest);
    }
    bool manifest_written = false;
    if (result == APP_ERROR_NONE) {
        result = storage_transaction_write_manifest(manifest);
        manifest_written = result == APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        result = create_staging(&transaction_id, staging, sizeof(staging));
    }
    if (result == APP_ERROR_NONE) {
        result = materialize_staging(document, staging);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_tree_validate(staging);
    }
    if (result == APP_ERROR_NONE) {
        manifest->phase = STORAGE_TRANSACTION_STAGED;
        result = storage_transaction_write_manifest(manifest);
    }
    if (result == APP_ERROR_NONE) {
        result = recover_restore(manifest);
    } else if (manifest_written && manifest->phase == STORAGE_TRANSACTION_PREPARED) {
        (void)recover_restore(manifest);
    }
    free(manifest);
    return result;
}

app_error_code_t storage_package_restore_backup(const char *data, size_t length) {
    if (data == NULL || length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {0};
    app_error_code_t result =
        storage_package_validate(data, length, STORAGE_PACKAGE_KIND_BACKUP, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    package_restore_document_t document = {0};
    result = open_document(data, length, &document);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_lock_take();
    }
    if (result == APP_ERROR_NONE) {
        result = restore_locked(&document);
        const app_error_code_t unlock = storage_repository_lock_give();
        if (result == APP_ERROR_NONE && unlock != APP_ERROR_NONE) {
            result = APP_ERROR_INTERNAL;
        }
    }
    close_document(&document);
    return result;
}
