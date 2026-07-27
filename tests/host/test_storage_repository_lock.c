#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "storage_repository_lock.h"
#include "test_assert.h"

/* A controllable fake backend: each operation can be told to fail, and take/give
 * calls are counted so the test can prove ordering and failure propagation. */
typedef struct {
    size_t init_calls;
    size_t take_calls;
    size_t give_calls;
    size_t deinit_calls;
    app_error_code_t init_result;
    app_error_code_t take_result;
    app_error_code_t give_result;
    app_error_code_t deinit_result;
} fake_lock_t;

static app_error_code_t fake_init(void *context) {
    fake_lock_t *lock = context;
    ++lock->init_calls;
    return lock->init_result;
}

static app_error_code_t fake_take(void *context) {
    fake_lock_t *lock = context;
    ++lock->take_calls;
    return lock->take_result;
}

static app_error_code_t fake_give(void *context) {
    fake_lock_t *lock = context;
    ++lock->give_calls;
    return lock->give_result;
}

static app_error_code_t fake_deinit(void *context) {
    fake_lock_t *lock = context;
    ++lock->deinit_calls;
    return lock->deinit_result;
}

/* The host default backend is a re-entrancy-detecting flag lock. */
static void test_default_backend_flag_semantics(void) {
    storage_repository_lock_set_ops(NULL);
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_take());

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_init());
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT, storage_repository_lock_init());

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_take());
    /* A re-entrant take (which would deadlock the production mutex) is rejected. */
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_take());
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_give());
    /* Giving a lock we do not hold is rejected. */
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_give());

    /* Re-acquire cleanly after a matched take/give. */
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_take());
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_give());

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_deinit());
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_deinit());
    /* After deinit the lock is unusable until re-initialized. */
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_take());
}

static void test_ops_seam_delegates_and_propagates(void) {
    fake_lock_t lock = {
        .init_result = APP_ERROR_NONE,
        .take_result = APP_ERROR_NONE,
        .give_result = APP_ERROR_NONE,
        .deinit_result = APP_ERROR_NONE,
    };
    const storage_repository_lock_ops_t ops = {
        .context = &lock,
        .init = fake_init,
        .take = fake_take,
        .give = fake_give,
        .deinit = fake_deinit,
    };
    storage_repository_lock_set_ops(&ops);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_init());
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_take());
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_give());
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_deinit());
    TEST_CHECK_EQ_U64(1U, lock.init_calls);
    TEST_CHECK_EQ_U64(1U, lock.take_calls);
    TEST_CHECK_EQ_U64(1U, lock.give_calls);
    TEST_CHECK_EQ_U64(1U, lock.deinit_calls);

    /* Backend failures propagate verbatim. */
    lock.take_result = APP_ERROR_INTERNAL;
    lock.give_result = APP_ERROR_INTERNAL;
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_take());
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_give());

    /* Restoring the default backend detaches the fake. */
    storage_repository_lock_set_ops(NULL);
    TEST_CHECK_EQ_U64(2U, lock.take_calls);
    TEST_CHECK_EQ_U64(2U, lock.give_calls);
}

static void test_missing_backend_callbacks_fail_closed(void) {
    const storage_repository_lock_ops_t empty = {0};
    storage_repository_lock_set_ops(&empty);
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_init());
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_take());
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_give());
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, storage_repository_lock_deinit());
    storage_repository_lock_set_ops(NULL);
}

int main(void) {
    test_default_backend_flag_semantics();
    test_ops_seam_delegates_and_propagates();
    test_missing_backend_callbacks_fail_closed();
    puts("storage repository lock tests passed");
    return EXIT_SUCCESS;
}
