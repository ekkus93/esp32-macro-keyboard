#include "auth_core.h"

#include <limits.h>

#include "auth_core_internal.h"

static void reset_window(auth_core_t *core, uint64_t now)
{
    core->failure_window_start_us = now;
    core->failure_count = 0U;
    core->failure_window_active = true;
}

app_error_code_t auth_core_login_attempt_allowed(
    auth_core_t *core,
    uint32_t *out_retry_after_seconds)
{
    if (core == NULL || out_retry_after_seconds == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_retry_after_seconds = 0U;
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    uint64_t now = 0U;
    result = auth_core_read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        if (!core->failure_window_active ||
            now - core->failure_window_start_us >=
                AUTH_CORE_FAILURE_WINDOW_US) {
            reset_window(core, now);
        }
        if (core->failure_count >= AUTH_CORE_MAX_FAILURES) {
            const uint64_t remaining =
                AUTH_CORE_FAILURE_WINDOW_US -
                (now - core->failure_window_start_us);
            *out_retry_after_seconds =
                (uint32_t)((remaining + 999999ULL) / 1000000ULL);
            result = APP_ERROR_RATE_LIMITED;
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        *out_retry_after_seconds = 0U;
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_login_record_failure(auth_core_t *core)
{
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    uint64_t now = 0U;
    result = auth_core_read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        if (!core->failure_window_active ||
            now - core->failure_window_start_us >=
                AUTH_CORE_FAILURE_WINDOW_US) {
            reset_window(core, now);
        }
        if (core->failure_count < UINT32_MAX) {
            ++core->failure_count;
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_login_record_success(auth_core_t *core)
{
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    core->failure_count = 0U;
    core->failure_window_start_us = 0U;
    core->failure_window_active = false;
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}
