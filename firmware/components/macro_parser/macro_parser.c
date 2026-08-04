#include "macro_parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "macro_keymap_us.h"

#define ASCII_SPACE 0x20U     /* first printable character */
#define ASCII_TILDE 0x7eU     /* last printable character */
#define ASCII_DELETE 0x7fU    /* first non-printable code above the ASCII range */
#define DECIMAL_BASE 10U      /* base for parsing decimal directive values */
#define HID_USAGE_ENTER 0x28U /* US HID usage for Enter (mirrors the keymap) */
#define HID_USAGE_TAB 0x2bU   /* US HID usage for Tab (mirrors the keymap) */

static void clear_plan(macro_plan_t *plan) {
    if (plan != NULL) {
        plan->actions = NULL;
        plan->action_count = 0U;
        plan->estimated_duration_ms = 0U;
    }
}

typedef struct {
    size_t line;
    size_t column;
} source_position_t;

static source_position_t locate(const char *source, size_t offset) {
    source_position_t position = {.line = 1U, .column = 1U};
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

static app_error_code_t fail(const char *source, size_t offset, const char *message,
                             app_error_code_t code, macro_parse_error_t *error) {
    if (error != NULL) {
        error->code = code;
        error->byte_offset = offset;
        const source_position_t position = locate(source, offset);
        error->line = position.line;
        error->column = position.column;
        const int written = snprintf(error->message, sizeof(error->message), "%s", message);
        if (written < 0) {
            error->message[0] = '\0';
        }
    }
    return code;
}

static bool safe_add_u32(uint32_t left, uint32_t right, uint32_t *out_value) {
    if (out_value == NULL || UINT32_MAX - left < right) {
        return false;
    }
    *out_value = left + right;
    return true;
}

static app_error_code_t append_action(macro_plan_t *plan, macro_action_t action,
                                      const macro_compile_options_t *options, const char *source,
                                      size_t offset, macro_parse_error_t *error) {
    if (plan->action_count >= APP_COMPILED_ACTION_MAX) {
        return fail(source, offset, "compiled action limit exceeded", APP_ERROR_MACRO_LIMIT, error);
    }

    uint32_t duration = 0U;
    if (action.type == MACRO_ACTION_DELAY) {
        duration = action.delay_ms;
    } else if (!safe_add_u32(options->key_press_ms, options->inter_key_ms, &duration)) {
        return fail(source, offset, "action duration overflow", APP_ERROR_MACRO_LIMIT, error);
    }

    uint32_t total = 0U;
    if (!safe_add_u32(plan->estimated_duration_ms, duration, &total) ||
        total > APP_ESTIMATED_DURATION_MAX_MS) {
        return fail(source, offset, "estimated duration limit exceeded", APP_ERROR_MACRO_LIMIT,
                    error);
    }
    plan->actions[plan->action_count++] = action;
    plan->estimated_duration_ms = total;
    return APP_ERROR_NONE;
}

static bool directive_has_invalid_character(const char *text, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        const unsigned char value = (unsigned char)text[index];
        if (value <= ASCII_SPACE || value >= ASCII_DELETE || text[index] == '{' ||
            text[index] == '}') {
            return true;
        }
    }
    return false;
}

static app_error_code_t parse_delay(const char *directive, size_t length,
                                    macro_action_t *out_action) {
    static const char prefix[] = "DELAY:";
    if (length <= sizeof(prefix) - 1U || memcmp(directive, prefix, sizeof(prefix) - 1U) != 0) {
        return APP_ERROR_MACRO_SYNTAX;
    }
    uint32_t value = 0U;
    for (size_t index = sizeof(prefix) - 1U; index < length; ++index) {
        const char character = directive[index];
        if (character < '0' || character > '9') {
            return APP_ERROR_MACRO_SYNTAX;
        }
        const uint32_t digit = (uint32_t)(character - '0');
        if (value > (UINT32_MAX - digit) / DECIMAL_BASE) {
            return APP_ERROR_MACRO_LIMIT;
        }
        value = value * DECIMAL_BASE + digit;
    }
    if (value == 0U || value > APP_DELAY_MAX_MS) {
        return APP_ERROR_MACRO_LIMIT;
    }
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_DELAY,
        .delay_ms = value,
    };
    return APP_ERROR_NONE;
}

