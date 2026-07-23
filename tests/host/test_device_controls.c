#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "device_controls_logic.h"
#include "test_assert.h"

static void test_active_levels(void)
{
    TEST_CHECK(device_controls_level_is_pressed(0, 0));
    TEST_CHECK(!device_controls_level_is_pressed(1, 0));
    TEST_CHECK(device_controls_level_is_pressed(1, 1));
    TEST_CHECK(!device_controls_level_is_pressed(0, 1));
    TEST_CHECK(!device_controls_level_is_pressed(0, -1));
    TEST_CHECK(!device_controls_level_is_pressed(1, 2));
}

static void test_debounce_press_hold_release_and_repress(void)
{
    device_controls_debounce_t button = {0};
    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK_EQ_U64(1U, button.candidate_count);
    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK_EQ_U64(2U, button.candidate_count);
    TEST_CHECK(device_controls_debounce_update(&button, true));
    TEST_CHECK(button.stable);
    TEST_CHECK_EQ_U64(DEVICE_CONTROLS_DEBOUNCE_SAMPLES, button.candidate_count);

    for (size_t index = 0U; index < 300U; ++index) {
        TEST_CHECK(!device_controls_debounce_update(&button, true));
    }
    TEST_CHECK_EQ_U64(DEVICE_CONTROLS_DEBOUNCE_SAMPLES, button.candidate_count);

    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK(!button.stable);
    TEST_CHECK_EQ_U64(DEVICE_CONTROLS_DEBOUNCE_SAMPLES, button.candidate_count);

    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK(device_controls_debounce_update(&button, true));
    TEST_CHECK(button.stable);
}

static void test_debounce_bounce_and_candidate_reset(void)
{
    device_controls_debounce_t button = {0};
    for (size_t index = 0U; index < 100U; ++index) {
        const bool sample = (index % 2U) == 0U;
        TEST_CHECK(!device_controls_debounce_update(&button, sample));
        TEST_CHECK(!button.stable);
        TEST_CHECK_EQ_U64(1U, button.candidate_count);
    }

    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK_EQ_U64(2U, button.candidate_count);
    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK_EQ_U64(1U, button.candidate_count);
    TEST_CHECK(!button.candidate);
    TEST_CHECK(!device_controls_debounce_update(NULL, true));
}

static void test_ready_and_unknown_indicators(void)
{
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_READY, 0U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_READY, UINT32_MAX));
    TEST_CHECK(!device_controls_indicator_on((device_indicator_state_t)99, 0U));
}

static void test_booting_indicator(void)
{
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 0U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 249U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 250U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 999U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 1000U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 1249U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_BOOTING, 1250U));
}

static void test_executing_indicator(void)
{
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_EXECUTING, 0U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_EXECUTING, 99U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_EXECUTING, 100U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_EXECUTING, 199U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_EXECUTING, 200U));
}

static void test_degraded_indicator(void)
{
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 0U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 249U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 250U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 499U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 500U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 749U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 750U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 1999U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_DEGRADED, 2000U));
}

static void test_fatal_indicator(void)
{
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_FATAL, 0U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_FATAL, 249U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_FATAL, 250U));
    TEST_CHECK(!device_controls_indicator_on(DEVICE_INDICATOR_FATAL, 499U));
    TEST_CHECK(device_controls_indicator_on(DEVICE_INDICATOR_FATAL, 500U));
    TEST_CHECK_EQ_INT((UINT32_MAX % 500U) < 250U,
                      device_controls_indicator_on(DEVICE_INDICATOR_FATAL,
                                                   UINT32_MAX));
}

int main(void)
{
    test_active_levels();
    test_debounce_press_hold_release_and_repress();
    test_debounce_bounce_and_candidate_reset();
    test_ready_and_unknown_indicators();
    test_booting_indicator();
    test_executing_indicator();
    test_degraded_indicator();
    test_fatal_indicator();
    puts("device controls tests passed");
    return EXIT_SUCCESS;
}
