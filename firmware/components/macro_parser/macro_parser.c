#include "macro_parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macro_keymap_us.h"
#include "macro_limits.h"

static void clear_plan(macro_plan_t *plan)
{
    if (plan != NULL) {
        plan->actions = NULL;
        plan->action_count = 0U;
        plan->estimated_duration_ms = 0U;
    }
}

static void locate(const char *source, size_t offset, size_t *line, size_t *column)
{
    *line = 1U;
    *column = 1U;
    for (size_t index = 0U; index < offset; ++index) {
        if (source[index] == '\n') {
            ++(*line);
            *column = 1U;
        } else {
            ++(*column);
        }
    }
}

static app_error_code_t fail(const char *source,
                             size_t offset,
                             app_error_code_t code,
                             const char *message,
                             macro_parse_error_t *error)
{
    if (error != NULL) {
        error->code = code;
        error->byte_offset = offset;
        locate(source, offset, &error->line, &error->column);
        const int written = snprintf(error->message, sizeof(error->message), "%s", message);
        if (written < 0) {
            error->message[0] = '\0';
        }
    }
    return code;
}

static bool safe_add_u32(uint32_t left, uint32_t right, uint32_t *out_value)
{
    if (UINT32_MAX - left < right) {
        return false;
    }
    *out_value = left + right;
    return true;
}

static app_error_code_t append_action(macro_plan_t *plan,
                                      macro_action_t action,
                                      const macro_compile_options_t *options,
                                      const char *source,
                                      size_t offset,
                                      macro_parse_error_t *error)
{
    if (plan->action_count >= APP_COMPILED_ACTION_MAX) {
        return fail(source, offset, APP_ERROR_MACRO_LIMIT, "compiled action limit exceeded", error);
    }
    uint32_t duration = action.type == MACRO_ACTION_DELAY
                            ? action.delay_ms
                            : options->key_press_ms + options->inter_key_ms;
    uint32_t total = 0U;
    if (!safe_add_u32(plan->estimated_duration_ms, duration, &total) ||
        total > APP_ESTIMATED_DURATION_MAX_MS) {
        return fail(source, offset, APP_ERROR_MACRO_LIMIT, "estimated duration limit exceeded", error);
    }
    plan->actions[plan->action_count++] = action;
    plan->estimated_duration_ms = total;
    return APP_ERROR_NONE;
}

static bool directive_has_invalid_character(const char *text, size_t length)
{
    for (size_t index = 0U; index < length; ++index) {
        const unsigned char value = (unsigned char)text[index];
        if (value <= 0x20U || value >= 0x7fU || text[index] == '{' || text[index] == '}') {
            return true;
        }
    }
    return false;
}

static app_error_code_t parse_delay(const char *directive,
                                    size_t length,
                                    macro_action_t *out_action)
{
    static const char prefix[] = "DELAY:";
    if (length <= sizeof(prefix) - 1U ||
        memcmp(directive, prefix, sizeof(prefix) - 1U) != 0) {
        return APP_ERROR_MACRO_SYNTAX;
    }
    uint32_t value = 0U;
    for (size_t index = sizeof(prefix) - 1U; index < length; ++index) {
        const char character = directive[index];
        if (character < '0' || character > '9') {
            return APP_ERROR_MACRO_SYNTAX;
        }
        const uint32_t digit = (uint32_t)(character - '0');
        if (value > (UINT32_MAX - digit) / 10U) {
            return APP_ERROR_MACRO_LIMIT;
        }
        value = value * 10U + digit;
    }
    if (value == 0U || value > APP_DELAY_MAX_MS) {
        return APP_ERROR_MACRO_LIMIT;
    }
    *out_action = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = value};
    return APP_ERROR_NONE;
}

static app_error_code_t parse_chord(char *directive, macro_action_t *out_action)
{
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
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_CHORD,
        .modifiers = (uint8_t)(modifiers | key.modifiers),
        .usage = key.usage,
        .delay_ms = 0U,
    };
    return APP_ERROR_NONE;
}

static app_error_code_t parse_directive(const char *source,
                                        size_t offset,
                                        const char *directive,
                                        size_t length,
                                        macro_action_t *out_action,
                                        macro_parse_error_t *error)
{
    if (length == 0U || length >= 64U || directive_has_invalid_character(directive, length)) {
        return fail(source, offset, APP_ERROR_MACRO_SYNTAX, "invalid directive", error);
    }
    char buffer[64U];
    memcpy(buffer, directive, length);
    buffer[length] = '\0';

    if (strncmp(buffer, "DELAY:", 6U) == 0) {
        const app_error_code_t result = parse_delay(buffer, length, out_action);
        if (result != APP_ERROR_NONE) {
            return fail(source, offset, result, "invalid delay directive", error);
        }
        return APP_ERROR_NONE;
    }

    if (strchr(buffer, '+') != NULL) {
        if (parse_chord(buffer, out_action) != APP_ERROR_NONE) {
            return fail(source, offset, APP_ERROR_MACRO_SYNTAX, "invalid chord directive", error);
        }
        return APP_ERROR_NONE;
    }

    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_named(buffer, &key)) {
        return fail(source, offset, APP_ERROR_MACRO_SYNTAX, "unknown key directive", error);
    }
    *out_action = (macro_action_t){
        .type = MACRO_ACTION_KEY,
        .modifiers = key.modifiers,
        .usage = key.usage,
        .delay_ms = 0U,
    };
    return APP_ERROR_NONE;
}

