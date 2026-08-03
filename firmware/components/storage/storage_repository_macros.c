#include "storage_repository.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage_object_json.h"
#include "storage_repository_document.h"
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"

/*
 * Every macro operation is: load the set file, mutate its ordered `macros`
 * array, write the file back (SPEC 12.1). There is no per-macro file and no
 * order file, so there is no pair of writes that could disagree with each
 * other, and array position IS the user's order.
 */

static bool location_valid(const app_uuid_t *set_id) {
    return set_id != NULL && app_uuid_is_valid_string(set_id->value);
}

/* Deep-copies one stored macro out of a loaded document, so the caller owns a
 * macro whose lifetime is independent of the document. */
static app_error_code_t copy_macro_out(const macro_t *source, macro_t *out_macro) {
    *out_macro = *source;
    out_macro->source = NULL;
    out_macro->source_length = 0U;
    char *copy = malloc(source->source_length + 1U);
    if (copy == NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
        return APP_ERROR_INTERNAL;
    }
    memcpy(copy, source->source, source->source_length + 1U);
    out_macro->source = copy;
    out_macro->source_length = source->source_length;
    return APP_ERROR_NONE;
}

/* Replaces the document's macro at `position` with `replacement`, taking
 * ownership of the replacement's source. */
static void assign_macro(storage_package_document_t *document, size_t position,
                         const macro_t *replacement) {
    macro_model_free_macro(&document->macros[position]);
    document->macros[position] = *replacement;
}

static app_error_code_t document_reserve(storage_package_document_t *document, size_t additional) {
    const size_t required = document->macro_count + additional;
    if (required > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_MACRO_LIMIT;
    }
    macro_t *grown = realloc(document->macros, required * sizeof(*grown));
    if (grown == NULL) {
        return APP_ERROR_INTERNAL;
    }
    document->macros = grown;
    memset(&document->macros[document->macro_count], 0, additional * sizeof(*grown));
    return APP_ERROR_NONE;
}

