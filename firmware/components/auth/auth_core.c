#include "auth_core.h"

#include <limits.h>
#include <string.h>

#define AUTH_PBKDF2_ITERATIONS 120000U
#define AUTH_SESSION_IDLE_US (15ULL * 60ULL * 1000000ULL)
#define AUTH_MAX_FAILURES 5U
#define AUTH_FAILURE_WINDOW_US (60ULL * 1000000ULL)
#define AUTH_PASSWORD_MIN_BYTES 12U
#define AUTH_PASSWORD_MAX_BYTES 128U
#define AUTH_TOKEN_GENERATION_ATTEMPTS 4U

static bool ops_valid(const auth_ops_t *ops)
{
    return ops != NULL && ops->lock != NULL && ops->unlock != NULL &&
           ops->random_fill != NULL && ops->now_us != NULL && ops->derive != NULL &&
           ops->secure_zero != NULL;
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

static app_error_code_t lock_core(auth_core_t *core)
{
    return core->ops.lock(core->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t unlock_core(auth_core_t *core)
{
    return core->ops.unlock(core->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t read_now(auth_core_t *core, uint64_t *out_now)
{
    if (out_now == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const uint64_t now = core->ops.now_us(core->ops.context);
    if (core->clock_initialized && now < core->last_now_us) {
        return APP_ERROR_INTERNAL;
    }
    core->last_now_us = now;
    core->clock_initialized = true;
    *out_now = now;
    return APP_ERROR_NONE;
}

static app_error_code_t random_hex(auth_core_t *core,
                                   char *output,
                                   size_t output_size,
                                   size_t random_bytes)
{
    if (output == NULL || random_bytes > APP_SESSION_TOKEN_BYTES ||
        output_size < (random_bytes * 2U) + 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char digits[] = "0123456789abcdef";
    uint8_t bytes[APP_SESSION_TOKEN_BYTES];
    if (!core->ops.random_fill(core->ops.context, bytes, random_bytes)) {
        core->ops.secure_zero(core->ops.context, bytes, sizeof(bytes));
        output[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < random_bytes; ++index) {
        output[index * 2U] = digits[bytes[index] >> 4U];
        output[index * 2U + 1U] = digits[bytes[index] & 0x0fU];
    }
    output[random_bytes * 2U] = '\0';
    core->ops.secure_zero(core->ops.context, bytes, sizeof(bytes));
    return APP_ERROR_NONE;
}

static bool token_exists(const auth_core_t *core, const char *token)
{
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        if (core->sessions[index].active &&
            constant_time_equal((const uint8_t *)core->sessions[index].view.session_token,
                                (const uint8_t *)token,
                                AUTH_TOKEN_HEX_BYTES - 1U)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t generate_session_tokens(auth_core_t *core,
                                                auth_session_view_t *view)
{
    for (size_t attempt = 0U; attempt < AUTH_TOKEN_GENERATION_ATTEMPTS; ++attempt) {
        memset(view->session_token, 0, sizeof(view->session_token));
        memset(view->csrf_token, 0, sizeof(view->csrf_token));
        app_error_code_t result = random_hex(core,
                                             view->session_token,
                                             sizeof(view->session_token),
                                             APP_SESSION_TOKEN_BYTES);
        if (result == APP_ERROR_NONE) {
            result = random_hex(core,
                                view->csrf_token,
                                sizeof(view->csrf_token),
                                APP_CSRF_TOKEN_BYTES);
        }
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (!constant_time_equal((const uint8_t *)view->session_token,
                                 (const uint8_t *)view->csrf_token,
                                 AUTH_TOKEN_HEX_BYTES - 1U) &&
            !token_exists(core, view->session_token)) {
            return APP_ERROR_NONE;
        }
    }
    memset(view->session_token, 0, sizeof(view->session_token));
    memset(view->csrf_token, 0, sizeof(view->csrf_token));
    return APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_init(auth_core_t *core, const auth_ops_t *ops)
{
    if (core == NULL || !ops_valid(ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(core, 0, sizeof(*core));
    core->ops = *ops;
    return APP_ERROR_NONE;
}

app_error_code_t auth_core_password_create(auth_core_t *core,
                                           const char *password,
                                           size_t password_length,
                                           auth_password_record_t *out_record)
{
    if (core == NULL || password == NULL || out_record == NULL ||
        password_length < AUTH_PASSWORD_MIN_BYTES || password_length > AUTH_PASSWORD_MAX_BYTES ||
        memchr(password, '\0', password_length) != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_record, 0, sizeof(*out_record));
    if (!core->ops.random_fill(core->ops.context,
                               out_record->salt,
                               sizeof(out_record->salt))) {
        return APP_ERROR_INTERNAL;
    }
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    if (core->ops.derive(core->ops.context,
                         password,
                         password_length,
                         out_record->salt,
                         out_record->iterations,
                         out_record->hash) != 0) {
        core->ops.secure_zero(core->ops.context, out_record, sizeof(*out_record));
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

bool auth_core_password_verify(auth_core_t *core,
                               const char *password,
                               size_t password_length,
                               const auth_password_record_t *record)
{
    if (core == NULL || password == NULL || record == NULL ||
        password_length < AUTH_PASSWORD_MIN_BYTES || password_length > AUTH_PASSWORD_MAX_BYTES ||
        record->iterations < AUTH_PBKDF2_ITERATIONS ||
        memchr(password, '\0', password_length) != NULL) {
        return false;
    }
    uint8_t derived[AUTH_HASH_BYTES];
    if (core->ops.derive(core->ops.context,
                         password,
                         password_length,
                         record->salt,
                         record->iterations,
                         derived) != 0) {
        core->ops.secure_zero(core->ops.context, derived, sizeof(derived));
        return false;
    }
    const bool matches = constant_time_equal(derived, record->hash, sizeof(derived));
    core->ops.secure_zero(core->ops.context, derived, sizeof(derived));
    return matches;
}

app_error_code_t auth_core_session_create(auth_core_t *core,
                                          auth_session_view_t *out_session)
{
    if (core == NULL || out_session == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_session, 0, sizeof(*out_session));
    app_error_code_t result = lock_core(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    uint64_t now = 0U;
    result = read_now(core, &now);
    auth_session_entry_t *slot = NULL;
    if (result == APP_ERROR_NONE) {
        for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
            if (!core->sessions[index].active || core->sessions[index].view.expires_at_us <= now) {
                slot = &core->sessions[index];
                break;
            }
        }
        if (slot == NULL) {
            result = APP_ERROR_CONFLICT;
        }
    }
    if (result == APP_ERROR_NONE) {
        memset(slot, 0, sizeof(*slot));
        result = generate_session_tokens(core, &slot->view);
        if (result == APP_ERROR_NONE && UINT64_MAX - now < AUTH_SESSION_IDLE_US) {
            result = APP_ERROR_INTERNAL;
        }
        if (result == APP_ERROR_NONE) {
            slot->view.expires_at_us = now + AUTH_SESSION_IDLE_US;
            slot->active = true;
            *out_session = slot->view;
        } else {
            memset(slot, 0, sizeof(*slot));
        }
    }
    const app_error_code_t unlock_result = unlock_core(core);
    if (unlock_result != APP_ERROR_NONE) {
        memset(out_session, 0, sizeof(*out_session));
        return unlock_result;
    }
    return result;
}

app_error_code_t auth_core_session_validate(auth_core_t *core,
                                            const char *session_token,
                                            const char *csrf_token)
{
    if (core == NULL || !valid_hex_token(session_token) || !valid_hex_token(csrf_token)) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    app_error_code_t result = lock_core(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    uint64_t now = 0U;
    result = read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        result = APP_ERROR_AUTH_REQUIRED;
        for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
            auth_session_entry_t *entry = &core->sessions[index];
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
                if (UINT64_MAX - now < AUTH_SESSION_IDLE_US) {
                    result = APP_ERROR_INTERNAL;
                } else {
                    entry->view.expires_at_us = now + AUTH_SESSION_IDLE_US;
                    result = APP_ERROR_NONE;
                }
                break;
            }
        }
    }
    return unlock_core(core) == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_session_logout(auth_core_t *core, const char *session_token)
{
    if (core == NULL || !valid_hex_token(session_token)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_core(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = APP_ERROR_NOT_FOUND;
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        if (core->sessions[index].active &&
            constant_time_equal((const uint8_t *)core->sessions[index].view.session_token,
                                (const uint8_t *)session_token,
                                AUTH_TOKEN_HEX_BYTES - 1U)) {
            memset(&core->sessions[index], 0, sizeof(core->sessions[index]));
            result = APP_ERROR_NONE;
            break;
        }
    }
    return unlock_core(core) == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_login_attempt_allowed(auth_core_t *core,
                                                 uint32_t *out_retry_after_seconds)
{
    if (core == NULL || out_retry_after_seconds == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_core(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    uint64_t now = 0U;
    result = read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        if (!core->failure_window_active ||
            now - core->failure_window_start_us >= AUTH_FAILURE_WINDOW_US) {
            core->failure_window_start_us = now;
            core->failure_count = 0U;
            core->failure_window_active = true;
        }
        *out_retry_after_seconds = 0U;
        if (core->failure_count >= AUTH_MAX_FAILURES) {
            const uint64_t remaining =
                AUTH_FAILURE_WINDOW_US - (now - core->failure_window_start_us);
            *out_retry_after_seconds = (uint32_t)((remaining + 999999ULL) / 1000000ULL);
            result = APP_ERROR_RATE_LIMITED;
        }
    }
    return unlock_core(core) == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_login_record_failure(auth_core_t *core)
{
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_core(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    uint64_t now = 0U;
    result = read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        if (!core->failure_window_active ||
            now - core->failure_window_start_us >= AUTH_FAILURE_WINDOW_US) {
            core->failure_window_start_us = now;
            core->failure_count = 0U;
            core->failure_window_active = true;
        }
        if (core->failure_count < UINT32_MAX) {
            ++core->failure_count;
        }
    }
    return unlock_core(core) == APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_login_record_success(auth_core_t *core)
{
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_core(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    core->failure_count = 0U;
    core->failure_window_start_us = 0U;
    core->failure_window_active = false;
    return unlock_core(core) == APP_ERROR_NONE ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}
