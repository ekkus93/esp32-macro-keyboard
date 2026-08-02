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

#define PACKAGE_RESTORE_ARRAY_COUNT 2U
#define PACKAGE_JSON_SUFFIX ".json"

typedef enum {
    PACKAGE_RESTORE_SETS = 0,
    PACKAGE_RESTORE_MACROS,
} package_restore_array_t;

typedef struct {
    cJSON *root;
    const cJSON *arrays[PACKAGE_RESTORE_ARRAY_COUNT];
} package_restore_document_t;

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

static app_error_code_t write_set_index(const char *data_root, const storage_set_index_t *index) {
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
                                  ? join_path(data_root, "set-index.json", path, sizeof(path))
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

static app_error_code_t create_set_directories(const char *sets_root, const app_uuid_t *set_id,
                                               char *out_set_root, size_t set_root_size) {
    app_error_code_t result = join_path(sets_root, set_id->value, out_set_root, set_root_size);
    if (result == APP_ERROR_NONE) {
        result = make_directory(out_set_root);
    }
    static const char *const children[] = {"macros"};
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
    /* storage_uuid_order_t is ~3.7 KB. With the transaction layer gone this
     * function inlines into materialize_sets, which put that frame at 5152
     * bytes against the ratchet's 4096 (scripts/check-stack-usage.sh). */
    storage_uuid_order_t *order = calloc(1U, sizeof(*order));
    if (order == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int count = cJSON_GetArraySize(document->arrays[PACKAGE_RESTORE_MACROS]);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        macro_t macro = {0};
        result = parse_macro_node(
            cJSON_GetArrayItem(document->arrays[PACKAGE_RESTORE_MACROS], index), &macro);
        const bool belongs = result == APP_ERROR_NONE && app_uuid_equal(&macro.set_id, &set->id);
        if (result == APP_ERROR_NONE && belongs) {
            if (order->count >= APP_MACROS_PER_SET_MAX) {
                result = APP_ERROR_INVALID_ARGUMENT;
            } else {
                result = write_macro_object(directory, &macro);
                if (result == APP_ERROR_NONE) {
                    order->ids[order->count++] = macro.id;
                }
            }
        }
        macro_model_free_macro(&macro);
    }
    if (result == APP_ERROR_NONE) {
        result = write_order_file(set_root, "macro-order.json", order, APP_MACROS_PER_SET_MAX);
    }
    free(order);
    return result;
}

static app_error_code_t materialize_sets(const package_restore_document_t *document,
                                         const char *data_root) {
    char sets_root[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(data_root, "sets", sets_root, sizeof(sets_root));
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
            index->ids[index->count++] = set.id;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = write_set_index(data_root, index);
    }
    free(index);
    return result;
}

/* Clears the existing repository so the backup's sets can be written in its
 * place. Only sets/ and the set index are touched: everything else under /data
 * belongs to other subsystems or does not exist. */
static app_error_code_t clear_existing_sets(void) {
    char sets_root[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(STORAGE_DATA_MOUNT, "sets", sets_root, sizeof(sets_root));
    if (result == APP_ERROR_NONE) {
        result = storage_repository_remove_tree(sets_root);
    }
    return result == APP_ERROR_NONE ? make_directory(sets_root) : result;
}

/* SPEC 13.5: restore is the only operation that writes more than one file, and
 * it is explicitly NOT atomic across sets -- it must not pretend to be. The
 * transaction manifest it used to write claimed an all-or-nothing guarantee the
 * design does not want and a 512 KiB partition cannot pay for, since honouring
 * it meant keeping a whole second copy of the repository in a trash directory.
 *
 * What this does now: clear sets/, then write each set from the backup. Every
 * individual file lands atomically via storage_atomic_write. An interruption
 * leaves the sets written so far, and the ones not yet written are simply
 * absent.
 *
 * Still owed by Phase 5, and deliberately not attempted here: per-set success
 * and failure reported back to the client, and running the whole loop on the
 * worker task instead of the httpd task. */
static app_error_code_t restore_locked(const package_restore_document_t *document) {
    app_error_code_t result = clear_existing_sets();
    if (result == APP_ERROR_NONE) {
        result = materialize_sets(document, STORAGE_DATA_MOUNT);
    }
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
