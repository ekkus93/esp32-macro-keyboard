#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "device_controls_reset.h"
#include "factory_reset_recovery_engine.h"
#include "test_assert.h"

typedef enum {
    LIVE_FAIL_NONE = 0,
    LIVE_FAIL_MARK,
    LIVE_FAIL_ERASE_SETTINGS,
    LIVE_FAIL_INVALIDATE_SESSIONS,
    LIVE_FAIL_DELETE_BLOBS,
    LIVE_FAIL_CLEANUP_TEMPORARY,
    LIVE_FAIL_CLEAR_PENDING,
} live_failure_t;

typedef enum {
    RECOVERY_FAIL_NONE = 0,
    RECOVERY_FAIL_SETTINGS_INIT,
    RECOVERY_FAIL_ERASE_SETTINGS,
    RECOVERY_FAIL_SETTINGS_DEINIT,
    RECOVERY_FAIL_STORAGE_MOUNT,
    RECOVERY_FAIL_DELETE_BLOBS,
    RECOVERY_FAIL_CLEANUP_TEMPORARY,
    RECOVERY_FAIL_STORAGE_UNMOUNT,
    RECOVERY_FAIL_CLEAR_PENDING,
} recovery_failure_t;

typedef struct {
    factory_reset_state_t reset_state;
    bool credentials_present;
    bool settings_present;
    bool session_active;
    size_t blobs_remaining;
    bool temporary_present;
    live_failure_t live_failure;
    recovery_failure_t recovery_failure;
    bool delayed_restart_arm_fails;
    bool delayed_restart_requested;
    bool immediate_restart_requested;
} fixture_t;

static fixture_t provisioned_fixture(void) {
    return (fixture_t){
        .reset_state = FACTORY_RESET_STATE_NONE,
        .credentials_present = true,
        .settings_present = true,
        .session_active = true,
        .blobs_remaining = 3U,
        .temporary_present = true,
    };
}

static bool normal_authority_available(const fixture_t *fixture) {
    return fixture->reset_state == FACTORY_RESET_STATE_NONE;
}

static bool setup_authority_available(const fixture_t *fixture) {
    return fixture->reset_state == FACTORY_RESET_STATE_NONE && !fixture->credentials_present &&
           !fixture->settings_present && fixture->blobs_remaining == 0U &&
           !fixture->temporary_present;
}

static void assert_pending_blocks_old_authority(const fixture_t *fixture) {
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_PENDING, fixture->reset_state);
    TEST_CHECK(!normal_authority_available(fixture));
    TEST_CHECK(!setup_authority_available(fixture));
}

static void assert_fully_unprovisioned(const fixture_t *fixture) {
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, fixture->reset_state);
    TEST_CHECK(!fixture->credentials_present);
    TEST_CHECK(!fixture->settings_present);
    TEST_CHECK(!fixture->session_active);
    TEST_CHECK_EQ_U64(0U, fixture->blobs_remaining);
    TEST_CHECK(!fixture->temporary_present);
    TEST_CHECK(setup_authority_available(fixture));
}

static app_error_code_t live_mark_pending(void *context) {
    fixture_t *fixture = context;
    if (fixture->live_failure == LIVE_FAIL_MARK) {
        return APP_ERROR_IO;
    }
    fixture->reset_state = FACTORY_RESET_STATE_PENDING;
    return APP_ERROR_NONE;
}

static app_error_code_t live_erase_settings(void *context) {
    fixture_t *fixture = context;
    if (fixture->live_failure == LIVE_FAIL_ERASE_SETTINGS) {
        return APP_ERROR_IO;
    }
    fixture->credentials_present = false;
    fixture->settings_present = false;
    return APP_ERROR_NONE;
}

static app_error_code_t live_invalidate_sessions(void *context) {
    fixture_t *fixture = context;
    if (fixture->live_failure == LIVE_FAIL_INVALIDATE_SESSIONS) {
        return APP_ERROR_IO;
    }
    fixture->session_active = false;
    return APP_ERROR_NONE;
}

static app_error_code_t live_delete_blobs(void *context) {
    fixture_t *fixture = context;
    if (fixture->live_failure == LIVE_FAIL_DELETE_BLOBS) {
        if (fixture->blobs_remaining > 0U) {
            fixture->blobs_remaining = 1U;
        }
        return APP_ERROR_IO;
    }
    fixture->blobs_remaining = 0U;
    return APP_ERROR_NONE;
}

