#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "storage.h"
#include "storage_mount_core.h"
#include "test_assert.h"

typedef struct {
    size_t mount_web;
    size_t mount_data;
    size_t prepare;
    size_t unmount_web;
    size_t unmount_data;
    app_error_code_t prepare_result;
} fixture_t;

static app_error_code_t mount_web(void *context) {
    ++((fixture_t *)context)->mount_web;
    return APP_ERROR_NONE;
}

static app_error_code_t mount_data(void *context) {
    ++((fixture_t *)context)->mount_data;
    return APP_ERROR_NONE;
}

static app_error_code_t prepare(void *context) {
    fixture_t *fixture = context;
    ++fixture->prepare;
    return fixture->prepare_result;
}

static app_error_code_t unmount_web(void *context) {
    ++((fixture_t *)context)->unmount_web;
    return APP_ERROR_NONE;
}

static app_error_code_t unmount_data(void *context) {
    ++((fixture_t *)context)->unmount_data;
    return APP_ERROR_NONE;
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

static void test_prepare_failure_rolls_back(void) {
    fixture_t fixture = {.prepare_result = APP_ERROR_IO};
    storage_mount_state_t state = {0};
    const storage_mount_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_mount_core_mount(&ops, &state));
    TEST_CHECK_EQ_U64(1U, fixture.unmount_data);
    TEST_CHECK_EQ_U64(1U, fixture.unmount_web);
}

int main(void) {
    test_mount_and_unmount();
    test_prepare_failure_rolls_back();
    puts("storage mount tests passed");
    return EXIT_SUCCESS;
}
