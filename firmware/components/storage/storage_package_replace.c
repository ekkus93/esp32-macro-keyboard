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
#include "storage_json.h"
#include "storage_object_json.h"
#include "storage_repository_document.h"
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

/* A package `sets` entry is set metadata only; the package keeps sets and macros
 * in sibling arrays, unlike the stored set file that holds its macros inline. */
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

/* Assembles the replacement set as one document. Macro identities and revisions
 * come from the package unchanged: replacement substitutes the set's contents,
 * it does not mint new identities the way import-new does. */
static app_error_code_t materialize_set(const package_replace_document_t *document,
                                        storage_set_document_t *out_stored) {
    const cJSON *array = document->arrays[PACKAGE_REPLACE_MACROS];
    const int count = cJSON_GetArraySize(array);
    if (count < 0 || (size_t)count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_stored, 0, sizeof(*out_stored));
    out_stored->set = document->replacement;
    if (count > 0) {
        out_stored->macros = calloc((size_t)count, sizeof(*out_stored->macros));
        if (out_stored->macros == NULL) {
            return APP_ERROR_INTERNAL;
        }
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        macro_t macro = {0};
        result = parse_macro_node(cJSON_GetArrayItem(array, index), &macro);
        if (result == APP_ERROR_NONE && !app_uuid_equal(&macro.set_id, &document->replacement.id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        if (result != APP_ERROR_NONE) {
            macro_model_free_macro(&macro);
            break;
        }
        out_stored->macros[out_stored->macro_count] = macro;
        ++out_stored->macro_count;
    }
    if (result != APP_ERROR_NONE) {
        storage_set_document_free(out_stored);
    }
    return result;
}

/* SPEC 8.7 replacement, in one atomic write.
 *
 * A set is one file now, so the whole replacement is a single
 * storage_atomic_write: stage `<set>.json.tmp`, verify it, rename it over the
 * target. An interruption at any point leaves either the complete old set or
 * the complete new one, with no staging directory, no manifest, and no window
 * in which the set is half-written.
 *
 * Phase 3 had to remove the old tree and rebuild it in place, which was not
 * atomic and said so at this call site. That window is closed.
 */
static app_error_code_t replace_locked(const app_uuid_t *target_set_id, uint32_t expected_revision,
                                       package_replace_document_t *document, macro_set_t *out_set) {
    macro_set_t current = {0};
    app_error_code_t result = storage_set_read_locked(target_set_id, &current);
    if (result == APP_ERROR_NONE && current.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    storage_set_document_t stored = {0};
    if (result == APP_ERROR_NONE) {
        result = materialize_set(document, &stored);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_store_set_document(&stored);
    }
    storage_set_document_free(&stored);
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
