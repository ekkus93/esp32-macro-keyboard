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

    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    const macro_execution_status_t failed_status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(failed_status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, failed_status.error);
}

void executor_run_terminal_tests(void)
{
    test_press_release_and_final_release_errors();
    test_recovery_after_each_terminal_outcome();
    test_cancel_and_status_lock_failures();
}
