#include "storage_package.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_package_reader.h"
#include "storage_repository_document.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_sets_internal.h"

/* The package's own tree, plus the set identity it was exported under.
 * `sets[0]` is that set: SPEC 12.3 requires a set package to carry exactly one,
 * and storage_package_validate has already enforced it by the time this is
 * built. Import needs it separately from the tree because every macro must name
 * it, and every macro is then restamped with the new identity. */
typedef struct {
    package_tree_t tree;
    macro_set_t source_set;
} package_import_document_t;

/* Rewrite target for materializing a parsed package under a brand-new set identity:
 * every macro node in the package must already declare
 * `source_set_id` (self-consistency with the package's own sets[0].id), and gets
 * `new_set_id` stamped on write, mirroring storage_repository_set_operations.c's
 * write_duplicate_macro pattern applied to package content instead of live objects. */
typedef struct {
    const app_uuid_t *source_set_id;
    const app_uuid_t *new_set_id;
} package_import_rewrite_t;

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    return APP_ERROR_IO;
}

static void close_document(package_import_document_t *document) {
    if (document == NULL) {
        return;
    }
    package_tree_close(&document->tree);
    memset(document, 0, sizeof(*document));
}

static app_error_code_t open_document(const char *data, size_t length,
                                      package_import_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    app_error_code_t result = package_tree_open(data, length, &out_document->tree);
    if (result == APP_ERROR_NONE) {
        result = package_parse_set_node(cJSON_GetArrayItem(out_document->tree.sets, 0),
                                        &out_document->source_set);
    }
    if (result != APP_ERROR_NONE) {
        close_document(out_document);
    }
    return result;
}

/* Assembles the imported set as one document and writes it as one file.
 * Every macro is stamped with the new set identity and reset to revision 1,
 * because import-new mints a fresh identity for everything it materializes. */
static app_error_code_t materialize_set(const package_import_document_t *document,
                                        const macro_set_t *new_set,
                                        const package_import_rewrite_t *rewrite) {
    const cJSON *array = document->tree.macros;
    const int count = cJSON_GetArraySize(array);
    if (count < 0 || (size_t)count > APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_set_document_t stored = {.set = *new_set};
    if (count > 0) {
        stored.macros = calloc((size_t)count, sizeof(*stored.macros));
        if (stored.macros == NULL) {
            return APP_ERROR_INTERNAL;
        }
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        macro_t macro = {0};
        result = package_parse_macro_node(cJSON_GetArrayItem(array, index), &macro);
        if (result == APP_ERROR_NONE && !app_uuid_equal(&macro.set_id, rewrite->source_set_id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        if (result != APP_ERROR_NONE) {
            macro_model_free_macro(&macro);
            break;
        }
        macro.set_id = *rewrite->new_set_id;
        macro.revision = 1U;
        stored.macros[stored.macro_count] = macro;
        ++stored.macro_count;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_repository_store_set_document(&stored);
    }
    storage_set_document_free(&stored);
    return result;
}

static app_error_code_t import_set_accepts_new_id(const storage_set_index_t *index,
                                                  const app_uuid_t *new_set_id) {
    if (index->count >= APP_MACRO_SETS_MAX) {
        return APP_ERROR_STORAGE_FULL;
    }
    for (size_t position = 0U; position < index->count; ++position) {
        if (app_uuid_equal(&index->ids[position], new_set_id)) {
            return APP_ERROR_CONFLICT;
        }
    }
    char destination[APP_PATH_MAX_BYTES];
    app_error_code_t result = storage_make_set_path(new_set_id, destination, sizeof(destination));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(destination, &metadata) == 0) {
        return APP_ERROR_CONFLICT;
    }
    return errno == ENOENT ? APP_ERROR_NONE : map_error_number(errno);
}

static app_error_code_t import_locked(const app_uuid_t *new_set_id,
                                      package_import_document_t *document, macro_set_t *out_set) {
    storage_set_index_t index = {0};
    app_error_code_t result = storage_repository_load_index(&index);
    if (result == APP_ERROR_NONE) {
        result = import_set_accepts_new_id(&index, new_set_id);
    }
    macro_set_t new_set = document->source_set;
    if (result == APP_ERROR_NONE) {
        new_set.id = *new_set_id;
        new_set.revision = 1U;
    }
    const package_import_rewrite_t rewrite = {
        .source_set_id = &document->source_set.id,
        .new_set_id = new_set_id,
    };
    bool written = false;
    if (result == APP_ERROR_NONE) {
        result = materialize_set(document, &new_set, &rewrite);
        written = result == APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        /* Index last: the imported set is unreferenced until this write lands. */
        index.ids[index.count++] = new_set.id;
        result = storage_repository_write_index(&index);
    }
    if (result != APP_ERROR_NONE && written) {
        const app_error_code_t cleanup = storage_repository_remove_set_file(&new_set.id);
        if (cleanup != APP_ERROR_NONE) {
            result = cleanup;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = storage_set_read_locked(new_set_id, out_set);
    }
    if (result == APP_ERROR_NONE && out_set->revision != 1U) {
        memset(out_set, 0, sizeof(*out_set));
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

app_error_code_t storage_package_import_set(const app_uuid_t *new_set_id, const char *data,
                                            size_t length, macro_set_t *out_set) {
    if (out_set != NULL) {
        memset(out_set, 0, sizeof(*out_set));
    }
    if (new_set_id == NULL || data == NULL || length == 0U || out_set == NULL ||
        !app_uuid_is_valid_string(new_set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {0};
    app_error_code_t result =
        storage_package_validate(data, length, STORAGE_PACKAGE_KIND_SET, &summary);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    package_import_document_t document = {0};
    result = open_document(data, length, &document);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_lock_take();
    }
    if (result == APP_ERROR_NONE) {
        result = import_locked(new_set_id, &document, out_set);
        const app_error_code_t unlock = storage_repository_lock_give();
        if (unlock != APP_ERROR_NONE) {
            memset(out_set, 0, sizeof(*out_set));
            result = APP_ERROR_INTERNAL;
        }
    }
    close_document(&document);
    return result;
}
