#include "storage_atomic_validators.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_fs_ops.h"
#include "storage_quarantine_internal.h"
#include "storage_repository_internal.h"
#include "storage_transaction_internal.h"

#define SCHEMA_MARKER_MAX_BYTES 128U

typedef struct {
    const storage_fs_ops_t *ops;
} validate_context_t;

/* Match a path of the exact form <prefix><36-char-uuid><suffix> and, when it does,
 * parse the embedded UUID into out_uuid. */
static bool match_prefixed_uuid(const char *path, const char *prefix, const char *suffix,
                                app_uuid_t *out_uuid) {
    const size_t prefix_length = strlen(prefix);
    const size_t suffix_length = strlen(suffix);
    const size_t path_length = strlen(path);
    if (path_length != prefix_length + APP_UUID_STRING_LENGTH + suffix_length) {
        return false;
    }
    if (strncmp(path, prefix, prefix_length) != 0 ||
        strcmp(path + prefix_length + APP_UUID_STRING_LENGTH, suffix) != 0) {
        return false;
    }
    char uuid_text[APP_UUID_BUFFER_LENGTH];
    memcpy(uuid_text, path + prefix_length, APP_UUID_STRING_LENGTH);
    uuid_text[APP_UUID_STRING_LENGTH] = '\0';
    if (!app_uuid_is_valid_string(uuid_text)) {
        return false;
    }
    return out_uuid == NULL || app_uuid_parse(uuid_text, out_uuid) == APP_ERROR_NONE;
}

static bool is_under_set_subdirectory(const char *path, const char *subdirectory) {
    static const char prefix[] = STORAGE_DATA_MOUNT "/sets/";
    return strncmp(path, prefix, sizeof(prefix) - 1U) == 0 && strstr(path, subdirectory) != NULL;
}

storage_atomic_object_type_t storage_atomic_classify_destination(const char *destination) {
    if (destination == NULL) {
        return STORAGE_ATOMIC_OBJECT_UNKNOWN;
    }
    if (strcmp(destination, STORAGE_SCHEMA_FILE_PATH) == 0) {
        return STORAGE_ATOMIC_OBJECT_SCHEMA_MARKER;
    }
    if (strcmp(destination, STORAGE_SET_INDEX_FILE_PATH) == 0) {
        return STORAGE_ATOMIC_OBJECT_SET_INDEX;
    }
    if (strcmp(destination, STORAGE_GLOBAL_ORDER_FILE_PATH) == 0) {
        return STORAGE_ATOMIC_OBJECT_GLOBAL_MACRO_INDEX;
    }
    if (match_prefixed_uuid(destination, STORAGE_DATA_MOUNT "/transactions/", ".bin", NULL)) {
        return STORAGE_ATOMIC_OBJECT_TRANSACTION_MANIFEST;
    }
    if (match_prefixed_uuid(destination, STORAGE_DATA_MOUNT "/quarantine/", ".json", NULL)) {
        return STORAGE_ATOMIC_OBJECT_QUARANTINE_RECORD;
    }
    if (match_prefixed_uuid(destination, STORAGE_DATA_MOUNT "/sets/", "/set.json", NULL)) {
        return STORAGE_ATOMIC_OBJECT_SET_METADATA;
    }
    /* Object repositories completed in Phase 15: recognized for classification but
     * intentionally without a validator (recovery refuses to activate them). */
    if (is_under_set_subdirectory(destination, "/macros/")) {
        return STORAGE_ATOMIC_OBJECT_MACRO;
    }
    if (is_under_set_subdirectory(destination, "/procedures/")) {
        return STORAGE_ATOMIC_OBJECT_PROCEDURE;
    }
    if (is_under_set_subdirectory(destination, "/progress/")) {
        return STORAGE_ATOMIC_OBJECT_PROGRESS;
    }
    return STORAGE_ATOMIC_OBJECT_UNKNOWN;
}

