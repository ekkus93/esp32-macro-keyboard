#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "esp_timer.h"
#include "unity.h"

/* PBKDF2 iteration count baked into the fixed known-answer vector below. */
#define TEST_VECTOR_ITERATIONS 120000U
#define PBKDF2_BENCHMARK_SAMPLE_COUNT 10U

static void sort_timings(int64_t *timings, size_t count) {
    for (size_t index = 1U; index < count; ++index) {
        const int64_t value = timings[index];
        size_t insertion = index;
        while (insertion > 0U && timings[insertion - 1U] > value) {
            timings[insertion] = timings[insertion - 1U];
            --insertion;
        }
        timings[insertion] = value;
    }
}

TEST_CASE("authentication adapters create and validate secrets", "[device][auth]") {
    const app_error_code_t init_result = auth_init();
    TEST_ASSERT_TRUE(init_result == APP_ERROR_NONE || init_result == APP_ERROR_CONFLICT);

    static const char password[] = "correct horse battery staple";
    auth_password_record_t record = {0};
    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      auth_password_create(password, sizeof(password) - 1U, &record));
    bool matches = false;
    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      auth_password_verify(password, sizeof(password) - 1U, &record, &matches));
    TEST_ASSERT_TRUE(matches);
    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      auth_password_verify("incorrect password", strlen("incorrect password"),
                                           &record, &matches));
    TEST_ASSERT_FALSE(matches);

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
    auth_password_record_t vector = {.iterations = TEST_VECTOR_ITERATIONS};
    memcpy(vector.salt, vector_salt, sizeof(vector_salt));
    memcpy(vector.hash, vector_hash, sizeof(vector_hash));
    TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                      auth_password_verify(password, sizeof(password) - 1U, &vector, &matches));
    TEST_ASSERT_TRUE(matches);

    auth_session_view_t session = {0};
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, auth_session_create(&session));
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, auth_session_validate(session.session_token));
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, auth_session_logout(session.session_token));
    TEST_ASSERT_EQUAL(APP_ERROR_AUTH_REQUIRED, auth_session_validate(session.session_token));

    memset(&record, 0, sizeof(record));
    memset(&vector, 0, sizeof(vector));
    memset(&session, 0, sizeof(session));
}

TEST_CASE("PBKDF2 candidate timings are reported", "[device][auth][benchmark]") {
    const app_error_code_t init_result = auth_init();
    TEST_ASSERT_TRUE(init_result == APP_ERROR_NONE || init_result == APP_ERROR_CONFLICT);

    static const char benchmark_password[] = "phase1-benchmark-password";
    static const uint32_t candidates[] = {60000U, 90000U, 120000U, 150000U};

    for (size_t candidate_index = 0U;
         candidate_index < sizeof(candidates) / sizeof(candidates[0]);
         ++candidate_index) {
        auth_password_record_t record = {.iterations = candidates[candidate_index]};
        for (size_t index = 0U; index < sizeof(record.salt); ++index) {
            record.salt[index] = (uint8_t)index;
        }

        int64_t timings[PBKDF2_BENCHMARK_SAMPLE_COUNT] = {0};
        for (size_t sample = 0U; sample < PBKDF2_BENCHMARK_SAMPLE_COUNT; ++sample) {
            bool matches = true;
            const int64_t started_us = esp_timer_get_time();
            TEST_ASSERT_EQUAL(APP_ERROR_NONE,
                              auth_password_verify(benchmark_password,
                                                   sizeof(benchmark_password) - 1U, &record,
                                                   &matches));
            timings[sample] = esp_timer_get_time() - started_us;
            TEST_ASSERT_FALSE(matches);
        }

        sort_timings(timings, PBKDF2_BENCHMARK_SAMPLE_COUNT);
        const size_t median_index = PBKDF2_BENCHMARK_SAMPLE_COUNT / 2U;
        const size_t p90_index = (PBKDF2_BENCHMARK_SAMPLE_COUNT * 9U) / 10U - 1U;
        const size_t worst_index = PBKDF2_BENCHMARK_SAMPLE_COUNT - 1U;
        (void)printf("PBKDF2_BENCH iterations=%" PRIu32 " samples=%u median_us=%" PRId64
                     " p90_us=%" PRId64 " worst_us=%" PRId64 "\n",
                     record.iterations, (unsigned int)PBKDF2_BENCHMARK_SAMPLE_COUNT,
                     timings[median_index], timings[p90_index], timings[worst_index]);
        memset(&record, 0, sizeof(record));
        memset(timings, 0, sizeof(timings));
    }
}
