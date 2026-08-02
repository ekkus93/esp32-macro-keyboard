#include "storage_repository_document.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository_internal.h"

/* A set is one file (SPEC 13.3), so every read is "parse one file" and every
 * write is "serialize one file, stage it as .tmp, rename it" (SPEC 13.4). These
 * two functions are the whole of that, and the set and macro repositories are
 * both written on top of them rather than each reimplementing it. */

app_error_code_t storage_repository_load_set_document(const app_uuid_t *set_id,
                                                      storage_set_document_t *out_document) {
    if (out_document != NULL) {
        memset(out_document, 0, sizeof(*out_document));
    }
    if (set_id == NULL || out_document == NULL || !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(set_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char *data = NULL;
    size_t length = 0U;
    result = storage_repository_read_bounded_file(path, STORAGE_SET_FILE_MAX_BYTES, &data, &length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = storage_set_document_parse(data, length, out_document);
    free(data);
    if (result == APP_ERROR_NONE && !app_uuid_equal(&out_document->set.id, set_id)) {
        /* A set file whose own id does not match its name is corrupt, not a set
         * that has moved. */
        storage_set_document_free(out_document);
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        /* SPEC 13.6: discard the damaged file and report the failure. */
        const app_error_code_t discard = storage_repository_discard_corrupt_file(path);
        return discard == APP_ERROR_NONE ? result : discard;
    }
    return result;
}

app_error_code_t storage_repository_store_set_document(const storage_set_document_t *document) {
    if (document == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(&document->set.id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    char *json = NULL;
    size_t length = 0U;
    result = storage_set_document_serialize(document, &json, &length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = storage_atomic_write(path, json, length, true);
    cJSON_free(json);
    return result;
}

app_error_code_t storage_repository_remove_set_file(const app_uuid_t *set_id) {
    char path[APP_PATH_MAX_BYTES];
    const app_error_code_t result = storage_make_set_path(set_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (unlink(path) == 0) {
        return APP_ERROR_NONE;
    }
    return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
}

size_t storage_repository_find_macro(const storage_set_document_t *document,
                                     const app_uuid_t *macro_id) {
    for (size_t index = 0U; index < document->macro_count; ++index) {
        if (app_uuid_equal(&document->macros[index].id, macro_id)) {
            return index;
        }
    }
    return SIZE_MAX;
}