app_error_code_t macro_compile(const char *source,
                               size_t source_length,
                               const macro_compile_options_t *options,
                               macro_plan_t *out_plan,
                               macro_parse_error_t *out_error)
{
    const macro_compile_options_t defaults = {
        .key_press_ms = APP_KEY_PRESS_DEFAULT_MS,
        .inter_key_ms = APP_INTER_KEY_DEFAULT_MS,
    };
    const macro_compile_options_t *effective = options == NULL ? &defaults : options;

    if (out_plan == NULL || (source == NULL && source_length != 0U) ||
        source_length > APP_MACRO_SOURCE_MAX_BYTES || effective->key_press_ms == 0U ||
        effective->inter_key_ms > APP_DELAY_MAX_MS) {
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
    macro_plan_t working = {.actions = actions, .action_count = 0U, .estimated_duration_ms = 0U};

    for (size_t offset = 0U; offset < source_length;) {
        const unsigned char byte = (unsigned char)source[offset];
        macro_action_t action = {0};
        app_error_code_t result = APP_ERROR_NONE;

        if (byte >= 0x80U || byte == 0U) {
            result = fail(source, offset, APP_ERROR_MACRO_SYNTAX, "unsupported character", out_error);
        } else if (source[offset] == '\r') {
            if (offset + 1U < source_length && source[offset + 1U] == '\n') {
                ++offset;
                action = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 0x28U};
                result = append_action(&working, action, effective, source, offset, out_error);
                ++offset;
                if (result == APP_ERROR_NONE) {
                    continue;
                }
            } else {
                result = fail(source, offset, APP_ERROR_MACRO_SYNTAX, "lone carriage return", out_error);
            }
        } else if (source[offset] == '\n') {
            action = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 0x28U};
            result = append_action(&working, action, effective, source, offset, out_error);
            ++offset;
            if (result == APP_ERROR_NONE) {
                continue;
            }
        } else if (source[offset] == '\t') {
            action = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 0x2bU};
            result = append_action(&working, action, effective, source, offset, out_error);
            ++offset;
            if (result == APP_ERROR_NONE) {
                continue;
            }
        } else if (source[offset] == '{') {
            if (offset + 1U < source_length && source[offset + 1U] == '{') {
                macro_hid_key_t key = {0U, 0U};
                (void)macro_keymap_us_printable('{', &key);
                action = (macro_action_t){.type = MACRO_ACTION_KEY,
                                          .modifiers = key.modifiers,
                                          .usage = key.usage};
                result = append_action(&working, action, effective, source, offset, out_error);
                offset += 2U;
                if (result == APP_ERROR_NONE) {
                    continue;
                }
            } else {
                const char *closing = memchr(source + offset + 1U, '}', source_length - offset - 1U);
                if (closing == NULL) {
                    result = fail(source, offset, APP_ERROR_MACRO_SYNTAX, "unmatched opening brace", out_error);
                } else {
                    const size_t closing_offset = (size_t)(closing - source);
                    result = parse_directive(source, offset, source + offset + 1U,
                                             closing_offset - offset - 1U, &action, out_error);
                    if (result == APP_ERROR_NONE) {
                        result = append_action(&working, action, effective, source, offset, out_error);
                    }
                    offset = closing_offset + 1U;
                    if (result == APP_ERROR_NONE) {
                        continue;
                    }
                }
            }
        } else if (source[offset] == '}') {
            if (offset + 1U < source_length && source[offset + 1U] == '}') {
                macro_hid_key_t key = {0U, 0U};
                (void)macro_keymap_us_printable('}', &key);
                action = (macro_action_t){.type = MACRO_ACTION_KEY,
                                          .modifiers = key.modifiers,
                                          .usage = key.usage};
                result = append_action(&working, action, effective, source, offset, out_error);
                offset += 2U;
                if (result == APP_ERROR_NONE) {
                    continue;
                }
            } else {
                result = fail(source, offset, APP_ERROR_MACRO_SYNTAX, "unmatched closing brace", out_error);
            }
        } else if (byte < 0x20U || byte > 0x7eU) {
            result = fail(source, offset, APP_ERROR_MACRO_SYNTAX, "unsupported control character", out_error);
        } else {
            macro_hid_key_t key = {0U, 0U};
            if (!macro_keymap_us_printable(source[offset], &key)) {
                result = fail(source, offset, APP_ERROR_MACRO_SYNTAX, "unmappable character", out_error);
            } else {
                action = (macro_action_t){.type = MACRO_ACTION_KEY,
                                          .modifiers = key.modifiers,
                                          .usage = key.usage};
                result = append_action(&working, action, effective, source, offset, out_error);
                ++offset;
                if (result == APP_ERROR_NONE) {
                    continue;
                }
            }
        }

        free(working.actions);
        clear_plan(out_plan);
        return result;
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

void macro_plan_free(macro_plan_t *plan)
{
    if (plan == NULL) {
        return;
    }
    free(plan->actions);
    clear_plan(plan);
}
