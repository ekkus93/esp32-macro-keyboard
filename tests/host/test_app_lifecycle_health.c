#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "app_lifecycle_health.h"
#include "test_assert.h"

static void test_default_is_healthy(void) {
    app_lifecycle_health_reset();
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.cleanup_error);
    TEST_CHECK(!health.cleanup_incomplete);
}

static void test_stage_failure_reports_failed(void) {
    app_lifecycle_health_reset();
    app_lifecycle_health_record_stage_failure(APP_ERROR_STORAGE_UNAVAILABLE);
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, health.primary_error);
}

static void test_stage_failure_first_wins(void) {
    app_lifecycle_health_reset();
    app_lifecycle_health_record_stage_failure(APP_ERROR_STORAGE_UNAVAILABLE);
    app_lifecycle_health_record_stage_failure(APP_ERROR_IO);
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, health.primary_error);
}

static void test_degraded_without_error_reports_degraded(void) {
    app_lifecycle_health_reset();
    app_lifecycle_health_record_degraded();
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_DEGRADED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.primary_error);
}

static void test_cleanup_incomplete_alone_reports_failed_not_healthy(void) {
    /* The defect this exists to prevent: cleanup trouble must never read back
     * as healthy, even with no primary_error recorded. */
    app_lifecycle_health_reset();
    app_lifecycle_health_record_cleanup_failed(APP_ERROR_NONE, true);
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_cleanup_error_first_wins_and_marks_incomplete(void) {
    app_lifecycle_health_reset();
    app_lifecycle_health_record_cleanup_failed(APP_ERROR_STORAGE_CORRUPT, true);
    app_lifecycle_health_record_cleanup_failed(APP_ERROR_IO, true);
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, health.cleanup_error);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_degraded_then_failed_reports_failed(void) {
    /* A cleanup/primary failure is more severe than a degraded continuation
     * and must take priority in the derived state. */
    app_lifecycle_health_reset();
    app_lifecycle_health_record_degraded();
    app_lifecycle_health_record_stage_failure(APP_ERROR_TIMEOUT);
    const app_lifecycle_health_t health = app_lifecycle_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
}

int main(void) {
    test_default_is_healthy();
    test_stage_failure_reports_failed();
    test_stage_failure_first_wins();
    test_degraded_without_error_reports_degraded();
    test_cleanup_incomplete_alone_reports_failed_not_healthy();
    test_cleanup_error_first_wins_and_marks_incomplete();
    test_degraded_then_failed_reports_failed();
    puts("app core health tests passed");
    return EXIT_SUCCESS;
}
