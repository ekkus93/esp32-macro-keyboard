#include "storage_package.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage_json.h"
#include "storage_object_json.h"

#define STORAGE_PACKAGE_FIELD_NAME_BYTES 32U
#define STORAGE_PACKAGE_FIELD_COUNT 5U
/* "skipped" is the only optional member. A complete backup omits it entirely,
 * so complete packages are byte-identical to those written before partial
 * backups existed and remain readable by anything that predates this field. */
#define STORAGE_PACKAGE_OPTIONAL_FIELDS (UINT32_C(1) << PACKAGE_FIELD_SKIPPED)
#define STORAGE_PACKAGE_ARRAY_COUNT 2U
#define STORAGE_PACKAGE_LOCAL_MACROS_MAX                                                           \
    ((size_t)APP_MACRO_SETS_MAX * (size_t)APP_MACROS_PER_SET_MAX)

typedef enum {
    PACKAGE_FIELD_SCHEMA_VERSION = 0,
    PACKAGE_FIELD_TYPE,
    PACKAGE_FIELD_SETS,
    PACKAGE_FIELD_MACROS,
    PACKAGE_FIELD_SKIPPED,
} package_field_t;

typedef enum {
    PACKAGE_ARRAY_SETS = 0,
    PACKAGE_ARRAY_MACROS,
    PACKAGE_ARRAY_COUNT,
} package_array_t;

typedef struct {
    uint32_t schema_version;
    storage_package_kind_t kind;
    cJSON *root;
    const cJSON *arrays[STORAGE_PACKAGE_ARRAY_COUNT];
} package_document_t;

typedef struct {
    app_uuid_t id;
} package_metadata_t;

typedef struct {
    app_uuid_t id;
    app_uuid_t set_id;
} package_macro_metadata_t;

typedef struct {
    size_t count;
    size_t item_size;
} allocation_shape_t;

typedef enum {
    VALIDATION_ALLOCATION_SETS = 0,
    VALIDATION_ALLOCATION_MACROS,
    VALIDATION_ALLOCATION_SET_MACRO_COUNTS,
    VALIDATION_ALLOCATION_COUNT,
} validation_allocation_t;

typedef struct {
    package_metadata_t *sets;
    package_macro_metadata_t *macros;
    size_t *set_macro_counts;
    size_t set_capacity;
    size_t macro_capacity;
    size_t set_count;
    size_t macro_count;
} package_validation_state_t;

typedef app_error_code_t (*package_object_callback_t)(const cJSON *object, void *context);

static const char *const PACKAGE_FIELDS[STORAGE_PACKAGE_FIELD_COUNT] = {
    "schema_version", "package_type", "sets", "macros", "skipped",
};

/* The package is parsed once, by cJSON, and validated by walking that tree.
 *
 * This replaced a hand-rolled streaming scanner of about 545 lines whose only
 * product was per-object text slices -- which were then handed to cJSON one at
 * a time, so cJSON parsed every object anyway and the scanner was a second parse
 * of the envelope around them. Restore made that explicit: it validated with the
 * scanner and then parsed the identical bytes with cJSON in the same request.
 *
 * The usual reason to hand-roll one is bounded memory, and that argument does
 * not hold here: packages cap at APP_IMPORT_PACKAGE_MAX_BYTES (512 KiB) against
 * 8 MB of PSRAM, and the restore path already built the full tree. */
