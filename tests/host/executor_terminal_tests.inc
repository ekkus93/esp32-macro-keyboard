/* SPEC 11.3: "After every normal key or chord action, firmware MUST emit a
 * release-all report", and on any failure "firmware MUST attempt a release-all
 * report and transition the execution to a terminal state."
 *
 * The release count is 2 in every case below, including the ones where the
 * press itself failed and where the release itself failed: the report is
 * attempted regardless, because a key the host believes is held down repeats
 * into whatever has focus until something releases it. A failed release is
 * recorded separately in release_error so it cannot be mistaken for the
 * primary cause of the run failing. */
/* SPEC 24.3 item: final release-all on every terminal path */
static void test_press_release_and_final_release_errors(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.press_result = APP_ERROR_IO;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_IO, false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.release_results[0] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_USB_NOT_READY, false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.press_result = APP_ERROR_IO;
    fake.release_results[1] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, executor_execute_queued(&engine, &fake));
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, engine.status.release_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, engine.status.error);
    TEST_CHECK(!macro_executor_engine_get_status(&engine).available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);
    TEST_CHECK_EQ_U64(2U, fake.release_index);

    /* H7-073: the action press itself is the primary failure and that action's
     * immediate defensive release also fails. The later terminal release
     * succeeds, so this specifically proves the first release failure is not
     * overwritten or forgotten. */
    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.press_result = APP_ERROR_IO;
    fake.release_results[0] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, executor_execute_queued(&engine, &fake));
    const macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, status.error);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, status.release_error);
    TEST_CHECK(!status.available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, fake.last_release_failure);
    TEST_CHECK_EQ_U64(2U, fake.release_index);
}

static void test_recovery_after_each_terminal_outcome(void) {
    static const app_error_code_t primary_results[] = {
        APP_ERROR_NONE,
        APP_ERROR_EXECUTION_CANCELLED,
        APP_ERROR_IO,
        APP_ERROR_TIMEOUT,
    };
    for (size_t case_index = 0U;
         case_index < (sizeof(primary_results) / sizeof(primary_results[0])); ++case_index) {
        executor_fake_t fake;
        executor_fake_reset(&fake);
        macro_executor_engine_t engine;
        executor_init_engine(&engine, &fake);
        executor_submit_single_key(&engine, &fake, 4U);
        if (primary_results[case_index] == APP_ERROR_EXECUTION_CANCELLED) {
            TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(&engine));
        } else if (primary_results[case_index] == APP_ERROR_IO) {
            fake.press_result = APP_ERROR_IO;
        } else if (primary_results[case_index] == APP_ERROR_TIMEOUT) {
            fake.wait_failure_on = 1U;
            fake.wait_failure_result = APP_ERROR_TIMEOUT;
        }
        TEST_CHECK_APP_ERROR(primary_results[case_index], executor_execute_queued(&engine, &fake));
        TEST_CHECK(!engine.busy);
        TEST_CHECK(!engine.cancellation_requested);
        TEST_CHECK_EQ_U64(1U, fake.free_count);
        /* Every terminal outcome (success, cancellation, failure, timeout)
         * clears current_action and stamps completed_ms via finish_execution. */
        TEST_CHECK_EQ_STRING("none", engine.status.current_action);
        TEST_CHECK_EQ_U64(fake.now_ms, engine.status.completed_ms);

        fake.press_result = APP_ERROR_NONE;
        fake.wait_failure_on = 0U;
        fake.wait_failure_result = APP_ERROR_NONE;
        executor_submit_single_key(&engine, &fake, 5U);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
        TEST_CHECK_EQ_U64(2U, fake.free_count);
        executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
    }
}

static void test_cancel_and_status_lock_failures(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, macro_executor_engine_cancel(&engine));

    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_cancel(&engine));
    executor_clear_injected_failure(&fake);

    const macro_execution_status_t null_status = macro_executor_engine_get_status(NULL);
    TEST_CHECK(null_status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, null_status.error);
    /* The fail-closed default status must still carry the fixed-vocabulary
     * current_action string, never a NULL pointer (FIX1 §12.4). */
    TEST_CHECK_EQ_STRING("none", null_status.current_action);

    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    const macro_execution_status_t failed_status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(failed_status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, failed_status.error);
    TEST_CHECK_EQ_STRING("none", failed_status.current_action);
}

/* FIX1 §12.4: a key release that fails only after an otherwise successful run is
 * surfaced through release_error while the run still completes. */
