#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_core.h"
#include "fake_call_log.h"
#include "test_assert.h"

typedef struct {
    bool locked;
    uint64_t now_us;
    uint32_t random_state;
    bool random_fail;
    bool derive_fail;
    size_t derive_count;
    size_t zero_count;
    size_t last_zero_length;
    fake_call_log_t calls;
} auth_fake_t;

static void fake_reset(auth_fake_t *fake)
{
    memset(fake, 0, sizeof(*fake));
    fake->random_state = UINT32_C(0x12345678);
    fake_call_log_reset(&fake->calls);
}

static bool fake_lock(void *context)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(!fake->locked);
    if (fake_call_log_record(&fake->calls, "lock", 0U, 0U)) {
        return false;
    }
    fake->locked = true;
    return true;
}

static bool fake_unlock(void *context)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(fake->locked);
    const bool fail = fake_call_log_record(&fake->calls, "unlock", 0U, 0U);
    fake->locked = false;
    return !fail;
}

static bool fake_random(void *context, uint8_t *output, size_t length)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(output != NULL || length == 0U);
    (void)fake_call_log_record(&fake->calls, "random", length, fake->random_state);
    if (fake->random_fail) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        fake->random_state = fake->random_state * UINT32_C(1664525) +
                             UINT32_C(1013904223);
        output[index] = (uint8_t)(fake->random_state >> 24U);
    }
    return true;
}

static uint64_t fake_now(void *context)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "now", fake->now_us, 0U);
    return fake->now_us;
}

static int fake_derive(void *context,
                       const char *password,
                       size_t password_length,
                       const uint8_t *salt,
                       uint32_t iterations,
                       uint8_t *output)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(password != NULL);
    TEST_CHECK(password_length > 0U);
    TEST_CHECK(salt != NULL);
    TEST_CHECK(output != NULL);
    ++fake->derive_count;
    (void)fake_call_log_record(&fake->calls, "derive", password_length, iterations);
    if (fake->derive_fail) {
        return -1;
    }
    for (size_t index = 0U; index < AUTH_HASH_BYTES; ++index) {
        const uint8_t iteration_byte =
            (uint8_t)(iterations >> (uint32_t)((index % 4U) * 8U));
        output[index] = (uint8_t)((uint8_t)password[index % password_length] ^
                                  salt[index % AUTH_SALT_BYTES] ^ iteration_byte);
    }
    return 0;
}

static void fake_zero(void *context, void *memory, size_t length)
{
    auth_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(memory != NULL || length == 0U);
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
    ++fake->zero_count;
    fake->last_zero_length = length;
    (void)fake_call_log_record(&fake->calls, "zero", length, 0U);
}

static void init_core(auth_core_t *core, auth_fake_t *fake)
{
    const auth_ops_t ops = {
        .context = fake,
        .lock = fake_lock,
        .unlock = fake_unlock,
        .random_fill = fake_random,
        .now_us = fake_now,
        .derive = fake_derive,
        .secure_zero = fake_zero,
    };
    TEST_CHECK(auth_core_init(core, &ops) == APP_ERROR_NONE);
}

