#include "storage_repository.h"

#include <errno.h>
#include <stdbool.h>
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

/* Public set functions serialize their whole read-check-write transaction behind
 * the repository mutation lock (FIX1 §7.5); the `_locked` helpers below do the
 * work and must be called only with the lock held, never reacquiring it. */
static app_error_code_t storage_set_list_locked(storage_set_list_t *out_list);
static app_error_code_t storage_set_create_locked(const macro_set_t *set);
static app_error_code_t storage_set_update_locked(const macro_set_t *replacement,
                                                  uint32_t expected_revision,
                                                  macro_set_t *out_updated);
static app_error_code_t storage_set_delete_locked(const app_uuid_t *set_id,
                                                  uint32_t expected_revision);

app_error_code_t storage_set_read(const app_uuid_t *set_id, macro_set_t *out_set) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_read_locked(set_id, out_set);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_set_list(storage_set_list_t *out_list) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_list_locked(out_list);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_set_create(const macro_set_t *set) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_create_locked(set);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_set_update(const macro_set_t *replacement, uint32_t expected_revision,
                                    macro_set_t *out_updated) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        storage_set_update_locked(replacement, expected_revision, out_updated);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_set_delete(const app_uuid_t *set_id, uint32_t expected_revision) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_delete_locked(set_id, expected_revision);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set) {
    if (out_set != NULL) {
        memset(out_set, 0, sizeof(*out_set));
    }
    if (set_id == NULL || out_set == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_document_t document = {0};
    const app_error_code_t result = storage_repository_load_set_document(set_id, &document);
    if (result == APP_ERROR_NONE) {
        *out_set = document.set;
    }
    storage_set_document_free(&document);
    return result;
}

static app_error_code_t storage_set_list_locked(storage_set_list_t *out_list) {
    if (out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    /* The index is the order (SPEC 12.3); the list is built by walking it, never
     * by listing the directory. */
    for (size_t item = 0U; item < index.count; ++item) {
        result = storage_set_read_locked(&index.ids[item], &out_list->items[item]);
        if (result != APP_ERROR_NONE) {
            memset(out_list, 0, sizeof(*out_list));
            return result;
        }
    }
    out_list->count = index.count;
    return APP_ERROR_NONE;
}

static app_error_code_t prepare_set_create(const macro_set_t *set, storage_set_index_t *index) {
    app_error_code_t result = storage_repository_load_index(index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (index->count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t item = 0U; item < index->count; ++item) {
        if (app_uuid_equal(&index->ids[item], &set->id)) {
            return APP_ERROR_CONFLICT;
        }
    }
    char path[APP_PATH_MAX_BYTES];
    result = storage_make_set_path(&set->id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(path, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    return errno == ENOENT ? APP_ERROR_NONE : storage_repository_map_file_error();
}

static app_error_code_t storage_set_create_locked(const macro_set_t *set) {
    if (set == NULL || set->revision != 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_index_t index = {0};
    app_error_code_t result = prepare_set_create(set, &index);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    /* A new set is a file with an empty macros array -- one write, not a
     * directory tree with an order file to keep in step (SPEC 12.1). */
    const storage_set_document_t document = {.set = *set};
    result = storage_repository_store_set_document(&document);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    /* Index last: until this write lands the new file is unreferenced, and an
     * unreferenced file is invisible rather than corrupt. */
    index.ids[index.count++] = set->id;
    result = storage_repository_write_index(&index);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t cleanup = storage_repository_remove_set_file(&set->id);
        return cleanup == APP_ERROR_NONE ? result : cleanup;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t storage_set_update_locked(const macro_set_t *replacement,
                                                  uint32_t expected_revision,
                                                  macro_set_t *out_updated) {
    if (replacement == NULL || out_updated == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_document_t document = {0};
    app_error_code_t result = storage_repository_load_set_document(&replacement->id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (document.set.revision != expected_revision || replacement->revision != expected_revision) {
        storage_set_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    if (document.set.revision == UINT32_MAX) {
        storage_set_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    /* Only the set's own fields change; its macros are carried through
     * untouched, because they live in the same file. */
    const macro_set_t updated = {
        .schema_version = replacement->schema_version,
        .id = replacement->id,
        .revision = replacement->revision + 1U,
    };
    document.set = updated;
    memcpy(document.set.name, replacement->name, sizeof(document.set.name));
    result = storage_repository_store_set_document(&document);
    if (result == APP_ERROR_NONE) {
        *out_updated = document.set;
    }
    storage_set_document_free(&document);
    return result;
}

static app_error_code_t storage_set_delete_locked(const app_uuid_t *set_id,
                                                  uint32_t expected_revision) {
    if (set_id == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    macro_set_t current = {0};
    app_error_code_t result = storage_set_read_locked(set_id, &current);
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

    /* Deleting the active set clears it, in the same write that drops it from
     * the order -- the two cannot disagree because they are one file (SPEC
     * 12.3). The UI returns to set selection (SPEC 8.6 step 5). */
    if (index.has_active_set && app_uuid_equal(&index.active_set_id, set_id)) {
        index.has_active_set = false;
        memset(&index.active_set_id, 0, sizeof(index.active_set_id));
    }

    /* Deletion is permanent (SPEC 8.6). The index is written first so the set
     * stops being referenced before its bytes go: an interruption between the
     * two leaves an unreferenced file, which every reader ignores because they
     * all enumerate through the index. */
    for (size_t item = found; item + 1U < index.count; ++item) {
        index.ids[item] = index.ids[item + 1U];
    }
    --index.count;
    result = storage_repository_write_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return storage_repository_remove_set_file(set_id);
}

static app_error_code_t storage_set_select_locked(const app_uuid_t *set_id) {
    /* Reading the set first is what makes selection reject an id that is in the
     * index but whose file is missing or damaged, rather than activating a set
     * the user cannot use. */
    macro_set_t set = {0};
    app_error_code_t result = storage_set_read_locked(set_id, &set);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    storage_set_index_t index = {0};
    result = storage_repository_load_index(&index);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    bool present = false;
    for (size_t item = 0U; item < index.count; ++item) {
        present = present || app_uuid_equal(&index.ids[item], set_id);
    }
    if (!present) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (index.has_active_set && app_uuid_equal(&index.active_set_id, set_id)) {
        /* Already active: nothing to write, and no revision to burn. */
        return APP_ERROR_NONE;
    }
    index.has_active_set = true;
    index.active_set_id = *set_id;
    return storage_repository_write_index(&index);
}

app_error_code_t storage_set_select(const app_uuid_t *set_id) {
    if (set_id == NULL || !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_set_select_locked(set_id);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_active_set_read(bool *out_has_active_set, app_uuid_t *out_set_id) {
    if (out_has_active_set == NULL || out_set_id == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_has_active_set = false;
    memset(out_set_id, 0, sizeof(*out_set_id));
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    storage_set_index_t index = {0};
    const app_error_code_t result = storage_repository_load_index(&index);
    if (result == APP_ERROR_NONE) {
        *out_has_active_set = index.has_active_set;
        *out_set_id = index.active_set_id;
    }
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
