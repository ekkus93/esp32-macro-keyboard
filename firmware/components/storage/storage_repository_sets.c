#include "storage_repository.h"
#include "storage_repository_internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "storage.h"

app_error_code_t storage_set_read(const app_uuid_t *set_id, macro_set_t *out_set)
{
    if (set_id == NULL || out_set == NULL || !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_repository_set_file_path(set_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char *data = NULL;
    size_t length = 0U;
    result = storage_repository_read_bounded_file(path, STORAGE_SET_FILE_MAX_BYTES, &data, &length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = storage_repository_parse_set_json(data, length, out_set);
    free(data);
    if (result == APP_ERROR_NONE && !app_uuid_equal(set_id, &out_set->id)) {
        memset(out_set, 0, sizeof(*out_set));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        storage_quarantine_entry_t entry = {0};
        const app_error_code_t quarantine_result =
            storage_quarantine_file(path, "invalid set metadata", &entry);
        return quarantine_result == APP_ERROR_NONE ? result : quarantine_result;
    }
    return result;
}

app_error_code_t storage_set_list(storage_set_list_t *out_list)
{
    if (out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    for (size_t item = 0U; item < index.count; ++item) {
        result = storage_set_read(&index.ids[item], &out_list->items[item]);
        if (result != APP_ERROR_NONE) {
            memset(out_list, 0, sizeof(*out_list));
            return result;
        }
    }
    out_list->count = index.count;
    return APP_ERROR_NONE;
}

static app_error_code_t storage_repository_create_set_staging(const macro_set_t *set,
                                           const app_uuid_t *transaction_id,
                                           char *staging,
                                           size_t staging_size)
{
    const int staging_length = snprintf(staging, staging_size,
                                        STORAGE_DATA_MOUNT "/staging/%s",
                                        transaction_id->value);
    if (staging_length < 0 || (size_t)staging_length >= staging_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = storage_repository_make_directory(staging);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    static const char *const child_names[] = {"macros", "procedures", "progress"};
    for (size_t child = 0U; child < (sizeof(child_names) / sizeof(child_names[0])); ++child) {
        char path[APP_PATH_MAX_BYTES];
        const int written = snprintf(path, sizeof(path), "%s/%s", staging, child_names[child]);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        result = storage_repository_make_directory(path);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }

    char *json = NULL;
    size_t json_length = 0U;
    result = storage_repository_serialize_set_json(set, &json, &json_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char path[APP_PATH_MAX_BYTES];
    int written = snprintf(path, sizeof(path), "%s/set.json", staging);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        cJSON_free(json);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    result = storage_atomic_write(path, json, json_length, true);
    cJSON_free(json);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    static const char empty_order[] = "{\"schema_version\":1,\"ids\":[]}";
    const char *const order_names[] = {"macro-order.json", "procedure-order.json"};
    for (size_t order = 0U; order < (sizeof(order_names) / sizeof(order_names[0])); ++order) {
        written = snprintf(path, sizeof(path), "%s/%s", staging, order_names[order]);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        result = storage_atomic_write(path, empty_order, strlen(empty_order), true);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_set_create(const macro_set_t *set)
{
    if (set == NULL || set->revision != 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (index.count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t item = 0U; item < index.count; ++item) {
        if (app_uuid_equal(&index.ids[item], &set->id)) {
            return APP_ERROR_CONFLICT;
        }
    }

    char destination[APP_PATH_MAX_BYTES];
    result = storage_make_set_path(&set->id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    const int destination_stat = stat(destination, &metadata);
    if (destination_stat == 0) {
        return APP_ERROR_CONFLICT;
    }
    if (errno != ENOENT) {
        return storage_repository_map_file_error();
    }

    app_uuid_t transaction_id = {0};
    result = app_uuid_generate(&transaction_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char staging[APP_PATH_MAX_BYTES] = {0};
    result = storage_repository_create_set_staging(set, &transaction_id, staging, sizeof(staging));
    if (result != APP_ERROR_NONE) {
        if (staging[0] != '\0') {
            const app_error_code_t cleanup_result = storage_repository_remove_tree(staging);
            if (cleanup_result != APP_ERROR_NONE) {
                return cleanup_result;
            }
        }
        return result;
    }

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_IMPORT_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .expected_revision = 0U,
        .replacement_revision = set->revision,
    };
    const int staging_copy = snprintf(manifest.staging, sizeof(manifest.staging), "%s", staging);
    const int destination_copy =
        snprintf(manifest.destination, sizeof(manifest.destination), "%s", destination);
    if (staging_copy < 0 || (size_t)staging_copy >= sizeof(manifest.staging) ||
        destination_copy < 0 || (size_t)destination_copy >= sizeof(manifest.destination)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup_result = storage_repository_remove_tree(staging);
        return cleanup_result == APP_ERROR_NONE ? result : cleanup_result;
    }
    if (rename(staging, destination) != 0) {
        return storage_repository_map_file_error();
    }
    manifest.phase = STORAGE_TRANSACTION_ACTIVATED;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    index.ids[index.count++] = set->id;
    result = storage_repository_write_index(&index);
    if (result != APP_ERROR_NONE) {
        if (rename(destination, staging) != 0) {
            return APP_ERROR_IO;
        }
        return result;
    }
    manifest.phase = STORAGE_TRANSACTION_INDEXED;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return storage_repository_remove_manifest(&transaction_id);
}

app_error_code_t storage_set_update(const macro_set_t *replacement,
                                    uint32_t expected_revision,
                                    macro_set_t *out_updated)
{
    if (replacement == NULL || out_updated == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_set_t current = {0};
    app_error_code_t result = storage_set_read(&replacement->id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision || replacement->revision != expected_revision) {
        return APP_ERROR_CONFLICT;
    }
    macro_set_t updated = *replacement;
    if (updated.revision == UINT32_MAX) {
        return APP_ERROR_CONFLICT;
    }
    ++updated.revision;

    char *json = NULL;
    size_t json_length = 0U;
    result = storage_repository_serialize_set_json(&updated, &json, &json_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char path[APP_PATH_MAX_BYTES];
    result = storage_repository_set_file_path(&updated.id, path, sizeof(path));
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, json_length, true);
    }
    cJSON_free(json);
    if (result == APP_ERROR_NONE) {
        *out_updated = updated;
    }
    return result;
}

app_error_code_t storage_set_delete(const app_uuid_t *set_id, uint32_t expected_revision)
{
    if (set_id == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_set_t current = {0};
    app_error_code_t result = storage_set_read(set_id, &current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (current.revision != expected_revision) {
        return APP_ERROR_CONFLICT;
    }
    storage_set_index_t index = {0};
    result = storage_repository_load_index(&index);
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
    if (found == index.count) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    app_uuid_t transaction_id = {0};
    result = app_uuid_generate(&transaction_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char source[APP_PATH_MAX_BYTES];
    result = storage_make_set_path(set_id, source, sizeof(source));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char trash[APP_PATH_MAX_BYTES];
    const int trash_length = snprintf(trash, sizeof(trash),
                                      STORAGE_DATA_MOUNT "/trash/%s-%s",
                                      set_id->value, transaction_id.value);
    if (trash_length < 0 || (size_t)trash_length >= sizeof(trash)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_DELETE_SET,
        .phase = STORAGE_TRANSACTION_PREPARED,
        .expected_revision = expected_revision,
        .replacement_revision = 0U,
    };
    const int source_copy = snprintf(manifest.source, sizeof(manifest.source), "%s", source);
    const int trash_copy = snprintf(manifest.backup, sizeof(manifest.backup), "%s", trash);
    if (source_copy < 0 || (size_t)source_copy >= sizeof(manifest.source) ||
        trash_copy < 0 || (size_t)trash_copy >= sizeof(manifest.backup)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (rename(source, trash) != 0) {
        return storage_repository_map_file_error();
    }
    manifest.phase = STORAGE_TRANSACTION_BACKED_UP;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    for (size_t item = found; item + 1U < index.count; ++item) {
        index.ids[item] = index.ids[item + 1U];
    }
    --index.count;
    result = storage_repository_write_index(&index);
    if (result != APP_ERROR_NONE) {
        if (rename(trash, source) != 0) {
            return APP_ERROR_IO;
        }
        return result;
    }
    manifest.phase = STORAGE_TRANSACTION_INDEXED;
    result = storage_transaction_write_manifest(&manifest);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return storage_repository_remove_manifest(&transaction_id);
}
