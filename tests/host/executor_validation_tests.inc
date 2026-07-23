static void test_engine_and_request_validation(void)
{
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    macro_executor_ops_t ops = executor_make_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(NULL, &ops));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(&engine, NULL));
    ops.lock = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(&engine, &ops));

    executor_init_engine(&engine, &fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, NULL));

    macro_execution_request_t request = executor_make_request(1U);
    free(request.plan.actions);
    request.plan.actions = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));

    request = executor_make_request(0U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.plan.action_count = APP_COMPILED_ACTION_MAX + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.plan.estimated_duration_ms = APP_ESTIMATED_DURATION_MAX_MS + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.key_press_ms = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    request.key_press_ms = APP_DELAY_MAX_MS + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.inter_key_ms = APP_DELAY_MAX_MS + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.execution_id.value[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);
    request = executor_make_request(1U);
    request.set_id.value[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);
    request = executor_make_request(1U);
    request.macro_id.value[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);
    request = executor_make_request(1U);
    request.macro_revision = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    fake.usb_is_ready = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    executor_free_unowned_request(&request);
}

static void test_submission_ownership_and_recovery(void)
{
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);
    executor_clear_injected_failure(&fake);

    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);
    executor_clear_injected_failure(&fake);

    fake.queue_result = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);

    fake.queue_result = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions == NULL);
    TEST_CHECK(engine.busy);

    macro_execution_request_t second = executor_make_request(1U);
    second.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    macro_action_t *const second_actions = second.plan.actions;
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTOR_BUSY,
                         macro_executor_engine_submit(&engine, &second));
    TEST_CHECK(second.plan.actions == second_actions);
    executor_free_unowned_request(&second);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(1U, fake.free_count);
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);

    executor_submit_single_key(&engine, &fake, 6U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.free_count);
}

void executor_run_validation_tests(void)
{
    test_engine_and_request_validation();
    test_submission_ownership_and_recovery();
}
