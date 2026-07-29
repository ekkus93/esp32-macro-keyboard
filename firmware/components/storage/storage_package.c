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

#define STORAGE_PACKAGE_MAX_JSON_DEPTH 64U
#define STORAGE_PACKAGE_FIELD_NAME_BYTES 32U
#define STORAGE_PACKAGE_FIELD_COUNT 7U
#define STORAGE_PACKAGE_ARRAY_COUNT 5U
#define STORAGE_PACKAGE_LOCAL_MACROS_MAX \
    ((size_t)APP_MACRO_SETS_MAX * (size_t)APP_MACROS_PER_SET_MAX)
#define STORAGE_PACKAGE_PROCEDURES_MAX \
    ((size_t)APP_MACRO_SETS_MAX * (size_t)APP_PROCEDURES_PER_SET_MAX)
#define STORAGE_PACKAGE_GLOBAL_MACROS_MAX ((size_t)APP_MACROS_PER_SET_MAX)

typedef struct {
    const char *data;
    size_t length;
} json_span_t;

typedef struct {
    const char *data;
    size_t length;
    size_t offset;
} json_cursor_t;

typedef enum {
    PACKAGE_FIELD_SCHEMA_VERSION = 0,
    PACKAGE_FIELD_TYPE,
    PACKAGE_FIELD_SETS,
    PACKAGE_FIELD_MACROS,
    PACKAGE_FIELD_GLOBAL_MACROS,
    PACKAGE_FIELD_PROCEDURES,
    PACKAGE_FIELD_PROGRESS,
} package_field_t;

typedef enum {
    PACKAGE_ARRAY_SETS = 0,
    PACKAGE_ARRAY_MACROS,
    PACKAGE_ARRAY_GLOBAL_MACROS,
    PACKAGE_ARRAY_PROCEDURES,
    PACKAGE_ARRAY_PROGRESS,
} package_array_t;

typedef struct {
    uint32_t schema_version;
    storage_package_kind_t kind;
    json_span_t arrays[STORAGE_PACKAGE_ARRAY_COUNT];
} package_document_t;

typedef struct {
    app_uuid_t id;
} package_set_metadata_t;

typedef struct {
    app_uuid_t id;
    macro_scope_t scope;
    app_uuid_t set_id;
} package_macro_metadata_t;

typedef struct {
    app_uuid_t id;
    app_uuid_t set_id;
    uint32_t revision;
    json_span_t source;
} package_procedure_metadata_t;

typedef struct {
    app_uuid_t procedure_id;
} package_progress_metadata_t;

typedef struct {
    size_t count;
    size_t item_size;
} allocation_shape_t;

typedef struct {
    package_set_metadata_t *sets;
    package_macro_metadata_t *macros;
    package_procedure_metadata_t *procedures;
    package_progress_metadata_t *progress;
    size_t *set_macro_counts;
    size_t *set_procedure_counts;
    size_t set_capacity;
    size_t macro_capacity;
    size_t procedure_capacity;
    size_t progress_capacity;
    size_t set_count;
    size_t macro_count;
    size_t procedure_count;
    size_t progress_count;
} package_validation_state_t;

typedef struct {
    package_validation_state_t *state;
    macro_scope_t expected_scope;
} macro_validation_context_t;

typedef app_error_code_t (*package_object_callback_t)(const json_span_t *object, void *context);

static const char *const PACKAGE_FIELDS[STORAGE_PACKAGE_FIELD_COUNT] = {
    "schema_version", "package_type", "sets", "macros", "global_macros", "procedures",
    "progress",
};

static void skip_whitespace(json_cursor_t *cursor) {
    while (cursor->offset < cursor->length) {
        const char value = cursor->data[cursor->offset];
        if (value != ' ' && value != '\t' && value != '\n' && value != '\r') {
            break;
        }
        ++cursor->offset;
    }
}

static bool is_hex_digit(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
           (value >= 'A' && value <= 'F');
}

static app_error_code_t scan_json_value(json_cursor_t *cursor, size_t depth);

