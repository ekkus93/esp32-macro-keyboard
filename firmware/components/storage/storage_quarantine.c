#include "storage.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"
#include "storage_quarantine_internal.h"

#define ASCII_DELETE 0x7fU
#define QUARANTINE_RECORD_MAX_BYTES 1024U
#define QUARANTINE_ID_ATTEMPTS 4U
/* rwxr-x--- for the per-entry directories we create (matches storage topology). */
#define QUARANTINE_DIR_MODE 0750
/* rw------- for the record file we create. */
#define QUARANTINE_FILE_MODE 0600

/* Directory-per-entry layout (FIX1 §7.4 / §8):
 *   committed: /data/quarantine/<id>/{record.json,evidence}
 *   staging:   /data/staging/quarantine-<id>/{record.json,evidence}
 * The evidence path is fully determined by the entry id, so it is derived rather
 * than stored in the record. */
#define QUARANTINE_RECORD_NAME "record.json"
#define QUARANTINE_EVIDENCE_NAME "evidence"

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static app_error_code_t production_uuid_generate(void *context, app_uuid_t *out_uuid) {
    (void)context;
    return app_uuid_generate(out_uuid);
}

static bool safe_source_path(const char *path) {
    static const char data_prefix[] = STORAGE_DATA_MOUNT "/";
    static const char quarantine_prefix[] = STORAGE_DATA_MOUNT "/quarantine/";
    if (path == NULL || strncmp(path, data_prefix, sizeof(data_prefix) - 1U) != 0 ||
        strncmp(path, quarantine_prefix, sizeof(quarantine_prefix) - 1U) == 0 ||
        path[sizeof(data_prefix) - 1U] == '\0' || strlen(path) >= APP_PATH_MAX_BYTES ||
        strstr(path, "..") != NULL || strstr(path, "//") != NULL || strchr(path, '\\') != NULL) {
        return false;
    }
    for (const unsigned char *cursor = (const unsigned char *)path; *cursor != 0U; ++cursor) {
        if (*cursor < 0x20U || *cursor == ASCII_DELETE) {
            return false;
        }
    }
    return true;
}

