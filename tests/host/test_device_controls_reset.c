#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device_controls_reset.h"
#include "test_assert.h"

typedef struct {
    app_error_code_t reset_settings_result;
    app_error_code_t mark_pending_result;
    app_error_code_t erase_all_settings_result;
    app_error_code_t invalidate_sessions_result;
    app_error_code_t delete_blobs_result;
    app_error_code_t cleanup_temporary_result;
    app_error_code_t clear_pending_result;
    unsigned int reset_settings_calls;
    unsigned int mark_pending_calls;
    unsigned int erase_all_settings_calls;
    unsigned int invalidate_sessions_calls;
    unsigned int delete_blobs_calls;
    unsigned int cleanup_temporary_calls;
    unsigned int clear_pending_calls;
    unsigned int schedule_restart_calls;
    unsigned int sequence;
    unsigned int mark_pending_sequence;
    unsigned int erase_all_settings_sequence;
    unsigned int invalidate_sessions_sequence;
    unsigned int delete_blobs_sequence;
    unsigned int cleanup_temporary_sequence;
    unsigned int clear_pending_sequence;
    unsigned int restart_sequence;
    uint32_t last_delay_ms;
} fake_reset_t;

static void fake_init(fake_reset_t *fake) {
    memset(fake, 0, sizeof(*fake));
}

static app_error_code_t fake_reset_settings(void *context) {
    fake_reset_t *fake = context;
    ++fake->reset_settings_calls;
    return fake->reset_settings_result;
}

static app_error_code_t fake_mark_pending(void *context) {
    fake_reset_t *fake = context;
    ++fake->mark_pending_calls;
    fake->mark_pending_sequence = ++fake->sequence;
    return fake->mark_pending_result;
}

static app_error_code_t fake_erase_all_settings(void *context) {
    fake_reset_t *fake = context;
    ++fake->erase_all_settings_calls;
    fake->erase_all_settings_sequence = ++fake->sequence;
    return fake->erase_all_settings_result;
}

static app_error_code_t fake_invalidate_sessions(void *context) {
    fake_reset_t *fake = context;
    ++fake->invalidate_sessions_calls;
    fake->invalidate_sessions_sequence = ++fake->sequence;
    return fake->invalidate_sessions_result;
}

static app_error_code_t fake_delete_blobs(void *context) {
    fake_reset_t *fake = context;
    ++fake->delete_blobs_calls;
    fake->delete_blobs_sequence = ++fake->sequence;
    return fake->delete_blobs_result;
}

static app_error_code_t fake_cleanup_temporary(void *context) {
    fake_reset_t *fake = context;
    ++fake->cleanup_temporary_calls;
    fake->cleanup_temporary_sequence = ++fake->sequence;
    return fake->cleanup_temporary_result;
}

static app_error_code_t fake_clear_pending(void *context) {
    fake_reset_t *fake = context;
    ++fake->clear_pending_calls;
    fake->clear_pending_sequence = ++fake->sequence;
    return fake->clear_pending_result;
}

static void fake_schedule_restart(void *context, uint32_t delay_ms) {
    fake_reset_t *fake = context;
    ++fake->schedule_restart_calls;
    fake->restart_sequence = ++fake->sequence;
    fake->last_delay_ms = delay_ms;
}

static device_controls_reset_ops_t fake_ops(fake_reset_t *fake) {
    return (device_controls_reset_ops_t){
        .context = fake,
        .reset_settings_noncredential = fake_reset_settings,
        .mark_factory_reset_pending = fake_mark_pending,
        .erase_all_settings = fake_erase_all_settings,
        .invalidate_all_sessions = fake_invalidate_sessions,
        .delete_all_blobs = fake_delete_blobs,
        .cleanup_temporary_files = fake_cleanup_temporary,
        .clear_factory_reset_pending = fake_clear_pending,
        .schedule_restart = fake_schedule_restart,
    };
}

static void test_ops_validation(void) {
    fake_reset_t fake;
    fake_init(&fake);
    device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK(device_controls_reset_ops_is_valid(&operations));
    TEST_CHECK(!device_controls_reset_ops_is_valid(NULL));

#define CHECK_MISSING(member)                                                                      \
    do {                                                                                           \
        operations = fake_ops(&fake);                                                              \
        operations.member = NULL;                                                                  \
        TEST_CHECK(!device_controls_reset_ops_is_valid(&operations));                              \
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,                                           \
                             device_controls_reset_engine_restart(&operations, 500U));             \
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,                                           \
                             device_controls_reset_engine_reset_settings(&operations, 500U));      \
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,                                           \
                             device_controls_reset_engine_factory_reset(&operations, 500U));       \
    } while (0)

    CHECK_MISSING(reset_settings_noncredential);
    CHECK_MISSING(mark_factory_reset_pending);
    CHECK_MISSING(erase_all_settings);
    CHECK_MISSING(invalidate_all_sessions);
    CHECK_MISSING(delete_all_blobs);
    CHECK_MISSING(cleanup_temporary_files);
    CHECK_MISSING(clear_factory_reset_pending);
    CHECK_MISSING(schedule_restart);
