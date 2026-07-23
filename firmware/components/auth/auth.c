#include "auth.h"

#include <limits.h>
#include <string.h>

#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"

#define AUTH_PBKDF2_ITERATIONS 120000U
#define AUTH_SESSION_IDLE_US (15ULL * 60ULL * 1000000ULL)
#define AUTH_MAX_FAILURES 5U
#define AUTH_FAILURE_WINDOW_US (60ULL * 1000000ULL)
#define AUTH_PASSWORD_MIN_BYTES 12U
#define AUTH_PASSWORD_MAX_BYTES 128U

typedef struct {
    bool active;
    auth_session_view_t view;
} auth_session_entry_t;

static SemaphoreHandle_t auth_mutex;
static auth_session_entry_t sessions[APP_SESSION_TABLE_MAX];
static uint32_t failure_count;
static uint64_t failure_window_start_us;

static bool lock_auth(void)
{
    return auth_mutex != NULL && xSemaphoreTake(auth_mutex, portMAX_DELAY) == pdTRUE;
}

static bool unlock_auth(void)
{
    return xSemaphoreGive(auth_mutex) == pdTRUE;
}

static bool constant_time_equal(const uint8_t *left, const uint8_t *right, size_t length)
{
    uint8_t difference = 0U;
    for (size_t index = 0U; index < length; ++index) {
        difference = (uint8_t)(difference | (uint8_t)(left[index] ^ right[index]));
    }
    return difference == 0U;
}

static bool valid_hex_token(const char *token)
{
    if (token == NULL || strlen(token) != AUTH_TOKEN_HEX_BYTES - 1U) {
        return false;
    }
    for (size_t index = 0U; index < AUTH_TOKEN_HEX_BYTES - 1U; ++index) {
        if (!((token[index] >= '0' && token[index] <= '9') ||
              (token[index] >= 'a' && token[index] <= 'f'))) {
            return false;
        }
    }
    return true;
}

