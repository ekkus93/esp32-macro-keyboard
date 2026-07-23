#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_uuid.h"
#include "macro_keymap_us.h"
#include "macro_model.h"
#include "macro_parser.h"

#include "test_assert.h"

static void expect_success(const char *source, size_t expected_actions)
{
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result = macro_compile(source, strlen(source), NULL, &plan, &error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result);
    TEST_CHECK(plan.action_count == expected_actions);
    macro_plan_free(&plan);
}

static void expect_failure(const char *source)
{
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result = macro_compile(source, strlen(source), NULL, &plan, &error);
    TEST_CHECK(result != APP_ERROR_NONE);
    TEST_CHECK(plan.actions == NULL);
    TEST_CHECK(plan.action_count == 0U);
    TEST_CHECK(error.line >= 1U);
    TEST_CHECK(error.column >= 1U);
}

static void test_uuid_validation(void)
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
        const app_error_code_t result = macro_compile(source, length, NULL, &plan, &error);
        if (result == APP_ERROR_NONE) {
            TEST_CHECK(plan.action_count <= APP_COMPILED_ACTION_MAX);
            macro_plan_free(&plan);
        } else {
            TEST_CHECK(plan.actions == NULL);
            TEST_CHECK(plan.action_count == 0U);
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
}

int main(void)
{
    test_uuid_validation();
    test_printable_ascii();
    test_fuzz_corpus();
    expect_success("Hello, world!{ENTER}", 14U);
    expect_success("{{literal braces}}", 16U);
    expect_success("{CTRL+ALT+T}{DELAY:500}cd /tmp{ENTER}", 10U);
    expect_failure("{WAIT:500}");
    expect_failure("{CTRL+}");
    expect_failure("{CTRL+SHIFT}");
    expect_failure("{CTRL+A+B}");
    expect_failure("{DELAY:0}");
    expect_failure("{DELAY:10001}");
    expect_failure("{");
    expect_failure("}");
    expect_failure("\xc3\xa9");
    puts("macro parser tests passed");
    return EXIT_SUCCESS;
}
