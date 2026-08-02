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

#define PACKAGE_REPLACE_ARRAY_COUNT 2U
#define PACKAGE_JSON_SUFFIX ".json"

typedef enum {
    PACKAGE_REPLACE_SETS = 0,
    PACKAGE_REPLACE_MACROS,
} package_replace_array_t;

typedef struct {
    cJSON *root;
    const cJSON *arrays[PACKAGE_REPLACE_ARRAY_COUNT];
    macro_set_t replacement;
} package_replace_document_t;

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

static app_error_code_t write_set(const char *set_root, const macro_set_t *set) {
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

static app_error_code_t write_order(const char *set_root, const char *name,
                                    const storage_uuid_order_t *order, size_t maximum) {
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_serialize_order_json(order, maximum, &json, &length);
    char path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(set_root, name, path, sizeof(path));
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

static app_error_code_t write_macros(const char *set_root, const cJSON *array,
                                     const app_uuid_t *set_id) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(set_root, "macros", directory, sizeof(directory));
    /* storage_uuid_order_t is ~4 KB; keep it off the task stack. */
    storage_uuid_order_t *order = calloc(1U, sizeof(*order));
    if (order == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int count = cJSON_GetArraySize(array);
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        result = write_macro_node(directory, cJSON_GetArrayItem(array, index), set_id, order);
    }
    if (result == APP_ERROR_NONE) {
        result = write_order(set_root, "macro-order.json", order, APP_MACROS_PER_SET_MAX);
    }
    free(order);
    return result;
}

static app_error_code_t create_set_directories(const char *set_root) {
    app_error_code_t result = make_directory(set_root);
    static const char *const children[] = {"macros"};
    for (size_t index = 0U;
         result == APP_ERROR_NONE && index < sizeof(children) / sizeof(children[0]); ++index) {
        char path[APP_PATH_MAX_BYTES];
        result = join_path(set_root, children[index], path, sizeof(path));
        if (result == APP_ERROR_NONE) {
            result = make_directory(path);
        }
    }
    return result;
}

static app_error_code_t materialize_set(const package_replace_document_t *document,
                                        const char *set_root) {
    app_error_code_t result = write_set(set_root, &document->replacement);
    if (result == APP_ERROR_NONE) {
        result = write_macros(set_root, document->arrays[PACKAGE_REPLACE_MACROS],
                              &document->replacement.id);
    }
    return result;
}

/* SPEC 8.7 replacement, without the transaction machinery.
 *
 * A set is still a directory of files at this point in the sequence, so the old
 * tree is removed and the new one written in its place. That is NOT atomic: an
 * interruption between the two leaves the set partially written while the index
 * still references it, and it will read back as corrupt -- which SPEC 13.6
 * handles by reporting and deleting it, not by silently substituting a default.
 * The old contents are gone either way, because the design forbids keeping a
 * second on-device copy to roll back to (SPEC 22, invariant 16).
 *
 * Phase 4 closes this window completely: once a set is a single file, the whole
 * replacement is one `storage_atomic_write`, and `rename()` makes it atomic
 * with no staging directory and no manifest. This is the one place in Phase 3
 * where crash-safety is temporarily weaker than what it replaced. */
static app_error_code_t replace_locked(const app_uuid_t *target_set_id, uint32_t expected_revision,
                                       package_replace_document_t *document, macro_set_t *out_set) {
    macro_set_t current = {0};
    app_error_code_t result = storage_set_read_locked(target_set_id, &current);
    if (result == APP_ERROR_NONE && current.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    char destination[APP_PATH_MAX_BYTES] = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_make_set_path(target_set_id, destination, sizeof(destination));
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_remove_tree(destination);
    }
    if (result == APP_ERROR_NONE) {
        result = create_set_directories(destination);
    }
    if (result == APP_ERROR_NONE) {
        result = materialize_set(document, destination);
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
