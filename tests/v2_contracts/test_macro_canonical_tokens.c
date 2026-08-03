#include <stdio.h>
#include <string.h>

#include "macro_parser.h"

static int failures = 0;

static void expect_valid(const char *source) {
    const macro_compile_options_t options = {.key_press_ms = 8U, .inter_key_ms = 15U};
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result =
        macro_compile_v2(source, strlen(source), &options, &plan, &error);
    if (result != APP_ERROR_NONE) {
        (void)fprintf(stderr, "[FAIL] expected valid chord %s: %s\n", source, error.message);
        ++failures;
    }
    macro_plan_v2_free(&plan);
}

static void expect_invalid_chord(const char *source) {
    const macro_compile_options_t options = {.key_press_ms = 8U, .inter_key_ms = 15U};
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result =
        macro_compile_v2(source, strlen(source), &options, &plan, &error);
    if (result != APP_ERROR_MACRO_SYNTAX || strcmp(error.message, "invalid chord") != 0) {
        (void)fprintf(stderr, "[FAIL] expected invalid chord %s\n", source);
        ++failures;
    }
    macro_plan_v2_free(&plan);
}

int main(void) {
    expect_valid("{CTRL+A}");
    expect_valid("{ALT+1}");
    expect_valid("{SHIFT+F12}");
    expect_invalid_chord("{CTRL+a}");
    expect_invalid_chord("{CTRL+!}");

    if (failures != 0) {
        (void)fprintf(stderr, "%d canonical chord-token test(s) failed\n", failures);
        return 1;
    }
    (void)printf("all canonical v2 chord-token tests passed\n");
    return 0;
}