static app_error_code_t parse_chord(char *directive, macro_action_t *out_action) {
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
            if ((modifiers & modifier) != 0U || have_key) {
                return APP_ERROR_MACRO_SYNTAX;
            }
            modifiers = (uint8_t)(modifiers | modifier);
        } else {
            if (have_key || !macro_keymap_us_named(cursor, &key)) {
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
    /*
     * A chord's modifiers come only from its explicit modifier tokens. The key
     * contributes its usage alone; any implicit shift the keymap attaches to an
     * uppercase letter (needed when typing literal text) must not leak into the
     * chord. Per SPEC 10.4, {CTRL+L} means Ctrl+L, not Ctrl+Shift+L.
     */
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_CHORD,
        .modifiers = modifiers,
        .usage = key.usage,
        .delay_ms = 0U,
    };
    return APP_ERROR_NONE;
}

static app_error_code_t parse_directive(const char *source, size_t offset, const char *directive,
                                        size_t length, macro_action_t *out_action,
                                        macro_parse_error_t *error) {
    if (length == 0U || length >= 64U || directive_has_invalid_character(directive, length)) {
        return fail(source, offset, "invalid directive", APP_ERROR_MACRO_SYNTAX, error);
    }
    char buffer[64U];
    memcpy(buffer, directive, length);
    buffer[length] = '\0';

    if (strncmp(buffer, "DELAY:", sizeof("DELAY:") - 1U) == 0) {
        const app_error_code_t result = parse_delay(buffer, length, out_action);
        if (result != APP_ERROR_NONE) {
            return fail(source, offset, "invalid delay directive", result, error);
        }
        return APP_ERROR_NONE;
    }

    if (strchr(buffer, '+') != NULL) {
        if (parse_chord(buffer, out_action) != APP_ERROR_NONE) {
            return fail(source, offset, "invalid chord directive", APP_ERROR_MACRO_SYNTAX, error);
        }
        return APP_ERROR_NONE;
    }

    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_named(buffer, &key)) {
        return fail(source, offset, "unknown key directive", APP_ERROR_MACRO_SYNTAX, error);
    }
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_KEY,
        .modifiers = key.modifiers,
        .usage = key.usage,
        .delay_ms = 0U,
    };
    return APP_ERROR_NONE;
}

static macro_action_t key_action(uint8_t modifiers, uint8_t usage) {
    return (macro_action_t){
        .type = MACRO_ACTION_KEY,
        .modifiers = modifiers,
        .usage = usage,
    };
}

static app_error_code_t parse_open_brace(const char *source, size_t source_length, size_t offset,
                                         macro_action_t *out_action, size_t *out_consumed,
                                         macro_parse_error_t *out_error) {
    if (offset + 1U < source_length && source[offset + 1U] == '{') {
        macro_hid_key_t key = {0U, 0U};
        (void)macro_keymap_us_printable('{', &key);
        *out_action = key_action(key.modifiers, key.usage);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }
    const char *closing = memchr(source + offset + 1U, '}', source_length - offset - 1U);
    if (closing == NULL) {
        return fail(source, offset, "unmatched opening brace", APP_ERROR_MACRO_SYNTAX, out_error);
    }
    const size_t closing_offset = (size_t)(closing - source);
    const app_error_code_t result = parse_directive(
        source, offset, source + offset + 1U, closing_offset - offset - 1U, out_action, out_error);
    *out_consumed = closing_offset + 1U - offset;
    return result;
}

static app_error_code_t parse_close_brace(const char *source, size_t source_length, size_t offset,
                                          macro_action_t *out_action, size_t *out_consumed,
                                          macro_parse_error_t *out_error) {
    if (offset + 1U < source_length && source[offset + 1U] == '}') {
        macro_hid_key_t key = {0U, 0U};
        (void)macro_keymap_us_printable('}', &key);
        *out_action = key_action(key.modifiers, key.usage);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }
    return fail(source, offset, "unmatched closing brace", APP_ERROR_MACRO_SYNTAX, out_error);
}

static app_error_code_t parse_printable(const char *source, size_t offset,
                                        macro_action_t *out_action, size_t *out_consumed,
                                        macro_parse_error_t *out_error) {
    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_printable(source[offset], &key)) {
        return fail(source, offset, "unmappable character", APP_ERROR_MACRO_SYNTAX, out_error);
    }
    *out_action = key_action(key.modifiers, key.usage);
    *out_consumed = 1U;
    return APP_ERROR_NONE;
}

