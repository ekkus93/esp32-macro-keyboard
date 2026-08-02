#include "storage_package.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_json.h"
#include "storage_object_json.h"
#include "storage_repository_document.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_sets_internal.h"

#define PACKAGE_IMPORT_ARRAY_COUNT 2U

typedef enum {
    PACKAGE_IMPORT_SETS = 0,
    PACKAGE_IMPORT_MACROS,
} package_import_array_t;

typedef struct {
    cJSON *root;
    const cJSON *arrays[PACKAGE_IMPORT_ARRAY_COUNT];
    macro_set_t source_set;
} package_import_document_t;

/* Rewrite target for materializing a parsed package under a brand-new set identity:
 * every macro node in the package must already declare
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

/* A package `sets` entry is set metadata only -- the package keeps sets and
 * macros in sibling arrays, which is a different container from the stored set
 * file that holds its macros inline (SPEC 12.1, 12.2). */
static app_error_code_t parse_set_node(const cJSON *node, macro_set_t *out_set) {
    memset(out_set, 0, sizeof(*out_set));
    if (!cJSON_IsObject(node)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {"schema_version", "id", "revision", "name"};
    app_error_code_t result = storage_json_check_object_fields(node, fields, 4U, 4U);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(node, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_set->schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(node, "id", &out_set->id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(node, "revision", 1U, UINT32_MAX, &out_set->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_string(node, "name", out_set->name, sizeof(out_set->name), true);
    }
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

/* Assembles the imported set as one document and writes it as one file.
 * Every macro is stamped with the new set identity and reset to revision 1,
 * because import-new mints a fresh identity for everything it materializes. */
static app_error_code_t materialize_set(const package_import_document_t *document,
                                        const macro_set_t *new_set,
                                        const package_import_rewrite_t *rewrite) {
    const cJSON *array = document->arrays[PACKAGE_IMPORT_MACROS];
    const int count = cJSON_GetArraySize(array);
    if (count < 0 || (size_t)count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_document_t stored = {.set = *new_set};
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
        if (result == APP_ERROR_NONE && !app_uuid_equal(&macro.set_id, rewrite->source_set_id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        if (result != APP_ERROR_NONE) {
            macro_model_free_macro(&macro);
            break;
        }
        macro.set_id = *rewrite->new_set_id;
        macro.revision = 1U;
        stored.macros[stored.macro_count] = macro;
        ++stored.macro_count;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_store_set_document(&stored);
    }
    storage_set_document_free(&stored);
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
    }
    const package_import_rewrite_t rewrite = {
        .source_set_id = &document->source_set.id,
        .new_set_id = new_set_id,
    };
    bool written = false;
    if (result == APP_ERROR_NONE) {
        result = materialize_set(document, &new_set, &rewrite);
        written = result == APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        /* Index last: the imported set is unreferenced until this write lands. */
        index.ids[index.count++] = new_set.id;
        result = storage_repository_write_index(&index);
    }
    if (result != APP_ERROR_NONE && written) {
        const app_error_code_t cleanup = storage_repository_remove_set_file(&new_set.id);
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