static app_error_code_t live_cleanup_temporary(void *context) {
    fixture_t *fixture = context;
    if (fixture->live_failure == LIVE_FAIL_CLEANUP_TEMPORARY) {
        return APP_ERROR_IO;
    }
    fixture->temporary_present = false;
    return APP_ERROR_NONE;
}

static app_error_code_t live_clear_pending(void *context) {
    fixture_t *fixture = context;
    if (fixture->live_failure == LIVE_FAIL_CLEAR_PENDING) {
        return APP_ERROR_IO;
    }
    fixture->reset_state = FACTORY_RESET_STATE_NONE;
    return APP_ERROR_NONE;
}

static app_error_code_t unused_reset_settings(void *context) {
    (void)context;
    return APP_ERROR_INTERNAL;
}

static app_error_code_t live_schedule_restart(void *context, uint32_t delay_ms) {
    fixture_t *fixture = context;
    TEST_CHECK(delay_ms > 0U);
    if (fixture->delayed_restart_arm_fails) {
        fixture->immediate_restart_requested = true;
    } else {
        fixture->delayed_restart_requested = true;
    }
    return APP_ERROR_NONE;
}

static device_controls_reset_ops_t live_ops(fixture_t *fixture) {
    return (device_controls_reset_ops_t){
        .context = fixture,
        .reset_settings_noncredential = unused_reset_settings,
        .mark_factory_reset_pending = live_mark_pending,
        .erase_all_settings = live_erase_settings,
        .invalidate_all_sessions = live_invalidate_sessions,
        .delete_all_blobs = live_delete_blobs,
        .cleanup_temporary_files = live_cleanup_temporary,
        .clear_factory_reset_pending = live_clear_pending,
        .schedule_restart = live_schedule_restart,
    };
}

static app_error_code_t recovery_result(const fixture_t *fixture, recovery_failure_t stage) {
    return fixture->recovery_failure == stage ? APP_ERROR_IO : APP_ERROR_NONE;
}

static app_error_code_t recovery_read_state(void *context, factory_reset_state_t *out_state) {
    fixture_t *fixture = context;
    *out_state = fixture->reset_state;
    return APP_ERROR_NONE;
}

static app_error_code_t recovery_settings_init(void *context) {
    return recovery_result(context, RECOVERY_FAIL_SETTINGS_INIT);
}

static app_error_code_t recovery_erase_settings(void *context) {
    fixture_t *fixture = context;
    if (recovery_result(fixture, RECOVERY_FAIL_ERASE_SETTINGS) != APP_ERROR_NONE) {
        return APP_ERROR_IO;
    }
    fixture->credentials_present = false;
    fixture->settings_present = false;
    return APP_ERROR_NONE;
}

static app_error_code_t recovery_settings_deinit(void *context) {
    return recovery_result(context, RECOVERY_FAIL_SETTINGS_DEINIT);
}

static app_error_code_t recovery_storage_mount(void *context) {
    return recovery_result(context, RECOVERY_FAIL_STORAGE_MOUNT);
}

static app_error_code_t recovery_delete_blobs(void *context) {
    fixture_t *fixture = context;
    if (recovery_result(fixture, RECOVERY_FAIL_DELETE_BLOBS) != APP_ERROR_NONE) {
        if (fixture->blobs_remaining > 0U) {
            fixture->blobs_remaining = 1U;
        }
        return APP_ERROR_IO;
    }
    fixture->blobs_remaining = 0U;
    return APP_ERROR_NONE;
}

static app_error_code_t recovery_cleanup_temporary(void *context) {
    fixture_t *fixture = context;
    if (recovery_result(fixture, RECOVERY_FAIL_CLEANUP_TEMPORARY) != APP_ERROR_NONE) {
        return APP_ERROR_IO;
    }
    fixture->temporary_present = false;
    return APP_ERROR_NONE;
}

static app_error_code_t recovery_storage_unmount(void *context) {
    return recovery_result(context, RECOVERY_FAIL_STORAGE_UNMOUNT);
}

