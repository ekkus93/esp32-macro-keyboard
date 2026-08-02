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
#define STORAGE_PACKAGE_FIELD_COUNT 5U
/* "skipped" is the only optional member. A complete backup omits it entirely,
 * so complete packages are byte-identical to those written before partial
 * backups existed and remain readable by anything that predates this field. */
#define STORAGE_PACKAGE_OPTIONAL_FIELDS (UINT32_C(1) << PACKAGE_FIELD_SKIPPED)
#define STORAGE_PACKAGE_ARRAY_COUNT 2U
#define STORAGE_PACKAGE_LOCAL_MACROS_MAX                                                           \
    ((size_t)APP_MACRO_SETS_MAX * (size_t)APP_MACROS_PER_SET_MAX)
#define STORAGE_PACKAGE_JSON_FRAME_COUNT (STORAGE_PACKAGE_MAX_JSON_DEPTH + 1U)
#define JSON_UNICODE_ESCAPE_DIGITS 4U
#define JSON_DECIMAL_RADIX 10U

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
    json_span_t arrays[STORAGE_PACKAGE_ARRAY_COUNT];
} package_document_t;

typedef struct {
    app_uuid_t id;
} package_set_metadata_t;

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
    package_set_metadata_t *sets;
    package_macro_metadata_t *macros;
    size_t *set_macro_counts;
    size_t set_capacity;
    size_t macro_capacity;
    size_t set_count;
    size_t macro_count;
} package_validation_state_t;

typedef app_error_code_t (*package_object_callback_t)(const json_span_t *object, void *context);

