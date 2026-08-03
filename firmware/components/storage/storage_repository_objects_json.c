#include "storage_repository_objects_json.h"

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
#include "storage_object_json.h"

/* Package macro entries carry `set_id` as the envelope field SPEC 12.2 permits.
 * Stored macros do not: they live inline in their set file, which identifies the
 * set on its own. See storage_object_json.h. */
#define MACRO_FIELD_COUNT 8U
#define MACRO_REQUIRED_FIELD_COUNT 8U
#define KEY_PRESS_STORAGE_MAX_MS 1000U

static const char *const MACRO_FIELDS[MACRO_FIELD_COUNT] = {
    "schema_version", "id", "revision", "set_id", "name", "source", "key_press_ms", "inter_key_ms",
};

static bool canonical_buffer(const char *value, size_t capacity, size_t *out_length) {
    if (value == NULL || capacity == 0U || out_length == NULL) {
        return false;
    }
    const void *terminator = memchr(value, '\0', capacity);
    if (terminator == NULL) {
        return false;
    }
    const size_t length = (size_t)((const char *)terminator - value);
    for (size_t index = length + 1U; index < capacity; ++index) {
        if (value[index] != '\0') {
            return false;
        }
    }
    *out_length = length;
    return true;
}

static bool macro_shape_valid(const macro_t *macro) {
    if (macro == NULL || macro->schema_version != APP_SCHEMA_VERSION || macro->revision == 0U ||
        !app_uuid_is_valid_string(macro->id.value) || macro->source == NULL ||
        macro->source_length > APP_MACRO_SOURCE_MAX_BYTES ||
        macro->source[macro->source_length] != '\0' ||
        strlen(macro->source) != macro->source_length || macro->key_press_ms == 0U ||
        macro->key_press_ms > KEY_PRESS_STORAGE_MAX_MS || macro->inter_key_ms > APP_DELAY_MAX_MS) {
        return false;
    }
    size_t name_length = 0U;
    if (!canonical_buffer(macro->name, sizeof(macro->name), &name_length) || name_length == 0U) {
        return false;
    }
    return app_uuid_is_valid_string(macro->set_id.value);
}

/* Parse an object that is already a node of a larger document. The package
 * validator walks one cJSON tree instead of re-parsing every element from its
 * own text, which is all the hand-rolled scanner it replaced ever did. */
