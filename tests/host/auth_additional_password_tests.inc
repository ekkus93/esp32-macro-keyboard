static void test_null_inputs_and_known_vector(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    auth_ops_t ops = {
        .context = &fake,
        .lock = fake_lock,
        .unlock = fake_unlock,
        .random_fill = fake_random,
        .now_us = fake_now,
        .derive = fake_derive,
        .secure_zero = fake_zero,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, auth_core_init(NULL, &ops));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, auth_core_init(&core, NULL));
    ops.derive = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, auth_core_init(&core, &ops));
    init_core(&core, &fake);

    static const char password[] = "correct horse battery staple";
    auth_password_record_t record = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         auth_core_password_create(NULL,
                                                   password,
                                                   sizeof(password) - 1U,
                                                   &record));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         auth_core_password_create(&core,
                                                   NULL,
                                                   sizeof(password) - 1U,
                                                   &record));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         auth_core_password_create(&core,
                                                   password,
                                                   sizeof(password) - 1U,
                                                   NULL));
    TEST_CHECK(!auth_core_password_verify(NULL,
                                          password,
                                          sizeof(password) - 1U,
                                          &record));
    TEST_CHECK(!auth_core_password_verify(&core,
                                          NULL,
                                          sizeof(password) - 1U,
                                          &record));
    TEST_CHECK(!auth_core_password_verify(&core,
                                          password,
                                          sizeof(password) - 1U,
                                          NULL));

    memset(&record, 0xa5, sizeof(record));
    fake.random_fail = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         auth_core_password_create(&core,
                                                   password,
                                                   sizeof(password) - 1U,
                                                   &record));
    static const uint8_t zero_record[sizeof(record)] = {0};
    TEST_CHECK_EQ_BUFFER(zero_record, &record, sizeof(record));
    fake.random_fail = false;

    static const uint8_t vector_salt[AUTH_SALT_BYTES] = {
        0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
        0x08U, 0x09U, 0x0aU, 0x0bU, 0x0cU, 0x0dU, 0x0eU, 0x0fU,
    };
    static const uint8_t vector_hash[AUTH_HASH_BYTES] = {
        0xe9U, 0x62U, 0xebU, 0xd8U, 0x26U, 0x7bU, 0xc8U, 0x39U,
        0x38U, 0x6dU, 0x46U, 0x08U, 0xbbU, 0xc3U, 0xc8U, 0xacU,
        0x36U, 0xbfU, 0xb2U, 0x15U, 0xfaU, 0xa8U, 0x54U, 0x4eU,
        0x3aU, 0x4eU, 0x2cU, 0xbcU, 0xceU, 0xc8U, 0x48U, 0x06U,
    };
    memcpy(record.salt, vector_salt, sizeof(vector_salt));
    memcpy(record.hash, vector_hash, sizeof(vector_hash));
    record.iterations = 120000U;
    fake.known_vector_derive = true;
    TEST_CHECK(auth_core_password_verify(&core,
                                         password,
                                         sizeof(password) - 1U,
                                         &record));
}

static void test_password_verification_boundaries(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    char password[130U];
    memset(password, 'p', sizeof(password));
    auth_password_record_t record = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_password_create(&core, password, 12U, &record));
    TEST_CHECK(!auth_core_password_verify(&core, password, 11U, &record));
    TEST_CHECK(!auth_core_password_verify(&core, password, 129U, &record));
    password[5] = '\0';
    TEST_CHECK(!auth_core_password_verify(&core, password, 12U, &record));
    password[5] = 'p';
    record.iterations = 119999U;
    TEST_CHECK(!auth_core_password_verify(&core, password, 12U, &record));
}