static void test_key_release_failure_after_success(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    /* The per-action release (index 0) succeeds; the post-run release_all
     * (index 1) in finish_execution fails. */
    fake.release_results[0] = APP_ERROR_NONE;
    fake.release_results[1] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, engine.status.release_error);
    TEST_CHECK(!macro_executor_engine_get_status(&engine).available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, fake.last_release_failure);
    TEST_CHECK_EQ_U64(2U, fake.release_index);
}

/* H7-073: cancellation and timeout remain the primary terminal outcome even if
 * the corresponding defensive release-all also fails. Both cases must latch
 * availability false and retain the release error separately. */
static void test_cancel_timeout_release_failure_latches_unavailable(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(&engine));
    fake.release_results[0] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));
    macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_CANCELLED);
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, status.error);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, status.release_error);
    TEST_CHECK(!status.available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.wait_failure_on = 1U;
    fake.wait_failure_result = APP_ERROR_TIMEOUT;
    fake.release_results[0] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, executor_execute_queued(&engine, &fake));
    status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_TIMED_OUT);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, status.error);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, status.release_error);
    TEST_CHECK(!status.available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);
}

/* H7-071/H7-073: a release fault cannot be cleared by ordinary submission. The
 * engine only clears the latch at lifecycle re-init, and even then the normal
 * USB readiness gate still prevents accepting work until the higher-level HID
 * transport has actually been re-established. */
static void test_release_fault_recovery_requires_reinit_and_ready_usb(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.release_results[1] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    TEST_CHECK(!macro_executor_engine_get_status(&engine).available);

    macro_execution_request_t rejected = executor_make_request(1U);
    rejected.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &rejected));
    TEST_CHECK(rejected.plan.actions != NULL);
    executor_free_unowned_request(&rejected);

    /* Model a full higher-level lifecycle reinitialization whose USB transport
     * is not ready yet: the old release fault is reset, but no send is accepted. */
    fake.usb_is_ready = false;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t not_ready = executor_make_request(1U);
    not_ready.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 6U};
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY,
                         macro_executor_engine_submit(&engine, &not_ready));
    TEST_CHECK(not_ready.plan.actions != NULL);
    executor_free_unowned_request(&not_ready);

    /* Once the reinitialized HID transport reports ready, normal submission can
     * resume. The previous fault's configured release slot has already been
     * consumed, so this new lifecycle's releases succeed. */
    fake.usb_is_ready = true;
    executor_submit_single_key(&engine, &fake, 7U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
}

/* FIX1 §12.3: when the terminal state cannot be published, the engine latches
 * unavailable and rejects new submissions rather than appearing falsely idle. */
static void test_terminal_publish_failure_leaves_executor_unavailable(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    /* A single zero-length delay keeps the lock sequence fixed and free of the
     * per-slice cancellation reads a key action would add: RUNNING publish (1),
     * per-action publish (2), cancellation read (3), terminal publish (4),
     * terminal reset (5). Failing the terminal publish (lock 4) makes
     * finish_execution fail after an otherwise successful run. */
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 0U};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    fake_call_log_fail_on(&fake.calls, "lock", 4U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, executor_execute_queued(&engine, &fake));
    executor_clear_injected_failure(&fake);

    /* The lock now works again, but the engine stays latched unavailable and
     * refuses the next submission; the caller keeps ownership of its plan. */
    macro_execution_request_t rejected = executor_make_request(1U);
    rejected.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &rejected));
    TEST_CHECK(rejected.plan.actions != NULL);
    executor_free_unowned_request(&rejected);
}

/* SPEC 8.4: "The next macro in the list MUST NOT execute automatically.
 * Advancing is a display convenience only; every send is a separate explicit
 * user action." SPEC 8.4 also states the device stores no notion of a macro
 * being "done" -- no completion state, no skip state -- so there is no cursor
 * for the executor to advance and nothing for it to resume from.
 *
 * The observable form of a violation is the engine putting work on the queue
 * that no caller submitted. Every terminal outcome is checked, not just
 * success: a cancelled or failed run is exactly where a "move on to the next
 * one" convenience would be tempting. */
