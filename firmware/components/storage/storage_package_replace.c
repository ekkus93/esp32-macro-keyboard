#include "storage_package.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_object_json.h"
#include "storage_package_reader.h"
#include "storage_repository_document.h"
#include "storage_repository_lock.h"
#include "storage_repository_packages_internal.h"

/* The package's own tree, plus the single set it replaces the target with.
 * `sets[0]` is that set: a set package carries exactly one, which
 * storage_package_validate has already enforced by the time this is built.
 * SPEC 8.7 describes a set export in the singular; the count is checked in
 * storage_package.c, not stated as a rule anywhere in the specification. */
typedef struct {
    package_tree_t tree;
    macro_package_t replacement;
} package_replace_document_t;

static void close_document(package_replace_document_t *document) {
    if (document == NULL) {
        return;
    }
    package_tree_close(&document->tree);
    memset(document, 0, sizeof(*document));
}

static app_error_code_t open_document(const char *data, size_t length,
                                      package_replace_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    app_error_code_t result = package_tree_open(data, length, &out_document->tree);
    if (result == APP_ERROR_NONE) {
        result = package_parse_metadata_node(cJSON_GetArrayItem(out_document->tree.sets, 0),
                                             &out_document->replacement);
    }
    if (result != APP_ERROR_NONE) {
        close_document(out_document);
    }
    return result;
}

/* Assembles the replacement set as one document. Macro identities and revisions
 * come from the package unchanged: replacement substitutes the set's contents,
 * it does not mint new identities the way import-new does. */
static app_error_code_t materialize_package(const package_replace_document_t *document,
                                            storage_package_document_t *out_stored) {
    const cJSON *array = document->tree.macros;
    const int count = cJSON_GetArraySize(array);
    if (count < 0 || (size_t)count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_stored, 0, sizeof(*out_stored));
    out_stored->set = document->replacement;
    if (count > 0) {
        out_stored->macros = calloc((size_t)count, sizeof(*out_stored->macros));
        if (out_stored->macros == NULL) {
            return APP_ERROR_INTERNAL;
        }
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        macro_t macro = {0};
        result = package_parse_macro_node(cJSON_GetArrayItem(array, index), &macro);
        if (result == APP_ERROR_NONE && !app_uuid_equal(&macro.set_id, &document->replacement.id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        if (result != APP_ERROR_NONE) {
            macro_model_free_macro(&macro);
            break;
        }
        out_stored->macros[out_stored->macro_count] = macro;
        ++out_stored->macro_count;
    }
    if (result != APP_ERROR_NONE) {
        storage_package_document_free(out_stored);
    }
    return result;
}

/* SPEC 8.7 replacement, in one atomic write.
 *
 * A set is one file now, so the whole replacement is a single
 * storage_atomic_write: stage `<set>.json.tmp`, verify it, rename it over the
 * target. An interruption at any point leaves either the complete old set or
 * the complete new one, with no staging directory, no manifest, and no window
 * in which the set is half-written.
 *
 * Phase 3 had to remove the old tree and rebuild it in place, which was not
 * atomic and said so at this call site. That window is closed.
 */
static app_error_code_t replace_locked(const app_uuid_t *target_package_id,
                                       uint32_t expected_revision,
                                       package_replace_document_t *document,
                                       macro_package_t *out_package) {
    macro_package_t current = {0};
    app_error_code_t result = storage_package_read_locked(target_package_id, &current);
    if (result == APP_ERROR_NONE && current.revision != expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    storage_package_document_t stored = {0};
    if (result == APP_ERROR_NONE) {
        result = materialize_package(document, &stored);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_store_package_document(&stored);
    }
    storage_package_document_free(&stored);
    if (result == APP_ERROR_NONE) {
        result = storage_package_read_locked(target_package_id, out_package);
    }
    if (result == APP_ERROR_NONE && out_package->revision != document->replacement.revision) {
        memset(out_package, 0, sizeof(*out_package));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

app_error_code_t storage_package_replace(const app_uuid_t *target_package_id,
                                         uint32_t expected_revision, const char *data,
                                         size_t length, macro_package_t *out_package) {
    if (out_package != NULL) {
        memset(out_package, 0, sizeof(*out_package));
    }
    if (target_package_id == NULL || expected_revision == 0U || data == NULL || length == 0U ||
        out_package == NULL || !app_uuid_is_valid_string(target_package_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {0};
    app_error_code_t result =
        storage_package_validate(data, length, STORAGE_PACKAGE_KIND_SET, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    package_replace_document_t document = {0};
    result = open_document(data, length, &document);
    if (result == APP_ERROR_NONE && !app_uuid_equal(target_package_id, &document.replacement.id)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_lock_take();
    }
    if (result == APP_ERROR_NONE) {
        result = replace_locked(target_package_id, expected_revision, &document, out_package);
        const app_error_code_t unlock = storage_repository_lock_give();
        if (unlock != APP_ERROR_NONE) {
            memset(out_package, 0, sizeof(*out_package));
            result = APP_ERROR_INTERNAL;
        }
    }
    close_document(&document);
    return result;
}
