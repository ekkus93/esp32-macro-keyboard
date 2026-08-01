static void test_press_release_and_final_release_errors(void)
{
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
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY,
                         executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    executor_assert_terminal(&engine,
                             EXECUTION_FAILED,
                             APP_ERROR_USB_NOT_READY,
                             false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.press_result = APP_ERROR_IO;
    fake.release_results[1] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, executor_execute_queued(&engine, &fake));
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, engine.status.release_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, engine.status.error);
    TEST_CHECK_EQ_U64(2U, fake.release_index);
}

static void test_recovery_after_each_terminal_outcome(void)
{
    static const app_error_code_t primary_results[] = {
        APP_ERROR_NONE,
        APP_ERROR_EXECUTION_CANCELLED,
        APP_ERROR_IO,
        APP_ERROR_TIMEOUT,
    };
    for (size_t case_index = 0U;
         case_index < (sizeof(primary_results) / sizeof(primary_results[0]));
         ++case_index) {
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
        TEST_CHECK_APP_ERROR(primary_results[case_index],
                             executor_execute_queued(&engine, &fake));
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

static void test_cancel_and_status_lock_failures(void)
{
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
static void test_key_release_failure_after_success(void)
{
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
    TEST_CHECK_EQ_U64(2U, fake.release_index);
}

/* FIX1 §12.3: when the terminal state cannot be published, the engine latches
 * unavailable and rejects new submissions rather than appearing falsely idle. */
static void test_terminal_publish_failure_leaves_executor_unavailable(void)
{
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

void executor_run_terminal_tests(void)
{
    test_press_release_and_final_release_errors();
    test_recovery_after_each_terminal_outcome();
    test_cancel_and_status_lock_failures();
    test_key_release_failure_after_success();
    test_terminal_publish_failure_leaves_executor_unavailable();
}
