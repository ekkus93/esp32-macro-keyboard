#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_uuid.h"
#include "macro_keymap_us.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "macro_parser.h"

#include "test_assert.h"

static app_error_code_t compile_explicit(const char *source,
                                         size_t source_length,
                                         const macro_compile_options_t *options,
                                         macro_plan_t *out_plan,
                                         macro_parse_error_t *out_error)
{
    TEST_CHECK(out_plan != NULL);
    memset(out_plan, 0, sizeof(*out_plan));
    if (out_error != NULL) {
        memset(out_error, 0, sizeof(*out_error));
    }
    return macro_compile(source, source_length, options, out_plan, out_error);
}

static void expect_success_length(const char *source,
                                  size_t source_length,
                                  const macro_compile_options_t *options,
                                  size_t expected_actions,
                                  uint32_t expected_duration)
{
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         compile_explicit(source,
                                          source_length,
                                          options,
                                          &plan,
                                          &error));
    TEST_CHECK_EQ_U64(expected_actions, plan.action_count);
    TEST_CHECK_EQ_U64(expected_duration, plan.estimated_duration_ms);
    if (expected_actions == 0U) {
        TEST_CHECK(plan.actions == NULL);
    } else {
        TEST_CHECK(plan.actions != NULL);
    }
    macro_plan_free(&plan);
    TEST_CHECK(plan.actions == NULL);
    TEST_CHECK_EQ_U64(0U, plan.action_count);
    TEST_CHECK_EQ_U64(0U, plan.estimated_duration_ms);
}

static void expect_success(const char *source, size_t expected_actions)
{
    expect_success_length(source,
                          strlen(source),
                          NULL,
                          expected_actions,
                          (uint32_t)(expected_actions *
                                     (APP_KEY_PRESS_DEFAULT_MS +
                                      APP_INTER_KEY_DEFAULT_MS)));
}

static macro_parse_error_t expect_failure_length(const char *source,
                                                 size_t source_length,
                                                 const macro_compile_options_t *options,
                                                 app_error_code_t expected)
{
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    TEST_CHECK_APP_ERROR(expected,
                         compile_explicit(source,
                                          source_length,
                                          options,
                                          &plan,
                                          &error));
    TEST_CHECK(plan.actions == NULL);
    TEST_CHECK_EQ_U64(0U, plan.action_count);
    TEST_CHECK_EQ_U64(0U, plan.estimated_duration_ms);
    return error;
}

static void expect_failure(const char *source, app_error_code_t expected)
{
    const macro_parse_error_t error =
        expect_failure_length(source, strlen(source), NULL, expected);
    TEST_CHECK(error.line >= 1U);
    TEST_CHECK(error.column >= 1U);
}

static void test_uuid_and_revision_validation(void)
{
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, app_uuid_parse("123e4567-e89b-42d3-a456-426614174000", &uuid));
    TEST_CHECK(app_uuid_is_valid_string(uuid.value));
    TEST_CHECK(!app_uuid_is_valid_string("123e4567-e89b-12d3-a456-426614174000"));
    TEST_CHECK(!app_uuid_is_valid_string("../bad"));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_revision(0U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_model_validate_revision(1U));
}

static void test_null_empty_and_output_arguments(void)
{
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         macro_compile(NULL, 0U, NULL, &plan, &error));
    TEST_CHECK(plan.actions == NULL);
    TEST_CHECK_EQ_U64(0U, plan.action_count);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_compile(NULL, 1U, NULL, &plan, &error));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_compile("a", 1U, NULL, NULL, &error));

    plan.action_count = 99U;
    plan.estimated_duration_ms = 77U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         macro_compile("", 0U, NULL, &plan, &error));
    TEST_CHECK(plan.actions == NULL);
    TEST_CHECK_EQ_U64(0U, plan.action_count);
    TEST_CHECK_EQ_U64(0U, plan.estimated_duration_ms);
}

