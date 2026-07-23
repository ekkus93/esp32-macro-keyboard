static void mutate_token(char *token, char replacement)
{
    TEST_CHECK(token != NULL);
    token[0] = replacement;
}

static void test_session_token_formats_and_failures(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_session_view_t session = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, auth_core_session_create(NULL, &session));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, auth_core_session_create(&core, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_session_create(&core, &session));

    char wrong_session[AUTH_TOKEN_HEX_BYTES];
    char wrong_csrf[AUTH_TOKEN_HEX_BYTES];
    memcpy(wrong_session, session.session_token, sizeof(wrong_session));
    memcpy(wrong_csrf, session.csrf_token, sizeof(wrong_csrf));
    mutate_token(wrong_session, wrong_session[0] == '0' ? '1' : '0');
    mutate_token(wrong_csrf, wrong_csrf[0] == '0' ? '1' : '0');
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&core,
                                                    wrong_session,
                                                    session.csrf_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&core,
                                                    session.session_token,
                                                    wrong_csrf));

    char long_token[AUTH_TOKEN_HEX_BYTES + 1U];
    memset(long_token, 'a', sizeof(long_token) - 1U);
    long_token[sizeof(long_token) - 1U] = '\0';
    char uppercase[AUTH_TOKEN_HEX_BYTES];
    memcpy(uppercase, session.session_token, sizeof(uppercase));
    uppercase[0] = 'A';
    char non_hex[AUTH_TOKEN_HEX_BYTES];
    memcpy(non_hex, session.session_token, sizeof(non_hex));
    non_hex[0] = 'g';
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&core, "short", session.csrf_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&core, long_token, session.csrf_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&core, uppercase, session.csrf_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         auth_core_session_validate(&core, non_hex, session.csrf_token));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         auth_core_session_logout(&core, "short"));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         auth_core_session_logout(NULL, session.session_token));

    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    auth_session_view_t failed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, auth_core_session_create(&core, &failed));
    static const uint8_t zeros[sizeof(failed)] = {0};
    TEST_CHECK_EQ_BUFFER(zeros, &failed, sizeof(failed));
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;

    const auth_core_t before_create_unlock = core;
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, auth_core_session_create(&core, &failed));
    TEST_CHECK_EQ_BUFFER(zeros, &failed, sizeof(failed));
    TEST_CHECK_EQ_BUFFER(before_create_unlock.sessions,
                         core.sessions,
                         sizeof(core.sessions));
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;

    const uint64_t expiry_before_unlock = core.sessions[0].view.expires_at_us;
    fake.now_us += 1000000U;
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         auth_core_session_validate(&core,
                                                    session.session_token,
                                                    session.csrf_token));
    TEST_CHECK_EQ_U64(expiry_before_unlock, core.sessions[0].view.expires_at_us);
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;

    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         auth_core_session_logout(&core, session.session_token));
    TEST_CHECK(core.sessions[0].active);
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;
}

