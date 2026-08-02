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

/* Every field is required: a macro always belongs to a set, so `set_id` is no
 * longer optional and the `scope` discriminator is gone (SPEC §12.2). */
#define MACRO_FIELD_COUNT 8U
#define MACRO_REQUIRED_FIELD_COUNT 8U
#define ORDER_FIELD_COUNT 2U
#define KEY_PRESS_STORAGE_MAX_MS 1000U

static const char *const MACRO_FIELDS[MACRO_FIELD_COUNT] = {
    "schema_version", "id", "revision", "set_id", "name", "source", "key_press_ms", "inter_key_ms",
};
static const char *const ORDER_FIELDS[ORDER_FIELD_COUNT] = {"schema_version", "ids"};

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
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE || !macro_shape_valid(out_macro)) {
        macro_model_free_macro(out_macro);
        memset(out_macro, 0, sizeof(*out_macro));
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    return APP_ERROR_NONE;
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

static app_error_code_t add_uuid_array(cJSON *root, const char *name, const app_uuid_t *items,
                                       size_t count) {
    cJSON *array = cJSON_CreateArray();
    if (array == NULL || !cJSON_AddItemToObject(root, name, array)) {
        cJSON_Delete(array);
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < count; ++index) {
        cJSON *item = cJSON_CreateString(items[index].value);
        if (item == NULL || !cJSON_AddItemToArray(array, item)) {
            cJSON_Delete(item);
            return APP_ERROR_INTERNAL;
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_parse_order_json(const char *data, size_t length,
                                                     storage_uuid_order_t *out_order,
                                                     size_t maximum_count) {
    if (out_order != NULL) {
        memset(out_order, 0, sizeof(*out_order));
    }
    if (data == NULL || out_order == NULL || maximum_count > STORAGE_ORDER_MAX_IDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result =
        storage_json_parse_exact_object(data, length, ORDER_FIELDS, ORDER_FIELD_COUNT, &root);
    uint32_t schema_version = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &schema_version);
    }
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (result == APP_ERROR_NONE && !cJSON_IsArray(ids)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = result == APP_ERROR_NONE ? cJSON_GetArraySize(ids) : -1;
    if (result == APP_ERROR_NONE && (count < 0 || (size_t)count > maximum_count)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_order->ids[(size_t)index]) != APP_ERROR_NONE) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&out_order->ids[(size_t)prior], &out_order->ids[(size_t)index])) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
        }
    }
    if (result == APP_ERROR_NONE) {
        out_order->count = (size_t)count;
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        memset(out_order, 0, sizeof(*out_order));
    }
    return result;
}

app_error_code_t storage_repository_serialize_order_json(const storage_uuid_order_t *order,
                                                         size_t maximum_count, char **out_json,
                                                         size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (order == NULL || out_json == NULL || out_length == NULL ||
        maximum_count > STORAGE_ORDER_MAX_IDS || order->count > maximum_count) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < order->count; ++index) {
        if (!app_uuid_is_valid_string(order->ids[index].value)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&order->ids[prior], &order->ids[index])) {
                return APP_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", APP_SCHEMA_VERSION) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = add_uuid_array(root, "ids", order->ids, order->count);
    if (result == APP_ERROR_NONE) {
        result = print_bounded_json(root, STORAGE_ORDER_FILE_MAX_BYTES, out_json, out_length);
    }
    cJSON_Delete(root);
    return result;
}
