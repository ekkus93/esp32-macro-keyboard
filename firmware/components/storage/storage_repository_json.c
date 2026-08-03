#include "storage_object_json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_json.h"

/* SPEC 12.1: schema_version, id, revision, name, macros. Nothing else. */
#define SET_FIELD_COUNT 5U
/* SPEC 12.2, as stored: no set_id. The owning set is the file the macro is in. */
#define STORED_MACRO_FIELD_COUNT 7U
#define KEY_PRESS_STORAGE_MAX_MS 1000U

static const char *const SET_FIELDS[SET_FIELD_COUNT] = {
    "schema_version", "id", "revision", "name", "macros",
};
static const char *const STORED_MACRO_FIELDS[STORED_MACRO_FIELD_COUNT] = {
    "schema_version", "id", "revision", "name", "source", "key_press_ms", "inter_key_ms",
};

void storage_package_document_free(storage_package_document_t *document) {
    if (document == NULL) {
        return;
    }
    for (size_t index = 0U; index < document->macro_count; ++index) {
        macro_model_free_macro(&document->macros[index]);
    }
    free(document->macros);
    memset(document, 0, sizeof(*document));
}

static app_error_code_t parse_stored_macro(const cJSON *node, const app_uuid_t *owning_package_id,
                                           macro_t *out_macro) {
    memset(out_macro, 0, sizeof(*out_macro));
    if (!cJSON_IsObject(node)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    app_error_code_t result = storage_json_check_object_fields(
        node, STORED_MACRO_FIELDS, STORED_MACRO_FIELD_COUNT, STORED_MACRO_FIELD_COUNT);
    uint32_t schema_version = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(node, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &schema_version);
    }
    if (result == APP_ERROR_NONE) {
        out_macro->schema_version = schema_version;
        result = storage_json_get_uuid(node, "id", &out_macro->id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(node, "revision", 1U, UINT32_MAX, &out_macro->revision);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_string(node, "name", out_macro->name, sizeof(out_macro->name), true);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_allocated_string(node, "source", APP_MACRO_SOURCE_MAX_BYTES, false,
                                              &out_macro->source, &out_macro->source_length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(node, "key_press_ms", 1U, KEY_PRESS_STORAGE_MAX_MS,
                                      &out_macro->key_press_ms);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(node, "inter_key_ms", 0U, APP_DELAY_MAX_MS,
                                      &out_macro->inter_key_ms);
    }
    if (result != APP_ERROR_NONE) {
        macro_model_free_macro(out_macro);
        memset(out_macro, 0, sizeof(*out_macro));
        return result;
    }
    /* The set the macro belongs to is the file it was read from, so it is
     * stamped here rather than trusted from the object (SPEC 12.2). */
    out_macro->set_id = *owning_package_id;
    return APP_ERROR_NONE;
}

static app_error_code_t parse_macro_array(const cJSON *root,
                                          storage_package_document_t *out_document) {
    const cJSON *macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    if (!cJSON_IsArray(macros)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = cJSON_GetArraySize(macros);
    if (count < 0 || (size_t)count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (count == 0) {
        return APP_ERROR_NONE;
    }
    out_document->macros = calloc((size_t)count, sizeof(*out_document->macros));
    if (out_document->macros == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (int index = 0; index < count; ++index) {
        const app_error_code_t result =
            parse_stored_macro(cJSON_GetArrayItem(macros, index), &out_document->set.id,
                               &out_document->macros[(size_t)index]);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        out_document->macro_count = (size_t)index + 1U;
        for (size_t prior = 0U; prior < (size_t)index; ++prior) {
            if (app_uuid_equal(&out_document->macros[prior].id,
                               &out_document->macros[(size_t)index].id)) {
                return APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_package_document_parse(const char *data, size_t length,
                                                storage_package_document_t *out_document) {
    if (out_document != NULL) {
        memset(out_document, 0, sizeof(*out_document));
    }
    if (data == NULL || out_document == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result =
        storage_json_parse_exact_object(data, length, SET_FIELDS, SET_FIELD_COUNT, &root);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_document->set.schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "id", &out_document->set.id);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_u32(root, "revision", 1U, UINT32_MAX, &out_document->set.revision);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_string(root, "name", out_document->set.name,
                                         sizeof(out_document->set.name), true);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_macro_array(root, out_document);
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        storage_package_document_free(out_document);
    }
    return result;
}

static app_error_code_t add_stored_macro(cJSON *array, const macro_t *macro) {
    cJSON *node = cJSON_CreateObject();
    if (node == NULL || !cJSON_AddItemToArray(array, node)) {
        cJSON_Delete(node);
        return APP_ERROR_INTERNAL;
    }
    return cJSON_AddNumberToObject(node, "schema_version", (double)macro->schema_version) != NULL &&
                   cJSON_AddStringToObject(node, "id", macro->id.value) != NULL &&
                   cJSON_AddNumberToObject(node, "revision", (double)macro->revision) != NULL &&
                   cJSON_AddStringToObject(node, "name", macro->name) != NULL &&
                   cJSON_AddStringToObject(node, "source", macro->source) != NULL &&
                   cJSON_AddNumberToObject(node, "key_press_ms", (double)macro->key_press_ms) !=
                       NULL &&
                   cJSON_AddNumberToObject(node, "inter_key_ms", (double)macro->inter_key_ms) !=
                       NULL
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static bool document_shape_valid(const storage_package_document_t *document) {
    if (document == NULL || document->set.schema_version != APP_SCHEMA_VERSION ||
        document->set.revision == 0U || !app_uuid_is_valid_string(document->set.id.value) ||
        document->set.name[0] == '\0' || document->macro_count > APP_MACROS_PER_SET_MAX ||
        (document->macro_count != 0U && document->macros == NULL)) {
        return false;
    }
    for (size_t index = 0U; index < document->macro_count; ++index) {
        const macro_t *macro = &document->macros[index];
        if (macro->schema_version != APP_SCHEMA_VERSION || macro->revision == 0U ||
            !app_uuid_is_valid_string(macro->id.value) || macro->name[0] == '\0' ||
            macro->source == NULL || macro->source_length > APP_MACRO_SOURCE_MAX_BYTES ||
            macro->key_press_ms == 0U || macro->key_press_ms > KEY_PRESS_STORAGE_MAX_MS ||
            macro->inter_key_ms > APP_DELAY_MAX_MS) {
            return false;
        }
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&document->macros[prior].id, &macro->id)) {
                return false;
            }
        }
    }
    return true;
}

app_error_code_t storage_package_document_serialize(const storage_package_document_t *document,
                                                    char **out_json, size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (!document_shape_valid(document) || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *macros = cJSON_CreateArray();
    if (root == NULL || macros == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", (double)document->set.schema_version) ==
            NULL ||
        cJSON_AddStringToObject(root, "id", document->set.id.value) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)document->set.revision) == NULL ||
        cJSON_AddStringToObject(root, "name", document->set.name) == NULL ||
        !cJSON_AddItemToObject(root, "macros", macros)) {
        cJSON_Delete(macros);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }

    app_error_code_t result = APP_ERROR_NONE;
    for (size_t index = 0U; result == APP_ERROR_NONE && index < document->macro_count; ++index) {
        result = add_stored_macro(macros, &document->macros[index]);
    }
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        json = cJSON_PrintUnformatted(root);
        result = json == NULL ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t length = strlen(json);
    if (length == 0U) {
        cJSON_free(json);
        return APP_ERROR_INTERNAL;
    }
    /* Over the per-set byte budget is a storage-capacity refusal (507), not a
     * malformed object (SPEC 10.7, 17). */
    if (length > STORAGE_SET_FILE_MAX_BYTES) {
        cJSON_free(json);
        return APP_ERROR_STORAGE_FULL;
    }
    *out_json = json;
    *out_length = length;
    return APP_ERROR_NONE;
}