static app_error_code_t recovery_clear_pending(void *context) {
    fixture_t *fixture = context;
    if (recovery_result(fixture, RECOVERY_FAIL_CLEAR_PENDING) != APP_ERROR_NONE) {
        return APP_ERROR_IO;
    }
    fixture->reset_state = FACTORY_RESET_STATE_NONE;
    return APP_ERROR_NONE;
}

static factory_reset_recovery_ops_t recovery_ops(fixture_t *fixture) {
    return (factory_reset_recovery_ops_t){
        .context = fixture,
        .read_state = recovery_read_state,
        .settings_init = recovery_settings_init,
        .erase_settings = recovery_erase_settings,
        .settings_deinit = recovery_settings_deinit,
        .storage_mount = recovery_storage_mount,
        .delete_blobs = recovery_delete_blobs,
        .cleanup_temporary_files = recovery_cleanup_temporary,
        .storage_unmount = recovery_storage_unmount,
        .clear_pending = recovery_clear_pending,
    };
}

static void simulate_reboot(fixture_t *fixture) {
    /* Auth sessions are RAM-owned. A real restart discards the table even if
     * live invalidation failed just before reboot. */
    fixture->session_active = false;
    fixture->delayed_restart_requested = false;
    fixture->immediate_restart_requested = false;
}

static void recover_to_completion(fixture_t *fixture) {
    const factory_reset_recovery_ops_t operations = recovery_ops(fixture);
    bool recovered = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         factory_reset_recovery_engine_run(&operations, &recovered));
    TEST_CHECK(recovered);
    assert_fully_unprovisioned(fixture);
}

static void test_precommit_failure_is_not_accepted_or_destructive(void) {
    fixture_t fixture = provisioned_fixture();
    fixture.live_failure = LIVE_FAIL_MARK;
    const device_controls_reset_ops_t operations = live_ops(&fixture);
    const device_controls_factory_reset_outcome_t outcome =
        device_controls_reset_engine_factory_reset(&operations, 500U);

    TEST_CHECK(!outcome.durably_accepted);
    TEST_CHECK(!outcome.recovery_required);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.primary_error);
    TEST_CHECK_EQ_INT(FACTORY_RESET_STATE_NONE, fixture.reset_state);
    TEST_CHECK(fixture.credentials_present);
    TEST_CHECK(fixture.settings_present);
    TEST_CHECK(fixture.session_active);
    TEST_CHECK_EQ_U64(3U, fixture.blobs_remaining);
    TEST_CHECK(!fixture.delayed_restart_requested);
    TEST_CHECK(!fixture.immediate_restart_requested);
}

static void assert_live_postcommit_failure_recovers(live_failure_t failure) {
    fixture_t fixture = provisioned_fixture();
    fixture.live_failure = failure;
    const device_controls_reset_ops_t operations = live_ops(&fixture);
    const device_controls_factory_reset_outcome_t outcome =
        device_controls_reset_engine_factory_reset(&operations, 500U);

    TEST_CHECK(outcome.durably_accepted);
    TEST_CHECK(outcome.recovery_required);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.primary_error);
    assert_pending_blocks_old_authority(&fixture);
    TEST_CHECK(fixture.delayed_restart_requested);

    simulate_reboot(&fixture);
    fixture.live_failure = LIVE_FAIL_NONE;
    recover_to_completion(&fixture);
}

static void test_live_failure_matrix_recovers_after_reboot(void) {
    assert_live_postcommit_failure_recovers(LIVE_FAIL_ERASE_SETTINGS);
    assert_live_postcommit_failure_recovers(LIVE_FAIL_INVALIDATE_SESSIONS);
    assert_live_postcommit_failure_recovers(LIVE_FAIL_DELETE_BLOBS);
    assert_live_postcommit_failure_recovers(LIVE_FAIL_CLEANUP_TEMPORARY);
    assert_live_postcommit_failure_recovers(LIVE_FAIL_CLEAR_PENDING);
}

