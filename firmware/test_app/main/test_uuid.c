#include <string.h>

#include "app_uuid.h"
#include "unity.h"

TEST_CASE("hardware RNG generates distinct UUID v4 values", "[device][uuid]")
{
    app_uuid_t first = {0};
    app_uuid_t second = {0};
    app_uuid_t parsed = {0};

    TEST_ASSERT_EQUAL(APP_ERROR_NONE, app_uuid_generate(&first));
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, app_uuid_generate(&second));
    TEST_ASSERT_TRUE(app_uuid_is_valid_string(first.value));
    TEST_ASSERT_TRUE(app_uuid_is_valid_string(second.value));
    TEST_ASSERT_FALSE(app_uuid_equal(&first, &second));
    TEST_ASSERT_EQUAL_CHAR('4', first.value[14]);
    TEST_ASSERT_NOT_NULL(strchr("89ab", first.value[19]));

    TEST_ASSERT_EQUAL(APP_ERROR_NONE, app_uuid_parse(first.value, &parsed));
    TEST_ASSERT_TRUE(app_uuid_equal(&first, &parsed));
}

TEST_CASE("UUID parser rejects malformed and non-v4 values", "[device][uuid]")
{
    app_uuid_t parsed = {0};

    TEST_ASSERT_EQUAL(APP_ERROR_INVALID_ARGUMENT, app_uuid_parse("../bad", &parsed));
    TEST_ASSERT_EQUAL(APP_ERROR_INVALID_ARGUMENT,
                      app_uuid_parse("123e4567-e89b-12d3-a456-426614174000", &parsed));
    TEST_ASSERT_FALSE(app_uuid_is_valid_string("123e4567-e89b-42d3-a456-42661417400"));
}