app_error_code_t storage_repository_parse_macro_node(const struct cJSON *root, macro_t *out_macro) {
    if (out_macro != NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
    }
    if (root == NULL || out_macro == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = storage_json_check_object_fields(
        root, MACRO_FIELDS, MACRO_FIELD_COUNT, MACRO_REQUIRED_FIELD_COUNT);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_macro->schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "id", &out_macro->id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "revision", 1U, UINT32_MAX, &out_macro->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "set_id", &out_macro->set_id);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_string(root, "name", out_macro->name, sizeof(out_macro->name), true);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_allocated_string(root, "source", APP_MACRO_SOURCE_MAX_BYTES, false,
                                              &out_macro->source, &out_macro->source_length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "key_press_ms", 1U, KEY_PRESS_STORAGE_MAX_MS,
                                      &out_macro->key_press_ms);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "inter_key_ms", 0U, APP_DELAY_MAX_MS,
                                      &out_macro->inter_key_ms);
    }
    if (result != APP_ERROR_NONE || !macro_shape_valid(out_macro)) {
        macro_model_free_macro(out_macro);
        memset(out_macro, 0, sizeof(*out_macro));
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_parse_macro_json(const char *data, size_t length,
                                                     macro_t *out_macro) {
    if (out_macro != NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
    }
    if (data == NULL || out_macro == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result = storage_json_parse_object_fields(
        data, length, MACRO_FIELDS, MACRO_FIELD_COUNT, MACRO_REQUIRED_FIELD_COUNT, &root);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_macro_node(root, out_macro);
    }
    cJSON_Delete(root);
    return result;
}

static app_error_code_t add_macro_fields(cJSON *root, const macro_t *macro) {
    if (cJSON_AddNumberToObject(root, "schema_version", (double)macro->schema_version) == NULL ||
        cJSON_AddStringToObject(root, "id", macro->id.value) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)macro->revision) == NULL ||
        cJSON_AddStringToObject(root, "set_id", macro->set_id.value) == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return cJSON_AddStringToObject(root, "name", macro->name) != NULL &&
                   cJSON_AddStringToObject(root, "source", macro->source) != NULL &&
                   cJSON_AddNumberToObject(root, "key_press_ms", (double)macro->key_press_ms) !=
                       NULL &&
                   cJSON_AddNumberToObject(root, "inter_key_ms", (double)macro->inter_key_ms) !=
                       NULL
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static app_error_code_t print_bounded_json(cJSON *root, size_t maximum, char **out_json,
                                           size_t *out_length) {
    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(json);
    if (length == 0U || length > maximum) {
        cJSON_free(json);
        return APP_ERROR_MACRO_LIMIT;
    }
    *out_json = json;
    *out_length = length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_serialize_macro_json(const macro_t *macro, char **out_json,
                                                         size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (!macro_shape_valid(macro) || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = add_macro_fields(root, macro);
    if (result == APP_ERROR_NONE) {
        result = print_bounded_json(root, STORAGE_MACRO_FILE_MAX_BYTES, out_json, out_length);
    }
    cJSON_Delete(root);
    return result;
}

/* The package and API envelope form of a set: metadata only, with no `macros`
 * array. A package keeps sets and macros in sibling arrays, and a set list
 * response carries no macros at all, so neither wants the stored set-file shape
 * that holds macros inline (SPEC 12.1). Reading and writing a set file goes
 * through storage_package_document_parse / _serialize instead. */
#define SET_ENVELOPE_FIELD_COUNT 4U

static const char *const SET_ENVELOPE_FIELDS[SET_ENVELOPE_FIELD_COUNT] = {
    "schema_version",
    "id",
    "revision",
    "name",
};

app_error_code_t storage_repository_parse_package_node(const struct cJSON *root,
                                                       macro_package_t *out_package) {
    if (out_package != NULL) {
        memset(out_package, 0, sizeof(*out_package));
    }
    if (root == NULL || out_package == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = storage_json_check_object_fields(
        root, SET_ENVELOPE_FIELDS, SET_ENVELOPE_FIELD_COUNT, SET_ENVELOPE_FIELD_COUNT);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_package->schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "id", &out_package->id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "revision", 1U, UINT32_MAX, &out_package->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_string(root, "name", out_package->name, sizeof(out_package->name),
                                         true);
    }
    if (result != APP_ERROR_NONE) {
        memset(out_package, 0, sizeof(*out_package));
    }
    return result;
}

app_error_code_t storage_repository_parse_package_json(const char *data, size_t length,
                                                       macro_package_t *out_package) {
    if (out_package != NULL) {
        memset(out_package, 0, sizeof(*out_package));
    }
    if (data == NULL || out_package == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result = storage_json_parse_exact_object(data, length, SET_ENVELOPE_FIELDS,
                                                              SET_ENVELOPE_FIELD_COUNT, &root);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_package_node(root, out_package);
    }
    cJSON_Delete(root);
    return result;
}

app_error_code_t storage_repository_serialize_package_json(const macro_package_t *set,
                                                           char **out_json, size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (set == NULL || out_json == NULL || out_length == NULL ||
        set->schema_version != APP_SCHEMA_VERSION || set->revision == 0U ||
        !app_uuid_is_valid_string(set->id.value) || set->name[0] == '\0') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", (double)set->schema_version) == NULL ||
        cJSON_AddStringToObject(root, "id", set->id.value) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)set->revision) == NULL ||
        cJSON_AddStringToObject(root, "name", set->name) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    const app_error_code_t result =
        print_bounded_json(root, STORAGE_MACRO_FILE_MAX_BYTES, out_json, out_length);
    cJSON_Delete(root);
    return result;
}
