#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "app_operation_result.h"
#include "test_assert.h"

static void test_success_is_ok(void) {
    app_operation_result_t result = app_operation_success();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result.cleanup_error);
    TEST_CHECK(!result.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_COMMIT_NOT_APPLICABLE, result.commit_state);
    TEST_CHECK(app_operation_result_ok(result));
}

static void test_record_primary_first_wins(void) {
    app_operation_result_t result = app_operation_success();

    app_operation_record_primary(&result, APP_ERROR_IO);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, result.primary_error);
    TEST_CHECK(!app_operation_result_ok(result));

    /* A later primary error must not overwrite the first one. */
    app_operation_record_primary(&result, APP_ERROR_CONFLICT);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, result.primary_error);
}

static void test_record_primary_none_is_noop(void) {
    app_operation_result_t result = app_operation_success();

    /* Recording APP_ERROR_NONE never marks the operation as failed. */
    app_operation_record_primary(&result, APP_ERROR_NONE);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result.primary_error);
    TEST_CHECK(app_operation_result_ok(result));
}

static void test_record_cleanup_first_error_wins_and_marks_incomplete(void) {
    app_operation_result_t result = app_operation_success();

    app_operation_record_cleanup(&result, APP_ERROR_STORAGE_CORRUPT);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, result.cleanup_error);
    TEST_CHECK(result.cleanup_incomplete);
    TEST_CHECK(!app_operation_result_ok(result));

    /* A second cleanup failure keeps the first error but stays incomplete. */
    app_operation_record_cleanup(&result, APP_ERROR_IO);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, result.cleanup_error);
    TEST_CHECK(result.cleanup_incomplete);
}

static void test_record_cleanup_none_is_noop(void) {
    app_operation_result_t result = app_operation_success();

    app_operation_record_cleanup(&result, APP_ERROR_NONE);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result.cleanup_error);
    TEST_CHECK(!result.cleanup_incomplete);
    TEST_CHECK(app_operation_result_ok(result));
}

static void test_primary_and_cleanup_errors_coexist(void) {
    /* The defect this type fixes: a primary failure and a cleanup failure must
     * both survive; neither overwrites the other. */
    app_operation_result_t result = app_operation_success();

    app_operation_record_primary(&result, APP_ERROR_TIMEOUT);
    app_operation_record_cleanup(&result, APP_ERROR_IO);

    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, result.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, result.cleanup_error);
    TEST_CHECK(result.cleanup_incomplete);
    TEST_CHECK(!app_operation_result_ok(result));
}

static void test_cleanup_incomplete_alone_is_not_ok(void) {
    /* cleanup_incomplete can be set even when both error codes were NONE at the
     * time (defensive: the first recorded cleanup error is always non-NONE here,
     * but result_ok must still gate on the flag). */
    app_operation_result_t result = app_operation_success();
    result.cleanup_incomplete = true;
    TEST_CHECK(!app_operation_result_ok(result));
}

static void test_commit_state_is_fail_closed(void) {
    app_operation_result_t result = app_operation_success();

    result.commit_state = APP_OPERATION_COMMITTED;
    TEST_CHECK(app_operation_result_ok(result));

    result.commit_state = APP_OPERATION_NOT_COMMITTED;
    TEST_CHECK(!app_operation_result_ok(result));

    result.commit_state = APP_OPERATION_COMMIT_UNCERTAIN;
    TEST_CHECK(!app_operation_result_ok(result));
}

static void test_public_error_mapping_preserves_primary(void) {
    app_operation_result_t result = app_operation_success();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_operation_result_error(result));

    app_operation_record_cleanup(&result, APP_ERROR_IO);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, app_operation_result_error(result));

    app_operation_record_primary(&result, APP_ERROR_TIMEOUT);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, app_operation_result_error(result));
}

static void test_null_result_is_safe(void) {
    /* Must not crash on a NULL result pointer. */
    app_operation_record_primary(NULL, APP_ERROR_IO);
    app_operation_record_cleanup(NULL, APP_ERROR_IO);
}

int main(void) {
    test_success_is_ok();
    test_record_primary_first_wins();
    test_record_primary_none_is_noop();
    test_record_cleanup_first_error_wins_and_marks_incomplete();
    test_record_cleanup_none_is_noop();
    test_primary_and_cleanup_errors_coexist();
    test_cleanup_incomplete_alone_is_not_ok();
    test_commit_state_is_fail_closed();
    test_public_error_mapping_preserves_primary();
    test_null_result_is_safe();
    puts("app operation result tests passed");
    return EXIT_SUCCESS;
}