/*
 * Parse one token at source[offset]: fill *out_action, the source span it
 * consumes (*out_consumed), and the offset used for an append error location
 * (*out_action_offset, which follows a CR/LF pair to the LF like a bare LF).
 */
static app_error_code_t parse_next_token(const char *source, size_t source_length, size_t offset,
                                         size_t *out_action_offset, macro_action_t *out_action,
                                         size_t *out_consumed, macro_parse_error_t *out_error) {
    const unsigned char byte = (unsigned char)source[offset];
    *out_action_offset = offset;
    if (byte >= 0x80U || byte == 0U) {
        return fail(source, offset, "unsupported character", APP_ERROR_MACRO_SYNTAX, out_error);
    }
    const char character = source[offset];
    if (character == '\r') {
        if (offset + 1U < source_length && source[offset + 1U] == '\n') {
            *out_action = key_action(0U, HID_USAGE_ENTER);
            *out_action_offset = offset + 1U;
            *out_consumed = 2U;
            return APP_ERROR_NONE;
        }
        return fail(source, offset, "lone carriage return", APP_ERROR_MACRO_SYNTAX, out_error);
    }
    if (character == '\n') {
        *out_action = key_action(0U, HID_USAGE_ENTER);
        *out_consumed = 1U;
        return APP_ERROR_NONE;
    }
    if (character == '\t') {
        *out_action = key_action(0U, HID_USAGE_TAB);
        *out_consumed = 1U;
        return APP_ERROR_NONE;
    }
    if (character == '{') {
        return parse_open_brace(source, source_length, offset, out_action, out_consumed, out_error);
    }
    if (character == '}') {
        return parse_close_brace(source, source_length, offset, out_action, out_consumed,
                                 out_error);
    }
    if (byte < ASCII_SPACE || byte > ASCII_TILDE) {
        return fail(source, offset, "unsupported control character", APP_ERROR_MACRO_SYNTAX,
                    out_error);
    }
    return parse_printable(source, offset, out_action, out_consumed, out_error);
}

app_error_code_t macro_compile(const char *source, size_t source_length,
                               const macro_compile_options_t *options, macro_plan_t *out_plan,
                               macro_parse_error_t *out_error) {
    const macro_compile_options_t defaults = {
        .key_press_ms = APP_KEY_PRESS_DEFAULT_MS,
        .inter_key_ms = APP_INTER_KEY_DEFAULT_MS,
    };
    const macro_compile_options_t *effective = options == NULL ? &defaults : options;

    if (out_plan == NULL || (source == NULL && source_length != 0U) ||
        source_length > APP_MACRO_SOURCE_MAX_BYTES || effective->key_press_ms == 0U ||
        effective->key_press_ms > APP_DELAY_MAX_MS || effective->inter_key_ms > APP_DELAY_MAX_MS) {
        if (out_plan != NULL) {
            clear_plan(out_plan);
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }
    clear_plan(out_plan);
    if (out_error != NULL) {
        memset(out_error, 0, sizeof(*out_error));
    }
    if (source_length == 0U) {
        return APP_ERROR_NONE;
    }

    macro_action_t *actions = calloc(APP_COMPILED_ACTION_MAX, sizeof(*actions));
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
        size_t action_offset = offset;
        size_t consumed = 0U;
        app_error_code_t result = parse_next_token(source, source_length, offset, &action_offset,
                                                   &action, &consumed, out_error);
        if (result == APP_ERROR_NONE) {
            result = append_action(&working, action, effective, source, action_offset, out_error);
        }
        if (result != APP_ERROR_NONE) {
            free(working.actions);
            clear_plan(out_plan);
            return result;
        }
        offset += consumed;
    }

    if (working.action_count == 0U) {
        free(working.actions);
        clear_plan(out_plan);
        return APP_ERROR_NONE;
    }
    macro_action_t *shrunk = realloc(working.actions, working.action_count * sizeof(*shrunk));
    if (shrunk != NULL) {
        working.actions = shrunk;
    }
    *out_plan = working;
    return APP_ERROR_NONE;
}

void macro_plan_free(macro_plan_t *plan) {
    if (plan == NULL) {
        return;
    }
    free(plan->actions);
    clear_plan(plan);
}
