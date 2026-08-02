#include "storage_repository_document.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository.h"
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
        const app_error_code_t discard = storage_repository_discard_corrupt_file(path, result);
        return discard == APP_ERROR_NONE ? result : discard;
    }
    return result;
}

/* Sums the bytes every set file currently occupies, optionally excluding one --
 * the set about to be rewritten, whose old bytes are being replaced rather than
 * added to.
 *
 * This measures what is actually on disk rather than trusting the per-object
 * limits, which is the whole point of SPEC 10.7: those limits would permit
 * 50 x 100 x 4096 bytes = 20 MB on a 512 KiB partition. */
/* The directory entry being measured and the one set path to skip. Grouped into
 * one struct so the two never become adjacent same-typed parameters that a
 * caller can transpose. */
typedef struct {
    const char *entry_name;
    const char *excluded_path;
} measure_request_t;

/* Size of one entry in the sets directory, or 0 for anything that should not be
 * counted (a subdirectory, the excluded set, a file that vanished between
 * readdir and stat). */
static app_error_code_t measure_entry(measure_request_t request, size_t *out_bytes) {
    *out_bytes = 0U;
    if (strcmp(request.entry_name, ".") == 0 || strcmp(request.entry_name, "..") == 0) {
        return APP_ERROR_NONE;
    }
    char path[APP_PATH_MAX_BYTES];
    const int written =
        snprintf(path, sizeof(path), STORAGE_DATA_MOUNT "/sets/%s", request.entry_name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (request.excluded_path[0] != '\0' && strcmp(path, request.excluded_path) == 0) {
        return APP_ERROR_NONE;
    }
    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
    }
    if (S_ISREG(metadata.st_mode)) {
        *out_bytes = (size_t)metadata.st_size;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_measure_user_data(const app_uuid_t *exclude_set_id,
                                                      size_t *out_bytes) {
    if (out_bytes == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_bytes = 0U;
    char excluded[APP_PATH_MAX_BYTES] = {0};
    if (exclude_set_id != NULL) {
        const app_error_code_t path_result =
            storage_make_set_path(exclude_set_id, excluded, sizeof(excluded));
        if (path_result != APP_ERROR_NONE) {
            return path_result;
        }
    }

    DIR *directory = opendir(STORAGE_DATA_MOUNT "/sets");
    if (directory == NULL) {
        /* No sets directory yet is an empty repository, not a failure. */
        return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
    }
    app_error_code_t result = APP_ERROR_NONE;
    size_t total = 0U;
    while (result == APP_ERROR_NONE) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                result = storage_repository_map_file_error();
            }
            break;
        }
        size_t entry_bytes = 0U;
        result = measure_entry(
            (measure_request_t){.entry_name = entry->d_name, .excluded_path = excluded},
            &entry_bytes);
        total += entry_bytes;
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = storage_repository_map_file_error();
    }
    if (result == APP_ERROR_NONE) {
        *out_bytes = total;
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
    /* Measured before the write, so an over-budget set is refused with 507
     * rather than filling the partition and failing partway (SPEC 10.7, 17).
     * The set's own current bytes are excluded because they are being replaced,
     * not added to. */
    size_t existing_bytes = 0U;
    result = storage_repository_measure_user_data(&document->set.id, &existing_bytes);
    if (result == APP_ERROR_NONE && existing_bytes + length > APP_USER_DATA_MAX_BYTES) {
        result = APP_ERROR_STORAGE_FULL;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_atomic_write(path, json, length, true);
    }
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