static app_error_code_t parse_package_document(const char *data, size_t length,
                                               package_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, false);
    if (root == NULL || parse_end != data + length || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* `skipped` is optional (SPEC 17): a complete package omits it entirely. */
    app_error_code_t result = storage_json_check_object_fields(
        root, PACKAGE_FIELDS, STORAGE_PACKAGE_FIELD_COUNT, STORAGE_PACKAGE_FIELD_COUNT - 1U);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_document->schema_version);
    }
    char kind[sizeof("backup")] = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_string(root, "package_type", kind, sizeof(kind), true);
    }
    if (result == APP_ERROR_NONE) {
        if (strcmp(kind, "set") == 0) {
            out_document->kind = STORAGE_PACKAGE_KIND_SET;
        } else if (strcmp(kind, "backup") == 0) {
            out_document->kind = STORAGE_PACKAGE_KIND_BACKUP;
        } else {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result == APP_ERROR_NONE) {
        static const char *const names[STORAGE_PACKAGE_ARRAY_COUNT] = {
            [PACKAGE_ARRAY_SETS] = "sets",
            [PACKAGE_ARRAY_MACROS] = "macros",
        };
        for (size_t index = 0U; index < STORAGE_PACKAGE_ARRAY_COUNT; ++index) {
            const cJSON *array = cJSON_GetObjectItemCaseSensitive(root, names[index]);
            if (!cJSON_IsArray(array)) {
                result = APP_ERROR_INVALID_ARGUMENT;
                break;
            }
            out_document->arrays[index] = array;
        }
    }
    if (result != APP_ERROR_NONE) {
        cJSON_Delete(root);
        memset(out_document, 0, sizeof(*out_document));
        /* A malformed envelope is a bad argument from the caller, not a corrupt
         * stored object: the package came in over the API. The field helpers
         * report APP_ERROR_STORAGE_CORRUPT because they are shared with the
         * repository, where the bytes did come off the filesystem. */
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_document->root = root;
    return APP_ERROR_NONE;
}

static void close_package_document(package_document_t *document) {
    cJSON_Delete(document->root);
    memset(document, 0, sizeof(*document));
}

static bool add_allocation_budget(const allocation_shape_t *shape, size_t *in_out_total) {
    if (shape->count == 0U) {
        return true;
    }
    if (shape->item_size == 0U || shape->count > SIZE_MAX / shape->item_size) {
        return false;
    }
    const size_t bytes = shape->count * shape->item_size;
    if (bytes > APP_IMPORT_PACKAGE_MAX_BYTES) {
        return false;
    }
    if (*in_out_total > APP_IMPORT_PACKAGE_MAX_BYTES - bytes) {
        return false;
    }
    *in_out_total += bytes;
    return true;
}

static app_error_code_t allocate_items(const allocation_shape_t *shape, void **out_items) {
    *out_items = NULL;
    if (shape->count == 0U) {
        return APP_ERROR_NONE;
    }
    if (shape->item_size == 0U || shape->count > SIZE_MAX / shape->item_size) {
        return APP_ERROR_MACRO_LIMIT;
    }
    void *items = calloc(shape->count, shape->item_size);
    if (items == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_items = items;
    return APP_ERROR_NONE;
}

static void free_validation_state(package_validation_state_t *state) {
    if (state == NULL) {
        return;
    }
    free(state->sets);
    free(state->macros);
    free(state->set_macro_counts);
    memset(state, 0, sizeof(*state));
}

static app_error_code_t allocate_validation_state(const storage_package_summary_t *summary,
                                                  package_validation_state_t *out_state) {
    memset(out_state, 0, sizeof(*out_state));
    out_state->set_capacity = summary->set_count;
    out_state->macro_capacity = summary->local_macro_count;

    const allocation_shape_t shapes[VALIDATION_ALLOCATION_COUNT] = {
        [VALIDATION_ALLOCATION_SETS] = {.count = out_state->set_capacity,
                                        .item_size = sizeof(*out_state->sets)},
        [VALIDATION_ALLOCATION_MACROS] = {.count = out_state->macro_capacity,
                                          .item_size = sizeof(*out_state->macros)},
        [VALIDATION_ALLOCATION_SET_MACRO_COUNTS] = {.count = out_state->set_capacity,
                                                    .item_size =
                                                        sizeof(*out_state->set_macro_counts)},
    };
    size_t allocation_bytes = 0U;
    for (size_t index = 0U; index < VALIDATION_ALLOCATION_COUNT; ++index) {
        if (!add_allocation_budget(&shapes[index], &allocation_bytes)) {
            return APP_ERROR_MACRO_LIMIT;
        }
    }

    app_error_code_t result =
        allocate_items(&shapes[VALIDATION_ALLOCATION_SETS], (void **)&out_state->sets);
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[VALIDATION_ALLOCATION_MACROS], (void **)&out_state->macros);
    }
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[VALIDATION_ALLOCATION_SET_MACRO_COUNTS],
                                (void **)&out_state->set_macro_counts);
    }
    if (result != APP_ERROR_NONE) {
        free_validation_state(out_state);
    }
    return result;
}

