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
#include "storage_object_json.h"
#include "storage_repository_internal.h"
#include "storage_transaction_internal.h"

#define SCHEMA_MARKER_MAX_BYTES 128U

typedef struct {
    const storage_fs_ops_t *ops;
} validate_context_t;

typedef struct {
    app_uuid_t set_id;
    app_uuid_t object_id;
} set_object_identity_t;

static bool parse_uuid_slice(const char *text, app_uuid_t *out_uuid) {
    char uuid_text[APP_UUID_BUFFER_LENGTH];
    memcpy(uuid_text, text, APP_UUID_STRING_LENGTH);
    uuid_text[APP_UUID_STRING_LENGTH] = '\0';
    return app_uuid_parse(uuid_text, out_uuid) == APP_ERROR_NONE;
}

static bool match_prefixed_uuid(const char *path, const char *prefix, const char *suffix,
                                app_uuid_t *out_uuid) {
    if (path == NULL || prefix == NULL || suffix == NULL) {
        return false;
    }
    const size_t prefix_length = strlen(prefix);
    const size_t suffix_length = strlen(suffix);
    const size_t path_length = strlen(path);
    if (path_length != prefix_length + APP_UUID_STRING_LENGTH + suffix_length ||
        strncmp(path, prefix, prefix_length) != 0 ||
        strcmp(path + prefix_length + APP_UUID_STRING_LENGTH, suffix) != 0) {
        return false;
    }
    app_uuid_t parsed = {0};
    if (!parse_uuid_slice(path + prefix_length, &parsed)) {
        return false;
    }
    if (out_uuid != NULL) {
        *out_uuid = parsed;
    }
    return true;
}

static bool match_set_object(const char *path, const char *directory,
                             set_object_identity_t *out_identity) {
    static const char set_prefix[] = STORAGE_DATA_MOUNT "/sets/";
    static const char json_suffix[] = ".json";
    if (path == NULL || directory == NULL || out_identity == NULL ||
        strncmp(path, set_prefix, sizeof(set_prefix) - 1U) != 0) {
        return false;
    }
    const char *set_text = path + sizeof(set_prefix) - 1U;
    if (!parse_uuid_slice(set_text, &out_identity->set_id)) {
        return false;
    }
    const char *separator = set_text + APP_UUID_STRING_LENGTH;
    const size_t directory_length = strlen(directory);
    if (separator[0] != '/' || strncmp(separator + 1, directory, directory_length) != 0 ||
        separator[1 + directory_length] != '/') {
        return false;
    }
    const char *object_text = separator + directory_length + 2U;
    if (!parse_uuid_slice(object_text, &out_identity->object_id)) {
        return false;
    }
    return strcmp(object_text + APP_UUID_STRING_LENGTH, json_suffix) == 0;
}

static bool match_set_order(const char *path, const char *filename, app_uuid_t *out_set_id) {
    static const char prefix[] = STORAGE_DATA_MOUNT "/sets/";
    if (path == NULL || filename == NULL || out_set_id == NULL ||
        strncmp(path, prefix, sizeof(prefix) - 1U) != 0) {
        return false;
    }
    const char *uuid_text = path + sizeof(prefix) - 1U;
    if (!parse_uuid_slice(uuid_text, out_set_id)) {
        return false;
    }
    const char *tail = uuid_text + APP_UUID_STRING_LENGTH;
    return tail[0] == '/' && strcmp(tail + 1, filename) == 0;
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
    if (match_prefixed_uuid(destination, STORAGE_DATA_MOUNT "/sets/", "/set.json", NULL)) {
        return STORAGE_ATOMIC_OBJECT_SET_METADATA;
    }
    app_uuid_t set_id = {0};
    if (match_set_order(destination, "macro-order.json", &set_id)) {
        return STORAGE_ATOMIC_OBJECT_SET_MACRO_INDEX;
    }
    if (match_set_order(destination, "procedure-order.json", &set_id)) {
        return STORAGE_ATOMIC_OBJECT_PROCEDURE_INDEX;
    }
    set_object_identity_t identity = {0};
    if (match_set_object(destination, "macros", &identity) ||
        match_prefixed_uuid(destination, STORAGE_DATA_MOUNT "/global/macros/", ".json",
                            &identity.object_id)) {
        return STORAGE_ATOMIC_OBJECT_MACRO;
    }
    if (match_set_object(destination, "procedures", &identity)) {
        return STORAGE_ATOMIC_OBJECT_PROCEDURE;
    }
    if (match_set_object(destination, "progress", &identity)) {
        return STORAGE_ATOMIC_OBJECT_PROGRESS;
    }
    return STORAGE_ATOMIC_OBJECT_UNKNOWN;
}

static app_error_code_t read_candidate(const validate_context_t *context,
                                       const storage_atomic_candidate_t *candidate, size_t maximum,
                                       char **out_data, size_t *out_length) {
    return storage_repository_read_bounded_file_with_ops(candidate->candidate_path, maximum,
                                                         out_data, out_length, context->ops);
}

