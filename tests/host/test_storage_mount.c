#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "app_operation_result.h"
#include "storage.h"
#include "storage_mount_core.h"
#include "test_assert.h"

typedef struct {
    size_t mount_web;
    size_t mount_data;
    size_t prepare;
    size_t unmount_web;
    size_t unmount_data;
    app_error_code_t mount_web_result;
    app_error_code_t mount_data_result;
    app_error_code_t prepare_result;
    app_error_code_t unmount_web_result;
    app_error_code_t unmount_data_result;
} fixture_t;

static app_error_code_t mount_web(void *context) {
    fixture_t *fixture = context;
    ++fixture->mount_web;
    return fixture->mount_web_result;
}

static app_error_code_t mount_data(void *context) {
    fixture_t *fixture = context;
    ++fixture->mount_data;
    return fixture->mount_data_result;
}

static app_error_code_t prepare(void *context) {
    fixture_t *fixture = context;
    ++fixture->prepare;
    return fixture->prepare_result;
}

static app_error_code_t unmount_web(void *context) {
    fixture_t *fixture = context;
    ++fixture->unmount_web;
    return fixture->unmount_web_result;
}

static app_error_code_t unmount_data(void *context) {
    fixture_t *fixture = context;
    ++fixture->unmount_data;
    return fixture->unmount_data_result;
}

static storage_mount_ops_t operations(fixture_t *fixture) {
    return (storage_mount_ops_t){
        .context = fixture,
        .mount_web = mount_web,
        .mount_data = mount_data,
        .prepare_directories = prepare,
        .unmount_web = unmount_web,
        .unmount_data = unmount_data,
    };
}

static void test_mount_and_unmount(void) {
    fixture_t fixture = {0};
    storage_mount_state_t state = {0};
    const storage_mount_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_mount_core_mount(&ops, &state));
    TEST_CHECK(state.web_mounted);
    TEST_CHECK(state.data_mounted);
    TEST_CHECK_EQ_U64(1U, fixture.prepare);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_mount_core_unmount(&ops, &state));
    TEST_CHECK(!state.web_mounted);
    TEST_CHECK(!state.data_mounted);
}

static void test_mount_failures_are_explicit_and_never_prepare(void) {
    fixture_t fixture = {.mount_web_result = APP_ERROR_STORAGE_UNAVAILABLE};
    storage_mount_state_t state = {0};
    storage_mount_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, storage_mount_core_mount(&ops, &state));
    TEST_CHECK_EQ_U64(1U, fixture.mount_web);
    TEST_CHECK_EQ_U64(0U, fixture.mount_data);
    TEST_CHECK_EQ_U64(0U, fixture.prepare);
    TEST_CHECK_EQ_U64(0U, fixture.unmount_web);
    TEST_CHECK(!state.web_mounted);
    TEST_CHECK(!state.data_mounted);

    fixture = (fixture_t){.mount_data_result = APP_ERROR_STORAGE_UNAVAILABLE};
    state = (storage_mount_state_t){0};
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, storage_mount_core_mount(&ops, &state));
    TEST_CHECK_EQ_U64(1U, fixture.mount_web);
    TEST_CHECK_EQ_U64(1U, fixture.mount_data);
    TEST_CHECK_EQ_U64(0U, fixture.prepare);
    TEST_CHECK_EQ_U64(1U, fixture.unmount_web);
    TEST_CHECK(!state.web_mounted);
    TEST_CHECK(!state.data_mounted);
}

static void test_prepare_failure_rolls_back(void) {
    fixture_t fixture = {.prepare_result = APP_ERROR_IO};
    storage_mount_state_t state = {0};
    const storage_mount_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_mount_core_mount(&ops, &state));
    TEST_CHECK_EQ_U64(1U, fixture.unmount_data);
    TEST_CHECK_EQ_U64(1U, fixture.unmount_web);
}

static void test_mount_data_failure_preserves_rollback_error(void) {
    fixture_t fixture = {
        .mount_data_result = APP_ERROR_STORAGE_UNAVAILABLE,
        .unmount_web_result = APP_ERROR_IO,
    };
    storage_mount_state_t state = {0};
    const storage_mount_ops_t ops = operations(&fixture);

    const app_operation_result_t detailed = storage_mount_core_mount_result(&ops, &state);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, detailed.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, detailed.cleanup_error);
    TEST_CHECK(detailed.cleanup_incomplete);
    TEST_CHECK(state.web_mounted);
    TEST_CHECK(!state.data_mounted);

    fixture = (fixture_t){
        .mount_data_result = APP_ERROR_STORAGE_UNAVAILABLE,
        .unmount_web_result = APP_ERROR_IO,
    };
    state = (storage_mount_state_t){0};
    const storage_mount_ops_t compatibility_ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         storage_mount_core_mount(&compatibility_ops, &state));
    TEST_CHECK(state.web_mounted);
}

static void test_prepare_failure_preserves_first_rollback_error(void) {
    fixture_t fixture = {
        .prepare_result = APP_ERROR_STORAGE_CORRUPT,
        .unmount_data_result = APP_ERROR_IO,
        .unmount_web_result = APP_ERROR_INTERNAL,
    };
    storage_mount_state_t state = {0};
    const storage_mount_ops_t ops = operations(&fixture);

    const app_operation_result_t detailed = storage_mount_core_mount_result(&ops, &state);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, detailed.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, detailed.cleanup_error);
    TEST_CHECK(detailed.cleanup_incomplete);
    TEST_CHECK(state.data_mounted);
    TEST_CHECK(state.web_mounted);
    TEST_CHECK_EQ_U64(1U, fixture.unmount_data);
    TEST_CHECK_EQ_U64(1U, fixture.unmount_web);
}

int main(void) {
    test_mount_and_unmount();
    test_mount_failures_are_explicit_and_never_prepare();
    test_prepare_failure_rolls_back();
    test_mount_data_failure_preserves_rollback_error();
    test_prepare_failure_preserves_first_rollback_error();
    puts("storage mount tests passed");
    return EXIT_SUCCESS;
}
