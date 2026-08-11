static void test_engine_and_request_validation(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    macro_executor_ops_t ops = executor_make_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(NULL, &ops));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(&engine, NULL));
    ops.lock = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(&engine, &ops));
    ops = executor_make_ops(&fake);
    ops.record_release_failure = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_init(&engine, &ops));

    executor_init_engine(&engine, &fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(&engine, NULL));

    macro_execution_request_t request = executor_make_request(1U);
    free(request.plan.actions);
    request.plan.actions = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));

    request = executor_make_request(0U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.plan.action_count = APP_COMPILED_ACTION_MAX + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.plan.estimated_duration_ms = APP_ESTIMATED_DURATION_MAX_MS + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    /* SPEC_V2 §7.11 documents the complete 0-10,000 ms range for keyPressMs;
     * key_press_ms == 0 is a valid "no dwell" request (see
     * test_zero_key_press_ms_is_accepted below) and must not be rejected here.
     * Only the upper bound is a validation error. */
    request = executor_make_request(1U);
    request.key_press_ms = APP_DELAY_MAX_MS + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.inter_key_ms = APP_DELAY_MAX_MS + 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.execution_id.value[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);
    request = executor_make_request(1U);
    request.set_id.value[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);
    request = executor_make_request(1U);
    request.macro_id.value[0] = 'x';
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);
    request = executor_make_request(1U);
    request.macro_revision = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_submit(&engine, &request));
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    fake.usb_is_ready = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_submit(NULL, &request));
    executor_free_unowned_request(&request);

    /* macro_executor_engine_execute() re-validates independently of submit():
     * engine == NULL and an invalid request are both rejected before any
     * queued/owned state is touched, so the caller keeps ownership of the
     * plan on both paths. */
    request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_execute(NULL, &request));
    TEST_CHECK(request.plan.actions != NULL);
    executor_free_unowned_request(&request);

    request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    request.macro_revision = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_executor_engine_execute(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    executor_free_unowned_request(&request);
}

static void test_cancel_confirm_null_argument_and_unavailable_and_double_cancel(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_cancel(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_executor_engine_confirm(NULL));

    /* A latched-unavailable engine rejects both cancel() and confirm() in the
     * executor/internal error domain -- never the unrelated storage-unavailable
     * code (post-v2 H7-072) -- without touching the lock at all. */
    atomic_store(&engine.unavailable, true);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_cancel(&engine));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_confirm(&engine));
    atomic_store(&engine.unavailable, false);

    /* SPEC_V2 §7.12's confirm() idempotent-conflict convention (already
     * covered by test_confirmation_granted_runs_the_macro_and_is_idempotent())
     * has a cancel() analogue: a second cancel() on the same send is a
     * conflict, not success. */
    executor_submit_single_key(&engine, &fake, 4U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(&engine));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, macro_executor_engine_cancel(&engine));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);
}

/* A failed unlock() inside cancel() itself -- after cancellation_requested has
 * already been set true -- is reported back to the caller and skips
 * notify_executor(), but the flag stays set (nothing rolls it back). */
static void test_cancel_unlock_failure_is_reported(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);

    /* fake_call_log_fail_on()'s occurrence counter starts from this call, not
     * from executor_fake_reset() -- submit()'s own unlock already happened
     * above and does not count. cancel()'s own unlock is the 1st occurrence
     * from here. */
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_cancel(&engine));
    TEST_CHECK(engine.cancellation_requested);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "notify") != 0);
    }
    executor_clear_injected_failure(&fake);

    /* This test never runs the queued request through execute() -- it only
     * exercises cancel()'s own unlock-failure branch -- so the plan submit()
     * handed to the (fake) queue is otherwise never freed. */
    executor_free_unowned_request(&fake.queued);
}

