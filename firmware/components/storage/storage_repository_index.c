#include "storage_repository.h"
#include "storage_repository_internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "storage.h"

static app_error_code_t storage_repository_parse_index(const char *data, size_t length, storage_set_index_t *out_index)
{
    memset(out_index, 0, sizeof(*out_index));
    cJSON *root = cJSON_ParseWithLength(data, length);
    const cJSON *version = root == NULL ? NULL :
        cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (!cJSON_IsObject(root) || !cJSON_IsNumber(version) || version->valueint != 1 ||
        !cJSON_IsArray(ids)) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = cJSON_GetArraySize(ids);
    if (count < 0 || count > (int)APP_MACRO_SETS_MAX) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_index->ids[(size_t)index]) != APP_ERROR_NONE) {
            cJSON_Delete(root);
            memset(out_index, 0, sizeof(*out_index));
            return APP_ERROR_STORAGE_CORRUPT;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&out_index->ids[(size_t)prior],
                               &out_index->ids[(size_t)index])) {
                cJSON_Delete(root);
                memset(out_index, 0, sizeof(*out_index));
                return APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    out_index->count = (size_t)count;
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_load_index_path(const char *path, storage_set_index_t *out_index)
{
    char *data = NULL;
    size_t length = 0U;
    const app_error_code_t read_result =
        storage_repository_read_bounded_file(path, STORAGE_INDEX_FILE_MAX_BYTES, &data, &length);
    if (read_result != APP_ERROR_NONE) {
        return read_result;
    }
    const app_error_code_t parse_result = storage_repository_parse_index(data, length, out_index);
    free(data);
    if (parse_result == APP_ERROR_STORAGE_CORRUPT) {
        storage_quarantine_entry_t entry = {0};
        const app_error_code_t quarantine_result =
            storage_quarantine_file(path, "invalid ordering index", &entry);
        return quarantine_result == APP_ERROR_NONE ? parse_result : quarantine_result;
    }
    return parse_result;
}

app_error_code_t storage_repository_load_index(storage_set_index_t *out_index)
{
    return storage_repository_load_index_path(STORAGE_SET_INDEX_FILE_PATH, out_index);
}

app_error_code_t storage_repository_write_index(const storage_set_index_t *index)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *ids = cJSON_CreateArray();
    if (root == NULL || ids == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", 1.0) == NULL ||
        !cJSON_AddItemToObject(root, "ids", ids)) {
        cJSON_Delete(ids);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    for (size_t item = 0U; item < index->count; ++item) {
        cJSON *value = cJSON_CreateString(index->ids[item].value);
        if (value == NULL || !cJSON_AddItemToArray(ids, value)) {
            cJSON_Delete(value);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(json);
    const app_error_code_t result = length <= STORAGE_INDEX_FILE_MAX_BYTES
                                        ? storage_atomic_write(STORAGE_SET_INDEX_FILE_PATH, json, length, true)
                                        : APP_ERROR_STORAGE_CORRUPT;
    cJSON_free(json);
    return result;
}

app_error_code_t storage_repository_init(void)
{
    static const char schema[] = "{\"schema_version\":1}";
    static const char empty_index[] = "{\"schema_version\":1,\"ids\":[]}";

    struct stat schema_metadata;
    const int schema_stat = stat(STORAGE_SCHEMA_FILE_PATH, &schema_metadata);
    if (schema_stat != 0 && errno != ENOENT) {
        return storage_repository_map_file_error();
    }
    if (schema_stat != 0) {
        struct stat index_metadata;
        const bool index_exists = stat(STORAGE_SET_INDEX_FILE_PATH, &index_metadata) == 0;
        if (!index_exists && errno != ENOENT) {
            return storage_repository_map_file_error();
        }
        struct stat global_metadata;
        const bool global_order_exists = stat(STORAGE_GLOBAL_ORDER_FILE_PATH, &global_metadata) == 0;
        if (!global_order_exists && errno != ENOENT) {
            return storage_repository_map_file_error();
        }
        if (index_exists || global_order_exists ||
            storage_repository_directory_has_entries(STORAGE_DATA_MOUNT "/sets") ||
            storage_repository_directory_has_entries(STORAGE_DATA_MOUNT "/global/macros")) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        app_error_code_t result = storage_repository_ensure_initial_file(STORAGE_SCHEMA_FILE_PATH, schema);
        if (result == APP_ERROR_NONE) {
            result = storage_repository_ensure_initial_file(STORAGE_SET_INDEX_FILE_PATH, empty_index);
        }
        if (result == APP_ERROR_NONE) {
            result = storage_repository_ensure_initial_file(STORAGE_GLOBAL_ORDER_FILE_PATH, empty_index);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
    } else {
        struct stat metadata;
        if (stat(STORAGE_SET_INDEX_FILE_PATH, &metadata) != 0 ||
            stat(STORAGE_GLOBAL_ORDER_FILE_PATH, &metadata) != 0) {
            return errno == ENOENT ? APP_ERROR_STORAGE_CORRUPT : storage_repository_map_file_error();
        }
    }

    char *schema_data = NULL;
    size_t schema_length = 0U;
    app_error_code_t result =
        storage_repository_read_bounded_file(STORAGE_SCHEMA_FILE_PATH, 128U, &schema_data, &schema_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    cJSON *schema_root = cJSON_ParseWithLength(schema_data, schema_length);
    free(schema_data);
    const cJSON *schema_version = schema_root == NULL ? NULL :
        cJSON_GetObjectItemCaseSensitive(schema_root, "schema_version");
    const bool schema_valid = cJSON_IsObject(schema_root) && cJSON_IsNumber(schema_version) &&
                              schema_version->valueint == (int)APP_SCHEMA_VERSION;
    cJSON_Delete(schema_root);
    if (!schema_valid) {
        storage_quarantine_entry_t entry = {0};
        const app_error_code_t quarantine_result =
            storage_quarantine_file(STORAGE_SCHEMA_FILE_PATH, "invalid storage schema marker", &entry);
        return quarantine_result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT
                                                    : quarantine_result;
    }

    storage_set_index_t index = {0};
    result = storage_repository_load_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_set_index_t global_order = {0};
    return storage_repository_load_index_path(STORAGE_GLOBAL_ORDER_FILE_PATH, &global_order);
}

app_error_code_t storage_repository_set_index_presence(const app_uuid_t *set_id,
                                                       bool should_be_present)
{
    if (set_id == NULL || !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    size_t found = index.count;
    for (size_t item = 0U; item < index.count; ++item) {
        if (app_uuid_equal(&index.ids[item], set_id)) {
            found = item;
            break;
        }
    }
    if (should_be_present) {
        if (found < index.count) {
            return APP_ERROR_NONE;
        }
        if (index.count >= APP_MACRO_SETS_MAX) {
            return APP_ERROR_STORAGE_FULL;
        }
        index.ids[index.count++] = *set_id;
    } else {
        if (found == index.count) {
            return APP_ERROR_NONE;
        }
        for (size_t item = found; item + 1U < index.count; ++item) {
            index.ids[item] = index.ids[item + 1U];
        }
        --index.count;
    }
    return storage_repository_write_index(&index);
}
