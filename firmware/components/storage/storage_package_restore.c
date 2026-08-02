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
#include "storage_repository_document.h"
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

/* Restore writes one file per set (SPEC 13.5): each set from the backup is
 * assembled into a document and stored atomically. It is deliberately NOT atomic
 * across sets -- an interruption leaves the sets written so far, and Phase 5
 * makes the response say which those were. */
static app_error_code_t materialize_one_set(const package_restore_document_t *document,
                                            const macro_set_t *set) {
    const cJSON *array = document->arrays[PACKAGE_RESTORE_MACROS];
    const int count = cJSON_GetArraySize(array);
    if (count < 0) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_document_t stored = {.set = *set};
    if (count > 0) {
        stored.macros = calloc((size_t)count, sizeof(*stored.macros));
        if (stored.macros == NULL) {
            return APP_ERROR_INTERNAL;
        }
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        macro_t macro = {0};
        result = parse_macro_node(cJSON_GetArrayItem(array, index), &macro);
        if (result != APP_ERROR_NONE) {
            macro_model_free_macro(&macro);
            break;
        }
        /* The backup keeps every set's macros in one flat array, so each set
         * takes only the macros that name it. */
        if (!app_uuid_equal(&macro.set_id, &set->id)) {
            macro_model_free_macro(&macro);
            continue;
        }
        if (stored.macro_count >= APP_MACROS_PER_SET_MAX) {
            macro_model_free_macro(&macro);
            result = APP_ERROR_INVALID_ARGUMENT;
            break;
        }
        stored.macros[stored.macro_count] = macro;
        ++stored.macro_count;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_store_set_document(&stored);
    }
    storage_set_document_free(&stored);
    return result;
}

static app_error_code_t materialize_sets(const package_restore_document_t *document) {
    /* storage_set_index_t is several KiB; keep it off the task stack. */
    storage_set_index_t *index = calloc(1U, sizeof(*index));
    if (index == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = APP_ERROR_NONE;
    const int count = cJSON_GetArraySize(document->arrays[PACKAGE_RESTORE_SETS]);
    for (int item = 0; result == APP_ERROR_NONE && item < count; ++item) {
        macro_set_t set = {0};
        result =
            parse_set_node(cJSON_GetArrayItem(document->arrays[PACKAGE_RESTORE_SETS], item), &set);
        if (result == APP_ERROR_NONE && index->count >= APP_MACRO_SETS_MAX) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        if (result == APP_ERROR_NONE) {
            result = materialize_one_set(document, &set);
        }
        if (result == APP_ERROR_NONE) {
            index->ids[index->count++] = set.id;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_write_index(index);
    }
    free(index);
    return result;
}

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
        result = materialize_sets(document);
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