static app_error_code_t validate_schema_marker(void *context,
                                               const storage_atomic_candidate_t *candidate) {
    const validate_context_t *ctx = context;
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_read_bounded_file_with_ops(
        candidate->candidate_path, SCHEMA_MARKER_MAX_BYTES, &data, &length, ctx->ops);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    cJSON *root = cJSON_ParseWithLength(data, length);
    free(data);
    const cJSON *version =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    const bool valid = cJSON_IsObject(root) && cJSON_IsNumber(version) && version != NULL &&
                       version->valueint == (int)APP_SCHEMA_VERSION;
    cJSON_Delete(root);
    return valid ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t validate_index(void *context, const storage_atomic_candidate_t *candidate) {
    const validate_context_t *ctx = context;
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_read_bounded_file_with_ops(
        candidate->candidate_path, STORAGE_INDEX_FILE_MAX_BYTES, &data, &length, ctx->ops);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_set_index_t index;
    result = storage_repository_parse_index(data, length, &index);
    free(data);
    return result;
}

static app_error_code_t validate_set_metadata(void *context,
                                              const storage_atomic_candidate_t *candidate) {
    const validate_context_t *ctx = context;
    app_uuid_t expected_id;
    if (!match_prefixed_uuid(candidate->destination, STORAGE_DATA_MOUNT "/sets/", "/set.json",
                             &expected_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = storage_repository_read_bounded_file_with_ops(
        candidate->candidate_path, STORAGE_SET_FILE_MAX_BYTES, &data, &length, ctx->ops);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    macro_set_t set;
    result = storage_repository_parse_set_json(data, length, &set);
    free(data);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    /* The candidate's own id must match the set the destination path names. */
    return app_uuid_equal(&expected_id, &set.id) ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t validate_transaction_manifest(void *context,
                                                      const storage_atomic_candidate_t *candidate) {
    const validate_context_t *ctx = context;
    app_uuid_t expected_id;
    if (!match_prefixed_uuid(candidate->destination, STORAGE_DATA_MOUNT "/transactions/", ".bin",
                             &expected_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_transaction_manifest_t manifest;
    return storage_transaction_read_manifest_with_ops(candidate->candidate_path, &expected_id,
                                                      &manifest, ctx->ops);
}

static app_error_code_t validate_quarantine_record(void *context,
                                                   const storage_atomic_candidate_t *candidate) {
    const validate_context_t *ctx = context;
    app_uuid_t expected_id;
    if (!match_prefixed_uuid(candidate->destination, STORAGE_DATA_MOUNT "/quarantine/", ".json",
                             &expected_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_quarantine_entry_t entry;
    return storage_quarantine_read_record_with_ops(candidate->candidate_path, &expected_id, &entry,
                                                   ctx->ops);
}

static storage_atomic_validate_fn validator_for_type(storage_atomic_object_type_t type) {
    switch (type) {
    case STORAGE_ATOMIC_OBJECT_SCHEMA_MARKER:
        return validate_schema_marker;
    case STORAGE_ATOMIC_OBJECT_SET_INDEX:
    case STORAGE_ATOMIC_OBJECT_GLOBAL_MACRO_INDEX:
        return validate_index;
    case STORAGE_ATOMIC_OBJECT_SET_METADATA:
        return validate_set_metadata;
    case STORAGE_ATOMIC_OBJECT_TRANSACTION_MANIFEST:
        return validate_transaction_manifest;
    case STORAGE_ATOMIC_OBJECT_QUARANTINE_RECORD:
        return validate_quarantine_record;
    case STORAGE_ATOMIC_OBJECT_UNKNOWN:
    case STORAGE_ATOMIC_OBJECT_MACRO:
    case STORAGE_ATOMIC_OBJECT_PROCEDURE:
    case STORAGE_ATOMIC_OBJECT_PROGRESS:
    case STORAGE_ATOMIC_OBJECT_SETTINGS:
    default:
        return NULL;
    }
}

app_error_code_t storage_atomic_validate_candidate(const storage_fs_ops_t *operations,
                                                   const char *destination,
                                                   const char *candidate_path) {
    if (!storage_fs_ops_is_valid(operations) || destination == NULL || candidate_path == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const storage_atomic_object_type_t type = storage_atomic_classify_destination(destination);
    const storage_atomic_validate_fn validator = validator_for_type(type);
    if (validator == NULL) {
        /* No object-specific validator: recovery must never activate this candidate. */
        return APP_ERROR_NOT_FOUND;
    }
    validate_context_t context = {.ops = operations};
    const storage_atomic_candidate_t candidate = {.destination = destination,
                                                  .candidate_path = candidate_path};
    return validator(&context, &candidate);
}
