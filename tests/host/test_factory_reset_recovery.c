#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "factory_reset_recovery_engine.h"
#include "test_assert.h"

typedef enum {
    FAIL_NONE = 0,
    FAIL_SETTINGS_INIT,
    FAIL_ERASE_SETTINGS,
    FAIL_SETTINGS_DEINIT,
    FAIL_STORAGE_MOUNT,
    FAIL_DELETE_BLOBS,
    FAIL_CLEANUP_TEMPORARY,
    FAIL_STORAGE_UNMOUNT,
    FAIL_CLEAR_PENDING,
} failure_stage_t;

typedef struct {
    factory_reset_state_t state;
    failure_stage_t failure_stage;
    unsigned int read_state_calls;
    unsigned int settings_init_calls;
    unsigned int erase_settings_calls;
    unsigned int settings_deinit_calls;
    unsigned int storage_mount_calls;
    unsigned int delete_blobs_calls;
    unsigned int cleanup_temporary_calls;
    unsigned int storage_unmount_calls;
    unsigned int clear_pending_calls;
} fixture_t;

static app_error_code_t stage_result(const fixture_t *fixture, failure_stage_t stage) {
    return fixture->failure_stage == stage ? APP_ERROR_IO : APP_ERROR_NONE;
}

static app_error_code_t fake_read_state(void *context, factory_reset_state_t *out_state) {
    fixture_t *fixture = context;
    ++fixture->read_state_calls;
    *out_state = fixture->state;
    return APP_ERROR_NONE;
}

#define DEFINE_STAGE(name, member, stage)                                                          \
    static app_error_code_t fake_##name(void *context) {                                           \
        fixture_t *fixture = context;                                                              \
        ++fixture->member;                                                                         \
        return stage_result(fixture, stage);                                                       \
    }

DEFINE_STAGE(settings_init, settings_init_calls, FAIL_SETTINGS_INIT)
DEFINE_STAGE(erase_settings, erase_settings_calls, FAIL_ERASE_SETTINGS)
DEFINE_STAGE(settings_deinit, settings_deinit_calls, FAIL_SETTINGS_DEINIT)
DEFINE_STAGE(storage_mount, storage_mount_calls, FAIL_STORAGE_MOUNT)
DEFINE_STAGE(delete_blobs, delete_blobs_calls, FAIL_DELETE_BLOBS)
DEFINE_STAGE(cleanup_temporary, cleanup_temporary_calls, FAIL_CLEANUP_TEMPORARY)
DEFINE_STAGE(storage_unmount, storage_unmount_calls, FAIL_STORAGE_UNMOUNT)
#undef DEFINE_STAGE

static app_error_code_t fake_clear_pending(void *context) {
    fixture_t *fixture = context;
    ++fixture->clear_pending_calls;
    const app_error_code_t result = stage_result(fixture, FAIL_CLEAR_PENDING);
    if (result == APP_ERROR_NONE) {
        fixture->state = FACTORY_RESET_STATE_NONE;
    }
    return result;
}

static factory_reset_recovery_ops_t operations(fixture_t *fixture) {
    return (factory_reset_recovery_ops_t){
        .context = fixture,
        .read_state = fake_read_state,
        .settings_init = fake_settings_init,
        .erase_settings = fake_erase_settings,
        .settings_deinit = fake_settings_deinit,
        .storage_mount = fake_storage_mount,
        .delete_blobs = fake_delete_blobs,
        .cleanup_temporary_files = fake_cleanup_temporary,
        .storage_unmount = fake_storage_unmount,
        .clear_pending = fake_clear_pending,
    };
}

static void test_none_is_noop(void) {
    fixture_t fixture = {0};
    const factory_reset_recovery_ops_t ops = operations(&fixture);
    bool recovered = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_recovery_engine_run(&ops, &recovered));
    TEST_CHECK(!recovered);
    TEST_CHECK_EQ_U64(1U, fixture.read_state_calls);
    TEST_CHECK_EQ_U64(0U, fixture.settings_init_calls);
    TEST_CHECK_EQ_U64(0U, fixture.storage_mount_calls);
    TEST_CHECK_EQ_U64(0U, fixture.clear_pending_calls);
}

static void test_pending_completes_and_reentry_is_noop(void) {
    fixture_t fixture = {.state = FACTORY_RESET_STATE_PENDING};
    const factory_reset_recovery_ops_t ops = operations(&fixture);
    bool recovered = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_recovery_engine_run(&ops, &recovered));
    TEST_CHECK(recovered);
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, fixture.state);
    TEST_CHECK_EQ_U64(1U, fixture.erase_settings_calls);
    TEST_CHECK_EQ_U64(1U, fixture.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fixture.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(1U, fixture.clear_pending_calls);

    recovered = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_recovery_engine_run(&ops, &recovered));
    TEST_CHECK(!recovered);
    TEST_CHECK_EQ_U64(2U, fixture.read_state_calls);
    TEST_CHECK_EQ_U64(1U, fixture.erase_settings_calls);
    TEST_CHECK_EQ_U64(1U, fixture.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fixture.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(1U, fixture.clear_pending_calls);
}

static void assert_failure_retries_safely(failure_stage_t stage) {
    fixture_t fixture = {
        .state = FACTORY_RESET_STATE_PENDING,
        .failure_stage = stage,
    };
    const factory_reset_recovery_ops_t ops = operations(&fixture);
    bool recovered = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, factory_reset_recovery_engine_run(&ops, &recovered));
    TEST_CHECK(!recovered);
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_PENDING, fixture.state);

    fixture.failure_stage = FAIL_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, factory_reset_recovery_engine_run(&ops, &recovered));
    TEST_CHECK(recovered);
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, fixture.state);
}

static void test_every_interrupted_stage_is_retryable(void) {
    assert_failure_retries_safely(FAIL_SETTINGS_INIT);
    assert_failure_retries_safely(FAIL_ERASE_SETTINGS);
    assert_failure_retries_safely(FAIL_SETTINGS_DEINIT);
    assert_failure_retries_safely(FAIL_STORAGE_MOUNT);
    assert_failure_retries_safely(FAIL_DELETE_BLOBS);
    assert_failure_retries_safely(FAIL_CLEANUP_TEMPORARY);
    assert_failure_retries_safely(FAIL_STORAGE_UNMOUNT);
    assert_failure_retries_safely(FAIL_CLEAR_PENDING);
}

static void test_blob_and_temporary_cleanup_both_attempted(void) {
    fixture_t fixture = {
        .state = FACTORY_RESET_STATE_PENDING,
        .failure_stage = FAIL_DELETE_BLOBS,
    };
    const factory_reset_recovery_ops_t ops = operations(&fixture);
    bool recovered = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, factory_reset_recovery_engine_run(&ops, &recovered));
    TEST_CHECK_EQ_U64(1U, fixture.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fixture.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(1U, fixture.storage_unmount_calls);
    TEST_CHECK_EQ_U64(0U, fixture.clear_pending_calls);
}

int main(void) {
    test_none_is_noop();
    test_pending_completes_and_reentry_is_noop();
    test_every_interrupted_stage_is_retryable();
    test_blob_and_temporary_cleanup_both_attempted();
    puts("factory reset recovery tests passed");
    return EXIT_SUCCESS;
}
