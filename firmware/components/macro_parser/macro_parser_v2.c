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

/* key.usage == 0 means a standalone modifier tap (e.g. {CTRL} alone): the
 * action carries the modifier bit and no usage byte. Every other caller
 * passes a real key usage, giving usage_count 1. Takes the whole
 * macro_hid_key_t rather than separate modifiers/usage parameters -- every
 * call site already has one, and clang-tidy's bugprone-easily-swappable-
 * parameters correctly flags two bare adjacent uint8_t as a mistake risk. */
static macro_action_t v2_key_action(macro_hid_key_t key) {
    macro_action_t action = {
        .type = MACRO_ACTION_KEY,
        .modifiers = key.modifiers,
        .usage_count = 0U,
        .delay_ms = 0U,
    };
    if (key.usage != 0U) {
        action.usages[0] = key.usage;
        action.usage_count = 1U;
    }
    return action;
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
        .usage_count = 0U,
        .delay_ms = value,
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

    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_v2_named_directive(buffer, &key)) {
        return v2_fail(source, offset, "unknown key directive", APP_ERROR_MACRO_SYNTAX, error);
    }
    *out_action = v2_key_action(key);
    return APP_ERROR_NONE;
}

typedef struct {
    uint8_t modifiers;
    uint8_t usages[MACRO_ACTION_USAGES_MAX];
    uint8_t usage_count;
} v2_group_accumulator_t;

/* Merges one already-parsed single action into a [...] group's accumulated
 * modifiers/usages. Rejects a repeated modifier or ordinary key, and enforces
 * the MACRO_ACTION_USAGES_MAX (HID report) ceiling on ordinary keys; modifiers
 * do not count against that ceiling. `offset` is the position of the member
 * just parsed, for error locality. */
static app_error_code_t v2_merge_group_member(v2_group_accumulator_t *accumulator,
                                              uint8_t member_modifiers,
                                              const uint8_t *member_usages,
                                              uint8_t member_usage_count, const char *source,
                                              size_t offset, macro_parse_error_t *out_error) {
    if ((accumulator->modifiers & member_modifiers) != 0U) {
        return v2_fail(source, offset, "duplicate modifier in a simultaneous-key group",
                       APP_ERROR_MACRO_SYNTAX, out_error);
    }
    accumulator->modifiers = (uint8_t)(accumulator->modifiers | member_modifiers);
    for (uint8_t index = 0U; index < member_usage_count; ++index) {
        for (uint8_t existing = 0U; existing < accumulator->usage_count; ++existing) {
            if (accumulator->usages[existing] == member_usages[index]) {
                return v2_fail(source, offset, "duplicate key in a simultaneous-key group",
                               APP_ERROR_MACRO_SYNTAX, out_error);
            }
        }
        if (accumulator->usage_count >= MACRO_ACTION_USAGES_MAX) {
            return v2_fail(source, offset, "simultaneous-key group exceeds the 6-key limit",
                           APP_ERROR_MACRO_LIMIT, out_error);
        }
        accumulator->usages[accumulator->usage_count] = member_usages[index];
        ++accumulator->usage_count;
    }
    return APP_ERROR_NONE;
}

/* The [[ / ]] escape as a group member: merges one literal bracket character
 * the same way any other single-usage member merges. `bracket` is '[' or
 * ']'. */
static app_error_code_t v2_merge_literal_bracket(char bracket, v2_group_accumulator_t *accumulator,
                                                 const char *source, size_t offset,
                                                 macro_parse_error_t *out_error) {
    macro_hid_key_t key = {0U, 0U};
    if (!macro_keymap_us_printable(bracket, &key)) {
        return v2_fail(source, offset, "source contains unsupported character",
                       APP_ERROR_MACRO_SYNTAX, out_error);
    }
    const macro_action_t literal = v2_key_action(key);
    return v2_merge_group_member(accumulator, literal.modifiers, literal.usages,
                                 literal.usage_count, source, offset, out_error);
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
        *out_action = v2_key_action(key);
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
        *out_action = v2_key_action(key);
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
    *out_action = v2_key_action(key);
    *out_consumed = 1U;
    return APP_ERROR_NONE;
}

