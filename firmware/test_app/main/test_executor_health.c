#include "app_error.h"
#include "executor_health.h"
#include "subsystem_health.h"
#include "unity.h"

TEST_CASE("executor shutdown latch fails closed until worker stop is confirmed",
          "[device][executor][hardening]") {
    executor_shutdown_state_t state = {0};
    executor_shutdown_state_reset(&state);
    TEST_ASSERT_TRUE(executor_shutdown_state_accepts_submissions(&state));
    TEST_ASSERT_FALSE(executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_begin(&state);
    TEST_ASSERT_FALSE(executor_shutdown_state_accepts_submissions(&state));
    TEST_ASSERT_FALSE(executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_complete(&state, false);
    TEST_ASSERT_FALSE(executor_shutdown_state_accepts_submissions(&state));
    TEST_ASSERT_TRUE(executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_begin(&state);
    TEST_ASSERT_FALSE(executor_shutdown_state_accepts_submissions(&state));
    TEST_ASSERT_TRUE(executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_complete(&state, true);
    TEST_ASSERT_TRUE(executor_shutdown_state_accepts_submissions(&state));
    TEST_ASSERT_FALSE(executor_shutdown_state_fault_latched(&state));
}

TEST_CASE("executor cleanup health keeps the first release failure visible",
          "[device][executor][hardening]") {
    executor_health_reset();
    executor_health_record_cleanup(APP_ERROR_TIMEOUT, true);
    executor_health_record_cleanup(APP_ERROR_INTERNAL, false);

    const executor_health_t health = executor_health_snapshot();
    TEST_ASSERT_EQUAL(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_ASSERT_EQUAL(APP_ERROR_TIMEOUT, health.cleanup_error);
    TEST_ASSERT_TRUE(health.cleanup_incomplete);

    executor_health_reset();
    const executor_health_t reset = executor_health_snapshot();
    TEST_ASSERT_EQUAL(SUBSYSTEM_HEALTH_HEALTHY, reset.state);
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, reset.primary_error);
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, reset.cleanup_error);
    TEST_ASSERT_FALSE(reset.cleanup_incomplete);
}