static app_error_code_t validate_schema_marker(void *context,
                                               const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_candidate(validation, candidate, SCHEMA_MARKER_MAX_BYTES, &data, &length);
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

static size_t order_limit_for_type(storage_atomic_object_type_t type) {
    switch (type) {
    case STORAGE_ATOMIC_OBJECT_SET_INDEX:
        return APP_MACRO_SETS_MAX;
    case STORAGE_ATOMIC_OBJECT_GLOBAL_MACRO_INDEX:
    case STORAGE_ATOMIC_OBJECT_SET_MACRO_INDEX:
        return APP_MACROS_PER_SET_MAX;
    case STORAGE_ATOMIC_OBJECT_PROCEDURE_INDEX:
        return APP_PROCEDURES_PER_SET_MAX;
    default:
        return 0U;
    }
}

static app_error_code_t validate_index(void *context, const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    const storage_atomic_object_type_t type =
        storage_atomic_classify_destination(candidate->destination);
    const size_t maximum = order_limit_for_type(type);
    if (maximum == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_candidate(validation, candidate, STORAGE_ORDER_FILE_MAX_BYTES, &data, &length);
    if (result == APP_ERROR_NONE) {
        /* storage_uuid_order_t is ~8 KB and this also runs on main_task's
         * 8 KiB stack during startup recovery. */
        storage_uuid_order_t *order = calloc(1U, sizeof(*order));
        if (order == NULL) {
            free(data);
            return APP_ERROR_INTERNAL;
        }
        result = storage_repository_parse_order_json(data, length, order, maximum);
        free(order);
    }
    free(data);
    return result;
}

static app_error_code_t validate_set_metadata(void *context,
                                              const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    app_uuid_t expected_id = {0};
    if (!match_prefixed_uuid(candidate->destination, STORAGE_DATA_MOUNT "/sets/", "/set.json",
                             &expected_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_candidate(validation, candidate, STORAGE_SET_FILE_MAX_BYTES, &data, &length);
    macro_set_t set = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_set_json(data, length, &set);
    }
    free(data);
    return result == APP_ERROR_NONE && !app_uuid_equal(&expected_id, &set.id)
               ? APP_ERROR_STORAGE_CORRUPT
               : result;
}

static app_error_code_t validate_macro(void *context, const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    set_object_identity_t identity = {0};
    const bool set_local = match_set_object(candidate->destination, "macros", &identity);
    if (!set_local &&
        !match_prefixed_uuid(candidate->destination, STORAGE_DATA_MOUNT "/global/macros/", ".json",
                             &identity.object_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_candidate(validation, candidate, STORAGE_MACRO_FILE_MAX_BYTES, &data, &length);
    macro_t macro = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_macro_json(data, length, &macro);
    }
    free(data);
    if (result == APP_ERROR_NONE && !app_uuid_equal(&identity.object_id, &macro.id)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE &&
        (set_local != (macro.scope == MACRO_SCOPE_SET) ||
         (set_local && !app_uuid_equal(&identity.set_id, &macro.set_id)))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    macro_model_free_macro(&macro);
    return result;
}

static app_error_code_t validate_procedure(void *context,
                                           const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    set_object_identity_t identity = {0};
    if (!match_set_object(candidate->destination, "procedures", &identity)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_candidate(validation, candidate, STORAGE_PROCEDURE_FILE_MAX_BYTES, &data, &length);
    procedure_t procedure = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_procedure_json(data, length, &procedure);
    }
    free(data);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(&identity.object_id, &procedure.id) ||
                                     !app_uuid_equal(&identity.set_id, &procedure.set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    macro_model_free_procedure(&procedure);
    return result;
}

static app_error_code_t validate_progress(void *context,
                                          const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    set_object_identity_t identity = {0};
    if (!match_set_object(candidate->destination, "progress", &identity)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_candidate(validation, candidate, STORAGE_PROGRESS_FILE_MAX_BYTES, &data, &length);
    /* procedure_progress_t is ~16 KB. This validator runs from
     * storage_atomic_recover_all() during startup, i.e. on main_task's 8 KiB
     * stack, so a stack local here made an interrupted progress write into an
     * unbootable device. */
    procedure_progress_t *progress = calloc(1U, sizeof(*progress));
    if (progress == NULL) {
        free(data);
        return APP_ERROR_INTERNAL;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_progress_json(data, length, progress);
    }
    free(data);
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&identity.object_id, &progress->procedure_id) ||
         !app_uuid_equal(&identity.set_id, &progress->set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    free(progress);
    return result;
}

static app_error_code_t validate_transaction_manifest(void *context,
                                                      const storage_atomic_candidate_t *candidate) {
    const validate_context_t *validation = context;
    app_uuid_t expected_id = {0};
    if (!match_prefixed_uuid(candidate->destination, STORAGE_DATA_MOUNT "/transactions/", ".bin",
                             &expected_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_transaction_manifest_t manifest = {0};
    return storage_transaction_read_manifest_with_ops(candidate->candidate_path, &expected_id,
                                                      &manifest, validation->ops);
}

static storage_atomic_validate_fn validator_for_type(storage_atomic_object_type_t type) {
    switch (type) {
    case STORAGE_ATOMIC_OBJECT_SCHEMA_MARKER:
        return validate_schema_marker;
    case STORAGE_ATOMIC_OBJECT_SET_INDEX:
    case STORAGE_ATOMIC_OBJECT_GLOBAL_MACRO_INDEX:
    case STORAGE_ATOMIC_OBJECT_SET_MACRO_INDEX:
    case STORAGE_ATOMIC_OBJECT_PROCEDURE_INDEX:
        return validate_index;
    case STORAGE_ATOMIC_OBJECT_SET_METADATA:
        return validate_set_metadata;
    case STORAGE_ATOMIC_OBJECT_MACRO:
        return validate_macro;
    case STORAGE_ATOMIC_OBJECT_PROCEDURE:
        return validate_procedure;
    case STORAGE_ATOMIC_OBJECT_PROGRESS:
        return validate_progress;
    case STORAGE_ATOMIC_OBJECT_TRANSACTION_MANIFEST:
        return validate_transaction_manifest;
    case STORAGE_ATOMIC_OBJECT_UNKNOWN:
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
        return APP_ERROR_NOT_FOUND;
    }
    validate_context_t context = {.ops = operations};
    const storage_atomic_candidate_t candidate = {
        .destination = destination,
        .candidate_path = candidate_path,
    };
    return validator(&context, &candidate);
}
