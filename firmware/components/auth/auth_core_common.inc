#include "auth_core.h"

#include <string.h>

#include "auth_core_internal.h"

static bool operations_valid(const auth_ops_t *operations)
{
    return operations != NULL && operations->lock != NULL &&
           operations->unlock != NULL && operations->random_fill != NULL &&
           operations->now_us != NULL && operations->derive != NULL &&
           operations->secure_zero != NULL;
}

bool auth_core_constant_time_equal(const uint8_t *left,
                                   const uint8_t *right,
                                   size_t length)
{
    uint8_t difference = 0U;
    for (size_t index = 0U; index < length; ++index) {
        difference = (uint8_t)(difference | (uint8_t)(left[index] ^ right[index]));
    }
    return difference == 0U;
}

bool auth_core_valid_hex_token(const char *token)
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

app_error_code_t auth_core_lock(auth_core_t *core)
{
    return core->ops.lock(core->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_unlock(auth_core_t *core)
{
    return core->ops.unlock(core->ops.context) ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

app_error_code_t auth_core_read_now(auth_core_t *core, uint64_t *out_now)
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
            auth_core_constant_time_equal(
                (const uint8_t *)core->sessions[index].view.session_token,
                (const uint8_t *)token,
                AUTH_TOKEN_HEX_BYTES - 1U)) {
            return true;
        }
    }
    return false;
}

app_error_code_t auth_core_generate_session_tokens(auth_core_t *core,
                                                   auth_session_view_t *view)
{
    for (size_t attempt = 0U; attempt < AUTH_CORE_TOKEN_GENERATION_ATTEMPTS;
         ++attempt) {
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
        if (!auth_core_constant_time_equal(
                (const uint8_t *)view->session_token,
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

void auth_core_snapshot_state(const auth_core_t *core,
                              auth_core_state_snapshot_t *snapshot)
{
    memcpy(snapshot->sessions, core->sessions, sizeof(snapshot->sessions));
    snapshot->failure_count = core->failure_count;
    snapshot->failure_window_start_us = core->failure_window_start_us;
    snapshot->failure_window_active = core->failure_window_active;
    snapshot->last_now_us = core->last_now_us;
    snapshot->clock_initialized = core->clock_initialized;
}

void auth_core_restore_state(auth_core_t *core,
                             const auth_core_state_snapshot_t *snapshot)
{
    memcpy(core->sessions, snapshot->sessions, sizeof(core->sessions));
    core->failure_count = snapshot->failure_count;
    core->failure_window_start_us = snapshot->failure_window_start_us;
    core->failure_window_active = snapshot->failure_window_active;
    core->last_now_us = snapshot->last_now_us;
    core->clock_initialized = snapshot->clock_initialized;
}

app_error_code_t auth_core_init(auth_core_t *core, const auth_ops_t *operations)
{
    if (core == NULL || !operations_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(core, 0, sizeof(*core));
    core->ops = *operations;
    return APP_ERROR_NONE;
}
