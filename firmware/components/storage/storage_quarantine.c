#include "storage.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cJSON.h"
#include "storage_quarantine_internal.h"

#define QUARANTINE_RECORD_MAX_BYTES 1024U
#define QUARANTINE_ID_ATTEMPTS 4U
#define QUARANTINE_JSON_SUFFIX ".json"
#define QUARANTINE_EVIDENCE_SUFFIX ".evidence"

static app_error_code_t map_error_number(int error_number)
{
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static app_error_code_t production_uuid_generate(void *context,
                                                 app_uuid_t *out_uuid)
{
    (void)context;
    return app_uuid_generate(out_uuid);
}

static bool safe_source_path(const char *path)
{
    static const char data_prefix[] = STORAGE_DATA_MOUNT "/";
    static const char quarantine_prefix[] = STORAGE_DATA_MOUNT "/quarantine/";
    if (path == NULL || strncmp(path, data_prefix, sizeof(data_prefix) - 1U) != 0 ||
        strncmp(path, quarantine_prefix, sizeof(quarantine_prefix) - 1U) == 0 ||
        path[sizeof(data_prefix) - 1U] == '\0' || strlen(path) >= APP_PATH_MAX_BYTES ||
        strstr(path, "..") != NULL || strstr(path, "//") != NULL ||
        strchr(path, '\\') != NULL) {
        return false;
    }
    for (const unsigned char *cursor = (const unsigned char *)path;
         *cursor != 0U;
         ++cursor) {
        if (*cursor < 0x20U || *cursor == 0x7FU) {
            return false;
        }
    }
    return true;
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
    const int written = snprintf(path,
                                 path_size,
                                 STORAGE_DATA_MOUNT "/quarantine/%s%s",
                                 id->value,
                                 suffix);
    return written >= 0 && (size_t)written < path_size
               ? APP_ERROR_NONE
               : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t path_exists_with_ops(const char *path,
                                             const storage_fs_ops_t *operations,
                                             bool *out_exists)
{
    if (out_exists != NULL) {
        *out_exists = false;
    }
    if (path == NULL || out_exists == NULL ||
        !storage_fs_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        *out_exists = true;
        return APP_ERROR_NONE;
    }
    const int stat_error = errno;
    return stat_error == ENOENT ? APP_ERROR_NONE : map_error_number(stat_error);
}

static app_error_code_t create_unique_entry_paths(
    storage_quarantine_entry_t *entry,
    char *record,
    size_t record_size,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context)
{
    for (size_t attempt = 0U; attempt < QUARANTINE_ID_ATTEMPTS; ++attempt) {
        memset(&entry->id, 0, sizeof(entry->id));
        app_error_code_t result = generate_uuid(uuid_context, &entry->id);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        result = record_path(&entry->id,
                             QUARANTINE_EVIDENCE_SUFFIX,
                             entry->evidence_path,
                             sizeof(entry->evidence_path));
        if (result == APP_ERROR_NONE) {
            result = record_path(&entry->id,
                                 QUARANTINE_JSON_SUFFIX,
                                 record,
                                 record_size);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }

        bool record_exists = false;
        bool evidence_exists = false;
        result = path_exists_with_ops(record, operations, &record_exists);
        if (result == APP_ERROR_NONE) {
            result = path_exists_with_ops(entry->evidence_path,
                                          operations,
                                          &evidence_exists);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (!record_exists && !evidence_exists) {
            return APP_ERROR_NONE;
        }
    }
    return APP_ERROR_CONFLICT;
}

static app_error_code_t serialize_entry(const storage_quarantine_entry_t *entry,
                                        char **out_json,
                                        size_t *out_length)
{
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (entry == NULL || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", 1.0) == NULL ||
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

static bool object_has_exact_fields(const cJSON *root)
{
    static const char *const names[] = {
        "schema_version", "id", "source_path", "evidence_path", "reason",
    };
    bool found[sizeof(names) / sizeof(names[0])] = {false};
    size_t count = 0U;
    for (const cJSON *child = root == NULL ? NULL : root->child;
         child != NULL;
         child = child->next) {
        if (child->string == NULL) {
            return false;
        }
        bool matched = false;
        for (size_t index = 0U; index < sizeof(names) / sizeof(names[0]); ++index) {
            if (strcmp(child->string, names[index]) == 0) {
                if (found[index]) {
                    return false;
                }
                found[index] = true;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
        ++count;
    }
    if (count != sizeof(names) / sizeof(names[0])) {
        return false;
    }
    for (size_t index = 0U; index < sizeof(found) / sizeof(found[0]); ++index) {
        if (!found[index]) {
            return false;
        }
    }
    return true;
}

static app_error_code_t parse_entry(const char *data,
                                    size_t length,
                                    const app_uuid_t *expected_id,
                                    storage_quarantine_entry_t *out_entry)
{
    if (out_entry != NULL) {
        memset(out_entry, 0, sizeof(*out_entry));
    }
    if (data == NULL || length == 0U || expected_id == NULL ||
        out_entry == NULL || memchr(data, '\0', length) != NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data,
                                           length + 1U,
                                           &parse_end,
                                           true);
    const cJSON *version = root == NULL
                               ? NULL
                               : cJSON_GetObjectItemCaseSensitive(root,
                                                                  "schema_version");
    const cJSON *id = root == NULL
                          ? NULL
                          : cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *source = root == NULL
                              ? NULL
                              : cJSON_GetObjectItemCaseSensitive(root,
                                                                 "source_path");
    const cJSON *evidence = root == NULL
                                ? NULL
                                : cJSON_GetObjectItemCaseSensitive(root,
                                                                   "evidence_path");
    const cJSON *reason = root == NULL
                              ? NULL
                              : cJSON_GetObjectItemCaseSensitive(root, "reason");

    app_uuid_t parsed_id = {0};
    const bool valid = cJSON_IsObject(root) && object_has_exact_fields(root) &&
                       parse_end == data + length && cJSON_IsNumber(version) &&
                       version->valuedouble == 1.0 && cJSON_IsString(id) &&
                       id->valuestring != NULL &&
                       app_uuid_parse(id->valuestring, &parsed_id) == APP_ERROR_NONE &&
                       app_uuid_equal(&parsed_id, expected_id) &&
                       cJSON_IsString(source) && source->valuestring != NULL &&
                       safe_source_path(source->valuestring) &&
                       cJSON_IsString(evidence) && evidence->valuestring != NULL &&
                       cJSON_IsString(reason) && reason->valuestring != NULL &&
                       reason->valuestring[0] != '\0' &&
                       strlen(source->valuestring) < sizeof(out_entry->source_path) &&
                       strlen(evidence->valuestring) < sizeof(out_entry->evidence_path) &&
                       strlen(reason->valuestring) < sizeof(out_entry->reason);
    if (!valid) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }

    char expected_evidence[APP_PATH_MAX_BYTES];
    if (record_path(&parsed_id,
                    QUARANTINE_EVIDENCE_SUFFIX,
                    expected_evidence,
                    sizeof(expected_evidence)) != APP_ERROR_NONE ||
        strcmp(evidence->valuestring, expected_evidence) != 0) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }

    out_entry->id = parsed_id;
    const int source_length = snprintf(out_entry->source_path,
                                       sizeof(out_entry->source_path),
                                       "%s",
                                       source->valuestring);
    const int evidence_length = snprintf(out_entry->evidence_path,
                                         sizeof(out_entry->evidence_path),
                                         "%s",
                                         evidence->valuestring);
    const int reason_length = snprintf(out_entry->reason,
                                       sizeof(out_entry->reason),
                                       "%s",
                                       reason->valuestring);
    cJSON_Delete(root);
    if (source_length < 0 ||
        (size_t)source_length >= sizeof(out_entry->source_path) ||
        evidence_length < 0 ||
        (size_t)evidence_length >= sizeof(out_entry->evidence_path) ||
        reason_length < 0 || (size_t)reason_length >= sizeof(out_entry->reason)) {
        memset(out_entry, 0, sizeof(*out_entry));
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_quarantine_file_with_ops(
    const char *source_path,
    const char *reason,
    storage_quarantine_entry_t *out_entry,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context)
{
    if (out_entry != NULL) {
        memset(out_entry, 0, sizeof(*out_entry));
    }
    if (!safe_source_path(source_path) || reason == NULL || reason[0] == '\0' ||
        strlen(reason) >= STORAGE_QUARANTINE_REASON_MAX_BYTES || out_entry == NULL ||
        !storage_fs_ops_is_valid(operations) || generate_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    struct stat metadata;
    if (operations->stat_path(operations->context, source_path, &metadata) != 0) {
        const int stat_error = errno;
        return map_error_number(stat_error);
    }
    if (!S_ISREG(metadata.st_mode)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    storage_quarantine_entry_t entry = {0};
    const int source_length = snprintf(entry.source_path,
                                       sizeof(entry.source_path),
                                       "%s",
                                       source_path);
    const int reason_length = snprintf(entry.reason,
                                       sizeof(entry.reason),
                                       "%s",
                                       reason);
    if (source_length < 0 ||
        (size_t)source_length >= sizeof(entry.source_path) ||
        reason_length < 0 || (size_t)reason_length >= sizeof(entry.reason)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char record[APP_PATH_MAX_BYTES];
    app_error_code_t result = create_unique_entry_paths(&entry,
                                                        record,
                                                        sizeof(record),
                                                        operations,
                                                        generate_uuid,
                                                        uuid_context);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    char *json = NULL;
    size_t json_length = 0U;
    result = serialize_entry(&entry, &json, &json_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = storage_atomic_write_with_ops(record,
                                           json,
                                           json_length,
                                           true,
                                           operations,
                                           generate_uuid,
                                           uuid_context);
    cJSON_free(json);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    if (operations->rename_path(operations->context,
                                source_path,
                                entry.evidence_path) != 0) {
        const int rename_error = errno;
        const app_error_code_t rename_result = map_error_number(rename_error);
        if (operations->unlink_path(operations->context, record) != 0) {
            const int unlink_error = errno;
            return unlink_error == ENOENT ? rename_result
                                          : map_error_number(unlink_error);
        }
        return rename_result;
    }

    *out_entry = entry;
    return APP_ERROR_NONE;
}

app_error_code_t storage_quarantine_file(const char *source_path,
                                         const char *reason,
                                         storage_quarantine_entry_t *out_entry)
{
    return storage_quarantine_file_with_ops(source_path,
                                            reason,
                                            out_entry,
                                            storage_fs_ops_posix(),
                                            production_uuid_generate,
                                            NULL);
}

static app_error_code_t read_record_with_ops(
    const char *path,
    const app_uuid_t *expected_id,
    storage_quarantine_entry_t *out_entry,
    const storage_fs_ops_t *operations)
{
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        const int stat_error = errno;
        return map_error_number(stat_error);
    }
    if (metadata.st_size <= 0 ||
        (uint64_t)metadata.st_size > QUARANTINE_RECORD_MAX_BYTES) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    if (data == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int descriptor =
        operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        const int open_error = errno;
        free(data);
        return map_error_number(open_error);
    }

    size_t offset = 0U;
    app_error_code_t result = APP_ERROR_NONE;
    while (offset < length) {
        const ssize_t count = operations->read_file(operations->context,
                                                    descriptor,
                                                    data + offset,
                                                    length - offset);
        if (count < 0) {
            const int read_error = errno;
            if (read_error == EINTR) {
                continue;
            }
            result = map_error_number(read_error);
            break;
        }
        if (count == 0) {
            result = APP_ERROR_IO;
            break;
        }
        offset += (size_t)count;
    }
    if (result == APP_ERROR_NONE) {
        char extra = '\0';
        const ssize_t count = operations->read_file(operations->context,
                                                    descriptor,
                                                    &extra,
                                                    1U);
        if (count < 0) {
            const int read_error = errno;
            result = map_error_number(read_error);
        } else if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result == APP_ERROR_NONE) {
        data[length] = '\0';
        result = parse_entry(data, length, expected_id, out_entry);
    }
    free(data);
    if (result != APP_ERROR_NONE) {
        memset(out_entry, 0, sizeof(*out_entry));
    }
    return result;
}

static app_error_code_t id_from_filename(const char *name,
                                         const char *suffix,
                                         app_uuid_t *out_id)
{
    if (out_id != NULL) {
        memset(out_id, 0, sizeof(*out_id));
    }
    if (name == NULL || suffix == NULL || out_id == NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const size_t suffix_length = strlen(suffix);
    const size_t name_length = strlen(name);
    if (name_length != APP_UUID_STRING_LENGTH + suffix_length ||
        strcmp(name + APP_UUID_STRING_LENGTH, suffix) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char value[APP_UUID_STRING_LENGTH + 1U];
    memcpy(value, name, APP_UUID_STRING_LENGTH);
    value[APP_UUID_STRING_LENGTH] = '\0';
    return app_uuid_parse(value, out_id) == APP_ERROR_NONE
               ? APP_ERROR_NONE
               : APP_ERROR_STORAGE_CORRUPT;
}

static int compare_entries(const void *left, const void *right)
{
    const storage_quarantine_entry_t *left_entry = left;
    const storage_quarantine_entry_t *right_entry = right;
    return strcmp(left_entry->id.value, right_entry->id.value);
}

static int compare_ids(const void *left, const void *right)
{
    const app_uuid_t *left_id = left;
    const app_uuid_t *right_id = right;
    return strcmp(left_id->value, right_id->value);
}

app_error_code_t storage_quarantine_list_with_ops(
    storage_quarantine_list_t *out_list,
    const storage_fs_ops_t *operations)
{
    if (out_list != NULL) {
        memset(out_list, 0, sizeof(*out_list));
    }
    if (out_list == NULL || !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    void *directory = operations->open_directory(
        operations->context, STORAGE_DATA_MOUNT "/quarantine");
    if (directory == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_STORAGE_UNAVAILABLE
                                    : map_error_number(open_error);
    }

    app_uuid_t evidence_ids[STORAGE_QUARANTINE_MAX_ENTRIES];
    size_t evidence_count = 0U;
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context,
                                       directory,
                                       name,
                                       sizeof(name),
                                       &end) != 0) {
            const int read_error = errno;
            result = map_error_number(read_error);
            break;
        }
        if (end) {
            break;
        }
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        const size_t name_length = strlen(name);
        if (name_length >= sizeof(QUARANTINE_JSON_SUFFIX) - 1U &&
            strcmp(name + name_length - (sizeof(QUARANTINE_JSON_SUFFIX) - 1U),
                   QUARANTINE_JSON_SUFFIX) == 0) {
            if (out_list->count >= STORAGE_QUARANTINE_MAX_ENTRIES) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
            app_uuid_t id = {0};
            result = id_from_filename(name, QUARANTINE_JSON_SUFFIX, &id);
            if (result != APP_ERROR_NONE) {
                break;
            }
            char path[APP_PATH_MAX_BYTES];
            result = record_path(&id,
                                 QUARANTINE_JSON_SUFFIX,
                                 path,
                                 sizeof(path));
            if (result == APP_ERROR_NONE) {
                result = read_record_with_ops(path,
                                              &id,
                                              &out_list->items[out_list->count],
                                              operations);
            }
            if (result != APP_ERROR_NONE) {
                break;
            }
            ++out_list->count;
            continue;
        }

        if (name_length >= sizeof(QUARANTINE_EVIDENCE_SUFFIX) - 1U &&
            strcmp(name + name_length -
                              (sizeof(QUARANTINE_EVIDENCE_SUFFIX) - 1U),
                   QUARANTINE_EVIDENCE_SUFFIX) == 0) {
            if (evidence_count >= STORAGE_QUARANTINE_MAX_ENTRIES) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
            result = id_from_filename(name,
                                      QUARANTINE_EVIDENCE_SUFFIX,
                                      &evidence_ids[evidence_count]);
            if (result != APP_ERROR_NONE) {
                break;
            }
            char path[APP_PATH_MAX_BYTES];
            result = record_path(&evidence_ids[evidence_count],
                                 QUARANTINE_EVIDENCE_SUFFIX,
                                 path,
                                 sizeof(path));
            if (result != APP_ERROR_NONE) {
                break;
            }
            struct stat metadata;
            if (operations->stat_path(operations->context, path, &metadata) != 0) {
                const int stat_error = errno;
                result = map_error_number(stat_error);
                break;
            }
            if (!S_ISREG(metadata.st_mode)) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
            ++evidence_count;
            continue;
        }

        result = APP_ERROR_STORAGE_CORRUPT;
        break;
    }

    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result == APP_ERROR_NONE) {
        qsort(out_list->items,
              out_list->count,
              sizeof(out_list->items[0]),
              compare_entries);
        qsort(evidence_ids,
              evidence_count,
              sizeof(evidence_ids[0]),
              compare_ids);
        if (out_list->count != evidence_count) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
    }
    if (result == APP_ERROR_NONE) {
        for (size_t index = 0U; index < out_list->count; ++index) {
            if (!app_uuid_equal(&out_list->items[index].id,
                                &evidence_ids[index])) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
        }
    }
    if (result != APP_ERROR_NONE) {
        memset(out_list, 0, sizeof(*out_list));
    }
    return result;
}

app_error_code_t storage_quarantine_list(storage_quarantine_list_t *out_list)
{
    return storage_quarantine_list_with_ops(out_list, storage_fs_ops_posix());
}