app_error_code_t storage_macro_read_locked(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                           macro_t *out_macro) {
    if (out_macro != NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
    }
    if (!location_valid(set_id) || macro_id == NULL || out_macro == NULL ||
        !app_uuid_is_valid_string(macro_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t position = storage_repository_find_macro(&document, macro_id);
    result = position == SIZE_MAX ? APP_ERROR_NOT_FOUND
                                  : copy_macro_out(&document.macros[position], out_macro);
    storage_package_document_free(&document);
    return result;
}

app_error_code_t storage_macro_list_detail_locked(const app_uuid_t *set_id,
                                                  storage_macro_list_t *out_list,
                                                  storage_object_ref_t *out_failed,
                                                  storage_skip_record_t *out_skips) {
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
    if (out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_list, 0, sizeof(*out_list));
    if (!location_valid(set_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* out_skips exists for the tolerant backup read (SPEC 17). With macros
     * inline there is no such thing as one individually unreadable macro: the
     * set file parses as a whole or not at all, so nothing is ever skipped and
     * a damaged file is reported as the failure it is. */
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        if (out_failed != NULL) {
            out_failed->has_id = true;
            out_failed->id = *set_id;
        }
        return result;
    }
    if (document.macro_count != 0U) {
        out_list->items = calloc(document.macro_count, sizeof(*out_list->items));
        if (out_list->items == NULL) {
            storage_package_document_free(&document);
            return APP_ERROR_INTERNAL;
        }
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < document.macro_count; ++index) {
        result = copy_macro_out(&document.macros[index], &out_list->items[index]);
        if (result == APP_ERROR_NONE) {
            out_list->count = index + 1U;
        }
    }
    storage_package_document_free(&document);
    if (result != APP_ERROR_NONE) {
        storage_macro_list_free(out_list);
    }
    (void)out_skips;
    return result;
}

app_error_code_t storage_macro_list_locked(const app_uuid_t *set_id,
                                           storage_macro_list_t *out_list) {
    return storage_macro_list_detail_locked(set_id, out_list, NULL, NULL);
}

/* SPEC 3.10: "Reject malformed or unsafe state rather than silently substituting
 * defaults." A macro whose source will not compile is malformed, and storing one
 * defers the failure to whoever runs it -- or, as happened on a real device, to
 * whoever next tries to back the repository up, since the export validates the
 * package it writes and that validation compiles every macro.
 *
 * The parser is the authority on what a macro means, so it is the authority on
 * whether one can be stored. The same check already backs
 * POST /api/v1/sets/{id}/macros/validate; creation and update simply did not
 * use it, and returned 201 for sources the device could never execute. */
static app_error_code_t macro_source_is_storable(const macro_t *macro) {
    const macro_compile_options_t options = {
        .key_press_ms = macro->key_press_ms,
        .inter_key_ms = macro->inter_key_ms,
    };
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result =
        macro_compile(macro->source, macro->source_length, &options, &plan, &error);
    macro_plan_free(&plan);
    return result;
}

static app_error_code_t macro_create_locked(const app_uuid_t *set_id, const macro_t *macro) {
    if (!location_valid(set_id) || macro == NULL || macro->revision != 1U ||
        !app_uuid_is_valid_string(macro->id.value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t storable = macro_source_is_storable(macro);
    if (storable != APP_ERROR_NONE) {
        return storable;
    }
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (storage_repository_find_macro(&document, &macro->id) != SIZE_MAX) {
        storage_package_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    result = document_reserve(&document, 1U);
    macro_t stored = {0};
    if (result == APP_ERROR_NONE) {
        result = copy_macro_out(macro, &stored);
    }
    if (result == APP_ERROR_NONE) {
        stored.set_id = *set_id;
        /* Appended, because a new macro goes at the end of the user's order. */
        document.macros[document.macro_count] = stored;
        ++document.macro_count;
        result = storage_repository_store_package_document(&document);
    }
    storage_package_document_free(&document);
    return result;
}

static app_error_code_t macro_update_locked(const app_uuid_t *set_id, const macro_t *replacement,
                                            uint32_t expected_revision, macro_t *out_updated) {
    if (out_updated != NULL) {
        memset(out_updated, 0, sizeof(*out_updated));
    }
    if (!location_valid(set_id) || replacement == NULL || out_updated == NULL ||
        expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t storable = macro_source_is_storable(replacement);
    if (storable != APP_ERROR_NONE) {
        return storable;
    }
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t position = storage_repository_find_macro(&document, &replacement->id);
    if (position == SIZE_MAX) {
        storage_package_document_free(&document);
        return APP_ERROR_NOT_FOUND;
    }
    if (document.macros[position].revision != expected_revision ||
        replacement->revision != expected_revision || expected_revision == UINT32_MAX) {
        storage_package_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    macro_t updated = {0};
    result = copy_macro_out(replacement, &updated);
    if (result == APP_ERROR_NONE) {
        updated.revision = expected_revision + 1U;
        updated.set_id = *set_id;
        /* Updated in place: an edit must not move a macro in the user's
         * order. */
        assign_macro(&document, position, &updated);
        result = storage_repository_store_package_document(&document);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_macro_out(&document.macros[position], out_updated);
    }
    storage_package_document_free(&document);
    return result;
}

static app_error_code_t macro_delete_locked(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                            uint32_t expected_revision) {
    if (!location_valid(set_id) || macro_id == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t position = storage_repository_find_macro(&document, macro_id);
    if (position == SIZE_MAX) {
        storage_package_document_free(&document);
        return APP_ERROR_NOT_FOUND;
    }
    if (document.macros[position].revision != expected_revision) {
        storage_package_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    macro_model_free_macro(&document.macros[position]);
    for (size_t index = position; index + 1U < document.macro_count; ++index) {
        document.macros[index] = document.macros[index + 1U];
    }
    --document.macro_count;
    result = storage_repository_store_package_document(&document);
    storage_package_document_free(&document);
    return result;
}

static app_error_code_t macro_duplicate_locked(const app_uuid_t *set_id,
                                               const app_uuid_t *source_id,
                                               const app_uuid_t *duplicate_id,
                                               const char *duplicate_name, macro_t *out_duplicate) {
    if (out_duplicate != NULL) {
        memset(out_duplicate, 0, sizeof(*out_duplicate));
    }
    if (!location_valid(set_id) || source_id == NULL || duplicate_id == NULL ||
        duplicate_name == NULL || out_duplicate == NULL ||
        !app_uuid_is_valid_string(duplicate_id->value) || duplicate_name[0] == '\0' ||
        strlen(duplicate_name) > APP_MACRO_NAME_MAX_BYTES ||
        app_uuid_equal(source_id, duplicate_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t position = storage_repository_find_macro(&document, source_id);
    if (position == SIZE_MAX) {
        storage_package_document_free(&document);
        return APP_ERROR_NOT_FOUND;
    }
    if (storage_repository_find_macro(&document, duplicate_id) != SIZE_MAX) {
        storage_package_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    macro_t duplicate = {0};
    result = copy_macro_out(&document.macros[position], &duplicate);
    if (result == APP_ERROR_NONE) {
        duplicate.id = *duplicate_id;
        duplicate.revision = 1U;
        duplicate.set_id = *set_id;
        memset(duplicate.name, 0, sizeof(duplicate.name));
        memcpy(duplicate.name, duplicate_name, strlen(duplicate_name));
        result = document_reserve(&document, 1U);
    }
    if (result == APP_ERROR_NONE) {
        document.macros[document.macro_count] = duplicate;
        ++document.macro_count;
        result = storage_repository_store_package_document(&document);
    } else {
        macro_model_free_macro(&duplicate);
    }
    if (result == APP_ERROR_NONE) {
        result = copy_macro_out(&document.macros[document.macro_count - 1U], out_duplicate);
    }
    storage_package_document_free(&document);
    return result;
}

static app_error_code_t macro_reorder_locked(const app_uuid_t *set_id,
                                             const app_uuid_t *ordered_ids, size_t count) {
    if (!location_valid(set_id) || (ordered_ids == NULL && count != 0U) ||
        count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_document_t document = {0};
    app_error_code_t result = storage_repository_load_package_document(set_id, &document);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    /* A reorder must be a permutation of exactly the macros the set already
     * has: a request that adds, drops, or repeats one is a conflict, not a
     * partial reorder to apply as far as it goes. */
    if (count != document.macro_count) {
        storage_package_document_free(&document);
        return APP_ERROR_CONFLICT;
    }
    macro_t *reordered = count == 0U ? NULL : calloc(count, sizeof(*reordered));
    if (count != 0U && reordered == NULL) {
        storage_package_document_free(&document);
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < count; ++index) {
        const size_t position = storage_repository_find_macro(&document, &ordered_ids[index]);
        bool duplicated = false;
        for (size_t prior = 0U; prior < index; ++prior) {
            duplicated = duplicated || app_uuid_equal(&ordered_ids[prior], &ordered_ids[index]);
        }
        if (position == SIZE_MAX || duplicated) {
            free(reordered);
            storage_package_document_free(&document);
            return APP_ERROR_CONFLICT;
        }
        reordered[index] = document.macros[position];
    }
    free(document.macros);
    document.macros = reordered;
    result = storage_repository_store_package_document(&document);
    storage_package_document_free(&document);
    return result;
}

app_error_code_t storage_macro_list(const app_uuid_t *set_id, storage_macro_list_t *out_list) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_macro_list_locked(set_id, out_list);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

void storage_macro_list_free(storage_macro_list_t *list) {
    if (list == NULL) {
        return;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        macro_model_free_macro(&list->items[index]);
    }
    free(list->items);
    memset(list, 0, sizeof(*list));
}

app_error_code_t storage_macro_create(const app_uuid_t *set_id, const macro_t *macro) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_create_locked(set_id, macro);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_read(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                    macro_t *out_macro) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = storage_macro_read_locked(set_id, macro_id, out_macro);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_update(const app_uuid_t *set_id, const macro_t *replacement,
                                      uint32_t expected_revision, macro_t *out_updated) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_update_locked(set_id, replacement, expected_revision, out_updated);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_delete(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                      uint32_t expected_revision) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_delete_locked(set_id, macro_id, expected_revision);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_duplicate(const app_uuid_t *set_id, const app_uuid_t *source_id,
                                         const app_uuid_t *duplicate_id, const char *duplicate_name,
                                         macro_t *out_duplicate) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result =
        macro_duplicate_locked(set_id, source_id, duplicate_id, duplicate_name, out_duplicate);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t storage_macro_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,
                                       size_t count) {
    const app_error_code_t lock = storage_repository_lock_take();
    if (lock != APP_ERROR_NONE) {
        return lock;
    }
    const app_error_code_t result = macro_reorder_locked(set_id, ordered_ids, count);
    const app_error_code_t unlock = storage_repository_lock_give();
    return unlock == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}
