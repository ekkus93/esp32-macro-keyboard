#include "storage_repository.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
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
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository_document.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_sets_internal.h"

static app_error_code_t copy_text(char *destination, size_t destination_size, const char *source,
                                  size_t maximum_length) {
    if (destination == NULL || destination_size == 0U || source == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t length = strlen(source);
    if (length == 0U || length > maximum_length || length >= destination_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(destination, 0, destination_size);
    memcpy(destination, source, length + 1U);
    return APP_ERROR_NONE;
}

static bool index_contains_set(const storage_set_index_t *index, const app_uuid_t *set_id) {
    for (size_t position = 0U; position < index->count; ++position) {
        if (app_uuid_equal(&index->ids[position], set_id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t index_accepts_duplicate(const storage_set_index_t *index,
                                                const app_uuid_t *duplicate_id) {
    if (index->count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t index_position = 0U; index_position < index->count; ++index_position) {
        if (app_uuid_equal(&index->ids[index_position], duplicate_id)) {
            return APP_ERROR_CONFLICT;
        }
    }
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(duplicate_id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(destination, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
}

/* Duplicating a set is now a read and a write of one file each: load the source
 * document, stamp the duplicate's identity onto it, store it. Every macro keeps
 * its own id and is reset to revision 1, and the order is preserved because the
 * array order IS the order (SPEC 12.1). */
static app_error_code_t build_duplicate_document(const app_uuid_t *source_id,
                                                 const macro_set_t *duplicate,
                                                 storage_set_document_t *out_document) {
    app_error_code_t result = storage_repository_load_set_document(source_id, out_document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    out_document->set = *duplicate;
    for (size_t index = 0U; index < out_document->macro_count; ++index) {
        out_document->macros[index].revision = 1U;
        out_document->macros[index].set_id = duplicate->id;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t storage_set_duplicate_locked(const app_uuid_t *source_id,
                                                     uint32_t expected_revision,
                                                     const app_uuid_t *duplicate_id,
                                                     const char *duplicate_name,
                                                     macro_set_t *out_duplicate) {
    if (out_duplicate != NULL) {
        memset(out_duplicate, 0, sizeof(*out_duplicate));
    }
    if (source_id == NULL || expected_revision == 0U || duplicate_id == NULL ||
        duplicate_name == NULL || out_duplicate == NULL ||
        !app_uuid_is_valid_string(source_id->value) ||
        !app_uuid_is_valid_string(duplicate_id->value) || app_uuid_equal(source_id, duplicate_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result == APP_ERROR_NONE && !index_contains_set(&index, source_id)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = index_accepts_duplicate(&index, duplicate_id);
    }
    macro_set_t source = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_set_read_locked(source_id, &source);
    }
    if (result == APP_ERROR_NONE && source.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    macro_set_t duplicate = source;
    if (result == APP_ERROR_NONE) {
        duplicate.id = *duplicate_id;
        duplicate.revision = 1U;
        result =
            copy_text(duplicate.name, sizeof(duplicate.name), duplicate_name, APP_NAME_MAX_BYTES);
    }

    storage_set_document_t document = {0};
    bool written = false;
    if (result == APP_ERROR_NONE) {
        result = build_duplicate_document(source_id, &duplicate, &document);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_store_set_document(&document);
        written = result == APP_ERROR_NONE;
    }
    storage_set_document_free(&document);
    if (result == APP_ERROR_NONE) {
        /* Index last, as in set creation: the duplicate is unreferenced until
         * this write lands. */
        index.ids[index.count++] = duplicate.id;
        result = storage_repository_write_index(&index);
    }
    if (result != APP_ERROR_NONE && written) {
        const app_error_code_t cleanup = storage_repository_remove_set_file(&duplicate.id);
        if (cleanup != APP_ERROR_NONE) {
            result = cleanup;
        }
    }
    if (result == APP_ERROR_NONE) {
        *out_duplicate = duplicate;
    }
    return result;
}
app_error_code_t storage_set_duplicate(const app_uuid_t *source_id, uint32_t expected_revision,
                                       const app_uuid_t *duplicate_id, const char *duplicate_name,
                                       macro_set_t *out_duplicate) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_duplicate_locked(
        source_id, expected_revision, duplicate_id, duplicate_name, out_duplicate);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

static bool order_has_exact_members(const storage_set_index_t *current,
                                    const app_uuid_t *ordered_ids, size_t count) {
    if (current->count != count) {
        return false;
    }
    for (size_t replacement_index = 0U; replacement_index < count; ++replacement_index) {
        if (!app_uuid_is_valid_string(ordered_ids[replacement_index].value)) {
            return false;
        }
        bool found = false;
        for (size_t current_index = 0U; current_index < current->count; ++current_index) {
            if (app_uuid_equal(&ordered_ids[replacement_index], &current->ids[current_index])) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        for (size_t earlier = 0U; earlier < replacement_index; ++earlier) {
            if (app_uuid_equal(&ordered_ids[replacement_index], &ordered_ids[earlier])) {
                return false;
            }
        }
    }
    return true;
}

static app_error_code_t storage_set_reorder_locked(const app_uuid_t *ordered_ids, size_t count) {
    if ((ordered_ids == NULL && count != 0U) || count > APP_MACRO_SETS_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_index_t current = {0};
    app_error_code_t result = storage_repository_load_index(&current);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!order_has_exact_members(&current, ordered_ids, count)) {
        return APP_ERROR_CONFLICT;
    }
    storage_set_index_t replacement = {.count = count};
    if (count > 0U) {
        memcpy(replacement.ids, ordered_ids, count * sizeof(*ordered_ids));
    }
    return storage_repository_write_index(&replacement);
}

app_error_code_t storage_set_reorder(const app_uuid_t *ordered_ids, size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_reorder_locked(ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
