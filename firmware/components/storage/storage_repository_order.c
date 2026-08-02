#include "storage_repository_order.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"

app_error_code_t storage_repository_load_order_locked(const char *path, size_t maximum_count,
                                                      storage_uuid_order_t *out_order) {
    if (path == NULL || out_order == NULL || maximum_count > STORAGE_ORDER_MAX_IDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_read_bounded_file(path, STORAGE_ORDER_FILE_MAX_BYTES, &data, &length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = storage_repository_parse_order_json(data, length, out_order, maximum_count);
    free(data);
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        const app_error_code_t discard = storage_repository_discard_corrupt_file(path);
        return discard == APP_ERROR_NONE ? result : discard;
    }
    return result;
}

app_error_code_t storage_repository_write_order_locked(const char *path, size_t maximum_count,
                                                       const storage_uuid_order_t *order) {
    if (path == NULL || order == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *json = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_serialize_order_json(order, maximum_count, &json, &length);
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, length, true);
    }
    cJSON_free(json);
    return result;
}

bool storage_repository_order_contains(const storage_uuid_order_t *order, const app_uuid_t *item_id,
                                       size_t *out_index) {
    if (out_index != NULL) {
        *out_index = 0U;
    }
    if (order == NULL || item_id == NULL) {
        return false;
    }
    for (size_t index = 0U; index < order->count; ++index) {
        if (app_uuid_equal(&order->ids[index], item_id)) {
            if (out_index != NULL) {
                *out_index = index;
            }
            return true;
        }
    }
    return false;
}

app_error_code_t storage_repository_order_append(storage_uuid_order_t *order, size_t maximum_count,
                                                 const app_uuid_t *item_id) {
    if (order == NULL || item_id == NULL || maximum_count > STORAGE_ORDER_MAX_IDS ||
        !app_uuid_is_valid_string(item_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (storage_repository_order_contains(order, item_id, NULL)) {
        return APP_ERROR_CONFLICT;
    }
    if (order->count >= maximum_count) {
        return APP_ERROR_STORAGE_FULL;
    }
    order->ids[order->count] = *item_id;
    ++order->count;
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_order_remove(storage_uuid_order_t *order,
                                                 const app_uuid_t *item_id) {
    if (order == NULL || item_id == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    size_t index = 0U;
    if (!storage_repository_order_contains(order, item_id, &index)) {
        return APP_ERROR_NOT_FOUND;
    }
    for (size_t item = index; item + 1U < order->count; ++item) {
        order->ids[item] = order->ids[item + 1U];
    }
    --order->count;
    return APP_ERROR_NONE;
}

bool storage_repository_order_same_members(const storage_uuid_order_t *left,
                                           const storage_uuid_order_t *right) {
    if (left == NULL || right == NULL || left->count != right->count) {
        return false;
    }
    for (size_t index = 0U; index < left->count; ++index) {
        if (!storage_repository_order_contains(right, &left->ids[index], NULL)) {
            return false;
        }
    }
    return true;
}