static app_error_code_t committed_dir_path(const app_uuid_t *entry_id, char *path,
                                           size_t path_size) {
    if (entry_id == NULL || path == NULL || !app_uuid_is_valid_string(entry_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written =
        snprintf(path, path_size, STORAGE_DATA_MOUNT "/quarantine/%s", entry_id->value);
    return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE
                                                       : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t staging_dir_path(const app_uuid_t *entry_id, char *path, size_t path_size) {
    if (entry_id == NULL || path == NULL || !app_uuid_is_valid_string(entry_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(
        path, path_size, STORAGE_DATA_MOUNT "/staging/" STORAGE_QUARANTINE_STAGING_PREFIX "%s",
        entry_id->value);
    return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE
                                                       : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t child_path(const char *directory, const char *child, char *path,
                                   size_t path_size) {
    if (directory == NULL || child == NULL || path == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(path, path_size, "%s/%s", directory, child);
    return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE
                                                       : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t committed_evidence_path(const app_uuid_t *entry_id, char *path,
                                                size_t path_size) {
    char directory[APP_PATH_MAX_BYTES];
    const app_error_code_t result = committed_dir_path(entry_id, directory, sizeof(directory));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return child_path(directory, QUARANTINE_EVIDENCE_NAME, path, path_size);
}

static app_error_code_t path_exists_with_ops(const char *path, const storage_fs_ops_t *operations,
                                             bool *out_exists) {
    if (out_exists != NULL) {
        *out_exists = false;
    }
    if (path == NULL || out_exists == NULL || !storage_fs_ops_is_valid(operations)) {
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

static app_error_code_t serialize_entry(const storage_quarantine_entry_t *entry, char **out_json,
                                        size_t *out_length) {
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
    if (root == NULL || cJSON_AddNumberToObject(root, "schema_version", 1.0) == NULL ||
        cJSON_AddStringToObject(root, "id", entry->id.value) == NULL ||
        cJSON_AddStringToObject(root, "source_path", entry->source_path) == NULL ||
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

static bool object_has_exact_fields(const cJSON *root) {
    static const char *const names[] = {
        "schema_version",
        "id",
        "source_path",
        "reason",
    };
    bool found[sizeof(names) / sizeof(names[0])] = {false};
    size_t count = 0U;
    for (const cJSON *child = root == NULL ? NULL : root->child; child != NULL;
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

static app_error_code_t parse_entry(const char *data, size_t length, const app_uuid_t *expected_id,
                                    storage_quarantine_entry_t *out_entry) {
    if (out_entry != NULL) {
        memset(out_entry, 0, sizeof(*out_entry));
    }
    if (data == NULL || length == 0U || expected_id == NULL || out_entry == NULL ||
        memchr(data, '\0', length) != NULL) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length + 1U, &parse_end, true);
    const cJSON *version =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    const cJSON *id_field = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "id");
    const cJSON *source =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "source_path");
    const cJSON *reason = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "reason");

    app_uuid_t parsed_id = {0};
    const bool valid =
        cJSON_IsObject(root) && object_has_exact_fields(root) && parse_end == data + length &&
        cJSON_IsNumber(version) && version->valuedouble == 1.0 && cJSON_IsString(id_field) &&
        id_field->valuestring != NULL &&
        app_uuid_parse(id_field->valuestring, &parsed_id) == APP_ERROR_NONE &&
        app_uuid_equal(&parsed_id, expected_id) && cJSON_IsString(source) &&
        source->valuestring != NULL && safe_source_path(source->valuestring) &&
        cJSON_IsString(reason) && reason->valuestring != NULL && reason->valuestring[0] != '\0' &&
        strlen(source->valuestring) < sizeof(out_entry->source_path) &&
        strlen(reason->valuestring) < sizeof(out_entry->reason);
    if (!valid) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }

    out_entry->id = parsed_id;
    const int source_length =
        snprintf(out_entry->source_path, sizeof(out_entry->source_path), "%s", source->valuestring);
    const int reason_length =
        snprintf(out_entry->reason, sizeof(out_entry->reason), "%s", reason->valuestring);
    const app_error_code_t evidence_result = committed_evidence_path(
        &parsed_id, out_entry->evidence_path, sizeof(out_entry->evidence_path));
    cJSON_Delete(root);
    if (source_length < 0 || (size_t)source_length >= sizeof(out_entry->source_path) ||
        reason_length < 0 || (size_t)reason_length >= sizeof(out_entry->reason) ||
        evidence_result != APP_ERROR_NONE) {
        memset(out_entry, 0, sizeof(*out_entry));
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

/* Read and validate a single quarantine record at `path`, requiring its JSON id to
 * equal `expected_id`. Does not move or modify any file. */
static app_error_code_t read_record_at(const char *path, const app_uuid_t *expected_id,
                                       storage_quarantine_entry_t *out_entry,
                                       const storage_fs_ops_t *operations) {
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        const int stat_error = errno;
        return map_error_number(stat_error);
    }
    if (metadata.st_size <= 0 || (uint64_t)metadata.st_size > QUARANTINE_RECORD_MAX_BYTES) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    if (data == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int descriptor = operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        const int open_error = errno;
        free(data);
        return map_error_number(open_error);
    }

    size_t offset = 0U;
    app_error_code_t result = APP_ERROR_NONE;
    while (offset < length) {
        const ssize_t count =
            operations->read_file(operations->context, descriptor, data + offset, length - offset);
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
        const ssize_t count = operations->read_file(operations->context, descriptor, &extra, 1U);
        if (count < 0) {
            const int read_error = errno;
            result = map_error_number(read_error);
        } else if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 && result == APP_ERROR_NONE) {
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

static app_error_code_t write_all(const storage_fs_ops_t *operations, int descriptor,
                                  const char *data, size_t length) {
    size_t written = 0U;
    while (written < length) {
        const ssize_t count = operations->write_file(operations->context, descriptor,
                                                     data + written, length - written);
        if (count < 0) {
            const int write_error = errno;
            if (write_error == EINTR) {
                continue;
            }
            return map_error_number(write_error);
        }
        if (count == 0) {
            return APP_ERROR_IO;
        }
        written += (size_t)count;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t unlink_if_present(const char *path, const storage_fs_ops_t *operations) {
    if (operations->unlink_path(operations->context, path) == 0) {
        return APP_ERROR_NONE;
    }
    const int unlink_error = errno;
    return unlink_error == ENOENT ? APP_ERROR_NONE : map_error_number(unlink_error);
}

/* The paths a staged quarantine threads through creation and rollback. */
typedef struct {
    const char *source;
    const char *staging_dir;
    const char *staging_record;
    const char *staging_evidence;
    const char *committed_dir;
} staged_paths_t;

/* Step 3: write the record durably into the staging directory (plain create, not
 * the atomic .tmp/.bak dance -- the staging directory itself is the atomicity
 * boundary, committed by the directory rename in step 7). */
static app_error_code_t write_record_file(const staged_paths_t *paths, const char *json,
                                          size_t json_length, const storage_fs_ops_t *operations) {
    const int descriptor = operations->open_file(operations->context, paths->staging_record,
                                                 O_WRONLY | O_CREAT | O_EXCL, QUARANTINE_FILE_MODE);
    if (descriptor < 0) {
        const int open_error = errno;
        return map_error_number(open_error);
    }
    app_error_code_t result = write_all(operations, descriptor, json, json_length);
    if (result == APP_ERROR_NONE && operations->sync_file(operations->context, descriptor) != 0) {
        const int sync_error = errno;
        result = map_error_number(sync_error);
    }
    if (operations->close_file(operations->context, descriptor) != 0 && result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = unlink_if_present(paths->staging_record, operations);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    return APP_ERROR_NONE;
}

/* Step 4 (evidence half): flush the moved evidence file's data to stable storage.
 * On platforms without a directory-fsync primitive this file sync, together with
 * the completed rename in step 7, is the durability boundary. */
static app_error_code_t sync_evidence_file(const char *path, const storage_fs_ops_t *operations) {
    const int descriptor = operations->open_file(operations->context, path, O_RDONLY, 0);
    if (descriptor < 0) {
        const int open_error = errno;
        return map_error_number(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    if (operations->sync_file(operations->context, descriptor) != 0) {
        const int sync_error = errno;
        if (sync_error != EINVAL && sync_error != ENOTSUP) {
            result = map_error_number(sync_error);
        }
    }
    if (operations->close_file(operations->context, descriptor) != 0 && result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    return result;
}

/* Step 5: prove the staged entry is complete before committing it -- the record
 * parses and matches its id, and the evidence is a regular file. */
static app_error_code_t validate_staged_entry(const staged_paths_t *paths,
                                              const app_uuid_t *entry_id,
                                              const storage_fs_ops_t *operations) {
    storage_quarantine_entry_t parsed;
    const app_error_code_t record_result =
        read_record_at(paths->staging_record, entry_id, &parsed, operations);
    if (record_result != APP_ERROR_NONE) {
        return record_result;
    }
    struct stat metadata;
    if (operations->stat_path(operations->context, paths->staging_evidence, &metadata) != 0) {
        const int stat_error = errno;
        return map_error_number(stat_error);
    }
    return S_ISREG(metadata.st_mode) ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

/* Undo a staged creation once the source has already been renamed into the staging
 * evidence (steps 3-6 failed). Restore the source and remove the staging entry;
 * never delete the evidence without first putting the source back. Returns the
 * original error unless a rollback step itself fails (that is surfaced instead). */
static app_error_code_t rollback_after_move(const staged_paths_t *paths,
                                            const storage_fs_ops_t *operations,
                                            app_error_code_t original_error) {
    const app_error_code_t record_cleanup = unlink_if_present(paths->staging_record, operations);
    if (operations->rename_path(operations->context, paths->staging_evidence, paths->source) != 0) {
        const int restore_error = errno;
        return map_error_number(restore_error);
    }
    app_error_code_t result = record_cleanup;
    if (operations->remove_directory(operations->context, paths->staging_dir) != 0 &&
        result == APP_ERROR_NONE) {
        const int rmdir_error = errno;
        result = map_error_number(rmdir_error);
    }
    return result == APP_ERROR_NONE ? original_error : result;
}

/* Steps 3-6: populate the staging directory (record + evidence, synced and
 * validated) after the source has been moved in. On any failure the source is
 * restored and the staging directory removed. */
static app_error_code_t populate_staging(const staged_paths_t *paths, const app_uuid_t *entry_id,
                                         const char *json, size_t json_length,
                                         const storage_fs_ops_t *operations) {
    app_error_code_t result = write_record_file(paths, json, json_length, operations);
    if (result == APP_ERROR_NONE) {
        result = sync_evidence_file(paths->staging_evidence, operations);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_staged_entry(paths, entry_id, operations);
    }
    if (result == APP_ERROR_NONE &&
        storage_fs_sync_parent_path(operations->context, paths->staging_record) != 0) {
        const int sync_error = errno;
        result = map_error_number(sync_error);
    }
    if (result != APP_ERROR_NONE) {
        return rollback_after_move(paths, operations, result);
    }
    return APP_ERROR_NONE;
}

/* The staged 9-step creation (FIX1 §8.2), rename-based: the source is moved into
 * the staging evidence and only put back if the entry never commits.
 *
 * LittleFS/POSIX rename is atomic, so step 7 leaves EITHER the staging directory
 * OR the committed directory, never both -- an interrupted create is always
 * reconcilable at startup (FIX1 §8.3). */
static app_error_code_t create_staged_entry(const staged_paths_t *paths, const app_uuid_t *entry_id,
                                            const char *json, size_t json_length,
                                            const storage_fs_ops_t *operations) {
    if (operations->make_directory(operations->context, paths->staging_dir, QUARANTINE_DIR_MODE) !=
        0) {
        const int mkdir_error = errno;
        return map_error_number(mkdir_error);
    }
    if (operations->rename_path(operations->context, paths->source, paths->staging_evidence) != 0) {
        const int rename_error = errno;
        const app_error_code_t rename_result = map_error_number(rename_error);
        if (operations->remove_directory(operations->context, paths->staging_dir) != 0) {
            const int rmdir_error = errno;
            return rmdir_error == ENOENT ? rename_result : map_error_number(rmdir_error);
        }
        return rename_result;
    }
    const app_error_code_t populate_result =
        populate_staging(paths, entry_id, json, json_length, operations);
    if (populate_result != APP_ERROR_NONE) {
        return populate_result;
    }
    if (operations->rename_path(operations->context, paths->staging_dir, paths->committed_dir) !=
        0) {
        const int rename_error = errno;
        return rollback_after_move(paths, operations, map_error_number(rename_error));
    }
    if (storage_fs_sync_parent_path(operations->context, paths->committed_dir) != 0) {
        const int sync_error = errno;
        return map_error_number(sync_error);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t find_unique_id(app_uuid_t *out_id, char *committed_dir,
                                       size_t committed_size, char *staging_dir,
                                       size_t staging_size, const storage_fs_ops_t *operations,
                                       storage_uuid_generate_fn generate_uuid, void *uuid_context) {
    for (size_t attempt = 0U; attempt < QUARANTINE_ID_ATTEMPTS; ++attempt) {
        memset(out_id, 0, sizeof(*out_id));
        app_error_code_t result = generate_uuid(uuid_context, out_id);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        result = committed_dir_path(out_id, committed_dir, committed_size);
        if (result == APP_ERROR_NONE) {
            result = staging_dir_path(out_id, staging_dir, staging_size);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }

        bool committed_exists = false;
        bool staging_exists = false;
        result = path_exists_with_ops(committed_dir, operations, &committed_exists);
        if (result == APP_ERROR_NONE) {
            result = path_exists_with_ops(staging_dir, operations, &staging_exists);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (!committed_exists && !staging_exists) {
            return APP_ERROR_NONE;
        }
    }
    return APP_ERROR_CONFLICT;
}

app_error_code_t storage_quarantine_file_with_ops(const char *source_path, const char *reason,
                                                  storage_quarantine_entry_t *out_entry,
                                                  const storage_fs_ops_t *operations,
                                                  storage_uuid_generate_fn generate_uuid,
                                                  void *uuid_context) {
    if (out_entry != NULL) {
        memset(out_entry, 0, sizeof(*out_entry));
    }
    if (!safe_source_path(source_path) || reason == NULL || reason[0] == '\0' ||
        strlen(reason) >= STORAGE_QUARANTINE_REASON_MAX_BYTES || out_entry == NULL ||
        !storage_fs_ops_has_directory(operations) || generate_uuid == NULL) {
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
    const int source_length =
        snprintf(entry.source_path, sizeof(entry.source_path), "%s", source_path);
    const int reason_length = snprintf(entry.reason, sizeof(entry.reason), "%s", reason);
    if (source_length < 0 || (size_t)source_length >= sizeof(entry.source_path) ||
        reason_length < 0 || (size_t)reason_length >= sizeof(entry.reason)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char committed_dir[APP_PATH_MAX_BYTES];
    char staging_dir[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        find_unique_id(&entry.id, committed_dir, sizeof(committed_dir), staging_dir,
                       sizeof(staging_dir), operations, generate_uuid, uuid_context);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    char staging_record[APP_PATH_MAX_BYTES];
    char staging_evidence[APP_PATH_MAX_BYTES];
    result =
        child_path(staging_dir, QUARANTINE_RECORD_NAME, staging_record, sizeof(staging_record));
    if (result == APP_ERROR_NONE) {
        result = child_path(staging_dir, QUARANTINE_EVIDENCE_NAME, staging_evidence,
                            sizeof(staging_evidence));
    }
    if (result == APP_ERROR_NONE) {
        result =
            committed_evidence_path(&entry.id, entry.evidence_path, sizeof(entry.evidence_path));
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }

    char *json = NULL;
    size_t json_length = 0U;
    result = serialize_entry(&entry, &json, &json_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const staged_paths_t paths = {
        .source = source_path,
        .staging_dir = staging_dir,
        .staging_record = staging_record,
        .staging_evidence = staging_evidence,
        .committed_dir = committed_dir,
    };
    result = create_staged_entry(&paths, &entry.id, json, json_length, operations);
    cJSON_free(json);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    *out_entry = entry;
    return APP_ERROR_NONE;
}

app_error_code_t storage_quarantine_file(const char *source_path, const char *reason,
                                         storage_quarantine_entry_t *out_entry) {
    return storage_quarantine_file_with_ops(source_path, reason, out_entry, storage_fs_ops_posix(),
                                            production_uuid_generate, NULL);
}

static int compare_entries(const void *lhs, const void *rhs) {
    const storage_quarantine_entry_t *lhs_entry = lhs;
    const storage_quarantine_entry_t *rhs_entry = rhs;
    return strcmp(lhs_entry->id.value, rhs_entry->id.value);
}

static app_error_code_t entry_name_to_id(const char *name, app_uuid_t *out_id) {
    if (name == NULL || out_id == NULL || strlen(name) != APP_UUID_STRING_LENGTH ||
        !app_uuid_is_valid_string(name)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return app_uuid_parse(name, out_id) == APP_ERROR_NONE ? APP_ERROR_NONE
                                                          : APP_ERROR_STORAGE_CORRUPT;
}

/* Read one committed directory into the list. A per-entry defect (bad name, not a
 * directory, corrupt/missing record, missing evidence, or over the entry limit) is
 * counted as damaged and skipped rather than failing the whole list (FIX1 §8.3
 * rule 5). Only genuine I/O faults are returned as errors. */
static app_error_code_t list_add_committed(const char *name, storage_quarantine_list_t *out_list,
                                           const storage_fs_ops_t *operations) {
    app_uuid_t entry_id;
    if (entry_name_to_id(name, &entry_id) != APP_ERROR_NONE) {
        ++out_list->damaged_count;
        return APP_ERROR_NONE;
    }
    char directory[APP_PATH_MAX_BYTES];
    char record[APP_PATH_MAX_BYTES];
    char evidence[APP_PATH_MAX_BYTES];
    if (committed_dir_path(&entry_id, directory, sizeof(directory)) != APP_ERROR_NONE ||
        child_path(directory, QUARANTINE_RECORD_NAME, record, sizeof(record)) != APP_ERROR_NONE ||
        child_path(directory, QUARANTINE_EVIDENCE_NAME, evidence, sizeof(evidence)) !=
            APP_ERROR_NONE) {
        ++out_list->damaged_count;
        return APP_ERROR_NONE;
    }

    struct stat metadata;
    if (operations->stat_path(operations->context, directory, &metadata) != 0) {
        const int stat_error = errno;
        return stat_error == ENOENT ? APP_ERROR_NONE : map_error_number(stat_error);
    }
    if (!S_ISDIR(metadata.st_mode)) {
        ++out_list->damaged_count;
        return APP_ERROR_NONE;
    }

    storage_quarantine_entry_t entry;
    const app_error_code_t record_result = read_record_at(record, &entry_id, &entry, operations);
    if (record_result == APP_ERROR_STORAGE_CORRUPT || record_result == APP_ERROR_NOT_FOUND) {
        /* Corrupt or missing record: the entry is incomplete, not a filesystem
         * fault -- count it as damaged and keep listing the rest. */
        ++out_list->damaged_count;
        return APP_ERROR_NONE;
    }
    if (record_result != APP_ERROR_NONE) {
        return record_result;
    }
    if (operations->stat_path(operations->context, evidence, &metadata) != 0) {
        const int stat_error = errno;
        if (stat_error == ENOENT) {
            ++out_list->damaged_count;
            return APP_ERROR_NONE;
        }
        return map_error_number(stat_error);
    }
    if (!S_ISREG(metadata.st_mode) || out_list->count >= STORAGE_QUARANTINE_MAX_ENTRIES) {
        ++out_list->damaged_count;
        return APP_ERROR_NONE;
    }
    out_list->items[out_list->count] = entry;
    ++out_list->count;
    return APP_ERROR_NONE;
}

app_error_code_t storage_quarantine_list_with_ops(storage_quarantine_list_t *out_list,
                                                  const storage_fs_ops_t *operations) {
    if (out_list != NULL) {
        memset(out_list, 0, sizeof(*out_list));
    }
    if (out_list == NULL || !storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    void *directory =
        operations->open_directory(operations->context, STORAGE_DATA_MOUNT "/quarantine");
    if (directory == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_STORAGE_UNAVAILABLE : map_error_number(open_error);
    }

    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context, directory, name, sizeof(name), &end) !=
            0) {
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
        result = list_add_committed(name, out_list, operations);
        if (result != APP_ERROR_NONE) {
            break;
        }
    }

    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        const int close_error = errno;
        result = map_error_number(close_error);
    }
    if (result != APP_ERROR_NONE) {
        memset(out_list, 0, sizeof(*out_list));
        return result;
    }
    qsort(out_list->items, out_list->count, sizeof(out_list->items[0]), compare_entries);
    return APP_ERROR_NONE;
}

app_error_code_t storage_quarantine_list(storage_quarantine_list_t *out_list) {
    return storage_quarantine_list_with_ops(out_list, storage_fs_ops_posix());
}

typedef enum {
    EVIDENCE_ABSENT,
    EVIDENCE_REGULAR,
    EVIDENCE_IRREGULAR,
} evidence_state_t;

static bool has_staging_prefix(const char *name) {
    static const char prefix[] = STORAGE_QUARANTINE_STAGING_PREFIX;
    return strncmp(name, prefix, sizeof(prefix) - 1U) == 0;
}

static app_error_code_t staging_name_to_id(const char *name, app_uuid_t *out_id) {
    static const char prefix[] = STORAGE_QUARANTINE_STAGING_PREFIX;
    if (name == NULL || out_id == NULL || !has_staging_prefix(name)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const char *uuid_text = name + sizeof(prefix) - 1U;
    if (strlen(uuid_text) != APP_UUID_STRING_LENGTH || !app_uuid_is_valid_string(uuid_text)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return app_uuid_parse(uuid_text, out_id) == APP_ERROR_NONE ? APP_ERROR_NONE
                                                               : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t probe_evidence(const char *path, const storage_fs_ops_t *operations,
                                       evidence_state_t *out_state) {
    *out_state = EVIDENCE_ABSENT;
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        const int stat_error = errno;
        return stat_error == ENOENT ? APP_ERROR_NONE : map_error_number(stat_error);
    }
    *out_state = S_ISREG(metadata.st_mode) ? EVIDENCE_REGULAR : EVIDENCE_IRREGULAR;
    return APP_ERROR_NONE;
}

/* Rule 1: commit a provably complete staged entry by atomically renaming its
 * directory into the quarantine root. A pre-existing committed directory is an
 * anomaly (rename is atomic, so both cannot normally exist) -- treat it as
 * ambiguous and preserve the staging rather than clobber. */
static app_error_code_t finish_staged(const char *staging_dir, const char *committed_dir,
                                      const storage_fs_ops_t *operations) {
    if (operations->rename_path(operations->context, staging_dir, committed_dir) != 0) {
        const int rename_error = errno;
        if (rename_error == EEXIST || rename_error == ENOTEMPTY) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        return map_error_number(rename_error);
    }
    if (storage_fs_sync_parent_path(operations->context, committed_dir) != 0) {
        const int sync_error = errno;
        return map_error_number(sync_error);
    }
    return APP_ERROR_NONE;
}

/* Rule 2: the source was never durably moved (no evidence, no record), so it is
 * still in the active tree -- restoration is automatic and the empty staging
 * directory is simply removed. Unexpected residue is preserved, not force-removed. */
static app_error_code_t discard_empty_staging(const char *staging_dir,
                                              const storage_fs_ops_t *operations) {
    if (operations->remove_directory(operations->context, staging_dir) != 0) {
        const int rmdir_error = errno;
        if (rmdir_error == ENOENT) {
            return APP_ERROR_NONE;
        }
        if (rmdir_error == ENOTEMPTY || rmdir_error == EEXIST) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        return map_error_number(rmdir_error);
    }
    return APP_ERROR_NONE;
}

/* Reconcile one staged quarantine directory (FIX1 §8.3). Returns APP_ERROR_NONE
 * when finished or cleanly discarded, APP_ERROR_STORAGE_CORRUPT when the staging
 * is ambiguous and preserved as evidence (a health signal, not a fault), or a
 * filesystem error on a genuine I/O fault. */
static app_error_code_t recover_one_staging(const app_uuid_t *entry_id,
                                            const storage_fs_ops_t *operations) {
    char staging_dir[APP_PATH_MAX_BYTES];
    char staging_record[APP_PATH_MAX_BYTES];
    char staging_evidence[APP_PATH_MAX_BYTES];
    char committed_dir[APP_PATH_MAX_BYTES];
    if (staging_dir_path(entry_id, staging_dir, sizeof(staging_dir)) != APP_ERROR_NONE ||
        child_path(staging_dir, QUARANTINE_RECORD_NAME, staging_record, sizeof(staging_record)) !=
            APP_ERROR_NONE ||
        child_path(staging_dir, QUARANTINE_EVIDENCE_NAME, staging_evidence,
                   sizeof(staging_evidence)) != APP_ERROR_NONE ||
        committed_dir_path(entry_id, committed_dir, sizeof(committed_dir)) != APP_ERROR_NONE) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    evidence_state_t evidence = EVIDENCE_ABSENT;
    const app_error_code_t evidence_result =
        probe_evidence(staging_evidence, operations, &evidence);
    if (evidence_result != APP_ERROR_NONE) {
        return evidence_result;
    }
    storage_quarantine_entry_t entry;
    const app_error_code_t record_result =
        read_record_at(staging_record, entry_id, &entry, operations);
    if (record_result != APP_ERROR_NONE && record_result != APP_ERROR_NOT_FOUND &&
        record_result != APP_ERROR_STORAGE_CORRUPT) {
        return record_result;
    }

    if (record_result == APP_ERROR_NONE && evidence == EVIDENCE_REGULAR) {
        return finish_staged(staging_dir, committed_dir, operations);
    }
    if (record_result == APP_ERROR_NOT_FOUND && evidence == EVIDENCE_ABSENT) {
        return discard_empty_staging(staging_dir, operations);
    }
    /* Evidence present but the record is absent/corrupt (or vice versa): the
     * quarantine cannot be proven complete and its source is unknown, so the
     * evidence is preserved untouched and a health error is surfaced. */
    return APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t collect_staging_ids(app_uuid_t *ids, size_t capacity, size_t *out_count,
                                            bool *out_had_malformed,
                                            const storage_fs_ops_t *operations) {
    *out_count = 0U;
    *out_had_malformed = false;
    void *directory =
        operations->open_directory(operations->context, STORAGE_DATA_MOUNT "/staging");
    if (directory == NULL) {
        const int open_error = errno;
        return open_error == ENOENT ? APP_ERROR_STORAGE_UNAVAILABLE : map_error_number(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX];
        bool end = false;
        if (operations->read_directory(operations->context, directory, name, sizeof(name), &end) !=
            0) {
            result = map_error_number(errno);
            break;
        }
        if (end) {
            break;
        }
        if (!has_staging_prefix(name)) {
            continue;
        }
        app_uuid_t entry_id;
        if (staging_name_to_id(name, &entry_id) != APP_ERROR_NONE) {
            *out_had_malformed = true;
            continue;
        }
        if (*out_count >= capacity) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        ids[*out_count] = entry_id;
        ++(*out_count);
    }
    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    if (result != APP_ERROR_NONE) {
        *out_count = 0U;
    }
    return result;
}

app_error_code_t storage_quarantine_recover_all_with_ops(const storage_fs_ops_t *operations) {
    if (!storage_fs_ops_has_directory(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* Snapshot the staging ids before mutating the directory: reconciling one
     * entry renames or removes a child of the directory being scanned. */
    app_uuid_t ids[STORAGE_QUARANTINE_MAX_ENTRIES];
    size_t count = 0U;
    bool had_malformed = false;
    const app_error_code_t collect_result = collect_staging_ids(ids, STORAGE_QUARANTINE_MAX_ENTRIES,
                                                                &count, &had_malformed, operations);
    if (collect_result != APP_ERROR_NONE) {
        return collect_result;
    }

    app_error_code_t first_error = had_malformed ? APP_ERROR_STORAGE_CORRUPT : APP_ERROR_NONE;
    for (size_t index = 0U; index < count; ++index) {
        const app_error_code_t result = recover_one_staging(&ids[index], operations);
        if (result == APP_ERROR_NONE) {
            continue;
        }
        if (result != APP_ERROR_STORAGE_CORRUPT) {
            return result;
        }
        if (first_error == APP_ERROR_NONE) {
            first_error = result;
        }
    }
    return first_error;
}

app_error_code_t storage_quarantine_recover_all(void) {
    return storage_quarantine_recover_all_with_ops(storage_fs_ops_posix());
}
