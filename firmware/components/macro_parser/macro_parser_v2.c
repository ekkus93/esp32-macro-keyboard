#include "macro_parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "macro_keymap_us.h"
#include "macro_keymap_us_v2.h"

#define V2_ASCII_SPACE UINT8_C(0x20)
#define V2_ASCII_TILDE UINT8_C(0x7e)
#define V2_ASCII_DELETE UINT8_C(0x7f)
#define V2_DECIMAL_BASE UINT32_C(10)
#define V2_HID_USAGE_ENTER UINT8_C(0x28)
#define V2_HID_USAGE_TAB UINT8_C(0x2b)
#define V2_DIRECTIVE_BUFFER_BYTES 64U

typedef struct {
    size_t line;
    size_t column;
} v2_source_position_t;

static void v2_clear_plan(macro_plan_t *plan) {
    if (plan != NULL) {
        plan->actions = NULL;
        plan->action_count = 0U;
        plan->estimated_duration_ms = 0U;
    }
}

static v2_source_position_t v2_locate(const char *source, size_t offset) {
    v2_source_position_t position = {.line = 1U, .column = 1U};
    if (source == NULL) {
        return position;
    }
    for (size_t index = 0U; index < offset; ++index) {
        if (source[index] == '\n') {
            ++position.line;
            position.column = 1U;
        } else {
            ++position.column;
        }
    }
    return position;
}

static app_error_code_t v2_fail(const char *source, size_t offset, const char *message,
                                app_error_code_t code, macro_parse_error_t *error) {
    if (error != NULL) {
        error->code = code;
        error->byte_offset = offset;
        const v2_source_position_t position = v2_locate(source, offset);
        error->line = position.line;
        error->column = position.column;
        const int written = snprintf(error->message, sizeof(error->message), "%s", message);
        if (written < 0) {
            error->message[0] = '\0';
        }
    }
    return code;
}

static bool v2_safe_add_u32(uint32_t left, uint32_t right, uint32_t *out_value) {
    if (out_value == NULL || UINT32_MAX - left < right) {
        return false;
    }
    *out_value = left + right;
    return true;
}

static app_error_code_t v2_append_action(macro_plan_t *plan, macro_action_t action,
                                         const macro_compile_options_t *options, const char *source,
                                         size_t offset, macro_parse_error_t *error) {
    if (plan->action_count >= (size_t)APP_V2_COMPILED_ACTIONS_MAX) {
        return v2_fail(source, offset, "action limit exceeded", APP_ERROR_MACRO_LIMIT, error);
    }

    uint32_t action_duration = 0U;
    if (action.type == MACRO_ACTION_DELAY) {
        action_duration = action.delay_ms;
    } else if (!v2_safe_add_u32(options->key_press_ms, options->inter_key_ms, &action_duration)) {
        return v2_fail(source, offset, "action duration overflow", APP_ERROR_MACRO_LIMIT, error);
    }

    uint32_t total_duration = 0U;
    if (!v2_safe_add_u32(plan->estimated_duration_ms, action_duration, &total_duration) ||
        total_duration > APP_V2_ESTIMATED_DURATION_MAX_MS) {
        return v2_fail(source, offset, "estimated duration limit exceeded", APP_ERROR_MACRO_LIMIT,
                       error);
    }

    plan->actions[plan->action_count] = action;
    ++plan->action_count;
    plan->estimated_duration_ms = total_duration;
    return APP_ERROR_NONE;
}

static bool v2_directive_has_invalid_character(const char *text, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        const uint8_t value = (uint8_t)text[index];
        if (value <= V2_ASCII_SPACE || value >= V2_ASCII_DELETE || text[index] == '{' ||
            text[index] == '}') {
            return true;
        }
    }
    return false;
}

static app_error_code_t v2_parse_delay(const char *directive, size_t length,
                                       macro_action_t *out_action) {
    static const char prefix[] = "DELAY:";
    const size_t prefix_length = sizeof(prefix) - 1U;
    if (length <= prefix_length || memcmp(directive, prefix, prefix_length) != 0) {
        return APP_ERROR_MACRO_SYNTAX;
    }

    uint32_t value = 0U;
    for (size_t index = prefix_length; index < length; ++index) {
        const char character = directive[index];
        if (character < '0' || character > '9') {
            return APP_ERROR_MACRO_SYNTAX;
        }
        const uint32_t digit = (uint32_t)(character - '0');
        if (value > (UINT32_MAX - digit) / V2_DECIMAL_BASE) {
            return APP_ERROR_MACRO_LIMIT;
        }
        value = value * V2_DECIMAL_BASE + digit;
    }

    if (value == 0U || value > APP_V2_DELAY_DIRECTIVE_MAX_MS) {
        return APP_ERROR_MACRO_LIMIT;
    }
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_DELAY,
        .modifiers = 0U,
        .usage = 0U,
        .delay_ms = value,
    };
    return APP_ERROR_NONE;
}