static void test_source_and_action_limits(void)
{
    char *source = malloc(APP_MACRO_SOURCE_MAX_BYTES + 1U);
    TEST_CHECK(source != NULL);
    memset(source, 'a', APP_MACRO_SOURCE_MAX_BYTES + 1U);

    expect_success_length(source,
                          APP_MACRO_SOURCE_MAX_BYTES,
                          NULL,
                          APP_COMPILED_ACTION_MAX,
                          (uint32_t)(APP_COMPILED_ACTION_MAX *
                                     (APP_KEY_PRESS_DEFAULT_MS +
                                      APP_INTER_KEY_DEFAULT_MS)));

    (void)expect_failure_length(source,
                                APP_MACRO_SOURCE_MAX_BYTES + 1U,
                                NULL,
                                APP_ERROR_INVALID_ARGUMENT);
    free(source);
}

static void test_timing_boundaries(void)
{
    const macro_compile_options_t defaults = {
        .key_press_ms = APP_KEY_PRESS_DEFAULT_MS,
        .inter_key_ms = APP_INTER_KEY_DEFAULT_MS,
    };
    expect_success_length("a", 1U, &defaults, 1U,
                          APP_KEY_PRESS_DEFAULT_MS + APP_INTER_KEY_DEFAULT_MS);

    const macro_compile_options_t custom = {
        .key_press_ms = 3U,
        .inter_key_ms = 4U,
    };
    expect_success_length("ab", 2U, &custom, 2U, 14U);

    const macro_compile_options_t zero_key = {
        .key_press_ms = 0U,
        .inter_key_ms = 1U,
    };
    (void)expect_failure_length("a", 1U, &zero_key, APP_ERROR_INVALID_ARGUMENT);

    const macro_compile_options_t excessive_key = {
        .key_press_ms = APP_DELAY_MAX_MS + 1U,
        .inter_key_ms = 0U,
    };
    (void)expect_failure_length("a", 1U, &excessive_key, APP_ERROR_INVALID_ARGUMENT);

    const macro_compile_options_t excessive_inter_key = {
        .key_press_ms = 1U,
        .inter_key_ms = APP_DELAY_MAX_MS + 1U,
    };
    (void)expect_failure_length("a", 1U, &excessive_inter_key,
                                APP_ERROR_INVALID_ARGUMENT);

    const macro_compile_options_t maximum = {
        .key_press_ms = APP_DELAY_MAX_MS,
        .inter_key_ms = APP_DELAY_MAX_MS,
    };
    expect_success_length("abcdefghijklmno", 15U, &maximum, 15U,
                          APP_ESTIMATED_DURATION_MAX_MS);
    const macro_parse_error_t error =
        expect_failure_length("abcdefghijklmnop", 16U, &maximum,
                              APP_ERROR_MACRO_LIMIT);
    TEST_CHECK_EQ_U64(15U, error.byte_offset);
    TEST_CHECK_EQ_STRING("estimated duration limit exceeded", error.message);
}

static void test_delay_boundaries(void)
{
    expect_success_length("{DELAY:1}", strlen("{DELAY:1}"), NULL, 1U, 1U);
    expect_success_length("{DELAY:10000}", strlen("{DELAY:10000}"), NULL, 1U,
                          APP_DELAY_MAX_MS);
    expect_failure("{DELAY:0}", APP_ERROR_MACRO_LIMIT);
    expect_failure("{DELAY:10001}", APP_ERROR_MACRO_LIMIT);
    expect_failure("{DELAY:42949672960}", APP_ERROR_MACRO_LIMIT);
    expect_failure("{DELAY:-1}", APP_ERROR_MACRO_SYNTAX);
}

