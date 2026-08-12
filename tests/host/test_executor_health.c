#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "executor_health.h"
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
        executor_health_reset();
        executor_health_record_cleanup(APP_ERROR_TIMEOUT, true);
    }
    return NULL;
}

static void *health_reader_thread(void *argument) {
    health_stress_context_t *context = argument;
    await_stress_start(context);
    for (size_t index = 0U; index < (size_t)STRESS_ITERATIONS; ++index) {
        const executor_health_t health = executor_health_snapshot();
        const bool reset_state =
            health.state == SUBSYSTEM_HEALTH_HEALTHY && health.primary_error == APP_ERROR_NONE &&
            health.cleanup_error == APP_ERROR_NONE && !health.cleanup_incomplete;
        const bool cleanup_state =
            health.state == SUBSYSTEM_HEALTH_FAILED && health.primary_error == APP_ERROR_NONE &&
            health.cleanup_error == APP_ERROR_TIMEOUT && health.cleanup_incomplete;
        if (!reset_state && !cleanup_state) {
            atomic_store_explicit(&context->failed, true, memory_order_release);
            break;
        }
    }
    return NULL;
}

static void test_default_is_healthy(void) {
    executor_health_reset();
    const executor_health_t health = executor_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, health.cleanup_error);
    TEST_CHECK(!health.cleanup_incomplete);
}

static void test_primary_failure_reports_failed(void) {
    executor_health_reset();
    executor_health_record_primary(APP_ERROR_INTERNAL);
    const executor_health_t health = executor_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, health.primary_error);
}

static void test_primary_failure_first_wins(void) {
    executor_health_reset();
    executor_health_record_primary(APP_ERROR_INTERNAL);
    executor_health_record_primary(APP_ERROR_CONFLICT);
    const executor_health_t health = executor_health_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, health.primary_error);
}

static void test_cleanup_incomplete_alone_reports_failed(void) {
    executor_health_reset();
    executor_health_record_cleanup(APP_ERROR_NONE, true);
    const executor_health_t health = executor_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_cleanup_none_and_complete_is_noop(void) {
    executor_health_reset();
    executor_health_record_cleanup(APP_ERROR_NONE, false);
    const executor_health_t health = executor_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_HEALTHY, health.state);
}

static void test_unconfirmed_stop_latches_fail_closed_until_confirmed(void) {
    executor_shutdown_state_t state = {0};
    executor_shutdown_state_reset(&state);
    TEST_CHECK(executor_shutdown_state_accepts_submissions(&state));
    TEST_CHECK(!executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_begin(&state);
    TEST_CHECK(!executor_shutdown_state_accepts_submissions(&state));
    TEST_CHECK(!executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_complete(&state, false);
    TEST_CHECK(!executor_shutdown_state_accepts_submissions(&state));
    TEST_CHECK(executor_shutdown_state_fault_latched(&state));

    /* Retrying deinit does not clear the fault merely because another attempt
     * began. The latch clears only after the worker actually confirms stop. */
    executor_shutdown_state_begin(&state);
    TEST_CHECK(!executor_shutdown_state_accepts_submissions(&state));
    TEST_CHECK(executor_shutdown_state_fault_latched(&state));

    executor_shutdown_state_complete(&state, true);
    TEST_CHECK(executor_shutdown_state_accepts_submissions(&state));
    TEST_CHECK(!executor_shutdown_state_fault_latched(&state));
}

static void test_unconfirmed_stop_is_visible_in_health(void) {
    executor_health_reset();
    executor_health_record_cleanup(APP_ERROR_TIMEOUT, true);
    const executor_health_t health = executor_health_snapshot();
    TEST_CHECK_EQ_INT(SUBSYSTEM_HEALTH_FAILED, health.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, health.cleanup_error);
    TEST_CHECK(health.cleanup_incomplete);
}

static void test_concurrent_snapshot_never_observes_torn_cleanup_state(void) {
    health_stress_context_t context;
    atomic_init(&context.start, false);
    atomic_init(&context.failed, false);
    executor_health_reset();

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
    test_cleanup_incomplete_alone_reports_failed();
    test_cleanup_none_and_complete_is_noop();
    test_unconfirmed_stop_latches_fail_closed_until_confirmed();
    test_unconfirmed_stop_is_visible_in_health();
    test_concurrent_snapshot_never_observes_torn_cleanup_state();
    puts("executor health tests passed");
    return EXIT_SUCCESS;
}
