#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "macro_parser.h"
#include "unity.h"

static void assert_compile_failure(const char *source)
{
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};

    TEST_ASSERT_NOT_EQUAL(APP_ERROR_NONE,
                          macro_compile(source, strlen(source), NULL, &plan, &error));
    TEST_ASSERT_NULL(plan.actions);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)plan.action_count);
    TEST_ASSERT_TRUE(error.line >= 1U);
    TEST_ASSERT_TRUE(error.column >= 1U);
}

TEST_CASE("macro compiler builds a complete immutable plan", "[device][macro_parser]")
{
    static const char source[] = "Hello{{world}}{ENTER}{CTRL+ALT+T}{DELAY:25}";
    const macro_compile_options_t options = {
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};

    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      macro_compile(source, sizeof(source) - 1U, &options, &plan, &error));
    TEST_ASSERT_NOT_NULL(plan.actions);
    TEST_ASSERT_EQUAL_UINT32(15U, (uint32_t)plan.action_count);
    TEST_ASSERT_EQUAL(MACRO_ACTION_DELAY, plan.actions[14U].type);
    TEST_ASSERT_EQUAL_UINT32(25U, plan.actions[14U].delay_ms);
    TEST_ASSERT_TRUE(plan.estimated_duration_ms > 25U);

    macro_plan_free(&plan);
    TEST_ASSERT_NULL(plan.actions);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)plan.action_count);
}

TEST_CASE("macro compiler rejects malformed input without a partial plan",
          "[device][macro_parser]")
{
    assert_compile_failure("{WAIT:500}");
    assert_compile_failure("{CTRL+SHIFT}");
    assert_compile_failure("{CTRL+A+B}");
    assert_compile_failure("{DELAY:0}");
    assert_compile_failure("{DELAY:10001}");
    assert_compile_failure("\xc3\xa9");
}

TEST_CASE("macro compiler accepts the maximum directive delay", "[device][macro_parser]")
{
    static const char source[] = "{DELAY:10000}";
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};

    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      macro_compile(source, sizeof(source) - 1U, NULL, &plan, &error));
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)plan.action_count);
    TEST_ASSERT_EQUAL(MACRO_ACTION_DELAY, plan.actions[0U].type);
    TEST_ASSERT_EQUAL_UINT32(10000U, plan.actions[0U].delay_ms);
    macro_plan_free(&plan);
}