static void test_named_keys_and_modifiers(void)
{
    static const char *const named_keys[] = {
        "ENTER", "TAB", "ESC", "BACKSPACE", "DELETE", "INSERT", "HOME",
        "END", "PAGEUP", "PAGEDOWN", "UP", "DOWN", "LEFT", "RIGHT",
        "SPACE", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
        "F9", "F10", "F11", "F12",
    };
    for (size_t index = 0U;
         index < (sizeof(named_keys) / sizeof(named_keys[0]));
         ++index) {
        char source[32U];
        const int written =
            snprintf(source, sizeof(source), "{%s}", named_keys[index]);
        TEST_CHECK(written > 0 && (size_t)written < sizeof(source));
        expect_success(source, 1U);
    }

    static const struct {
        const char *source;
        uint8_t modifiers;
    } modifier_cases[] = {
        {"{CTRL+A}", 0x01U},
        {"{SHIFT+A}", 0x02U},
        {"{ALT+A}", 0x04U},
        {"{GUI+A}", 0x08U},
        {"{CTRL+SHIFT+ALT+GUI+A}", 0x0fU},
        {"{GUI+ALT+SHIFT+CTRL+F1}", 0x0fU},
    };
    for (size_t index = 0U;
         index < (sizeof(modifier_cases) / sizeof(modifier_cases[0]));
         ++index) {
        macro_plan_t plan = {0};
        TEST_CHECK_APP_ERROR(
            APP_ERROR_NONE,
            compile_explicit(modifier_cases[index].source,
                             strlen(modifier_cases[index].source),
                             NULL,
                             &plan,
                             NULL));
        TEST_CHECK_EQ_U64(1U, plan.action_count);
        TEST_CHECK_EQ_U64(MACRO_ACTION_CHORD, plan.actions[0].type);
        TEST_CHECK_EQ_U64(modifier_cases[index].modifiers,
                          plan.actions[0].modifiers);
        macro_plan_free(&plan);
    }

    expect_failure("{CTRL+CTRL+A}", APP_ERROR_MACRO_SYNTAX);
    expect_failure("{A+CTRL}", APP_ERROR_MACRO_SYNTAX);
    expect_failure("{CTRL+SHIFT}", APP_ERROR_MACRO_SYNTAX);
    expect_failure("{CTRL+A+B}", APP_ERROR_MACRO_SYNTAX);
    expect_failure("{CTRL+}", APP_ERROR_MACRO_SYNTAX);
}

static void test_case_whitespace_and_line_endings(void)
{
    expect_failure("{enter}", APP_ERROR_MACRO_SYNTAX);
    expect_failure("{ ENTER}", APP_ERROR_MACRO_SYNTAX);
    expect_failure("{ENTER }", APP_ERROR_MACRO_SYNTAX);
    expect_success("A B", 3U);

    macro_plan_t plan = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         compile_explicit("A\r\nB", 4U, NULL, &plan, NULL));
    TEST_CHECK_EQ_U64(3U, plan.action_count);
    TEST_CHECK_EQ_U64(0x28U, plan.actions[1].usage);
    macro_plan_free(&plan);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         compile_explicit("A\nB", 3U, NULL, &plan, NULL));
    TEST_CHECK_EQ_U64(3U, plan.action_count);
    TEST_CHECK_EQ_U64(0x28U, plan.actions[1].usage);
    macro_plan_free(&plan);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         compile_explicit("\t", 1U, NULL, &plan, NULL));
    TEST_CHECK_EQ_U64(1U, plan.action_count);
    TEST_CHECK_EQ_U64(0x2bU, plan.actions[0].usage);
    macro_plan_free(&plan);

    const macro_parse_error_t error =
        expect_failure_length("A\rB", 3U, NULL, APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_U64(1U, error.byte_offset);
    TEST_CHECK_EQ_U64(1U, error.line);
    TEST_CHECK_EQ_U64(2U, error.column);
}

