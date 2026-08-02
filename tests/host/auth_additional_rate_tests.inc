/* SPEC 16.3: "Authentication failures are rate-limited by a bounded global and
 * client-aware policy. The implementation MUST avoid unbounded per-IP state."
 *
 * The bound is the point. A per-IP table is the obvious way to write throttling
 * and the wrong one here: on a device with this much RAM, an attacker who can
 * vary source addresses turns the defence into the memory exhaustion it was
 * meant to prevent. auth_core keeps fixed state instead, so what is exercised
 * below is a policy whose cost does not grow with the number of attackers. */
static void test_complete_rate_limit_matrix(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    uint32_t retry = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, &retry));
    for (size_t failure = 1U; failure <= 4U; ++failure) {
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core));
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             auth_core_login_attempt_allowed(&core, &retry));
        TEST_CHECK_EQ_U64(0U, retry);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core));
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_attempt_allowed(&core, &retry));
    TEST_CHECK_EQ_U64(60U, retry);

    fake.now_us = 500001U;
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_attempt_allowed(&core, &retry));
    TEST_CHECK_EQ_U64(60U, retry);
    fake.now_us = 1000001U;
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_attempt_allowed(&core, &retry));
    TEST_CHECK_EQ_U64(59U, retry);
    fake.now_us = 60000000U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, &retry));

    core.failure_window_active = true;
    core.failure_window_start_us = fake.now_us;
    core.failure_count = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core));
    TEST_CHECK_EQ_U64(UINT32_MAX, core.failure_count);

    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    retry = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         auth_core_login_attempt_allowed(&core, &retry));
    TEST_CHECK_EQ_U64(0U, retry);
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;

    const uint32_t count_before_unlock = core.failure_count;
    const uint64_t start_before_unlock = core.failure_window_start_us;
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, auth_core_login_record_failure(&core));
    TEST_CHECK_EQ_U64(count_before_unlock, core.failure_count);
    TEST_CHECK_EQ_U64(start_before_unlock, core.failure_window_start_us);
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;

    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, auth_core_login_record_success(&core));
    TEST_CHECK_EQ_U64(count_before_unlock, core.failure_count);
    TEST_CHECK_EQ_U64(start_before_unlock, core.failure_window_start_us);
}

