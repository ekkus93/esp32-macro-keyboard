#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device_controls_reset.h"
#include "test_assert.h"

typedef struct {
    app_error_code_t reset_settings_result;
    app_error_code_t erase_all_settings_result;
    app_error_code_t invalidate_sessions_result;
    app_error_code_t delete_blobs_result;
    unsigned int reset_settings_calls;
    unsigned int erase_all_settings_calls;
    unsigned int invalidate_sessions_calls;
    unsigned int delete_blobs_calls;
    unsigned int schedule_restart_calls;
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

static app_error_code_t fake_erase_all_settings(void *context) {
    fake_reset_t *fake = context;
    ++fake->erase_all_settings_calls;
    return fake->erase_all_settings_result;
}

static app_error_code_t fake_invalidate_sessions(void *context) {
    fake_reset_t *fake = context;
    ++fake->invalidate_sessions_calls;
    return fake->invalidate_sessions_result;
}

static app_error_code_t fake_delete_blobs(void *context) {
    fake_reset_t *fake = context;
    ++fake->delete_blobs_calls;
    return fake->delete_blobs_result;
}

static void fake_schedule_restart(void *context, uint32_t delay_ms) {
    fake_reset_t *fake = context;
    ++fake->schedule_restart_calls;
    fake->last_delay_ms = delay_ms;
}

static device_controls_reset_ops_t fake_ops(fake_reset_t *fake) {
    return (device_controls_reset_ops_t){
        .context = fake,
        .reset_settings_noncredential = fake_reset_settings,
        .erase_all_settings = fake_erase_all_settings,
        .invalidate_all_sessions = fake_invalidate_sessions,
        .delete_all_blobs = fake_delete_blobs,
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
    CHECK_MISSING(erase_all_settings);
    CHECK_MISSING(invalidate_all_sessions);
    CHECK_MISSING(delete_all_blobs);
    CHECK_MISSING(schedule_restart);
#undef CHECK_MISSING
}

/* SPEC_V2.md §13.12 "restart": no settings, credential, or repository change,
 * only a scheduled reboot. */
static void test_restart_only_schedules_reboot(void) {
    fake_reset_t fake;
    fake_init(&fake);
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_reset_engine_restart(&operations, 750U));
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK_EQ_U64(750U, fake.last_delay_ms);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
}

/* SPEC_V2.md §11.4/§13.12 "reset-settings": settings reset, then invalidate
 * sessions, then schedule a reboot. Blobs are never touched. */
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
    TEST_CHECK_EQ_U64(0U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
}

/* A settings-write failure must abort before sessions are touched or a reboot
 * is scheduled -- nothing destructive begins from a failed first step. */
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

/* A session-invalidation failure is reported but still lets the reboot
 * happen, because the settings write already succeeded and must take
 * effect. */
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

/* SPEC_V2.md §11.4/§13.12 "factory-reset": erase settings, then invalidate
 * sessions and delete every blob, then schedule a reboot. */
static void test_factory_reset_happy_path(void) {
    fake_reset_t fake;
    fake_init(&fake);
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
}

/* If erasing settings itself fails, nothing else runs: no sessions
 * invalidated, no blobs deleted, no reboot scheduled. A failed factory reset
 * must not destroy the repository while leaving old credentials in place. */
static void test_factory_reset_aborts_on_settings_failure(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.erase_all_settings_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(0U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(0U, fake.schedule_restart_calls);
}

/* Once settings are erased, a session-invalidation failure must not stop
 * blob deletion or the reboot -- every remaining destructive step is still
 * attempted, and the first error is reported. */
static void test_factory_reset_continues_past_session_failure(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.invalidate_sessions_result = APP_ERROR_INTERNAL;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.erase_all_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

/* Symmetric case: a blob-deletion failure must not stop session invalidation
 * or the reboot, and the first error observed (session invalidation, which
 * runs first) wins over the later blob-deletion error. */
static void test_factory_reset_continues_past_blob_failure_first_error_wins(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.invalidate_sessions_result = APP_ERROR_INTERNAL;
    fake.delete_blobs_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_reset_engine_factory_reset(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_blobs_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

int main(void) {
    test_ops_validation();
    test_restart_only_schedules_reboot();
    test_reset_settings_happy_path();
    test_reset_settings_aborts_on_settings_failure();
    test_reset_settings_reports_session_failure_but_still_restarts();
    test_factory_reset_happy_path();
    test_factory_reset_aborts_on_settings_failure();
    test_factory_reset_continues_past_session_failure();
    test_factory_reset_continues_past_blob_failure_first_error_wins();
    puts("device controls reset engine tests passed");
    return EXIT_SUCCESS;
}