static size_t find_package_index(const package_validation_state_t *state,
                                 const app_uuid_t *set_id) {
    for (size_t index = 0U; index < state->set_count; ++index) {
        if (app_uuid_equal(&state->sets[index].id, set_id)) {
            return index;
        }
    }
    return SIZE_MAX;
}

static size_t find_macro_index(const package_validation_state_t *state,
                               const app_uuid_t *macro_id) {
    for (size_t index = 0U; index < state->macro_count; ++index) {
        if (app_uuid_equal(&state->macros[index].id, macro_id)) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t external_object_result(app_error_code_t result) {
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t validate_package_object(const cJSON *object, void *context) {
    package_validation_state_t *state = context;
    if (state == NULL || state->set_count >= state->set_capacity) {
        return APP_ERROR_INTERNAL;
    }
    macro_package_t set = {0};
    const app_error_code_t parsed =
        external_object_result(storage_repository_parse_package_node(object, &set));
    if (parsed != APP_ERROR_NONE) {
        return parsed;
    }
    if (find_package_index(state, &set.id) != SIZE_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    state->sets[state->set_count].id = set.id;
    ++state->set_count;
    return APP_ERROR_NONE;
}

static app_error_code_t compile_package_macro(const macro_t *macro) {
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

static app_error_code_t validate_macro_object(const cJSON *object, void *context) {
    package_validation_state_t *state = context;
    if (state == NULL || state->macro_count >= state->macro_capacity) {
        return APP_ERROR_INTERNAL;
    }
    macro_t macro = {0};
    app_error_code_t result =
        external_object_result(storage_repository_parse_macro_node(object, &macro));
    if (result == APP_ERROR_NONE && find_macro_index(state, &macro.id) != SIZE_MAX) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    size_t set_index = SIZE_MAX;
    if (result == APP_ERROR_NONE) {
        set_index = find_package_index(state, &macro.set_id);
        if (set_index == SIZE_MAX || state->set_macro_counts[set_index] >= APP_MACROS_PER_SET_MAX) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = compile_package_macro(&macro);
    }
    if (result == APP_ERROR_NONE) {
        package_macro_metadata_t *metadata = &state->macros[state->macro_count];
        metadata->id = macro.id;
        metadata->set_id = macro.set_id;
        ++state->macro_count;
        if (set_index != SIZE_MAX) {
            ++state->set_macro_counts[set_index];
        }
    }
    macro_model_free_macro(&macro);
    return result;
}

/* cJSON has already parsed the arrays, so counting is reading their length --
 * where the scanner had to walk every element a second time to do it. */
static app_error_code_t visit_object_array(const cJSON *array, size_t maximum_count,
                                           package_object_callback_t callback, void *context,
                                           size_t *out_count) {
    const int size = cJSON_GetArraySize(array);
    if (size < 0 || (size_t)size > maximum_count) {
        return APP_ERROR_MACRO_LIMIT;
    }
    if (out_count != NULL) {
        *out_count = (size_t)size;
    }
    if (callback == NULL) {
        return APP_ERROR_NONE;
    }
    const cJSON *element = NULL;
    cJSON_ArrayForEach(element, array) {
        if (!cJSON_IsObject(element)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        const app_error_code_t result = callback(element, context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t count_package_arrays(const package_document_t *document,
                                             storage_package_summary_t *out_summary) {
    app_error_code_t result =
        visit_object_array(document->arrays[PACKAGE_ARRAY_SETS], APP_MACRO_SETS_MAX, NULL, NULL,
                           &out_summary->set_count);
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(document->arrays[PACKAGE_ARRAY_MACROS],
                                    STORAGE_PACKAGE_LOCAL_MACROS_MAX, NULL, NULL,
                                    &out_summary->local_macro_count);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    /* A set package carries exactly one set; a backup carries the repository.
     * Either way the macros have to fit in the sets that are present. */
    if ((document->kind == STORAGE_PACKAGE_KIND_SET && out_summary->set_count != 1U) ||
        out_summary->local_macro_count > out_summary->set_count * APP_MACROS_PER_SET_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t validate_package_objects(const package_document_t *document,
                                                 const storage_package_summary_t *summary) {
    package_validation_state_t state = {0};
    app_error_code_t result = allocate_validation_state(summary, &state);
    size_t visited = 0U;
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(document->arrays[PACKAGE_ARRAY_SETS], state.set_capacity,
                                    validate_package_object, &state, &visited);
    }
    if (result == APP_ERROR_NONE) {
        result =
            visit_object_array(document->arrays[PACKAGE_ARRAY_MACROS], summary->local_macro_count,
                               validate_macro_object, &state, &visited);
    }
    if (result == APP_ERROR_NONE && (state.set_count != summary->set_count ||
                                     state.macro_count != summary->local_macro_count)) {
        result = APP_ERROR_INTERNAL;
    }
    free_validation_state(&state);
    return result;
}

/* Temporary diagnostic (2026-08-02). Restore and import both answer 422 with no
 * per-set outcomes on hardware while the same document validates on the host,
 * so the stage that rejects it has to be observed on the target rather than
 * reasoned about. Guarded so the host build is unchanged. Logs codes and counts
 * only -- never document content, which would carry macro text (SPEC 20.2). */
#if defined(ESP_PLATFORM)
#include "esp_log.h"
#define PACKAGE_DIAG(...) ESP_LOGE("package_diag", __VA_ARGS__)
#else
#define PACKAGE_DIAG(...) ((void)0)
#endif

app_error_code_t storage_package_validate(const char *data, size_t length,
                                          storage_package_kind_t expected_kind,
                                          storage_package_summary_t *out_summary) {
    if (out_summary != NULL) {
        memset(out_summary, 0, sizeof(*out_summary));
    }
    if (data == NULL || length == 0U || out_summary == NULL ||
        (expected_kind != STORAGE_PACKAGE_KIND_SET &&
         expected_kind != STORAGE_PACKAGE_KIND_BACKUP)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (length > APP_IMPORT_PACKAGE_MAX_BYTES) {
        return APP_ERROR_MACRO_LIMIT;
    }

    package_document_t document = {0};
    app_error_code_t result = parse_package_document(data, length, &document);
    if (result != APP_ERROR_NONE) {
        PACKAGE_DIAG("package parse failed: len=%u result=%d expected kind=%d", (unsigned)length,
                     (int)result, (int)expected_kind);
        return result;
    }
    if (result == APP_ERROR_NONE && document.kind != expected_kind) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {
        .kind = document.kind,
        .package_bytes = length,
    };
    if (result == APP_ERROR_NONE) {
        result = count_package_arrays(&document, &summary);
        if (result != APP_ERROR_NONE) {
            PACKAGE_DIAG("package count failed: result=%d sets=%u macros=%u", (int)result,
                         (unsigned)summary.set_count, (unsigned)summary.local_macro_count);
        }
    }
    if (result == APP_ERROR_NONE) {
        result = validate_package_objects(&document, &summary);
        if (result != APP_ERROR_NONE) {
            PACKAGE_DIAG("package object validation failed: result=%d", (int)result);
        }
    }
    if (result == APP_ERROR_NONE) {
        *out_summary = summary;
    }
    close_package_document(&document);
    return result;
}