static app_error_code_t v2_parse_chord(char *directive, macro_action_t *out_action) {
    uint8_t modifiers = 0U;
    macro_hid_key_t key = {0U, 0U};
    bool have_key = false;
    char *cursor = directive;

    while (true) {
        char *separator = strchr(cursor, '+');
        if (separator != NULL) {
            *separator = '\0';
        }
        if (*cursor == '\0') {
            return APP_ERROR_MACRO_SYNTAX;
        }

        uint8_t modifier = 0U;
        if (macro_keymap_us_modifier(cursor, &modifier)) {
            if (have_key || (modifiers & modifier) != 0U) {
                return APP_ERROR_MACRO_SYNTAX;
            }
            modifiers = (uint8_t)(modifiers | modifier);
        } else {
            if (have_key || !macro_keymap_us_v2_chord_key(cursor, &key)) {
                return APP_ERROR_MACRO_SYNTAX;
            }
            have_key = true;
        }

        if (separator == NULL) {
            break;
        }
        cursor = separator + 1;
    }

    if (!have_key || modifiers == 0U) {
        return APP_ERROR_MACRO_SYNTAX;
    }
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_CHORD,
        .modifiers = modifiers,
        .usage = key.usage,
        .delay_ms = 0U,
    };
    return APP_ERROR_NONE;
}

static app_error_code_t v2_parse_directive(const char *source, size_t offset, const char *directive,
                                           size_t length, macro_action_t *out_action,
                                           macro_parse_error_t *error) {
    if (length == 0U || length >= V2_DIRECTIVE_BUFFER_BYTES ||
        v2_directive_has_invalid_character(directive, length)) {
        return v2_fail(source, offset, "invalid directive", APP_ERROR_MACRO_SYNTAX, error);
    }

    char buffer[V2_DIRECTIVE_BUFFER_BYTES];
    memcpy(buffer, directive, length);
    buffer[length] = '\0';

    if (strncmp(buffer, "DELAY:", sizeof("DELAY:") - 1U) == 0) {
        const app_error_code_t result = v2_parse_delay(buffer, length, out_action);
        if (result == APP_ERROR_MACRO_LIMIT) {
            return v2_fail(source, offset, "delay is outside the allowed range", result, error);
        }
        if (result != APP_ERROR_NONE) {
            return v2_fail(source, offset, "invalid delay directive", result, error);
        }
        return APP_ERROR_NONE;
    }

    if (strchr(buffer, '+') != NULL) {
        if (v2_parse_chord(buffer, out_action) != APP_ERROR_NONE) {
            return v2_fail(source, offset, "invalid chord", APP_ERROR_MACRO_SYNTAX, error);
        }
        return APP_ERROR_NONE;
    }

    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_v2_named_directive(buffer, &key)) {
        return v2_fail(source, offset, "unknown key directive", APP_ERROR_MACRO_SYNTAX, error);
    }
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_KEY,
        .modifiers = key.modifiers,
        .usage = key.usage,
        .delay_ms = 0U,
    };
    return APP_ERROR_NONE;
}

static macro_action_t v2_key_action(uint8_t modifiers, uint8_t usage) {
    return (macro_action_t){
        .type = MACRO_ACTION_KEY,
        .modifiers = modifiers,
        .usage = usage,
        .delay_ms = 0U,
    };
}