/* SPEC 24.3 item: busy rejection */
static void test_submission_ownership_and_recovery(void) {
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
    /* This lock failure happens before "busy" is ever claimed for the request:
     * nothing was pressed, and SPEC_V2 §7.3's release-all triggers do not
     * include an outright rejection before submission is accepted. */
    TEST_CHECK_EQ_U64(0U, fake.release_index);
    executor_clear_injected_failure(&fake);

    /* SPEC_V2 §7.3 names "internal error" among the release-all triggers: once
     * "busy" is claimed, every abort path attempts release-all even though no
     * action for this particular request ever ran. */
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    executor_clear_injected_failure(&fake);

    /* SPEC_V2 §7.3 names "queue failure" explicitly. */
    fake.queue_result = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);
    TEST_CHECK_EQ_U64(2U, fake.release_index);

    fake.queue_result = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions == NULL);
    TEST_CHECK(engine.busy);

    macro_execution_request_t second = executor_make_request(1U);
    second.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    macro_action_t *const second_actions = second.plan.actions;
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTOR_BUSY, macro_executor_engine_submit(&engine, &second));
    TEST_CHECK(second.plan.actions == second_actions);
    executor_free_unowned_request(&second);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(1U, fake.free_count);
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);

    executor_submit_single_key(&engine, &fake, 6U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.free_count);
}

/* Post-v2 F-009 / Round 2 F-025: submission cleanup must not discard a failed
 * release-all. The initiating submit error stays primary, while get_status()
 * exposes the release failure separately even though no macro action ran. */
static void test_submission_cleanup_release_errors_are_published(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    fake.release_results[0] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 1U;
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    executor_clear_injected_failure(&fake);
    macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, status.error);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, status.release_error);
    TEST_CHECK(!status.available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, fake.last_release_failure);
    TEST_CHECK_EQ_STRING("none", status.current_action);

    /* H7-071/H7-073: once defensive release cannot prove a safe HID state, a
     * subsequent send is rejected before its plan ownership can transfer. */
    macro_execution_request_t rejected = executor_make_request(1U);
    rejected.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 6U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &rejected));
    TEST_CHECK(rejected.plan.actions != NULL);
    executor_free_unowned_request(&rejected);
    executor_free_unowned_request(&request);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    fake.queue_result = false;
    fake.release_results[0] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_FAILED);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, status.error);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, status.release_error);
    TEST_CHECK(!status.available);
    TEST_CHECK_EQ_U64(1U, fake.release_failure_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, fake.last_release_failure);
    TEST_CHECK_EQ_STRING("none", status.current_action);
    executor_free_unowned_request(&request);
}

/* PHASE_5_EXACT_V2_HTTP_API_2026-08-08.md Known gap #4: macro_compile_v2()
 * documents and accepts the complete 0-10,000 ms keyPressMs range, but the
 * executor's own validate_request() used to reject 0 as if it were still
 * bound by the retired v1 minimum of 1 ms. A keyPressMs of 0 must be accepted
 * and must execute with no press-duration delay. */
static void test_zero_key_press_ms_is_accepted(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    request.key_press_ms = 0U;
    request.inter_key_ms = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK(request.plan.actions == NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
    /* Neither the press-duration nor inter-key delay slice ran: only the
     * unconditional per-action release_all and the final terminal release_all. */
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    TEST_CHECK_EQ_U64(0U, fake.wait_count);
}

static void test_strict_get_status_sequence(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    /*
     * Strict fake enforcement (UNIT_TESTS1 L915): reading engine status must take
     * exactly one lock/unlock pair on the executor's operations and touch nothing
     * else. Strict mode aborts on any unexpected, missing, or out-of-order call.
     */
    static const char *const expected[] = {"lock", "unlock"};
    fake_call_log_set_strict(&fake.calls, true);
    for (size_t index = 0U; index < (sizeof(expected) / sizeof(expected[0])); ++index) {
        fake_call_log_expect(&fake.calls, expected[index]);
    }

    const macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    fake_call_log_verify(&fake.calls);
    TEST_CHECK(status.state == EXECUTION_IDLE);
}

void executor_run_validation_tests(void) {
    test_engine_and_request_validation();
    test_submission_ownership_and_recovery();
    test_submission_cleanup_release_errors_are_published();
    test_zero_key_press_ms_is_accepted();
    test_strict_get_status_sequence();
    test_cancel_confirm_null_argument_and_unavailable_and_double_cancel();
    test_cancel_unlock_failure_is_reported();
}