static void test_no_terminal_outcome_advances_to_another_macro(void) {
    static const execution_state_t outcomes[] = {EXECUTION_COMPLETED, EXECUTION_CANCELLED,
                                                 EXECUTION_FAILED};
    for (size_t index = 0U; index < sizeof(outcomes) / sizeof(outcomes[0]); ++index) {
        executor_fake_t fake;
        executor_fake_reset(&fake);
        macro_executor_engine_t engine;
        executor_init_engine(&engine, &fake);
        executor_submit_single_key(&engine, &fake, 4U);

        app_error_code_t expected = APP_ERROR_NONE;
        if (outcomes[index] == EXECUTION_CANCELLED) {
            TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(&engine));
            expected = APP_ERROR_EXECUTION_CANCELLED;
        } else if (outcomes[index] == EXECUTION_FAILED) {
            fake.wait_failure_on = 1U;
            fake.wait_failure_result = APP_ERROR_IO;
            expected = APP_ERROR_IO;
        }
        TEST_CHECK_APP_ERROR(expected, executor_execute_queued(&engine, &fake));
        executor_clear_injected_failure(&fake);

        /* The run is over and the queue is empty. Nothing re-queued itself on
         * the way out, so there is no next macro to run. */
        TEST_CHECK(!fake.has_queued);
        const size_t settled_calls = fake.calls.call_count;

        /* Draining the engine again changes nothing: no work exists to find. */
        TEST_CHECK(!fake.has_queued);
        TEST_CHECK_EQ_U64(settled_calls, fake.calls.call_count);
        TEST_CHECK(!engine.busy);

        /* A second macro runs only because the user submitted it. */
        executor_submit_single_key(&engine, &fake, 5U);
        TEST_CHECK(fake.has_queued);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
        TEST_CHECK(fake.calls.call_count > settled_calls);
        executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
        TEST_CHECK(!fake.has_queued);
    }
}

/* A lock() failure inside the main loop's own read_cancellation() call (not
 * cancellable_delay()'s) ends the run in EXECUTION_FAILED with that lock
 * failure as the primary error -- distinct from an actual cancellation. A
 * single zero-length delay keeps the lock sequence fixed, per
 * test_terminal_publish_failure_leaves_executor_unavailable()'s comment:
 * RUNNING publish (1), per-action publish (2), cancellation read (3). */
static void test_main_loop_cancellation_read_lock_failure(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 0U};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    fake_call_log_fail_on(&fake.calls, "lock", 3U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_INTERNAL, false);
}

/* A lock() failure inside reset_terminal_flags() itself (as opposed to the
 * terminal publish_status() call finish_execution() makes first) latches the
 * engine unavailable exactly as a terminal-publish failure does, but leaves
 * `busy` stuck true: reset_terminal_flags() returns before ever reaching the
 * assignments that clear busy/cancellation_requested/confirmed_requested.
 * Lock ordinal 5 is reset_terminal_flags()'s own lock, per the same fixed
 * zero-length-delay sequence (... terminal publish (4), terminal reset (5)). */
static void test_reset_terminal_flags_lock_failure_leaves_engine_stuck_busy(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 0U};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    fake_call_log_fail_on(&fake.calls, "lock", 5U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, executor_execute_queued(&engine, &fake));

    /* The terminal publish (lock 4) succeeded, so status reflects the
     * completed run even though the busy flag never got cleared. */
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, true);
    executor_clear_injected_failure(&fake);

    macro_execution_request_t rejected = executor_make_request(1U);
    rejected.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &rejected));
    executor_free_unowned_request(&rejected);
}

/* macro_executor_engine_get_status()'s own unlock() failure (as opposed to a
 * lock() failure, already covered by test_cancel_and_status_lock_failures())
 * forces the fail-closed FAILED/INTERNAL result even though the status read
 * itself succeeded. */
static void test_get_status_unlock_failure_is_fail_closed(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    const macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, status.error);
    TEST_CHECK_EQ_STRING("none", status.current_action);
}

void executor_run_terminal_tests(void) {
    test_no_terminal_outcome_advances_to_another_macro();
    test_press_release_and_final_release_errors();
    test_recovery_after_each_terminal_outcome();
    test_cancel_and_status_lock_failures();
    test_key_release_failure_after_success();
    test_cancel_timeout_release_failure_latches_unavailable();
    test_release_fault_recovery_requires_reinit_and_ready_usb();
    test_terminal_publish_failure_leaves_executor_unavailable();
    test_main_loop_cancellation_read_lock_failure();
    test_reset_terminal_flags_lock_failure_leaves_engine_stuck_busy();
    test_get_status_unlock_failure_is_fail_closed();
}
