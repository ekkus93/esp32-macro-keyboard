#include "storage.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"

#define QUARANTINE_RECORD_MAX_BYTES 1024U

static app_error_code_t map_file_error(void)
{
    if (errno == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (errno == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static bool safe_source_path(const char *path)
{
    const size_t prefix_length = strlen(STORAGE_DATA_MOUNT "/");
    return path != NULL && strncmp(path, STORAGE_DATA_MOUNT "/", prefix_length) == 0 &&
           strncmp(path, STORAGE_DATA_MOUNT "/quarantine/",
                   strlen(STORAGE_DATA_MOUNT "/quarantine/")) != 0 &&
           strstr(path, "..") == NULL;
}

static app_error_code_t record_path(const app_uuid_t *id,
                                    const char *suffix,
                                    char *path,
                                    size_t path_size)
{
    if (id == NULL || suffix == NULL || path == NULL ||
        !app_uuid_is_valid_string(id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(path, path_size, STORAGE_DATA_MOUNT "/quarantine/%s.%s",
                                 id->value, suffix);
    return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE
                                                       : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t serialize_entry(const storage_quarantine_entry_t *entry,
                                        char **out_json,
                                        size_t *out_length)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || cJSON_AddNumberToObject(root, "schema_version", 1.0) == NULL ||
        cJSON_AddStringToObject(root, "id", entry->id.value) == NULL ||
        cJSON_AddStringToObject(root, "source_path", entry->source_path) == NULL ||
        cJSON_AddStringToObject(root, "evidence_path", entry->evidence_path) == NULL ||
        cJSON_AddStringToObject(root, "reason", entry->reason) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(json);
    if (length == 0U || length > QUARANTINE_RECORD_MAX_BYTES) {
        cJSON_free(json);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_json = json;
    *out_length = length;
    return APP_ERROR_NONE;
}

static app_error_code_t parse_entry(const char *data,
                                    size_t length,
                                    storage_quarantine_entry_t *out_entry)
{
    cJSON *root = cJSON_ParseWithLength(data, length);
    const cJSON *version = root == NULL ? NULL :
        cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    const cJSON *id = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *source = root == NULL ? NULL :
        cJSON_GetObjectItemCaseSensitive(root, "source_path");
    const cJSON *evidence = root == NULL ? NULL :
        cJSON_GetObjectItemCaseSensitive(root, "evidence_path");
    const cJSON *reason = root == NULL ? NULL :
        cJSON_GetObjectItemCaseSensitive(root, "reason");
    if (!cJSON_IsObject(root) || !cJSON_IsNumber(version) || version->valueint != 1 ||
        !cJSON_IsString(id) || id->valuestring == NULL ||
        !cJSON_IsString(source) || source->valuestring == NULL ||
        !cJSON_IsString(evidence) || evidence->valuestring == NULL ||
        !cJSON_IsString(reason) || reason->valuestring == NULL ||
        strlen(source->valuestring) >= sizeof(out_entry->source_path) ||
        strlen(evidence->valuestring) >= sizeof(out_entry->evidence_path) ||
        strlen(reason->valuestring) >= sizeof(out_entry->reason) ||
        app_uuid_parse(id->valuestring, &out_entry->id) != APP_ERROR_NONE) {
        cJSON_Delete(root);
        memset(out_entry, 0, sizeof(*out_entry));
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int source_length = snprintf(out_entry->source_path,
                                       sizeof(out_entry->source_path),
                                       "%s", source->valuestring);
    const int evidence_length = snprintf(out_entry->evidence_path,
                                         sizeof(out_entry->evidence_path),
                                         "%s", evidence->valuestring);
    const int reason_length = snprintf(out_entry->reason,
                                       sizeof(out_entry->reason),
                                       "%s", reason->valuestring);
    cJSON_Delete(root);
    if (source_length < 0 || (size_t)source_length >= sizeof(out_entry->source_path) ||
        evidence_length < 0 || (size_t)evidence_length >= sizeof(out_entry->evidence_path) ||
        reason_length < 0 || (size_t)reason_length >= sizeof(out_entry->reason)) {
        memset(out_entry, 0, sizeof(*out_entry));
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_quarantine_file(const char *source_path,
                                         const char *reason,
                                         storage_quarantine_entry_t *out_entry)
{
    if (!safe_source_path(source_path) || reason == NULL || reason[0] == '\0' ||
        strlen(reason) >= STORAGE_QUARANTINE_REASON_MAX_BYTES || out_entry == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    struct stat metadata;
    if (stat(source_path, &metadata) != 0) {
        return map_file_error();
    }
    if (!S_ISREG(metadata.st_mode)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    storage_quarantine_entry_t entry = {0};
    app_error_code_t result = app_uuid_generate(&entry.id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const int source_length = snprintf(entry.source_path, sizeof(entry.source_path),
                                       "%s", source_path);
    const int reason_length = snprintf(entry.reason, sizeof(entry.reason), "%s", reason);
    if (source_length < 0 || (size_t)source_length >= sizeof(entry.source_path) ||
        reason_length < 0 || (size_t)reason_length >= sizeof(entry.reason)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    result = record_path(&entry.id, "evidence", entry.evidence_path,
                         sizeof(entry.evidence_path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char record[APP_PATH_MAX_BYTES];
    result = record_path(&entry.id, "json", record, sizeof(record));
    if (result != APP_ERROR_NONE) {
        return result;
    }

    char *json = NULL;
    size_t json_length = 0U;
    result = serialize_entry(&entry, &json, &json_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = storage_atomic_write(record, json, json_length, true);
    cJSON_free(json);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (rename(source_path, entry.evidence_path) != 0) {
        const app_error_code_t rename_result = map_file_error();
        if (unlink(record) != 0 && errno != ENOENT) {
            return APP_ERROR_IO;
        }
        return rename_result;
    }
    *out_entry = entry;
    return APP_ERROR_NONE;
}

static int compare_entries(const void *left, const void *right)
{
    const storage_quarantine_entry_t *left_entry = left;
    const storage_quarantine_entry_t *right_entry = right;
    return strcmp(left_entry->id.value, right_entry->id.value);
}

static app_error_code_t read_record(const char *path, storage_quarantine_entry_t *out_entry)
{
    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return map_file_error();
    }
    if (metadata.st_size < 0 || (uint64_t)metadata.st_size > QUARANTINE_RECORD_MAX_BYTES) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return map_file_error();
    }
    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    if (data == NULL) {
        if (fclose(file) != 0) {
            return APP_ERROR_IO;
        }
        return APP_ERROR_INTERNAL;
    }
    const size_t count = fread(data, 1U, length, file);
    app_error_code_t result = count == length ? APP_ERROR_NONE : APP_ERROR_IO;
    if (fclose(file) != 0 && result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    if (result == APP_ERROR_NONE) {
        data[length] = '\0';
        result = parse_entry(data, length, out_entry);
    }
    free(data);
    return result;
}

app_error_code_t storage_quarantine_list(storage_quarantine_list_t *out_list)
{
    if (out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    DIR *directory = opendir(STORAGE_DATA_MOUNT "/quarantine");
    if (directory == NULL) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            break;
        }
        const size_t name_length = strlen(entry->d_name);
        if (name_length < 6U || strcmp(entry->d_name + name_length - 5U, ".json") != 0) {
            continue;
        }
        if (out_list->count >= STORAGE_QUARANTINE_MAX_ENTRIES) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        char path[APP_PATH_MAX_BYTES];
        const int written = snprintf(path, sizeof(path),
                                     STORAGE_DATA_MOUNT "/quarantine/%s",
                                     entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        result = read_record(path, &out_list->items[out_list->count]);
        if (result != APP_ERROR_NONE) {
            break;
        }
        ++out_list->count;
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    if (result == APP_ERROR_NONE) {
        qsort(out_list->items, out_list->count, sizeof(out_list->items[0]), compare_entries);
    } else {
        memset(out_list, 0, sizeof(*out_list));
    }
    return result;
}
