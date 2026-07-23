static void test_action_order_delay_and_status_progress(void)
{
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(3U);
    request.plan.actions[0] = (macro_action_t){
        .type = MACRO_ACTION_CHORD,
        .modifiers = 2U,
        .usage = 4U,
    };
    request.plan.actions[1] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 25U};
    request.plan.actions[2] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    request.plan.estimated_duration_ms = 71U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));

    executor_assert_relevant_call(&fake, 0U, "press", 2U);
    executor_assert_relevant_call(&fake, 1U, "wait", 8U);
    executor_assert_relevant_call(&fake, 2U, "release", 0U);
    executor_assert_relevant_call(&fake, 3U, "wait", 10U);
    executor_assert_relevant_call(&fake, 4U, "wait", 5U);
    executor_assert_relevant_call(&fake, 5U, "wait", 10U);
    executor_assert_relevant_call(&fake, 6U, "wait", 10U);
    executor_assert_relevant_call(&fake, 7U, "wait", 5U);
    executor_assert_relevant_call(&fake, 8U, "press", 0U);
    executor_assert_relevant_call(&fake, 9U, "wait", 8U);
    executor_assert_relevant_call(&fake, 10U, "release", 1U);
    executor_assert_relevant_call(&fake, 11U, "wait", 10U);
    executor_assert_relevant_call(&fake, 12U, "wait", 5U);
    executor_assert_relevant_call(&fake, 13U, "release", 2U);
    TEST_CHECK_EQ_U64(3U, fake.release_index);

    size_t last_index = 0U;
    bool saw_zero = false;
    bool saw_one = false;
    bool saw_two = false;
    for (size_t index = 0U; index < fake.snapshot_count; ++index) {
        const size_t action_index = fake.snapshots[index].action_index;
        TEST_CHECK(action_index >= last_index);
        last_index = action_index;
        saw_zero = saw_zero || action_index == 0U;
        saw_one = saw_one || action_index == 1U;
        saw_two = saw_two || action_index == 2U;
    }
    TEST_CHECK(saw_zero && saw_one && saw_two);
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
}

static void test_cancel_before_and_during_actions(void)
{
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(&engine));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED,
                         executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(0U, fake.wait_count);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    executor_assert_terminal(&engine,
                             EXECUTION_CANCELLED,
                             APP_ERROR_EXECUTION_CANCELLED,
                             false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED,
                         executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    TEST_CHECK_EQ_U64(2U, fake.release_index);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 100U};
    request.plan.estimated_duration_ms = 100U;
    fake.cancel_on_wait = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED,
                         executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.wait_count);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
}

static void test_watchdog_and_wait_failures(void)
{
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 2000U};
    request.plan.estimated_duration_ms = 1U;
    fake.extra_advance_on_wait_ms = 2000U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, executor_execute_queued(&engine, &fake));
    TEST_CHECK(fake.wait_count > 0U);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_TIMEOUT, false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.wait_failure_on = 2U;
    fake.wait_failure_result = APP_ERROR_IO;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_IO, false);
}

void executor_run_execution_tests(void)
{
    test_action_order_delay_and_status_progress();
    test_cancel_before_and_during_actions();
    test_watchdog_and_wait_failures();
}