static const char *const PACKAGE_FIELDS[STORAGE_PACKAGE_FIELD_COUNT] = {
    "schema_version", "package_type", "sets", "macros", "skipped",
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

static bool is_decimal_digit(char value) {
    return value >= '0' && value <= '9';
}

static app_error_code_t scan_json_unicode_escape(json_cursor_t *cursor) {
    if (cursor->length - cursor->offset < JSON_UNICODE_ESCAPE_DIGITS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    bool all_zero = true;
    for (size_t index = 0U; index < JSON_UNICODE_ESCAPE_DIGITS; ++index) {
        const char digit = cursor->data[cursor->offset + index];
        if (!is_hex_digit(digit)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        all_zero = all_zero && digit == '0';
    }
    if (all_zero) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cursor->offset += JSON_UNICODE_ESCAPE_DIGITS;
    return APP_ERROR_NONE;
}

static app_error_code_t scan_json_escape(json_cursor_t *cursor) {
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char escape = cursor->data[cursor->offset++];
    switch (escape) {
    case '"':
    case '\\':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
        return APP_ERROR_NONE;
    case 'u':
        return scan_json_unicode_escape(cursor);
    default:
        return APP_ERROR_INVALID_ARGUMENT;
    }
}

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
        if (value == (unsigned char)'\\') {
            const app_error_code_t result = scan_json_escape(cursor);
            if (result != APP_ERROR_NONE) {
                return result;
            }
        }
    }
    return APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t scan_json_digits(json_cursor_t *cursor) {
    const size_t start = cursor->offset;
    while (cursor->offset < cursor->length && is_decimal_digit(cursor->data[cursor->offset])) {
        ++cursor->offset;
    }
    return cursor->offset > start ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t scan_json_integer_part(json_cursor_t *cursor) {
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (cursor->data[cursor->offset] == '0') {
        ++cursor->offset;
        return cursor->offset < cursor->length && is_decimal_digit(cursor->data[cursor->offset])
                   ? APP_ERROR_INVALID_ARGUMENT
                   : APP_ERROR_NONE;
    }
    if (cursor->data[cursor->offset] < '1' || cursor->data[cursor->offset] > '9') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return scan_json_digits(cursor);
}

static app_error_code_t scan_json_fraction(json_cursor_t *cursor) {
    if (cursor->offset >= cursor->length || cursor->data[cursor->offset] != '.') {
        return APP_ERROR_NONE;
    }
    ++cursor->offset;
    return scan_json_digits(cursor);
}

static app_error_code_t scan_json_exponent(json_cursor_t *cursor) {
    if (cursor->offset >= cursor->length ||
        (cursor->data[cursor->offset] != 'e' && cursor->data[cursor->offset] != 'E')) {
        return APP_ERROR_NONE;
    }
    ++cursor->offset;
    if (cursor->offset < cursor->length &&
        (cursor->data[cursor->offset] == '+' || cursor->data[cursor->offset] == '-')) {
        ++cursor->offset;
    }
    return scan_json_digits(cursor);
}

static app_error_code_t scan_json_number(json_cursor_t *cursor) {
    if (cursor->offset < cursor->length && cursor->data[cursor->offset] == '-') {
        ++cursor->offset;
    }
    app_error_code_t result = scan_json_integer_part(cursor);
    if (result == APP_ERROR_NONE) {
        result = scan_json_fraction(cursor);
    }
    if (result == APP_ERROR_NONE) {
        result = scan_json_exponent(cursor);
    }
    return result;
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

typedef enum {
    JSON_FRAME_ARRAY_VALUE_OR_END = 0,
    JSON_FRAME_ARRAY_VALUE_REQUIRED,
    JSON_FRAME_ARRAY_SEPARATOR_OR_END,
    JSON_FRAME_OBJECT_KEY_OR_END,
    JSON_FRAME_OBJECT_KEY_REQUIRED,
    JSON_FRAME_OBJECT_COLON,
    JSON_FRAME_OBJECT_VALUE_REQUIRED,
    JSON_FRAME_OBJECT_SEPARATOR_OR_END,
} json_frame_state_t;

typedef struct {
    json_frame_state_t state;
} json_frame_t;

static app_error_code_t complete_json_value(json_frame_t *frames, size_t depth,
                                            bool *out_complete) {
    if (depth == 0U) {
        *out_complete = true;
        return APP_ERROR_NONE;
    }
    json_frame_state_t *state = &frames[depth - 1U].state;
    switch (*state) {
    case JSON_FRAME_ARRAY_VALUE_OR_END:
    case JSON_FRAME_ARRAY_VALUE_REQUIRED:
        *state = JSON_FRAME_ARRAY_SEPARATOR_OR_END;
        return APP_ERROR_NONE;
    case JSON_FRAME_OBJECT_VALUE_REQUIRED:
        *state = JSON_FRAME_OBJECT_SEPARATOR_OR_END;
        return APP_ERROR_NONE;
    default:
        return APP_ERROR_INVALID_ARGUMENT;
    }
}

static app_error_code_t scan_json_scalar(json_cursor_t *cursor) {
    switch (cursor->data[cursor->offset]) {
    case '"':
        return scan_json_string(cursor);
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

static app_error_code_t begin_json_value(json_cursor_t *cursor, json_frame_t *frames,
                                         size_t *in_out_depth, bool *out_complete) {
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char token = cursor->data[cursor->offset];
    if (token == '[' || token == '{') {
        if (*in_out_depth >= STORAGE_PACKAGE_JSON_FRAME_COUNT) {
            return APP_ERROR_MACRO_LIMIT;
        }
        ++cursor->offset;
        frames[*in_out_depth].state =
            token == '[' ? JSON_FRAME_ARRAY_VALUE_OR_END : JSON_FRAME_OBJECT_KEY_OR_END;
        ++*in_out_depth;
        return APP_ERROR_NONE;
    }
    const app_error_code_t result = scan_json_scalar(cursor);
    return result == APP_ERROR_NONE ? complete_json_value(frames, *in_out_depth, out_complete)
                                    : result;
}

static app_error_code_t close_json_container(json_cursor_t *cursor, json_frame_t *frames,
                                             size_t *in_out_depth, bool *out_complete) {
    ++cursor->offset;
    --*in_out_depth;
    return complete_json_value(frames, *in_out_depth, out_complete);
}

static app_error_code_t process_array_value(json_cursor_t *cursor, json_frame_t *frames,
                                            size_t *in_out_depth, bool allow_empty,
                                            bool *out_complete) {
    skip_whitespace(cursor);
    if (allow_empty && cursor->offset < cursor->length && cursor->data[cursor->offset] == ']') {
        return close_json_container(cursor, frames, in_out_depth, out_complete);
    }
    return begin_json_value(cursor, frames, in_out_depth, out_complete);
}

static app_error_code_t process_array_separator(json_cursor_t *cursor, json_frame_t *frames,
                                                size_t *in_out_depth, bool *out_complete) {
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char separator = cursor->data[cursor->offset];
    if (separator == ']') {
        return close_json_container(cursor, frames, in_out_depth, out_complete);
    }
    if (separator != ',') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor->offset;
    frames[*in_out_depth - 1U].state = JSON_FRAME_ARRAY_VALUE_REQUIRED;
    return APP_ERROR_NONE;
}

static app_error_code_t process_object_key(json_cursor_t *cursor, json_frame_t *frames,
                                           size_t *in_out_depth, bool allow_empty,
                                           bool *out_complete) {
    skip_whitespace(cursor);
    if (allow_empty && cursor->offset < cursor->length && cursor->data[cursor->offset] == '}') {
        return close_json_container(cursor, frames, in_out_depth, out_complete);
    }
    const app_error_code_t result = scan_json_string(cursor);
    if (result == APP_ERROR_NONE) {
        frames[*in_out_depth - 1U].state = JSON_FRAME_OBJECT_COLON;
    }
    return result;
}

static app_error_code_t process_object_colon(json_cursor_t *cursor, json_frame_t *frames,
                                             size_t depth) {
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length || cursor->data[cursor->offset] != ':') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor->offset;
    frames[depth - 1U].state = JSON_FRAME_OBJECT_VALUE_REQUIRED;
    return APP_ERROR_NONE;
}

static app_error_code_t process_object_separator(json_cursor_t *cursor, json_frame_t *frames,
                                                 size_t *in_out_depth, bool *out_complete) {
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char separator = cursor->data[cursor->offset];
    if (separator == '}') {
        return close_json_container(cursor, frames, in_out_depth, out_complete);
    }
    if (separator != ',') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor->offset;
    frames[*in_out_depth - 1U].state = JSON_FRAME_OBJECT_KEY_REQUIRED;
    return APP_ERROR_NONE;
}

static app_error_code_t process_json_frame(json_cursor_t *cursor, json_frame_t *frames,
                                           size_t *in_out_depth, bool *out_complete) {
    switch (frames[*in_out_depth - 1U].state) {
    case JSON_FRAME_ARRAY_VALUE_OR_END:
        return process_array_value(cursor, frames, in_out_depth, true, out_complete);
    case JSON_FRAME_ARRAY_VALUE_REQUIRED:
        return process_array_value(cursor, frames, in_out_depth, false, out_complete);
    case JSON_FRAME_ARRAY_SEPARATOR_OR_END:
        return process_array_separator(cursor, frames, in_out_depth, out_complete);
    case JSON_FRAME_OBJECT_KEY_OR_END:
        return process_object_key(cursor, frames, in_out_depth, true, out_complete);
    case JSON_FRAME_OBJECT_KEY_REQUIRED:
        return process_object_key(cursor, frames, in_out_depth, false, out_complete);
    case JSON_FRAME_OBJECT_COLON:
        return process_object_colon(cursor, frames, *in_out_depth);
    case JSON_FRAME_OBJECT_VALUE_REQUIRED:
        return begin_json_value(cursor, frames, in_out_depth, out_complete);
    case JSON_FRAME_OBJECT_SEPARATOR_OR_END:
        return process_object_separator(cursor, frames, in_out_depth, out_complete);
    default:
        return APP_ERROR_INVALID_ARGUMENT;
    }
}

static app_error_code_t scan_json_value(json_cursor_t *cursor, size_t depth) {
    if (depth != 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    json_frame_t frames[STORAGE_PACKAGE_JSON_FRAME_COUNT] = {0};
    size_t frame_depth = 0U;
    bool complete = false;
    app_error_code_t result = begin_json_value(cursor, frames, &frame_depth, &complete);
    while (result == APP_ERROR_NONE && !complete) {
        result = process_json_frame(cursor, frames, &frame_depth, &complete);
    }
    return result;
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

static app_error_code_t read_plain_string(json_cursor_t *cursor, char *output, size_t output_size) {
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
        !is_decimal_digit(cursor->data[cursor->offset])) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    uint32_t value = 0U;
    const size_t start = cursor->offset;
    while (cursor->offset < cursor->length && is_decimal_digit(cursor->data[cursor->offset])) {
        const uint32_t digit = (uint32_t)(cursor->data[cursor->offset] - '0');
        if (value > (UINT32_MAX - digit) / JSON_DECIMAL_RADIX) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        value = value * JSON_DECIMAL_RADIX + digit;
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

static app_error_code_t read_package_kind(json_cursor_t *cursor, storage_package_kind_t *out_kind) {
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
    case PACKAGE_FIELD_SCHEMA_VERSION:
    case PACKAGE_FIELD_TYPE:
    default:
        return PACKAGE_ARRAY_COUNT;
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
    if (field_index == PACKAGE_FIELD_SKIPPED) {
        /* A record of what a partial backup dropped. It is descriptive only:
         * restore never reads objects from it, so it is checked for shape and
         * otherwise ignored. */
        json_span_t skipped = {0};
        const app_error_code_t captured = capture_json_value(cursor, &skipped);
        if (captured != APP_ERROR_NONE) {
            return captured;
        }
        return skipped.length != 0U && skipped.data[0] == '{' ? APP_ERROR_NONE
                                                              : APP_ERROR_INVALID_ARGUMENT;
    }
    const package_array_t array_index = package_array_for_field(field_index);
    if ((size_t)array_index >= STORAGE_PACKAGE_ARRAY_COUNT) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = capture_json_value(cursor, &out_document->arrays[array_index]);
    if (result == APP_ERROR_NONE && (out_document->arrays[array_index].length == 0U ||
                                     out_document->arrays[array_index].data[0] != '[')) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    return result;
}

static app_error_code_t parse_package_member(json_cursor_t *cursor,
                                             package_document_t *out_document,
                                             uint32_t *in_out_seen) {
    char name[STORAGE_PACKAGE_FIELD_NAME_BYTES];
    app_error_code_t result = read_plain_string(cursor, name, sizeof(name));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t field_index = package_field_index(name);
    if (field_index == SIZE_MAX || (*in_out_seen & (UINT32_C(1) << field_index)) != 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length || cursor->data[cursor->offset] != ':') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++cursor->offset;
    result = parse_package_field(cursor, field_index, out_document);
    if (result == APP_ERROR_NONE) {
        *in_out_seen |= UINT32_C(1) << field_index;
    }
    return result;
}

static app_error_code_t
parse_package_members(json_cursor_t *cursor, package_document_t *out_document, uint32_t *out_seen) {
    *out_seen = 0U;
    skip_whitespace(cursor);
    if (cursor->offset < cursor->length && cursor->data[cursor->offset] == '}') {
        ++cursor->offset;
        return APP_ERROR_NONE;
    }
    for (;;) {
        app_error_code_t result = parse_package_member(cursor, out_document, out_seen);
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
        if (cursor->offset >= cursor->length || cursor->data[cursor->offset] == '}') {
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
}

static app_error_code_t finish_package_document(json_cursor_t *cursor, uint32_t seen,
                                                const package_document_t *document) {
    skip_whitespace(cursor);
    const uint32_t required = ((UINT32_C(1) << STORAGE_PACKAGE_FIELD_COUNT) - UINT32_C(1)) &
                              ~STORAGE_PACKAGE_OPTIONAL_FIELDS;
    return cursor->offset == cursor->length && (seen & required) == required &&
                   document->schema_version == APP_SCHEMA_VERSION
               ? APP_ERROR_NONE
               : APP_ERROR_INVALID_ARGUMENT;
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
    const app_error_code_t result = parse_package_members(&cursor, out_document, &seen);
    return result == APP_ERROR_NONE ? finish_package_document(&cursor, seen, out_document) : result;
}

static app_error_code_t finish_object_array(json_cursor_t *cursor) {
    skip_whitespace(cursor);
    return cursor->offset == cursor->length ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t visit_object_array_item(json_cursor_t *cursor, size_t maximum_count,
                                                package_object_callback_t callback, void *context,
                                                size_t *in_out_count) {
    json_span_t object = {0};
    app_error_code_t result = capture_json_value(cursor, &object);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (object.length == 0U || object.data[0] != '{') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (*in_out_count >= maximum_count) {
        return APP_ERROR_MACRO_LIMIT;
    }
    if (callback != NULL) {
        result = callback(&object, context);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    ++*in_out_count;
    return APP_ERROR_NONE;
}

static app_error_code_t consume_object_array_separator(json_cursor_t *cursor, bool *out_done) {
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char separator = cursor->data[cursor->offset++];
    if (separator == ']') {
        *out_done = true;
        return APP_ERROR_NONE;
    }
    if (separator != ',') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    skip_whitespace(cursor);
    if (cursor->offset >= cursor->length || cursor->data[cursor->offset] == ']') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_done = false;
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
        return finish_object_array(&cursor);
    }
    bool done = false;
    while (!done) {
        app_error_code_t result =
            visit_object_array_item(&cursor, maximum_count, callback, context, out_count);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        result = consume_object_array_separator(&cursor, &done);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return finish_object_array(&cursor);
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

static app_error_code_t external_object_result(app_error_code_t result) {
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t parse_exact_set(const json_span_t *object, macro_set_t *out_set) {
    static const char *const fields[] = {
        "schema_version",
        "id",
        "revision",
        "name",
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
    package_validation_state_t *state = context;
    if (state == NULL || state->macro_count >= state->macro_capacity) {
        return APP_ERROR_INTERNAL;
    }
    macro_t macro = {0};
    app_error_code_t result = external_object_result(
        storage_repository_parse_macro_json(object->data, object->length, &macro));
    if (result == APP_ERROR_NONE && find_macro_index(state, &macro.id) != SIZE_MAX) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    size_t set_index = SIZE_MAX;
    if (result == APP_ERROR_NONE) {
        set_index = find_set_index(state, &macro.set_id);
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

static app_error_code_t count_package_arrays(const package_document_t *document,
                                             storage_package_summary_t *out_summary) {
    app_error_code_t result =
        visit_object_array(&document->arrays[PACKAGE_ARRAY_SETS], APP_MACRO_SETS_MAX, NULL, NULL,
                           &out_summary->set_count);
    if (result == APP_ERROR_NONE) {
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_MACROS],
                                    STORAGE_PACKAGE_LOCAL_MACROS_MAX, NULL, NULL,
                                    &out_summary->local_macro_count);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
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
        result = visit_object_array(&document->arrays[PACKAGE_ARRAY_SETS], state.set_capacity,
                                    validate_set_object, &state, &visited);
    }
    if (result == APP_ERROR_NONE) {
        result =
            visit_object_array(&document->arrays[PACKAGE_ARRAY_MACROS], summary->local_macro_count,
                               validate_macro_object, &state, &visited);
    }
    if (result == APP_ERROR_NONE && (state.set_count != summary->set_count ||
                                     state.macro_count != summary->local_macro_count)) {
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
