#include <string.h>

#include "auth.h"
#include "unity.h"

TEST_CASE("authentication adapters create and validate secrets", "[device][auth]") {
    const app_error_code_t init_result = auth_init();
    TEST_ASSERT_TRUE(init_result == APP_ERROR_NONE || init_result == APP_ERROR_CONFLICT);

    static const char password[] = "correct horse battery staple";
    auth_password_record_t record = {0};
    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      auth_password_create(password, sizeof(password) - 1U, &record));
    TEST_ASSERT_TRUE(auth_password_verify(password, sizeof(password) - 1U, &record));
    TEST_ASSERT_FALSE(auth_password_verify(
        "incorrect password", strlen("incorrect password"), &record));

    // clang-format off
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
    // clang-format on
    auth_password_record_t vector = {.iterations = 120000U};
    memcpy(vector.salt, vector_salt, sizeof(vector_salt));
    memcpy(vector.hash, vector_hash, sizeof(vector_hash));
    TEST_ASSERT_TRUE(auth_password_verify(password, sizeof(password) - 1U, &vector));

    auth_session_view_t session = {0};
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, auth_session_create(&session));
    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      auth_session_validate(session.session_token, session.csrf_token));
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, auth_session_logout(session.session_token));
    TEST_ASSERT_EQUAL(APP_ERROR_AUTH_REQUIRED,
                      auth_session_validate(session.session_token, session.csrf_token));

    memset(&record, 0, sizeof(record));
    memset(&vector, 0, sizeof(vector));
    memset(&session, 0, sizeof(session));
}