static void test_restart_timer_arm_failure_uses_immediate_reboot_fail_safe(void) {
    fixture_t fixture = provisioned_fixture();
    fixture.live_failure = LIVE_FAIL_ERASE_SETTINGS;
    fixture.delayed_restart_arm_fails = true;
    const device_controls_reset_ops_t operations = live_ops(&fixture);
    const device_controls_factory_reset_outcome_t outcome =
        device_controls_reset_engine_factory_reset(&operations, 500U);

    TEST_CHECK(outcome.durably_accepted);
    TEST_CHECK(outcome.recovery_required);
    assert_pending_blocks_old_authority(&fixture);
    TEST_CHECK(!fixture.delayed_restart_requested);
    TEST_CHECK(fixture.immediate_restart_requested);

    simulate_reboot(&fixture);
    fixture.live_failure = LIVE_FAIL_NONE;
    fixture.delayed_restart_arm_fails = false;
    recover_to_completion(&fixture);
}

typedef struct {
    bool credentials_present;
    bool settings_present;
    bool session_active;
    size_t blobs_remaining;
    bool temporary_present;
} durable_cut_t;

static void test_simulated_reboot_between_every_destructive_stage(void) {
    static const durable_cut_t cuts[] = {
        {true, true, true, 3U, true},     /* marker committed */
        {false, false, true, 3U, true},   /* settings erased */
        {false, false, false, 3U, true},  /* sessions invalidated */
        {false, false, false, 0U, true},  /* blobs deleted */
        {false, false, false, 0U, false}, /* temporary debris cleaned */
    };

    for (size_t index = 0U; index < sizeof(cuts) / sizeof(cuts[0]); ++index) {
        fixture_t fixture = {
            .reset_state = FACTORY_RESET_STATE_PENDING,
            .credentials_present = cuts[index].credentials_present,
            .settings_present = cuts[index].settings_present,
            .session_active = cuts[index].session_active,
            .blobs_remaining = cuts[index].blobs_remaining,
            .temporary_present = cuts[index].temporary_present,
        };
        assert_pending_blocks_old_authority(&fixture);
        simulate_reboot(&fixture);
        recover_to_completion(&fixture);

        const factory_reset_recovery_ops_t operations = recovery_ops(&fixture);
        bool recovered = true;
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             factory_reset_recovery_engine_run(&operations, &recovered));
        TEST_CHECK(!recovered);
        assert_fully_unprovisioned(&fixture);
    }
}

static void assert_recovery_failure_retries(recovery_failure_t failure) {
    fixture_t fixture = provisioned_fixture();
    fixture.reset_state = FACTORY_RESET_STATE_PENDING;
    simulate_reboot(&fixture);
    fixture.recovery_failure = failure;
    const factory_reset_recovery_ops_t operations = recovery_ops(&fixture);
    bool recovered = true;

    TEST_CHECK_APP_ERROR(APP_ERROR_IO, factory_reset_recovery_engine_run(&operations, &recovered));
    TEST_CHECK(!recovered);
    assert_pending_blocks_old_authority(&fixture);

    fixture.recovery_failure = RECOVERY_FAIL_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         factory_reset_recovery_engine_run(&operations, &recovered));
    TEST_CHECK(recovered);
    assert_fully_unprovisioned(&fixture);
}

static void test_recovery_stage_failure_matrix_retries_safely(void) {
    assert_recovery_failure_retries(RECOVERY_FAIL_SETTINGS_INIT);
    assert_recovery_failure_retries(RECOVERY_FAIL_ERASE_SETTINGS);
    assert_recovery_failure_retries(RECOVERY_FAIL_SETTINGS_DEINIT);
    assert_recovery_failure_retries(RECOVERY_FAIL_STORAGE_MOUNT);
    assert_recovery_failure_retries(RECOVERY_FAIL_DELETE_BLOBS);
    assert_recovery_failure_retries(RECOVERY_FAIL_CLEANUP_TEMPORARY);
    assert_recovery_failure_retries(RECOVERY_FAIL_STORAGE_UNMOUNT);
    assert_recovery_failure_retries(RECOVERY_FAIL_CLEAR_PENDING);
}

int main(void) {
    test_precommit_failure_is_not_accepted_or_destructive();
    test_live_failure_matrix_recovers_after_reboot();
    test_restart_timer_arm_failure_uses_immediate_reboot_fail_safe();
    test_simulated_reboot_between_every_destructive_stage();
    test_recovery_stage_failure_matrix_retries_safely();
    puts("factory reset failure matrix tests passed");
    return EXIT_SUCCESS;
}
