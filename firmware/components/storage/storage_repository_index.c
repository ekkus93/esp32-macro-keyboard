#include "storage_repository.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_json.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"

/* SPEC 12.3: schema_version, revision, active_set_id, set_ids. `active_set_id`
 * is nullable -- a device has no active set until the user selects one, and
 * SPEC 10.1 forbids inferring one. */
#define INDEX_FIELD_COUNT 4U

static const char *const INDEX_FIELDS[INDEX_FIELD_COUNT] = {
    "schema_version",
    "revision",
    "active_set_id",
    "set_ids",
};

static app_error_code_t parse_set_ids(const cJSON *root, storage_set_index_t *out_index) {
    const cJSON *ids = cJSON_GetObjectItemCaseSensitive(root, "set_ids");
    if (!cJSON_IsArray(ids)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = cJSON_GetArraySize(ids);
    if (count < 0 || (size_t)count > APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_index->ids[(size_t)index]) != APP_ERROR_NONE) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&out_index->ids[(size_t)prior], &out_index->ids[(size_t)index])) {
                return APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    out_index->count = (size_t)count;
    return APP_ERROR_NONE;
}

/* An active set that is not in `set_ids` is a corrupt index, not a stale hint to
 * drop quietly: something wrote one of the two fields without the other, and
 * silently clearing it would hide that from the user (SPEC 12.3, 13.6). */
static app_error_code_t parse_active_set(const cJSON *root, storage_set_index_t *out_index) {
    const cJSON *active = cJSON_GetObjectItemCaseSensitive(root, "active_set_id");
    if (cJSON_IsNull(active)) {
        return APP_ERROR_NONE;
    }
    if (!cJSON_IsString(active) || active->valuestring == NULL ||
        app_uuid_parse(active->valuestring, &out_index->active_set_id) != APP_ERROR_NONE) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t item = 0U; item < out_index->count; ++item) {
        if (app_uuid_equal(&out_index->ids[item], &out_index->active_set_id)) {
            out_index->has_active_set = true;
            return APP_ERROR_NONE;
        }
    }
    return APP_ERROR_STORAGE_CORRUPT;
}

app_error_code_t storage_repository_parse_index(const char *data, size_t length,
                                                storage_set_index_t *out_index) {
    if (out_index == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_index, 0, sizeof(*out_index));
    cJSON *root = NULL;
    app_error_code_t result =
        storage_json_parse_exact_object(data, length, INDEX_FIELDS, INDEX_FIELD_COUNT, &root);
    uint32_t schema_version = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "revision", 1U, UINT32_MAX, &out_index->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_set_ids(root, out_index);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_active_set(root, out_index);
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        memset(out_index, 0, sizeof(*out_index));
    }
    return result;
}

app_error_code_t storage_repository_load_index_path(const char *path,
                                                    storage_set_index_t *out_index) {
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
        const app_error_code_t discard =
            storage_repository_discard_corrupt_file(path, parse_result);
        return discard == APP_ERROR_NONE ? parse_result : discard;
    }
    return parse_result;
}

app_error_code_t storage_repository_load_index(storage_set_index_t *out_index) {
    return storage_repository_load_index_path(STORAGE_INDEX_FILE_PATH, out_index);
}

app_error_code_t storage_repository_write_index(const storage_set_index_t *index) {
    if (index == NULL || index->count > APP_MACRO_SETS_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *ids = cJSON_CreateArray();
    /* revision starts at 1 and advances with every write, so a caller that
     * loaded the index can tell whether it changed underneath. */
    const uint32_t revision = index->revision == UINT32_MAX ? UINT32_MAX : index->revision + 1U;
    if (root == NULL || ids == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", (double)APP_SCHEMA_VERSION) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)revision) == NULL ||
        (index->has_active_set
             ? cJSON_AddStringToObject(root, "active_set_id", index->active_set_id.value) == NULL
             : cJSON_AddNullToObject(root, "active_set_id") == NULL) ||
        !cJSON_AddItemToObject(root, "set_ids", ids)) {
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
    const app_error_code_t result =
        length <= STORAGE_INDEX_FILE_MAX_BYTES
            ? storage_atomic_write(STORAGE_INDEX_FILE_PATH, json, length, true)
            : APP_ERROR_STORAGE_CORRUPT;
    cJSON_free(json);
    return result;
}

static app_error_code_t initialize_fresh_storage(void) {
    static const char empty_index[] =
        "{\"schema_version\":1,\"revision\":1,\"active_set_id\":null,\"set_ids\":[]}";

    bool sets_have_entries = false;
    const app_error_code_t result =
        storage_repository_directory_has_entries(STORAGE_DATA_MOUNT "/sets", &sets_have_entries);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    /* Set files with no index is a corrupt repository, not an empty one:
     * rebuilding the index from the directory would invent a set order the user
     * never chose (SPEC 12.3). */
    if (sets_have_entries) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return storage_repository_ensure_initial_file(STORAGE_INDEX_FILE_PATH, empty_index);
}

static app_error_code_t storage_repository_init_locked(void) {
    struct stat metadata;
    const int index_stat = stat(STORAGE_INDEX_FILE_PATH, &metadata);
    if (index_stat != 0 && errno != ENOENT) {
        return storage_repository_map_file_error();
    }
    if (index_stat != 0) {
        const app_error_code_t result = initialize_fresh_storage();
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    storage_set_index_t index = {0};
    return storage_repository_load_index(&index);
}

app_error_code_t storage_repository_init(void) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_repository_init_locked();
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_repository_deinit(void) {
    /* The repository layer holds no in-memory resources: every index and object is
     * read from and written to the mounted filesystem on demand, and there is no
     * file-scope state, handle, or "initialized" flag to release. Its durable state
     * is released when the caller unmounts the filesystem (storage_unmount_all).
     * The function exists so app_core can reverse the init step uniformly. */
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_set_index_presence(const app_uuid_t *set_id,
                                                       bool should_be_present) {
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