static app_error_code_t scan_json_string(json_cursor_t *cursor) {
    if (cursor->offset >= cursor->length || cursor->data[cursor->offset] != '"') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor->offset;
    while (cursor->offset < cursor->length) {
        const unsigned char value = (unsigned char)cursor->data[cursor->offset++];
        if (value == (unsigned char)'"') {
            return APP_ERROR_NONE;
        }
        if (value < 0x20U) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        if (value != (unsigned char)'\\') {
            continue;
        }
        if (cursor->offset >= cursor->length) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        const char escape = cursor->data[cursor->offset++];
        if (escape == '"' || escape == '\\' || escape == '/' || escape == 'b' || escape == 'f' ||
            escape == 'n' || escape == 'r' || escape == 't') {
            continue;
        }
        if (escape != 'u' || cursor->length - cursor->offset < 4U) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        bool all_zero = true;
        for (size_t index = 0U; index < 4U; ++index) {
            const char digit = cursor->data[cursor->offset + index];
            if (!is_hex_digit(digit)) {
                return APP_ERROR_INVALID_ARGUMENT;
            }
            if (digit != '0') {
                all_zero = false;
            }
        }
        if (all_zero) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        cursor->offset += 4U;
    }
    return APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t scan_json_number(json_cursor_t *cursor) {
    const size_t start = cursor->offset;
    if (cursor->offset < cursor->length && cursor->data[cursor->offset] == '-') {
        ++cursor->offset;
    }
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (cursor->data[cursor->offset] == '0') {
        ++cursor->offset;
        if (cursor->offset < cursor->length && cursor->data[cursor->offset] >= '0' &&
            cursor->data[cursor->offset] <= '9') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
    } else {
        if (cursor->data[cursor->offset] < '1' || cursor->data[cursor->offset] > '9') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        while (cursor->offset < cursor->length && cursor->data[cursor->offset] >= '0' &&
               cursor->data[cursor->offset] <= '9') {
            ++cursor->offset;
        }
    }
    if (cursor->offset < cursor->length && cursor->data[cursor->offset] == '.') {
        ++cursor->offset;
        const size_t fraction_start = cursor->offset;
        while (cursor->offset < cursor->length && cursor->data[cursor->offset] >= '0' &&
               cursor->data[cursor->offset] <= '9') {
            ++cursor->offset;
        }
        if (cursor->offset == fraction_start) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (cursor->offset < cursor->length &&
        (cursor->data[cursor->offset] == 'e' || cursor->data[cursor->offset] == 'E')) {
        ++cursor->offset;
        if (cursor->offset < cursor->length &&
            (cursor->data[cursor->offset] == '+' || cursor->data[cursor->offset] == '-')) {
            ++cursor->offset;
        }
        const size_t exponent_start = cursor->offset;
        while (cursor->offset < cursor->length && cursor->data[cursor->offset] >= '0' &&
               cursor->data[cursor->offset] <= '9') {
            ++cursor->offset;
        }
        if (cursor->offset == exponent_start) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    return cursor->offset > start ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

static bool consume_literal(json_cursor_t *cursor, const char *literal) {
    const size_t length = strlen(literal);
    if (cursor->length - cursor->offset < length ||
        memcmp(cursor->data + cursor->offset, literal, length) != 0) {
        return false;
    }
    cursor->offset += length;
    return true;
}

static app_error_code_t scan_json_array(json_cursor_t *cursor, size_t depth) {
    ++cursor->offset;
    skip_whitespace(cursor);
    if (cursor->offset < cursor->length && cursor->data[cursor->offset] == ']') {
        ++cursor->offset;
        return APP_ERROR_NONE;
    }
    for (;;) {
        app_error_code_t result = scan_json_value(cursor, depth + 1U);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        skip_whitespace(cursor);
        if (cursor->offset >= cursor->length) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        const char separator = cursor->data[cursor->offset++];
        if (separator == ']') {
            return APP_ERROR_NONE;
        }
        if (separator != ',') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        skip_whitespace(cursor);
    }
}

static app_error_code_t scan_json_object(json_cursor_t *cursor, size_t depth) {
    ++cursor->offset;
    skip_whitespace(cursor);
    if (cursor->offset < cursor->length && cursor->data[cursor->offset] == '}') {
        ++cursor->offset;
        return APP_ERROR_NONE;
    }
    for (;;) {
        app_error_code_t result = scan_json_string(cursor);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        skip_whitespace(cursor);
        if (cursor->offset >= cursor->length || cursor->data[cursor->offset] != ':') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        ++cursor->offset;
        result = scan_json_value(cursor, depth + 1U);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        skip_whitespace(cursor);
        if (cursor->offset >= cursor->length) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        const char separator = cursor->data[cursor->offset++];
        if (separator == '}') {
            return APP_ERROR_NONE;
        }
        if (separator != ',') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        skip_whitespace(cursor);
    }
}

static app_error_code_t scan_json_value(json_cursor_t *cursor, size_t depth) {
    if (depth > STORAGE_PACKAGE_MAX_JSON_DEPTH) {
        return APP_ERROR_MACRO_LIMIT;
    }
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (cursor->data[cursor->offset]) {
    case '"':
        return scan_json_string(cursor);
    case '{':
        return scan_json_object(cursor, depth);
    case '[':
        return scan_json_array(cursor, depth);
    case 't':
        return consume_literal(cursor, "true") ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
    case 'f':
        return consume_literal(cursor, "false") ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
    case 'n':
        return consume_literal(cursor, "null") ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
    default:
        return scan_json_number(cursor);
    }
}

static app_error_code_t capture_json_value(json_cursor_t *cursor, json_span_t *out_span) {
    skip_whitespace(cursor);
    const size_t start = cursor->offset;
    const app_error_code_t result = scan_json_value(cursor, 0U);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    out_span->data = cursor->data + start;
    out_span->length = cursor->offset - start;
    return APP_ERROR_NONE;
}

static app_error_code_t read_plain_string(json_cursor_t *cursor, char *output,
                                          size_t output_size) {
    if (output == NULL || output_size == 0U || cursor->offset >= cursor->length ||
        cursor->data[cursor->offset] != '"') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor->offset;
    const size_t start = cursor->offset;
    while (cursor->offset < cursor->length && cursor->data[cursor->offset] != '"') {
        const unsigned char value = (unsigned char)cursor->data[cursor->offset];
        if (value < 0x20U || value == (unsigned char)'\\') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        ++cursor->offset;
    }
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t length = cursor->offset - start;
    if (length == 0U || length >= output_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(output, cursor->data + start, length);
    output[length] = '\0';
    ++cursor->offset;
    return APP_ERROR_NONE;
}

static app_error_code_t read_u32(json_cursor_t *cursor, uint32_t *out_value) {
    if (out_value == NULL || cursor->offset >= cursor->length ||
        cursor->data[cursor->offset] < '0' || cursor->data[cursor->offset] > '9') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    uint32_t value = 0U;
    const size_t start = cursor->offset;
    while (cursor->offset < cursor->length && cursor->data[cursor->offset] >= '0' &&
           cursor->data[cursor->offset] <= '9') {
        const uint32_t digit = (uint32_t)(cursor->data[cursor->offset] - '0');
        if (value > (UINT32_MAX - digit) / 10U) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        value = value * 10U + digit;
        ++cursor->offset;
    }
    if (cursor->offset - start > 1U && cursor->data[start] == '0') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_value = value;
    return APP_ERROR_NONE;
}

static size_t package_field_index(const char *name) {
    for (size_t index = 0U; index < STORAGE_PACKAGE_FIELD_COUNT; ++index) {
        if (strcmp(name, PACKAGE_FIELDS[index]) == 0) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t read_package_kind(json_cursor_t *cursor,
                                          storage_package_kind_t *out_kind) {
    char value[sizeof("backup")];
    app_error_code_t result = read_plain_string(cursor, value, sizeof(value));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (strcmp(value, "set") == 0) {
        *out_kind = STORAGE_PACKAGE_KIND_SET;
        return APP_ERROR_NONE;
    }
    if (strcmp(value, "backup") == 0) {
        *out_kind = STORAGE_PACKAGE_KIND_BACKUP;
        return APP_ERROR_NONE;
    }
    return APP_ERROR_INVALID_ARGUMENT;
}

static package_array_t package_array_for_field(size_t field_index) {
    switch ((package_field_t)field_index) {
    case PACKAGE_FIELD_SETS:
        return PACKAGE_ARRAY_SETS;
    case PACKAGE_FIELD_MACROS:
        return PACKAGE_ARRAY_MACROS;
    case PACKAGE_FIELD_GLOBAL_MACROS:
        return PACKAGE_ARRAY_GLOBAL_MACROS;
    case PACKAGE_FIELD_PROCEDURES:
        return PACKAGE_ARRAY_PROCEDURES;
    case PACKAGE_FIELD_PROGRESS:
        return PACKAGE_ARRAY_PROGRESS;
    case PACKAGE_FIELD_SCHEMA_VERSION:
    case PACKAGE_FIELD_TYPE:
    default:
        return STORAGE_PACKAGE_ARRAY_COUNT;
    }
}

static app_error_code_t parse_package_field(json_cursor_t *cursor, size_t field_index,
                                            package_document_t *out_document) {
    if (field_index == PACKAGE_FIELD_SCHEMA_VERSION) {
        return read_u32(cursor, &out_document->schema_version);
    }
    if (field_index == PACKAGE_FIELD_TYPE) {
        return read_package_kind(cursor, &out_document->kind);
    }
    const package_array_t array_index = package_array_for_field(field_index);
    if ((size_t)array_index >= STORAGE_PACKAGE_ARRAY_COUNT) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = capture_json_value(cursor, &out_document->arrays[array_index]);
    if (result == APP_ERROR_NONE &&
        (out_document->arrays[array_index].length == 0U ||
         out_document->arrays[array_index].data[0] != '[')) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    return result;
}

static app_error_code_t parse_package_document(const char *data, size_t length,
                                               package_document_t *out_document) {
    memset(out_document, 0, sizeof(*out_document));
    json_cursor_t cursor = {
        .data = data,
        .length = length,
        .offset = 0U,
    };
    skip_whitespace(&cursor);
    if (cursor.offset >= cursor.length || cursor.data[cursor.offset] != '{') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor.offset;
    uint32_t seen = 0U;
    for (;;) {
        skip_whitespace(&cursor);
        if (cursor.offset >= cursor.length || cursor.data[cursor.offset] == '}') {
            if (cursor.offset < cursor.length) {
                ++cursor.offset;
            }
            break;
        }
        char name[STORAGE_PACKAGE_FIELD_NAME_BYTES];
        app_error_code_t result = read_plain_string(&cursor, name, sizeof(name));
        if (result != APP_ERROR_NONE) {
            return result;
        }
        const size_t field_index = package_field_index(name);
        if (field_index == SIZE_MAX || (seen & (UINT32_C(1) << field_index)) != 0U) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        skip_whitespace(&cursor);
        if (cursor.offset >= cursor.length || cursor.data[cursor.offset] != ':') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        ++cursor.offset;
        result = parse_package_field(&cursor, field_index, out_document);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        seen |= UINT32_C(1) << field_index;
        skip_whitespace(&cursor);
        if (cursor.offset >= cursor.length) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        const char separator = cursor.data[cursor.offset++];
        if (separator == '}') {
            break;
        }
        if (separator != ',') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    skip_whitespace(&cursor);
    const uint32_t required = (UINT32_C(1) << STORAGE_PACKAGE_FIELD_COUNT) - UINT32_C(1);
    if (cursor.offset != cursor.length || seen != required ||
        out_document->schema_version != APP_SCHEMA_VERSION) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t visit_object_array(const json_span_t *array, size_t maximum_count,
                                           package_object_callback_t callback, void *context,
                                           size_t *out_count) {
    if (array == NULL || out_count == NULL || array->length < 2U || array->data[0] != '[') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_count = 0U;
    json_cursor_t cursor = {
        .data = array->data,
        .length = array->length,
        .offset = 1U,
    };
    skip_whitespace(&cursor);
    if (cursor.offset < cursor.length && cursor.data[cursor.offset] == ']') {
        ++cursor.offset;
        skip_whitespace(&cursor);
        return cursor.offset == cursor.length ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
    }
    for (;;) {
        json_span_t object = {0};
        app_error_code_t result = capture_json_value(&cursor, &object);
        if (result != APP_ERROR_NONE || object.length == 0U || object.data[0] != '{') {
            return result == APP_ERROR_NONE ? APP_ERROR_INVALID_ARGUMENT : result;
        }
        if (*out_count >= maximum_count) {
            return APP_ERROR_MACRO_LIMIT;
        }
        if (callback != NULL) {
            result = callback(&object, context);
            if (result != APP_ERROR_NONE) {
                return result;
            }
        }
        ++*out_count;
        skip_whitespace(&cursor);
        if (cursor.offset >= cursor.length) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        const char separator = cursor.data[cursor.offset++];
        if (separator == ']') {
            skip_whitespace(&cursor);
            return cursor.offset == cursor.length ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
        }
        if (separator != ',') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        skip_whitespace(&cursor);
    }
}

static bool add_allocation_budget(const allocation_shape_t *shape, size_t *in_out_total) {
    if (shape->count == 0U) {
        return true;
    }
    if (shape->item_size == 0U || shape->count > SIZE_MAX / shape->item_size) {
        return false;
    }
    const size_t bytes = shape->count * shape->item_size;
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
    free(state->procedures);
    free(state->progress);
    free(state->set_macro_counts);
    free(state->set_procedure_counts);
    memset(state, 0, sizeof(*state));
}

static app_error_code_t allocate_validation_state(const storage_package_summary_t *summary,
                                                  package_validation_state_t *out_state) {
    memset(out_state, 0, sizeof(*out_state));
    if (summary->local_macro_count > SIZE_MAX - summary->global_macro_count) {
        return APP_ERROR_MACRO_LIMIT;
    }
    out_state->set_capacity = summary->set_count;
    out_state->macro_capacity = summary->local_macro_count + summary->global_macro_count;
    out_state->procedure_capacity = summary->procedure_count;
    out_state->progress_capacity = summary->progress_count;

    const allocation_shape_t shapes[] = {
        {.count = out_state->set_capacity, .item_size = sizeof(*out_state->sets)},
        {.count = out_state->macro_capacity, .item_size = sizeof(*out_state->macros)},
        {.count = out_state->procedure_capacity, .item_size = sizeof(*out_state->procedures)},
        {.count = out_state->progress_capacity, .item_size = sizeof(*out_state->progress)},
        {.count = out_state->set_capacity, .item_size = sizeof(*out_state->set_macro_counts)},
        {.count = out_state->set_capacity, .item_size = sizeof(*out_state->set_procedure_counts)},
    };
    size_t allocation_bytes = 0U;
    for (size_t index = 0U; index < sizeof(shapes) / sizeof(shapes[0]); ++index) {
        if (!add_allocation_budget(&shapes[index], &allocation_bytes)) {
            return APP_ERROR_MACRO_LIMIT;
        }
    }

    app_error_code_t result = allocate_items(&shapes[0], (void **)&out_state->sets);
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[1], (void **)&out_state->macros);
    }
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[2], (void **)&out_state->procedures);
    }
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[3], (void **)&out_state->progress);
    }
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[4], (void **)&out_state->set_macro_counts);
    }
    if (result == APP_ERROR_NONE) {
        result = allocate_items(&shapes[5], (void **)&out_state->set_procedure_counts);
    }
    if (result != APP_ERROR_NONE) {
        free_validation_state(out_state);
    }
    return result;
}

static size_t find_set_index(const package_validation_state_t *state, const app_uuid_t *set_id) {
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

static size_t find_procedure_index(const package_validation_state_t *state,
                                   const app_uuid_t *procedure_id) {
    for (size_t index = 0U; index < state->procedure_count; ++index) {
        if (app_uuid_equal(&state->procedures[index].id, procedure_id)) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t external_object_result(app_error_code_t result) {
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t parse_exact_set(const json_span_t *object, macro_set_t *out_set) {
    static const char *const fields[] = {
        "schema_version", "id",    "revision",        "name",       "description",
        "manufacturer",   "model", "board",           "keyboard_layout", "sort_order",
    };
    cJSON *root = NULL;
    app_error_code_t result = storage_json_parse_exact_object(
        object->data, object->length, fields, sizeof(fields) / sizeof(fields[0]), &root);
    cJSON_Delete(root);
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_set_json(object->data, object->length, out_set);
    }
    return external_object_result(result);
}

static app_error_code_t validate_set_object(const json_span_t *object, void *context) {
    package_validation_state_t *state = context;
    if (state == NULL || state->set_count >= state->set_capacity) {
        return APP_ERROR_INTERNAL;
    }
    macro_set_t set = {0};
    app_error_code_t result = parse_exact_set(object, &set);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (find_set_index(state, &set.id) != SIZE_MAX) {
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

static app_error_code_t validate_macro_object(const json_span_t *object, void *context) {
    macro_validation_context_t *macro_context = context;
    if (macro_context == NULL || macro_context->state == NULL ||
        macro_context->state->macro_count >= macro_context->state->macro_capacity) {
        return APP_ERROR_INTERNAL;
    }
    package_validation_state_t *state = macro_context->state;
    macro_t macro = {0};
    app_error_code_t result = external_object_result(
        storage_repository_parse_macro_json(object->data, object->length, &macro));
    if (result == APP_ERROR_NONE && macro.scope != macro_context->expected_scope) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE && find_macro_index(state, &macro.id) != SIZE_MAX) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    size_t set_index = SIZE_MAX;
    if (result == APP_ERROR_NONE && macro.scope == MACRO_SCOPE_SET) {
        set_index = find_set_index(state, &macro.set_id);
        if (set_index == SIZE_MAX ||
            state->set_macro_counts[set_index] >= APP_MACROS_PER_SET_MAX) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = compile_package_macro(&macro);
    }
    if (result == APP_ERROR_NONE) {
        package_macro_metadata_t *metadata = &state->macros[state->macro_count];
        metadata->id = macro.id;
        metadata->scope = macro.scope;
        metadata->set_id = macro.set_id;
        ++state->macro_count;
        if (set_index != SIZE_MAX) {
            ++state->set_macro_counts[set_index];
        }
    }
    macro_model_free_macro(&macro);
    return result;
}

static bool procedure_macro_references_valid(const package_validation_state_t *state,
                                             const procedure_t *procedure) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        const procedure_step_t *step = &procedure->steps[index];
        if (!step->has_macro_id) {
            continue;
        }
        const size_t macro_index = find_macro_index(state, &step->macro_id);
        if (macro_index == SIZE_MAX) {
            return false;
        }
        const package_macro_metadata_t *macro = &state->macros[macro_index];
        if (macro->scope == MACRO_SCOPE_SET && !app_uuid_equal(&macro->set_id, &procedure->set_id)) {
            return false;
        }
    }
    return true;
}

static app_error_code_t validate_procedure_object(const json_span_t *object, void *context) {
    package_validation_state_t *state = context;
    if (state == NULL || state->procedure_count >= state->procedure_capacity) {
        return APP_ERROR_INTERNAL;
    }
    procedure_t procedure = {0};
    app_error_code_t result = external_object_result(
        storage_repository_parse_procedure_json(object->data, object->length, &procedure));
    size_t set_index = SIZE_MAX;
    if (result == APP_ERROR_NONE) {
        set_index = find_set_index(state, &procedure.set_id);
        if (set_index == SIZE_MAX ||
            state->set_procedure_counts[set_index] >= APP_PROCEDURES_PER_SET_MAX ||
            find_procedure_index(state, &procedure.id) != SIZE_MAX ||
            !procedure_macro_references_valid(state, &procedure)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    if (result == APP_ERROR_NONE) {
        package_procedure_metadata_t *metadata = &state->procedures[state->procedure_count];
        metadata->id = procedure.id;
        metadata->set_id = procedure.set_id;
        metadata->revision = procedure.revision;
        metadata->source = *object;
        ++state->procedure_count;
        ++state->set_procedure_counts[set_index];
    }
    macro_model_free_procedure(&procedure);
    return result;
}

static bool procedure_has_step(const procedure_t *procedure, const app_uuid_t *step_id) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        if (app_uuid_equal(&procedure->steps[index].id, step_id)) {
            return true;
        }
    }
    return false;
}

static bool progress_step_references_valid(const procedure_progress_t *progress,
                                           const procedure_t *procedure) {
    if (!procedure_has_step(procedure, &progress->current_step_id)) {
        return false;
    }
    for (size_t index = 0U; index < progress->completed_step_count; ++index) {
        if (!procedure_has_step(procedure, &progress->completed_step_ids[index])) {
            return false;
        }
    }
    for (size_t index = 0U; index < progress->skipped_step_count; ++index) {
        if (!procedure_has_step(procedure, &progress->skipped_step_ids[index])) {
            return false;
        }
    }
    return true;
}

static app_error_code_t validate_progress_object(const json_span_t *object, void *context) {
    package_validation_state_t *state = context;
    if (state == NULL || state->progress_count >= state->progress_capacity) {
        return APP_ERROR_INTERNAL;
    }
    procedure_progress_t progress = {0};
    app_error_code_t result = external_object_result(
        storage_repository_parse_progress_json(object->data, object->length, &progress));
    const size_t procedure_index =
        result == APP_ERROR_NONE ? find_procedure_index(state, &progress.procedure_id) : SIZE_MAX;
    if (result == APP_ERROR_NONE && procedure_index == SIZE_MAX) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < state->progress_count; ++index) {
        if (app_uuid_equal(&state->progress[index].procedure_id, &progress.procedure_id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
    }
    procedure_t procedure = {0};
    if (result == APP_ERROR_NONE) {
        const package_procedure_metadata_t *metadata = &state->procedures[procedure_index];
        if (!app_uuid_equal(&metadata->set_id, &progress.set_id) ||
            metadata->revision != progress.procedure_revision) {
            result = APP_ERROR_INVALID_ARGUMENT;
        } else {
            result = external_object_result(storage_repository_parse_procedure_json(
                metadata->source.data, metadata->source.length, &procedure));
        }
    }
    if (result == APP_ERROR_NONE && !progress_step_references_valid(&progress, &procedure)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    macro_model_free_procedure(&procedure);
    if (result == APP_ERROR_NONE) {
        state->progress[state->progress_count].procedure_id = progress.procedure_id;
        ++state->progress_count;
    }
    return result;
}

static app_error_code_t count_package_arrays(const package_document_t *document,
                                             storage_package_summary_t *out_summary) {
    app_error_code_t result = visit_object_array(
        &document->arrays[PACKAGE_ARRAY_SETS], APP_MACRO_SETS_MAX, NULL, NULL,
        &out_summary->set_count);
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_MACROS],
                                    STORAGE_PACKAGE_LOCAL_MACROS_MAX, NULL, NULL,
                                    &out_summary->local_macro_count);
    }
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_GLOBAL_MACROS],
                                    STORAGE_PACKAGE_GLOBAL_MACROS_MAX, NULL, NULL,
                                    &out_summary->global_macro_count);
    }
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_PROCEDURES],
                                    STORAGE_PACKAGE_PROCEDURES_MAX, NULL, NULL,
                                    &out_summary->procedure_count);
    }
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_PROGRESS],
                                    STORAGE_PACKAGE_PROCEDURES_MAX, NULL, NULL,
                                    &out_summary->progress_count);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if ((document->kind == STORAGE_PACKAGE_KIND_SET && out_summary->set_count != 1U) ||
        out_summary->local_macro_count > out_summary->set_count * APP_MACROS_PER_SET_MAX ||
        out_summary->procedure_count > out_summary->set_count * APP_PROCEDURES_PER_SET_MAX ||
        out_summary->progress_count > out_summary->procedure_count) {
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
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_SETS], state.set_capacity,
                                    validate_set_object, &state, &visited);
    }
    macro_validation_context_t local_macros = {
        .state = &state,
        .expected_scope = MACRO_SCOPE_SET,
    };
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_MACROS],
                                    summary->local_macro_count, validate_macro_object,
                                    &local_macros, &visited);
    }
    macro_validation_context_t global_macros = {
        .state = &state,
        .expected_scope = MACRO_SCOPE_GLOBAL,
    };
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_GLOBAL_MACROS],
                                    summary->global_macro_count, validate_macro_object,
                                    &global_macros, &visited);
    }
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_PROCEDURES],
                                    state.procedure_capacity, validate_procedure_object, &state,
                                    &visited);
    }
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_PROGRESS],
                                    state.progress_capacity, validate_progress_object, &state,
                                    &visited);
    }
    if (result == APP_ERROR_NONE &&
        (state.set_count != summary->set_count ||
         state.macro_count != summary->local_macro_count + summary->global_macro_count ||
         state.procedure_count != summary->procedure_count ||
         state.progress_count != summary->progress_count)) {
        result = APP_ERROR_INTERNAL;
    }
    free_validation_state(&state);
    return result;
}

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
    if (result == APP_ERROR_NONE && document.kind != expected_kind) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    storage_package_summary_t summary = {
        .kind = document.kind,
        .package_bytes = length,
    };
    if (result == APP_ERROR_NONE) {
        result = count_package_arrays(&document, &summary);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_package_objects(&document, &summary);
    }
    if (result == APP_ERROR_NONE) {
        *out_summary = summary;
    }
    return result;
}
