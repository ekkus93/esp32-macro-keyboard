#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "macro_parser.h"

typedef struct {
    const char *name;
    const uint8_t *source;
    size_t source_length;
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
    bool valid;
    uint32_t expected_duration_ms;
    const macro_action_t *expected_actions;
    size_t expected_action_count;
    app_error_code_t expected_error_code;
    size_t expected_byte_offset;
    size_t expected_line;
    size_t expected_column;
    const char *expected_message_class;
} v2_macro_case_t;

#include "generated_macro_corpus.inc"

static int failures = 0;

static void report_failure(const char *case_name, const char *message) {
    (void)fprintf(stderr, "[FAIL] %s: %s\n", case_name, message);
    ++failures;
}

static const char *message_class(const char *message) {
    if (strcmp(message, "estimated duration limit exceeded") == 0) {
        return "duration_limit";
    }
    if (strcmp(message, "unknown key directive") == 0) {
        return "unknown_key";
    }
    if (strcmp(message, "invalid chord directive") == 0 ||
        strcmp(message, "invalid chord") == 0) {
        return "invalid_chord";
    }
    if (strcmp(message, "invalid delay directive") == 0 ||
        strcmp(message, "delay is outside the allowed range") == 0) {
        return "invalid_delay";
    }
    if (strcmp(message, "lone carriage return") == 0 ||
        strcmp(message, "carriage return must be followed by line feed") == 0) {
        return "lone_carriage_return";
    }
    if (strcmp(message, "unmatched opening brace") == 0) {
        return "unmatched_opening_brace";
    }
    if (strcmp(message, "unmatched closing brace") == 0) {
        return "unmatched_closing_brace";
    }
    if (strcmp(message, "unsupported character") == 0 ||
        strcmp(message, "source contains unsupported character") == 0) {
        return "unsupported_character";
    }
    if (strcmp(message, "invalid directive") == 0) {
        return "invalid_directive";
    }
    if (strcmp(message, "compiled action limit exceeded") == 0 ||
        strcmp(message, "action limit exceeded") == 0) {
        return "action_limit";
    }
    if (strcmp(message, "macro source exceeds the byte limit") == 0) {
        return "source_limit";
    }
    if (strcmp(message, "invalid macro timing") == 0) {
        return "invalid_timing";
    }
    return "unclassified";
}

static bool actions_equal(const macro_action_t *expected, const macro_action_t *actual) {
    return expected->type == actual->type && expected->modifiers == actual->modifiers &&
           expected->usage == actual->usage && expected->delay_ms == actual->delay_ms;
}

static void verify_valid_case(const v2_macro_case_t *test_case, const macro_plan_t *plan) {
    if (plan->action_count != test_case->expected_action_count ||
        plan->estimated_duration_ms != test_case->expected_duration_ms) {
        report_failure(test_case->name, "action count or estimated duration differs");
        return;
    }
    for (size_t index = 0U; index < test_case->expected_action_count; ++index) {
        if (!actions_equal(&test_case->expected_actions[index], &plan->actions[index])) {
            report_failure(test_case->name, "compiled action differs");
            return;
        }
    }
}

static void verify_invalid_case(const v2_macro_case_t *test_case, app_error_code_t result,
                                const macro_parse_error_t *error) {
    if (result != test_case->expected_error_code || error->code != result ||
        error->byte_offset != test_case->expected_byte_offset ||
        error->line != test_case->expected_line ||
        error->column != test_case->expected_column ||
        strcmp(message_class(error->message), test_case->expected_message_class) != 0) {
        report_failure(test_case->name, "error code, location, or message class differs");
    }
}

static void run_case(const v2_macro_case_t *test_case) {
    const macro_compile_options_t options = {
        .key_press_ms = test_case->key_press_ms,
        .inter_key_ms = test_case->inter_key_ms,
    };
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const int failures_before = failures;
    const app_error_code_t result = macro_compile_v2(
        (const char *)test_case->source, test_case->source_length, &options, &plan, &error);

    if (test_case->valid) {
        if (result != APP_ERROR_NONE) {
            report_failure(test_case->name, "compiler rejected a valid corpus case");
        } else {
            verify_valid_case(test_case, &plan);
        }
    } else if (result == APP_ERROR_NONE) {
        report_failure(test_case->name, "compiler accepted an invalid corpus case");
    } else {
        verify_invalid_case(test_case, result, &error);
    }
    macro_plan_v2_free(&plan);

    if (failures == failures_before) {
        (void)printf("[PASS] %s\n", test_case->name);
    }
}

int main(void) {
    for (size_t index = 0U; index < v2_macro_case_count; ++index) {
        run_case(&v2_macro_cases[index]);
    }

    if (failures != 0) {
        (void)fprintf(stderr, "%d macro conformance case(s) failed\n", failures);
        return 1;
    }
    (void)printf("all %zu native macro conformance cases passed\n", v2_macro_case_count);
    return 0;
}