static void test_password_boundaries(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_password_record_t record;
    char password[130U];
    memset(password, 'a', sizeof(password));

    TEST_CHECK(auth_core_password_create(&core, password, 11U, &record) ==
               APP_ERROR_INVALID_ARGUMENT);
    TEST_CHECK(auth_core_password_create(&core, password, 12U, &record) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(120000U, record.iterations);
    static const uint8_t expected_salt[AUTH_SALT_BYTES] = {
        0x75U, 0xcdU, 0x25U, 0x4bU, 0x84U, 0xe2U, 0xeaU, 0xf2U,
        0xa6U, 0x81U, 0x20U, 0x67U, 0x43U, 0x34U, 0xb2U, 0x6eU,
    };
    TEST_CHECK_EQ_BUFFER(expected_salt, record.salt, sizeof(expected_salt));
    TEST_CHECK(auth_core_password_verify(&core, password, 12U, &record));
    password[0] = 'b';
    TEST_CHECK(!auth_core_password_verify(&core, password, 12U, &record));
    password[0] = 'a';
    TEST_CHECK(auth_core_password_create(&core, password, 128U, &record) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_password_create(&core, password, 129U, &record) ==
               APP_ERROR_INVALID_ARGUMENT);
    password[4] = '\0';
    TEST_CHECK(auth_core_password_create(&core, password, 12U, &record) ==
               APP_ERROR_INVALID_ARGUMENT);
}

static void test_derive_failures_zero_outputs(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_password_record_t record;
    memset(&record, 0xa5, sizeof(record));
    fake.derive_fail = true;
    static const char password[] = "abcdefghijkl";
    TEST_CHECK(auth_core_password_create(&core,
                                         password,
                                         sizeof(password) - 1U,
                                         &record) == APP_ERROR_INTERNAL);
    static const uint8_t zeros[sizeof(record)] = {0};
    TEST_CHECK_EQ_BUFFER(zeros, &record, sizeof(record));
    TEST_CHECK(fake.zero_count >= 1U);

    fake.derive_fail = false;
    TEST_CHECK(auth_core_password_create(&core,
                                         password,
                                         sizeof(password) - 1U,
                                         &record) == APP_ERROR_NONE);
    fake.derive_fail = true;
    const size_t zero_before = fake.zero_count;
    TEST_CHECK(!auth_core_password_verify(&core,
                                          password,
                                          sizeof(password) - 1U,
                                          &record));
    TEST_CHECK(fake.zero_count == zero_before + 1U);
    TEST_CHECK_EQ_U64(AUTH_HASH_BYTES, fake.last_zero_length);
}

static void test_sessions(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    fake.now_us = 100U;
    auth_core_t core;
    init_core(&core, &fake);
    auth_session_view_t session;
    TEST_CHECK(auth_core_session_create(&core, &session) == APP_ERROR_NONE);
    TEST_CHECK(strlen(session.session_token) == AUTH_TOKEN_HEX_BYTES - 1U);
    TEST_CHECK(strlen(session.csrf_token) == AUTH_TOKEN_HEX_BYTES - 1U);
    TEST_CHECK(strcmp(session.session_token, session.csrf_token) != 0);
    const uint64_t original_expiry = session.expires_at_us;
    TEST_CHECK(auth_core_session_validate(&core,
                                          session.session_token,
                                          session.csrf_token) == APP_ERROR_NONE);

    fake.now_us += 1000000U;
    TEST_CHECK(auth_core_session_validate(&core,
                                          session.session_token,
                                          session.csrf_token) == APP_ERROR_NONE);
    TEST_CHECK(core.sessions[0].view.expires_at_us > original_expiry);

    char wrong[AUTH_TOKEN_HEX_BYTES];
    memcpy(wrong, session.csrf_token, sizeof(wrong));
    wrong[0] = wrong[0] == '0' ? '1' : '0';
    const uint64_t expiry_before_failure = core.sessions[0].view.expires_at_us;
    TEST_CHECK(auth_core_session_validate(&core,
                                          session.session_token,
                                          wrong) == APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK_EQ_U64(expiry_before_failure, core.sessions[0].view.expires_at_us);
    TEST_CHECK(auth_core_session_validate(&core, "short", wrong) == APP_ERROR_AUTH_REQUIRED);

    TEST_CHECK(auth_core_session_logout(&core, session.session_token) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_session_logout(&core, session.session_token) == APP_ERROR_NOT_FOUND);
}

static void test_session_expiry_and_capacity(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_session_view_t sessions[APP_SESSION_TABLE_MAX];
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        TEST_CHECK(auth_core_session_create(&core, &sessions[index]) == APP_ERROR_NONE);
    }
    auth_session_view_t extra;
    TEST_CHECK(auth_core_session_create(&core, &extra) == APP_ERROR_CONFLICT);

    fake.now_us = sessions[0].expires_at_us;
    TEST_CHECK(auth_core_session_validate(&core,
                                          sessions[0].session_token,
                                          sessions[0].csrf_token) == APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK(auth_core_session_create(&core, &extra) == APP_ERROR_NONE);
}

static void test_rate_limit(void)
{
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    uint32_t retry = UINT32_MAX;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(0U, retry);
    for (size_t index = 0U; index < 5U; ++index) {
        TEST_CHECK(auth_core_login_record_failure(&core) == APP_ERROR_NONE);
    }
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_RATE_LIMITED);
    TEST_CHECK_EQ_U64(60U, retry);
    fake.now_us = 1500000U;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_RATE_LIMITED);
    TEST_CHECK_EQ_U64(59U, retry);
    fake.now_us = 60000000U;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(0U, retry);

    TEST_CHECK(auth_core_login_record_failure(&core) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_login_record_success(&core) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_NONE);

    fake.now_us = 50000000U;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_INTERNAL);
}

int main(void)
{
    test_password_boundaries();
    test_derive_failures_zero_outputs();
    test_sessions();
    test_session_expiry_and_capacity();
    test_rate_limit();
    puts("authentication tests passed");
    return EXIT_SUCCESS;
}
