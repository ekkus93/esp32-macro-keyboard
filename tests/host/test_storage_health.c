#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "storage_health.h"
#include "test_assert.h"

static void test_default_is_healthy(void) {
    storage_health_reset();
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.cleanup_error);
    TEST_CHECK(!health.cleanup_incomplete);
}

static void test_primary_failure_reports_failed(void) {
    storage_health_reset();
    storage_health_record_primary(APP_ERROR_STORAGE_UNAVAILABLE);
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, health.primary_error);
}

static void test_primary_failure_first_wins(void) {
    storage_health_reset();
    storage_health_record_primary(APP_ERROR_STORAGE_UNAVAILABLE);
    storage_health_record_primary(APP_ERROR_STORAGE_CORRUPT);
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, health.primary_error);
}

static void test_primary_none_is_noop(void) {
    storage_health_reset();
    storage_health_record_primary(APP_ERROR_NONE);
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
}

static void test_cleanup_incomplete_alone_reports_failed(void) {
    /* Mirrors app_core_health: incomplete cleanup must never read as healthy,
     * even with no error code and no primary failure. */
    storage_health_reset();
    storage_health_record_cleanup(APP_ERROR_NONE, true);
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_cleanup_error_first_wins_and_marks_incomplete(void) {
    storage_health_reset();
    storage_health_record_cleanup(APP_ERROR_IO, true);
    storage_health_record_cleanup(APP_ERROR_STORAGE_CORRUPT, true);
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, health.cleanup_error);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_cleanup_none_and_complete_is_noop(void) {
    storage_health_reset();
    storage_health_record_cleanup(APP_ERROR_NONE, false);
    const storage_health_t health = storage_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
    TEST_CHECK(!health.cleanup_incomplete);
}

int main(void) {
    test_default_is_healthy();
    test_primary_failure_reports_failed();
    test_primary_failure_first_wins();
    test_primary_none_is_noop();
    test_cleanup_incomplete_alone_reports_failed();
    test_cleanup_error_first_wins_and_marks_incomplete();
    test_cleanup_none_and_complete_is_noop();
    puts("storage health tests passed");
    return EXIT_SUCCESS;
}