static app_error_code_t random_hex(char *output, size_t output_size, size_t random_bytes)
{
    if (output == NULL || random_bytes > APP_SESSION_TOKEN_BYTES ||
        output_size < (random_bytes * 2U) + 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char digits[] = "0123456789abcdef";
    uint8_t bytes[APP_SESSION_TOKEN_BYTES];
    esp_fill_random(bytes, random_bytes);
    for (size_t index = 0U; index < random_bytes; ++index) {
        output[index * 2U] = digits[bytes[index] >> 4U];
        output[index * 2U + 1U] = digits[bytes[index] & 0x0fU];
    }
    output[random_bytes * 2U] = '\0';
    return APP_ERROR_NONE;
}

app_error_code_t auth_init(void)
{
    if (auth_mutex != NULL) {
        return APP_ERROR_CONFLICT;
    }
    auth_mutex = xSemaphoreCreateMutex();
    if (auth_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    memset(sessions, 0, sizeof(sessions));
    failure_count = 0U;
    failure_window_start_us = 0U;
    return APP_ERROR_NONE;
}

static int derive(const char *password,
                  size_t password_length,
                  const uint8_t *salt,
                  uint32_t iterations,
                  uint8_t *output)
{
    return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
                                        (const unsigned char *)password,
                                        password_length,
                                        salt,
                                        AUTH_SALT_BYTES,
                                        iterations,
                                        AUTH_HASH_BYTES,
                                        output);
}

app_error_code_t auth_password_create(const char *password,
                                      size_t password_length,
                                      auth_password_record_t *out_record)
{
    if (password == NULL || out_record == NULL || password_length < AUTH_PASSWORD_MIN_BYTES ||
        password_length > AUTH_PASSWORD_MAX_BYTES || memchr(password, '\0', password_length) != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_record, 0, sizeof(*out_record));
    esp_fill_random(out_record->salt, sizeof(out_record->salt));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    if (derive(password, password_length, out_record->salt, out_record->iterations,
               out_record->hash) != 0) {
        memset(out_record, 0, sizeof(*out_record));
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

bool auth_password_verify(const char *password,
                          size_t password_length,
                          const auth_password_record_t *record)
{
    if (password == NULL || record == NULL || password_length < AUTH_PASSWORD_MIN_BYTES ||
        password_length > AUTH_PASSWORD_MAX_BYTES || record->iterations < AUTH_PBKDF2_ITERATIONS ||
        memchr(password, '\0', password_length) != NULL) {
        return false;
    }
    uint8_t derived[AUTH_HASH_BYTES];
    if (derive(password, password_length, record->salt, record->iterations, derived) != 0) {
        return false;
    }
    const bool matches = constant_time_equal(derived, record->hash, sizeof(derived));
    memset(derived, 0, sizeof(derived));
    return matches;
}

app_error_code_t auth_session_create(auth_session_view_t *out_session)
{
    if (out_session == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!lock_auth()) {
        return APP_ERROR_INTERNAL;
    }
    auth_session_entry_t *slot = NULL;
    const uint64_t now = (uint64_t)esp_timer_get_time();
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        if (!sessions[index].active || sessions[index].view.expires_at_us <= now) {
            slot = &sessions[index];
            break;
        }
    }
    if (slot == NULL) {
        return unlock_auth() ? APP_ERROR_CONFLICT : APP_ERROR_INTERNAL;
    }
    memset(slot, 0, sizeof(*slot));
    app_error_code_t result = random_hex(slot->view.session_token,
                                         sizeof(slot->view.session_token),
                                         APP_SESSION_TOKEN_BYTES);
    if (result == APP_ERROR_NONE) {
        result = random_hex(slot->view.csrf_token,
                            sizeof(slot->view.csrf_token),
                            APP_CSRF_TOKEN_BYTES);
    }
    if (result == APP_ERROR_NONE) {
        slot->view.expires_at_us = now + AUTH_SESSION_IDLE_US;
        slot->active = true;
        *out_session = slot->view;
    } else {
        memset(slot, 0, sizeof(*slot));
    }
    if (!unlock_auth()) {
        memset(out_session, 0, sizeof(*out_session));
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token)
{
    if (!valid_hex_token(session_token) || !valid_hex_token(csrf_token)) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    if (!lock_auth()) {
        return APP_ERROR_INTERNAL;
    }
    const uint64_t now = (uint64_t)esp_timer_get_time();
    app_error_code_t result = APP_ERROR_AUTH_REQUIRED;
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        auth_session_entry_t *entry = &sessions[index];
        if (!entry->active) {
            continue;
        }
        if (entry->view.expires_at_us <= now) {
            memset(entry, 0, sizeof(*entry));
            continue;
        }
        if (constant_time_equal((const uint8_t *)entry->view.session_token,
                                (const uint8_t *)session_token,
                                AUTH_TOKEN_HEX_BYTES - 1U) &&
            constant_time_equal((const uint8_t *)entry->view.csrf_token,
                                (const uint8_t *)csrf_token,
                                AUTH_TOKEN_HEX_BYTES - 1U)) {
            entry->view.expires_at_us = now + AUTH_SESSION_IDLE_US;
            result = APP_ERROR_NONE;
            break;
        }
    }
    return unlock_auth() ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_session_logout(const char *session_token)
{
    if (!valid_hex_token(session_token)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!lock_auth()) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = APP_ERROR_NOT_FOUND;
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        if (sessions[index].active &&
            constant_time_equal((const uint8_t *)sessions[index].view.session_token,
                                (const uint8_t *)session_token,
                                AUTH_TOKEN_HEX_BYTES - 1U)) {
            memset(&sessions[index], 0, sizeof(sessions[index]));
            result = APP_ERROR_NONE;
            break;
        }
    }
    return unlock_auth() ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_login_attempt_allowed(uint32_t *out_retry_after_seconds)
{
    if (out_retry_after_seconds == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!lock_auth()) {
        return APP_ERROR_INTERNAL;
    }
    const uint64_t now = (uint64_t)esp_timer_get_time();
    if (failure_window_start_us == 0U || now - failure_window_start_us >= AUTH_FAILURE_WINDOW_US) {
        failure_window_start_us = now;
        failure_count = 0U;
    }
    app_error_code_t result = APP_ERROR_NONE;
    *out_retry_after_seconds = 0U;
    if (failure_count >= AUTH_MAX_FAILURES) {
        const uint64_t remaining = AUTH_FAILURE_WINDOW_US - (now - failure_window_start_us);
        *out_retry_after_seconds = (uint32_t)((remaining + 999999ULL) / 1000000ULL);
        result = APP_ERROR_RATE_LIMITED;
    }
    return unlock_auth() ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_login_record_failure(void)
{
    if (!lock_auth()) {
        return APP_ERROR_INTERNAL;
    }
    const uint64_t now = (uint64_t)esp_timer_get_time();
    if (failure_window_start_us == 0U || now - failure_window_start_us >= AUTH_FAILURE_WINDOW_US) {
        failure_window_start_us = now;
        failure_count = 0U;
    }
    if (failure_count < UINT32_MAX) {
        ++failure_count;
    }
    return unlock_auth() ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

app_error_code_t auth_login_record_success(void)
{
    if (!lock_auth()) {
        return APP_ERROR_INTERNAL;
    }
    failure_count = 0U;
    failure_window_start_us = 0U;
    return unlock_auth() ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}