/* Every token EXCEPT '[' and ']': carriage return/newline/tab, {directives},
 * and printable literals. Kept separate from v2_parse_next_token (which adds
 * the two bracket cases) so that v2_scan_group_content -- itself reachable
 * from a bracket case -- can delegate here without closing a call cycle back
 * through the bracket parsers (clang-tidy's misc-no-recursion forbids that
 * cycle regardless of it being runtime-bounded to one level, since groups
 * cannot nest). */
static app_error_code_t v2_parse_non_bracket_token(const char *source, size_t source_length,
                                                   size_t offset, macro_action_t *out_action,
                                                   size_t *out_consumed,
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
        *out_action =
            v2_key_action((macro_hid_key_t){.modifiers = 0U, .usage = V2_HID_USAGE_ENTER});
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }
    if (character == '\n') {
        *out_action =
            v2_key_action((macro_hid_key_t){.modifiers = 0U, .usage = V2_HID_USAGE_ENTER});
        *out_consumed = 1U;
        return APP_ERROR_NONE;
    }
    if (character == '\t') {
        *out_action = v2_key_action((macro_hid_key_t){.modifiers = 0U, .usage = V2_HID_USAGE_TAB});
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

/* Bundles a group scan's fixed inputs into one struct: mainly so
 * v2_scan_group_content takes one pointer instead of source/source_length/
 * bracket_offset as three separate parameters -- clang-tidy's
 * bugprone-easily-swappable-parameters flags size_t source_length next to
 * size_t bracket_offset (they are never combined in one expression the way
 * e.g. v2_parse_open_brace's source_length/offset are, so nothing else
 * signals they play different roles), and a single context pointer removes
 * the ambiguity outright rather than fighting the heuristic. */
typedef struct {
    const char *source;
    size_t source_length;
    size_t bracket_offset;
} v2_group_scan_context_t;

typedef enum { V2_GROUP_STEP_CONTINUE, V2_GROUP_STEP_DONE } v2_group_step_result_t;

/* Handles exactly one position within a [...] group's content: a doubled
 * bracket is a literal-character member, a bare ']' ends the group (or is
 * "empty simultaneous-key group" with nothing accumulated yet), a bare '['
 * is a nesting attempt, and anything else is one ordinary member delegated to
 * v2_parse_non_bracket_token and merged in. Split out of
 * v2_scan_group_content to keep that function's own cognitive complexity
 * (nesting × branches) below the enforced threshold. */
static app_error_code_t v2_scan_group_step(const v2_group_scan_context_t *context, size_t offset,
                                           v2_group_accumulator_t *out, size_t *out_consumed,
                                           v2_group_step_result_t *out_step,
                                           macro_parse_error_t *out_error) {
    const char *source = context->source;
    const bool doubled =
        offset + 1U < context->source_length && source[offset + 1U] == source[offset];

    if (source[offset] == ']') {
        if (doubled) {
            const app_error_code_t merge_result =
                v2_merge_literal_bracket(']', out, source, offset, out_error);
            if (merge_result == APP_ERROR_NONE) {
                *out_consumed = 2U;
                *out_step = V2_GROUP_STEP_CONTINUE;
            }
            return merge_result;
        }
        if (out->usage_count == 0U && out->modifiers == 0U) {
            return v2_fail(source, context->bracket_offset, "empty simultaneous-key group",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        *out_step = V2_GROUP_STEP_DONE;
        return APP_ERROR_NONE;
    }
    if (source[offset] == '[') {
        if (!doubled) {
            return v2_fail(source, offset, "simultaneous-key groups do not nest",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        const app_error_code_t merge_result =
            v2_merge_literal_bracket('[', out, source, offset, out_error);
        if (merge_result == APP_ERROR_NONE) {
            *out_consumed = 2U;
            *out_step = V2_GROUP_STEP_CONTINUE;
        }
        return merge_result;
    }

    macro_action_t member = {0};
    size_t consumed = 0U;
    const app_error_code_t result = v2_parse_non_bracket_token(
        source, context->source_length, offset, &member, &consumed, out_error);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (member.type == MACRO_ACTION_DELAY) {
        return v2_fail(source, offset, "a delay is not permitted inside a simultaneous-key group",
                       APP_ERROR_MACRO_SYNTAX, out_error);
    }
    const app_error_code_t merge_result = v2_merge_group_member(
        out, member.modifiers, member.usages, member.usage_count, source, offset, out_error);
    if (merge_result == APP_ERROR_NONE) {
        *out_consumed = consumed;
        *out_step = V2_GROUP_STEP_CONTINUE;
    }
    return merge_result;
}

/* Scans a [...] group's content, starting from the '[' itself
 * (`context->bracket_offset`) so the position math matches every other
 * delimiter parser in this file. Per-position logic lives in
 * v2_scan_group_step; this loop just drives it until the group ends or an
 * error surfaces. An unmatched opening bracket reports at bracket_offset,
 * the position a reader would look for the mistake. */
static app_error_code_t v2_scan_group_content(const v2_group_scan_context_t *context,
                                              v2_group_accumulator_t *out, size_t *out_group_end,
                                              macro_parse_error_t *out_error) {
    *out = (v2_group_accumulator_t){0};
    size_t offset = context->bracket_offset + 1U;
    while (true) {
        if (offset >= context->source_length) {
            return v2_fail(context->source, context->bracket_offset, "unmatched opening bracket",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        size_t consumed = 0U;
        v2_group_step_result_t step = V2_GROUP_STEP_CONTINUE;
        const app_error_code_t result =
            v2_scan_group_step(context, offset, out, &consumed, &step, out_error);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (step == V2_GROUP_STEP_DONE) {
            *out_group_end = offset + 1U;
            return APP_ERROR_NONE;
        }
        offset += consumed;
    }
}

static app_error_code_t v2_parse_open_bracket(const char *source, size_t source_length,
                                              size_t offset, macro_action_t *out_action,
                                              size_t *out_consumed,
                                              macro_parse_error_t *out_error) {
    if (offset + 1U < source_length && source[offset + 1U] == '[') {
        macro_hid_key_t key = {0U, 0U};
        if (!macro_keymap_us_printable('[', &key)) {
            return v2_fail(source, offset, "source contains unsupported character",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        *out_action = v2_key_action(key);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }

    v2_group_accumulator_t accumulator = {0};
    size_t group_end = 0U;
    const v2_group_scan_context_t context = {
        .source = source,
        .source_length = source_length,
        .bracket_offset = offset,
    };
    const app_error_code_t result =
        v2_scan_group_content(&context, &accumulator, &group_end, out_error);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    macro_action_t action = {
        .type = MACRO_ACTION_CHORD,
        .modifiers = accumulator.modifiers,
        .usage_count = accumulator.usage_count,
        .delay_ms = 0U,
    };
    memcpy(action.usages, accumulator.usages, sizeof(action.usages));
    *out_action = action;
    *out_consumed = group_end - offset;
    return APP_ERROR_NONE;
}

static app_error_code_t v2_parse_close_bracket(const char *source, size_t source_length,
                                               size_t offset, macro_action_t *out_action,
                                               size_t *out_consumed,
                                               macro_parse_error_t *out_error) {
    if (offset + 1U < source_length && source[offset + 1U] == ']') {
        macro_hid_key_t key = {0U, 0U};
        if (!macro_keymap_us_printable(']', &key)) {
            return v2_fail(source, offset, "source contains unsupported character",
                           APP_ERROR_MACRO_SYNTAX, out_error);
        }
        *out_action = v2_key_action(key);
        *out_consumed = 2U;
        return APP_ERROR_NONE;
    }
    return v2_fail(source, offset, "unmatched closing bracket", APP_ERROR_MACRO_SYNTAX, out_error);
}

static app_error_code_t v2_parse_next_token(const char *source, size_t source_length, size_t offset,
                                            macro_action_t *out_action, size_t *out_consumed,
                                            macro_parse_error_t *out_error) {
    if (source[offset] == '[') {
        return v2_parse_open_bracket(source, source_length, offset, out_action, out_consumed,
                                     out_error);
    }
    if (source[offset] == ']') {
        return v2_parse_close_bracket(source, source_length, offset, out_action, out_consumed,
                                      out_error);
    }
    return v2_parse_non_bracket_token(source, source_length, offset, out_action, out_consumed,
                                      out_error);
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