#undef CHECK_MISSING
}

static void test_restart_only_schedules_reboot(void) {
    fake_reset_t fake;
    fake_init(&fake);
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_reset_engine_restart(&operations, 750U));
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK_EQ_U64(750U, fake.last_delay_ms);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.mark_pending_calls);
    TEST_CHECK_EQ_U64(0U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(0U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
}

static void test_reset_settings_happy_path(void) {
    fake_reset_t fake;
    fake_init(&fake);
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_reset_engine_reset_settings(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK_EQ_U64(500U, fake.last_delay_ms);
    TEST_CHECK_EQ_U64(0U, fake.mark_pending_calls);
    TEST_CHECK_EQ_U64(0U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(0U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
}

static void test_reset_settings_aborts_on_settings_failure(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.reset_settings_result = APP_ERROR_STORAGE_UNAVAILABLE;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         device_controls_reset_engine_reset_settings(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(0U, fake.schedule_restart_calls);
}

static void test_reset_settings_reports_session_failure_but_still_restarts(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.invalidate_sessions_result = APP_ERROR_INTERNAL;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_reset_engine_reset_settings(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_factory_reset_happy_path_orders_durable_boundary_first(void) {
    fake_reset_t fake;
    fake_init(&fake);
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.mark_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(1U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK(fake.mark_pending_sequence < fake.erase_all_settings_sequence);
    TEST_CHECK(fake.erase_all_settings_sequence < fake.invalidate_sessions_sequence);
    TEST_CHECK(fake.invalidate_sessions_sequence < fake.delete_blobs_sequence);
    TEST_CHECK(fake.delete_blobs_sequence < fake.cleanup_temporary_sequence);
    TEST_CHECK(fake.cleanup_temporary_sequence < fake.clear_pending_sequence);
    TEST_CHECK(fake.clear_pending_sequence < fake.restart_sequence);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
}

static void test_factory_reset_marker_failure_is_nondestructive(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.mark_pending_result = APP_ERROR_STORAGE_UNAVAILABLE;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.mark_pending_calls);
    TEST_CHECK_EQ_U64(0U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(0U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(0U, fake.schedule_restart_calls);
}

static void test_factory_reset_settings_failure_keeps_marker_and_restarts(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.erase_all_settings_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.mark_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(0U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_factory_reset_cleanup_failure_keeps_marker_and_restarts(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.invalidate_sessions_result = APP_ERROR_INTERNAL;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_factory_reset_blob_failure_keeps_marker_and_first_error_wins(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.invalidate_sessions_result = APP_ERROR_INTERNAL;
    fake.delete_blobs_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_factory_reset_temporary_cleanup_failure_keeps_marker_and_restarts(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.cleanup_temporary_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_factory_reset_replay_is_safe(void) {
    fake_reset_t fake;
    fake_init(&fake);
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(2U, fake.mark_pending_calls);
    TEST_CHECK_EQ_U64(2U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(2U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(2U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(2U, fake.cleanup_temporary_calls);
    TEST_CHECK_EQ_U64(2U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(2U, fake.schedule_restart_calls);
}

static void test_factory_reset_marker_clear_failure_reboots_into_recovery(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.clear_pending_result = APP_ERROR_STORAGE_UNAVAILABLE;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.clear_pending_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK(fake.clear_pending_sequence < fake.restart_sequence);
}

int main(void) {
    test_ops_validation();
    test_restart_only_schedules_reboot();
    test_reset_settings_happy_path();
    test_reset_settings_aborts_on_settings_failure();
    test_reset_settings_reports_session_failure_but_still_restarts();
    test_factory_reset_happy_path_orders_durable_boundary_first();
    test_factory_reset_marker_failure_is_nondestructive();
    test_factory_reset_settings_failure_keeps_marker_and_restarts();
    test_factory_reset_cleanup_failure_keeps_marker_and_restarts();
    test_factory_reset_blob_failure_keeps_marker_and_first_error_wins();
    test_factory_reset_temporary_cleanup_failure_keeps_marker_and_restarts();
    test_factory_reset_replay_is_safe();
    test_factory_reset_marker_clear_failure_reboots_into_recovery();
    puts("device controls reset engine tests passed");
    return EXIT_SUCCESS;
}