static void test_error_locations_and_directive_boundaries(void)
{
    const macro_parse_error_t multiline =
        expect_failure_length("A\n{BAD}", 7U, NULL, APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_U64(2U, multiline.byte_offset);
    TEST_CHECK_EQ_U64(2U, multiline.line);
    TEST_CHECK_EQ_U64(1U, multiline.column);
    TEST_CHECK_EQ_STRING("unknown key directive", multiline.message);

    char directive_63[66U];
    directive_63[0] = '{';
    memset(directive_63 + 1U, 'A', 63U);
    directive_63[64] = '}';
    directive_63[65] = '\0';
    const macro_parse_error_t error_63 =
        expect_failure_length(directive_63, 65U, NULL, APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_STRING("unknown key directive", error_63.message);

    char directive_64[67U];
    directive_64[0] = '{';
    memset(directive_64 + 1U, 'A', 64U);
    directive_64[65] = '}';
    directive_64[66] = '\0';
    const macro_parse_error_t error_64 =
        expect_failure_length(directive_64, 66U, NULL, APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_STRING("invalid directive", error_64.message);
}

static void test_braces_and_character_policy(void)
{
    expect_success("{{literal braces}}", 16U);
    expect_success("{{{ENTER}}}", 3U);
    expect_failure("{", APP_ERROR_MACRO_SYNTAX);
    expect_failure("}", APP_ERROR_MACRO_SYNTAX);

    static const char valid_non_ascii_utf8[] = "\xc3\xa9";
    const macro_parse_error_t non_ascii =
        expect_failure_length(valid_non_ascii_utf8,
                              sizeof(valid_non_ascii_utf8) - 1U,
                              NULL,
                              APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_U64(0U, non_ascii.byte_offset);

    static const char invalid_utf8[] = {(char)0xc3, 'x'};
    const macro_parse_error_t invalid =
        expect_failure_length(invalid_utf8,
                              sizeof(invalid_utf8),
                              NULL,
                              APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_U64(0U, invalid.byte_offset);

    static const char embedded_nul[] = {'a', '\0', 'b'};
    const macro_parse_error_t nul =
        expect_failure_length(embedded_nul,
                              sizeof(embedded_nul),
                              NULL,
                              APP_ERROR_MACRO_SYNTAX);
    TEST_CHECK_EQ_U64(1U, nul.byte_offset);
}

static void test_output_plan_reuse_contract(void)
{
    macro_plan_t plan = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         macro_compile("first", 5U, NULL, &plan, NULL));
    TEST_CHECK_EQ_U64(5U, plan.action_count);
    macro_plan_free(&plan);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         macro_compile("{ENTER}", 7U, NULL, &plan, NULL));
    TEST_CHECK_EQ_U64(1U, plan.action_count);
    macro_plan_free(&plan);
    macro_plan_free(&plan);
}

static void test_fuzz_corpus(void)
{
    unsigned int state = 0x13579bdfU;
    char source[128U];
    for (size_t iteration = 0U; iteration < 10000U; ++iteration) {
        state = state * 1664525U + 1013904223U;
        const size_t length = (size_t)(state % (unsigned int)sizeof(source));
        for (size_t index = 0U; index < length; ++index) {
            state = state * 1664525U + 1013904223U;
            source[index] = (char)(state & 0xffU);
        }
        macro_plan_t plan = {0};
        macro_parse_error_t error = {0};
        const app_error_code_t result =
            macro_compile(source, length, NULL, &plan, &error);
        if (result == APP_ERROR_NONE) {
            TEST_CHECK(plan.action_count <= APP_COMPILED_ACTION_MAX);
            macro_plan_free(&plan);
        } else {
            TEST_CHECK(plan.actions == NULL);
            TEST_CHECK_EQ_U64(0U, plan.action_count);
        }
    }
}

static void test_printable_ascii(void)
{
    for (int value = 0x20; value <= 0x7e; ++value) {
        macro_hid_key_t key = {0U, 0U};
        TEST_CHECK(macro_keymap_us_printable((char)value, &key));
        TEST_CHECK(key.usage != 0U);
    }
    TEST_CHECK(!macro_keymap_us_printable('a', NULL));
}

int main(void)
{
    test_uuid_and_revision_validation();
    test_null_empty_and_output_arguments();
    test_source_and_action_limits();
    test_timing_boundaries();
    test_delay_boundaries();
    test_named_keys_and_modifiers();
    test_case_whitespace_and_line_endings();
    test_error_locations_and_directive_boundaries();
    test_braces_and_character_policy();
    test_output_plan_reuse_contract();
    test_printable_ascii();
    test_fuzz_corpus();
    expect_success("Hello, world!{ENTER}", 14U);
    expect_success_length("{CTRL+ALT+T}{DELAY:500}cd /tmp{ENTER}",
                          strlen("{CTRL+ALT+T}{DELAY:500}cd /tmp{ENTER}"),
                          NULL,
                          10U,
                          707U);
    puts("macro parser tests passed");
    return EXIT_SUCCESS;
}
