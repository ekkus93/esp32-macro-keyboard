#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "auth_health.h"
#include "test_assert.h"

static void test_default_is_healthy(void) {
    auth_health_reset();
    const auth_health_t health = auth_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.cleanup_error);
    TEST_CHECK(!health.cleanup_incomplete);
}

static void test_primary_failure_reports_failed(void) {
    auth_health_reset();
    auth_health_record_primary(APP_ERROR_INTERNAL);
    const auth_health_t health = auth_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, health.primary_error);
}

static void test_primary_failure_first_wins(void) {
    auth_health_reset();
    auth_health_record_primary(APP_ERROR_INTERNAL);
    auth_health_record_primary(APP_ERROR_CONFLICT);
    const auth_health_t health = auth_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, health.primary_error);
}

static void test_cleanup_incomplete_alone_reports_failed(void) {
    auth_health_reset();
    auth_health_record_cleanup(APP_ERROR_NONE, true);
    const auth_health_t health = auth_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_cleanup_none_and_complete_is_noop(void) {
    auth_health_reset();
    auth_health_record_cleanup(APP_ERROR_NONE, false);
    const auth_health_t health = auth_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
}

int main(void) {
    test_default_is_healthy();
    test_primary_failure_reports_failed();
    test_primary_failure_first_wins();
    test_cleanup_incomplete_alone_reports_failed();
    test_cleanup_none_and_complete_is_noop();
    puts("auth health tests passed");
    return EXIT_SUCCESS;
}
