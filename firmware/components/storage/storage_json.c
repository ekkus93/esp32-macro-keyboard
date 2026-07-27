#include "storage_json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"

#define STORAGE_JSON_MAX_FIELDS 64U

static bool contains_embedded_nul_escape(const char *data, size_t length) {
    static const char NUL_ESCAPE[] = "\\u0000";
    const size_t escape_length = sizeof(NUL_ESCAPE) - 1U;
    if (length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(data + index, NUL_ESCAPE, escape_length) == 0) {
            return true;
        }
    }
    return false;
}

static bool field_name_allowed(const char *name, const char *const *field_names, size_t field_count,
                               size_t *out_index) {
    if (name == NULL) {
        return false;
    }
    for (size_t index = 0U; index < field_count; ++index) {
        if (strcmp(name, field_names[index]) == 0) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return true;
        }
    }
    return false;
}

app_error_code_t storage_json_parse_object_fields(const char *data, size_t length,
                                                  const char *const *field_names,
                                                  size_t field_count, size_t required_count,
                                                  cJSON **out_root) {
    if (out_root != NULL) {
        *out_root = NULL;
    }
    if (data == NULL || length == 0U || field_names == NULL || field_count == 0U ||
        field_count > STORAGE_JSON_MAX_FIELDS || required_count > field_count || out_root == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (contains_embedded_nul_escape(data, length)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, 0);
    if (!cJSON_IsObject(root) || parse_end != data + length) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    uint64_t seen = UINT64_C(0);
    app_error_code_t result = APP_ERROR_NONE;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        size_t index = 0U;
        if (!field_name_allowed(item->string, field_names, field_count, &index)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        const uint64_t bit = UINT64_C(1) << index;
        if ((seen & bit) != 0U) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        seen |= bit;
    }
    if (result == APP_ERROR_NONE) {
        for (size_t index = 0U; index < required_count; ++index) {
            if ((seen & (UINT64_C(1) << index)) == 0U) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_Delete(root);
        return result;
    }
    *out_root = root;
    return APP_ERROR_NONE;
}

app_error_code_t storage_json_parse_exact_object(const char *data, size_t length,
                                                 const char *const *field_names, size_t field_count,
                                                 cJSON **out_root) {
    return storage_json_parse_object_fields(data, length, field_names, field_count, field_count,
                                            out_root);
}

app_error_code_t storage_json_get_string(const cJSON *object, const char *name, char *destination,
                                         size_t destination_size, bool require_nonempty) {
    if (object == NULL || name == NULL || destination == NULL || destination_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(destination, 0, destination_size);
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsString(value) || value->valuestring == NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const size_t length = strlen(value->valuestring);
    if ((require_nonempty && length == 0U) || length >= destination_size) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(destination, value->valuestring, length + 1U);
    return APP_ERROR_NONE;
}

app_error_code_t storage_json_get_allocated_string(const cJSON *object, const char *name,
                                                   size_t maximum, bool require_nonempty,
                                                   char **out_value, size_t *out_length) {
    if (out_value != NULL) {
        *out_value = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (object == NULL || name == NULL || out_value == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsString(value) || value->valuestring == NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const size_t length = strlen(value->valuestring);
    if ((require_nonempty && length == 0U) || length > maximum) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char *copy = malloc(length + 1U);
    if (copy == NULL) {
        return APP_ERROR_INTERNAL;
    }
    memcpy(copy, value->valuestring, length + 1U);
    *out_value = copy;
    *out_length = length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_json_get_u32(const cJSON *object, const char *name, uint32_t minimum,
                                      uint32_t maximum, uint32_t *out_value) {
    if (object == NULL || name == NULL || out_value == NULL || minimum > maximum) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_value = 0U;
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsNumber(value) || value->valuedouble < (double)minimum ||
        value->valuedouble > (double)maximum ||
        value->valuedouble != (double)(uint32_t)value->valuedouble) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_value = (uint32_t)value->valuedouble;
    return APP_ERROR_NONE;
}

app_error_code_t storage_json_get_i32(const cJSON *object, const char *name, int32_t *out_value) {
    if (object == NULL || name == NULL || out_value == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_value = 0;
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsNumber(value) || value->valuedouble < (double)INT32_MIN ||
        value->valuedouble > (double)INT32_MAX ||
        value->valuedouble != (double)(int32_t)value->valuedouble) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_value = (int32_t)value->valuedouble;
    return APP_ERROR_NONE;
}

app_error_code_t storage_json_get_bool(const cJSON *object, const char *name, bool *out_value) {
    if (object == NULL || name == NULL || out_value == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_value = false;
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsBool(value)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_value = cJSON_IsTrue(value);
    return APP_ERROR_NONE;
}

app_error_code_t storage_json_get_uuid(const cJSON *object, const char *name,
                                       app_uuid_t *out_value) {
    if (out_value != NULL) {
        memset(out_value, 0, sizeof(*out_value));
    }
    if (object == NULL || name == NULL || out_value == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char text[APP_UUID_BUFFER_LENGTH];
    app_error_code_t result = storage_json_get_string(object, name, text, sizeof(text), true);
    if (result == APP_ERROR_NONE && app_uuid_parse(text, out_value) != APP_ERROR_NONE) {
        memset(out_value, 0, sizeof(*out_value));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

bool storage_json_has_field(const cJSON *object, const char *name) {
    return object != NULL && name != NULL && cJSON_GetObjectItemCaseSensitive(object, name) != NULL;
}
