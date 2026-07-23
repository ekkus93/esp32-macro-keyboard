#include "storage_repository_internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "storage.h"

static app_error_code_t checked_json_string(const cJSON *object,
                                            const char *name,
                                            char *destination,
                                            size_t destination_size,
                                            bool require_nonempty)
{
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

static app_error_code_t checked_json_u32(const cJSON *object,
                                         const char *name,
                                         uint32_t minimum,
                                         uint32_t *out_value)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, name);
    if (!cJSON_IsNumber(value) || value->valuedouble < (double)minimum ||
        value->valuedouble > (double)UINT32_MAX ||
        value->valuedouble != (double)(uint32_t)value->valuedouble) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_value = (uint32_t)value->valuedouble;
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_parse_set_json(const char *data, size_t length, macro_set_t *out_set)
{
    if (data == NULL || out_set == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_set, 0, sizeof(*out_set));

    cJSON *root = cJSON_ParseWithLength(data, length);
    if (root == NULL || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }

    app_error_code_t result = checked_json_u32(root, "schema_version", 1U,
                                               &out_set->schema_version);
    if (result == APP_ERROR_NONE && out_set->schema_version != APP_SCHEMA_VERSION) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        char id[APP_UUID_BUFFER_LENGTH];
        result = checked_json_string(root, "id", id, sizeof(id), true);
        if (result == APP_ERROR_NONE) {
            result = app_uuid_parse(id, &out_set->id);
            if (result != APP_ERROR_NONE) {
                result = APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_u32(root, "revision", 1U, &out_set->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_string(root, "name", out_set->name, sizeof(out_set->name), true);
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_string(root, "description", out_set->description,
                                     sizeof(out_set->description), false);
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_string(root, "manufacturer", out_set->manufacturer,
                                     sizeof(out_set->manufacturer), false);
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_string(root, "model", out_set->model, sizeof(out_set->model), false);
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_string(root, "board", out_set->board, sizeof(out_set->board), false);
    }
    if (result == APP_ERROR_NONE) {
        result = checked_json_string(root, "keyboard_layout", out_set->keyboard_layout,
                                     sizeof(out_set->keyboard_layout), true);
        if (result == APP_ERROR_NONE && strcmp(out_set->keyboard_layout, "en-US") != 0) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
    }
    if (result == APP_ERROR_NONE) {
        const cJSON *sort_order = cJSON_GetObjectItemCaseSensitive(root, "sort_order");
        if (!cJSON_IsNumber(sort_order) || sort_order->valuedouble < (double)INT32_MIN ||
            sort_order->valuedouble > (double)INT32_MAX ||
            sort_order->valuedouble != (double)(int32_t)sort_order->valuedouble) {
            result = APP_ERROR_STORAGE_CORRUPT;
        } else {
            out_set->sort_order = (int32_t)sort_order->valuedouble;
        }
    }

    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        memset(out_set, 0, sizeof(*out_set));
    }
    return result;
}

app_error_code_t storage_repository_serialize_set_json(const macro_set_t *set, char **out_json, size_t *out_length)
{
    if (set == NULL || out_json == NULL || out_length == NULL ||
        set->schema_version != APP_SCHEMA_VERSION || set->revision == 0U ||
        !app_uuid_is_valid_string(set->id.value) || set->name[0] == '\0' ||
        strcmp(set->keyboard_layout, "en-US") != 0) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_json = NULL;
    *out_length = 0U;

    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", (double)set->schema_version) == NULL ||
        cJSON_AddStringToObject(root, "id", set->id.value) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)set->revision) == NULL ||
        cJSON_AddStringToObject(root, "name", set->name) == NULL ||
        cJSON_AddStringToObject(root, "description", set->description) == NULL ||
        cJSON_AddStringToObject(root, "manufacturer", set->manufacturer) == NULL ||
        cJSON_AddStringToObject(root, "model", set->model) == NULL ||
        cJSON_AddStringToObject(root, "board", set->board) == NULL ||
        cJSON_AddStringToObject(root, "keyboard_layout", set->keyboard_layout) == NULL ||
        cJSON_AddNumberToObject(root, "sort_order", (double)set->sort_order) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(json);
    if (length == 0U || length > STORAGE_SET_FILE_MAX_BYTES) {
        cJSON_free(json);
        return APP_ERROR_MACRO_LIMIT;
    }
    *out_json = json;
    *out_length = length;
    return APP_ERROR_NONE;
}
