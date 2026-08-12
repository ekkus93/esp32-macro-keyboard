#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "storage_health.h"
#include "test_assert.h"

#define STRESS_ITERATIONS 50000U
#define STRESS_READER_COUNT 4U

typedef struct {
    atomic_bool start;
    atomic_bool failed;
} health_stress_context_t;

static void await_stress_start(const health_stress_context_t *context) {
    while (!atomic_load_explicit(&context->start, memory_order_acquire)) {
    }
}

static void *health_writer_thread(void *argument) {
    health_stress_context_t *context = argument;
    await_stress_start(context);
    for (size_t index = 0U; index < (size_t)STRESS_ITERATIONS; ++index) {
        storage_health_reset();
        storage_health_record_cleanup(APP_ERROR_IO, true);
    }
    return NULL;
}

static void *health_reader_thread(void *argument) {
    health_stress_context_t *context = argument;
    await_stress_start(context);
    for (size_t index = 0U; index < (size_t)STRESS_ITERATIONS; ++index) {
        const storage_health_t health = storage_health_snapshot();
        const bool reset_state =
            health.state == SUBSYSTEM_HEALTH_HEALTHY && health.primary_error == APP_ERROR_NONE &&
            health.cleanup_error == APP_ERROR_NONE && !health.cleanup_incomplete;
        const bool cleanup_state =
            health.state == SUBSYSTEM_HEALTH_FAILED && health.primary_error == APP_ERROR_NONE &&
            health.cleanup_error == APP_ERROR_IO && health.cleanup_incomplete;
        if (!reset_state && !cleanup_state) {
            atomic_store_explicit(&context->failed, true, memory_order_release);
            break;
        }
    }
    return NULL;
}

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

static void test_concurrent_snapshot_never_observes_torn_cleanup_state(void) {
    health_stress_context_t context;
    atomic_init(&context.start, false);
    atomic_init(&context.failed, false);
    storage_health_reset();

    pthread_t writer;
    pthread_t readers[STRESS_READER_COUNT];
    TEST_CHECK_EQ_INT(0, pthread_create(&writer, NULL, health_writer_thread, &context));
    for (size_t index = 0U; index < (size_t)STRESS_READER_COUNT; ++index) {
        TEST_CHECK_EQ_INT(0, pthread_create(&readers[index], NULL, health_reader_thread, &context));
    }

    atomic_store_explicit(&context.start, true, memory_order_release);
    TEST_CHECK_EQ_INT(0, pthread_join(writer, NULL));
    for (size_t index = 0U; index < (size_t)STRESS_READER_COUNT; ++index) {
        TEST_CHECK_EQ_INT(0, pthread_join(readers[index], NULL));
    }
    TEST_CHECK(!atomic_load_explicit(&context.failed, memory_order_acquire));
}

int main(void) {
    test_default_is_healthy();
    test_primary_failure_reports_failed();
    test_primary_failure_first_wins();
    test_primary_none_is_noop();
    test_cleanup_incomplete_alone_reports_failed();
    test_cleanup_error_first_wins_and_marks_incomplete();
    test_cleanup_none_and_complete_is_noop();
    test_concurrent_snapshot_never_observes_torn_cleanup_state();
    puts("storage health tests passed");
    return EXIT_SUCCESS;
}
