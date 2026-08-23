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
        (void)fprintf(stderr, "[FAIL] expected valid %s: %s\n", source, error.message);
        ++failures;
    }
    macro_plan_v2_free(&plan);
}

static void expect_invalid(const char *source, const char *expected_message) {
    const macro_compile_options_t options = {.key_press_ms = 8U, .inter_key_ms = 15U};
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result =
        macro_compile_v2(source, strlen(source), &options, &plan, &error);
    if (result != APP_ERROR_MACRO_SYNTAX || strcmp(error.message, expected_message) != 0) {
        (void)fprintf(stderr, "[FAIL] expected %s for %s\n", expected_message, source);
        ++failures;
    }
    macro_plan_v2_free(&plan);
}

/* SPEC_V2 7.7/7.8: directive spelling is uppercase and canonical, for both a
 * standalone modifier tap and a modifier/key inside a [...] simultaneous-key
 * group -- there is no case-insensitive fallback anywhere in the grammar. The
 * retired {MOD+KEY} chord syntax this file used to cover no longer parses at
 * all (checked here too): '+' has no meaning inside a directive body since
 * the [...] group replaced it. */
int main(void) {
    expect_valid("{CTRL}");
    expect_valid("{ALT}");
    expect_valid("[{SHIFT}{F12}]");
    expect_valid("[{CTRL}a]");
    expect_invalid("{ctrl}", "unknown key directive");
    expect_invalid("[{ctrl}a]", "unknown key directive");
    expect_invalid("[{CTRL}{f2}]", "unknown key directive");
    expect_invalid("{CTRL+A}", "unknown key directive");
    expect_invalid("{A}", "unknown key directive");
    expect_invalid("{1}", "unknown key directive");

    if (failures != 0) {
        (void)fprintf(stderr, "%d canonical v2 token test(s) failed\n", failures);
        return 1;
    }
    (void)printf("all canonical v2 token tests passed\n");
    return 0;
}
