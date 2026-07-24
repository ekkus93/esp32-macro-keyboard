typedef struct {
    const char *missing_header;
    bool invalid_origin;
    app_error_code_t validation_result;
    size_t validation_calls;
    char call_order[64U];
    size_t call_order_length;
} authorization_fake_t;

static void append_call(authorization_fake_t *fake, const char *name)
{
    if (fake->call_order_length > 0U) {
        fake->call_order[fake->call_order_length++] = ',';
    }
    const size_t length = strlen(name);
    TEST_CHECK(fake->call_order_length + length < sizeof(fake->call_order));
    memcpy(fake->call_order + fake->call_order_length, name, length + 1U);
    fake->call_order_length += length;
}

static app_error_code_t get_authorization_header(void *context,
                                                  const char *name,
                                                  char *output,
                                                  size_t output_size)
{
    authorization_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    append_call(fake, name);
    if (fake->missing_header != NULL && strcmp(fake->missing_header, name) == 0) {
        output[0] = '\0';
        return APP_ERROR_AUTH_REQUIRED;
    }
    const char *value = NULL;
    if (strcmp(name, "Host") == 0) {
        value = "192.168.4.1";
    } else if (strcmp(name, "Origin") == 0) {
        value = fake->invalid_origin ? "http://attacker.invalid" : "http://192.168.4.1";
    } else if (strcmp(name, "Cookie") == 0) {
        value = "MKSESSION=" TOKEN;
    } else if (strcmp(name, "X-CSRF-Token") == 0) {
        value = TOKEN;
    } else {
        TEST_CHECK(false);
    }
    TEST_CHECK(strlen(value) < output_size);
    memcpy(output, value, strlen(value) + 1U);
    return APP_ERROR_NONE;
}

static app_error_code_t validate_authorization(void *context,
                                                const char *session_token,
                                                const char *csrf_token)
{
    authorization_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK_EQ_STRING(TOKEN, session_token);
    TEST_CHECK_EQ_STRING(TOKEN, csrf_token);
    ++fake->validation_calls;
    return fake->validation_result;
}

static void test_mutation_authorization(void)
{
    char session_token[AUTH_TOKEN_HEX_BYTES];
    authorization_fake_t fake = {.validation_result = APP_ERROR_NONE};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_authorize_mutation(get_authorization_header,
                                                        validate_authorization,
                                                        &fake,
                                                        session_token,
                                                        sizeof(session_token)));
    TEST_CHECK_EQ_STRING(TOKEN, session_token);
    TEST_CHECK_EQ_STRING("Host,Origin,Cookie,X-CSRF-Token", fake.call_order);
    TEST_CHECK_EQ_U64(1U, fake.validation_calls);

    fake = (authorization_fake_t){.invalid_origin = true};
    memcpy(session_token, "dirty", sizeof("dirty"));
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_adapter_authorize_mutation(get_authorization_header,
                                                        validate_authorization,
                                                        &fake,
                                                        session_token,
                                                        sizeof(session_token)));
    TEST_CHECK_EQ_STRING("", session_token);
    TEST_CHECK_EQ_STRING("Host,Origin", fake.call_order);
    TEST_CHECK_EQ_U64(0U, fake.validation_calls);

    fake = (authorization_fake_t){.missing_header = "Cookie"};
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_adapter_authorize_mutation(get_authorization_header,
                                                        validate_authorization,
                                                        &fake,
                                                        session_token,
                                                        sizeof(session_token)));
    TEST_CHECK_EQ_U64(0U, fake.validation_calls);

    fake = (authorization_fake_t){.validation_result = APP_ERROR_AUTH_FAILED};
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_FAILED,
                         web_adapter_authorize_mutation(get_authorization_header,
                                                        validate_authorization,
                                                        &fake,
                                                        session_token,
                                                        sizeof(session_token)));
    TEST_CHECK_EQ_STRING("", session_token);
}