static app_error_code_t v2_parse_open_brace(const char *source, size_t source_length, size_t offset,
                                            macro_action_t *out_action, size_t *out_consumed,
                                            macro_parse_error_t *out_error) {
    if (offset + 1U < source_length && source[offset + 1U] == '{') {
        macro_hid_key_t key = {0U, 0U};
        if (!macro_keymap_us_printable('{', &key)) {
            return v2_fail(source, offset, "source contains unsupported character",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        *out_action = v2_key_action(key.modifiers, key.usage);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }

    const char *closing = memchr(source + offset + 1U, '}', source_length - offset - 1U);
    if (closing == NULL) {
        return v2_fail(source, offset, "unmatched opening brace", APP_ERROR_MACRO_SYNTAX,
                       out_error);
    }
    const size_t closing_offset = (size_t)(closing - source);
    const app_error_code_t result = v2_parse_directive(
        source, offset, source + offset + 1U, closing_offset - offset - 1U, out_action, out_error);
    *out_consumed = closing_offset + 1U - offset;
    return result;
}

static app_error_code_t v2_parse_close_brace(const char *source, size_t source_length,
                                             size_t offset, macro_action_t *out_action,
                                             size_t *out_consumed, macro_parse_error_t *out_error) {
    if (offset + 1U < source_length && source[offset + 1U] == '}') {
        macro_hid_key_t key = {0U, 0U};
        if (!macro_keymap_us_printable('}', &key)) {
            return v2_fail(source, offset, "source contains unsupported character",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        *out_action = v2_key_action(key.modifiers, key.usage);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }
    return v2_fail(source, offset, "unmatched closing brace", APP_ERROR_MACRO_SYNTAX, out_error);
}

static app_error_code_t v2_parse_printable(const char *source, size_t offset,
                                           macro_action_t *out_action, size_t *out_consumed,
                                           macro_parse_error_t *out_error) {
    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_printable(source[offset], &key)) {
        return v2_fail(source, offset, "source contains unsupported character",
                       APP_ERROR_MACRO_SYNTAX, out_error);
    }
    *out_action = v2_key_action(key.modifiers, key.usage);
    *out_consumed = 1U;
    return APP_ERROR_NONE;
}

static app_error_code_t v2_parse_next_token(const char *source, size_t source_length, size_t offset,
                                            macro_action_t *out_action, size_t *out_consumed,
                                            macro_parse_error_t *out_error) {
    const uint8_t byte = (uint8_t)source[offset];
    if (byte >= UINT8_C(0x80) || byte == 0U) {
        return v2_fail(source, offset, "source contains unsupported character",
                       APP_ERROR_MACRO_SYNTAX, out_error);
    }

    const char character = source[offset];
    if (character == '\r') {
        if (offset + 1U >= source_length || source[offset + 1U] != '\n') {
            return v2_fail(source, offset, "carriage return must be followed by line feed",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        *out_action = v2_key_action(0U, V2_HID_USAGE_ENTER);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }
    if (character == '\n') {
        *out_action = v2_key_action(0U, V2_HID_USAGE_ENTER);
        *out_consumed = 1U;
        return APP_ERROR_NONE;
    }
    if (character == '\t') {
        *out_action = v2_key_action(0U, V2_HID_USAGE_TAB);
        *out_consumed = 1U;
        return APP_ERROR_NONE;
    }
    if (character == '{') {
        return v2_parse_open_brace(source, source_length, offset, out_action, out_consumed,
                                   out_error);
    }
    if (character == '}') {
        return v2_parse_close_brace(source, source_length, offset, out_action, out_consumed,
                                    out_error);
    }
    if (byte < V2_ASCII_SPACE || byte > V2_ASCII_TILDE) {
        return v2_fail(source, offset, "source contains unsupported character",
                       APP_ERROR_MACRO_SYNTAX, out_error);
    }
    return v2_parse_printable(source, offset, out_action, out_consumed, out_error);
}

app_error_code_t macro_compile_v2(const char *source, size_t source_length,
                                  const macro_compile_options_t *options, macro_plan_t *out_plan,
                                  macro_parse_error_t *out_error) {
    const macro_compile_options_t defaults = {
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    const macro_compile_options_t *effective = options == NULL ? &defaults : options;

    if (out_plan == NULL || (source == NULL && source_length != 0U)) {
        if (out_plan != NULL) {
            v2_clear_plan(out_plan);
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }
    v2_clear_plan(out_plan);
    if (out_error != NULL) {
        memset(out_error, 0, sizeof(*out_error));
    }

    if (effective->key_press_ms > APP_V2_KEY_PRESS_MAX_MS ||
        effective->inter_key_ms > APP_V2_INTER_KEY_MAX_MS) {
        return v2_fail(source, 0U, "invalid macro timing", APP_ERROR_INVALID_ARGUMENT, out_error);
    }
    if (source_length > (size_t)APP_V2_MACRO_SOURCE_MAX_BYTES) {
        return v2_fail(source, 0U, "macro source exceeds the byte limit", APP_ERROR_MACRO_LIMIT,
                       out_error);
    }
    if (source_length == 0U) {
        return APP_ERROR_NONE;
    }

    macro_action_t *actions = calloc((size_t)APP_V2_COMPILED_ACTIONS_MAX, sizeof(*actions));
    if (actions == NULL) {
        return APP_ERROR_INTERNAL;
    }
    macro_plan_t working = {
        .actions = actions,
        .action_count = 0U,
        .estimated_duration_ms = 0U,
    };

    for (size_t offset = 0U; offset < source_length;) {
        macro_action_t action = {0};
        size_t consumed = 0U;
        app_error_code_t result =
            v2_parse_next_token(source, source_length, offset, &action, &consumed, out_error);
        if (result == APP_ERROR_NONE) {
            result = v2_append_action(&working, action, effective, source, offset, out_error);
        }
        if (result != APP_ERROR_NONE) {
            free(working.actions);
            v2_clear_plan(out_plan);
            return result;
        }
        offset += consumed;
    }

    if (working.action_count == 0U) {
        free(working.actions);
        return APP_ERROR_NONE;
    }
    macro_action_t *shrunk = realloc(working.actions, working.action_count * sizeof(*shrunk));
    if (shrunk != NULL) {
        working.actions = shrunk;
    }
    *out_plan = working;
    return APP_ERROR_NONE;
}
