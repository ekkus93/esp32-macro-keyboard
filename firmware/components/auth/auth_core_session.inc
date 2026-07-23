#include "auth_core.h"

#include <string.h>

#include "auth_core_internal.h"

app_error_code_t auth_core_session_create(auth_core_t *core,
                                          auth_session_view_t *out_session)
{
    if (core == NULL || out_session == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_session, 0, sizeof(*out_session));
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    uint64_t now = 0U;
    result = auth_core_read_now(core, &now);
    auth_session_entry_t *slot = NULL;
    if (result == APP_ERROR_NONE) {
        for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
            if (!core->sessions[index].active ||
                core->sessions[index].view.expires_at_us <= now) {
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
        result = auth_core_generate_session_tokens(core, &slot->view);
        if (result == APP_ERROR_NONE &&
            UINT64_MAX - now < AUTH_CORE_SESSION_IDLE_US) {
            result = APP_ERROR_INTERNAL;
        }
        if (result == APP_ERROR_NONE) {
            slot->view.expires_at_us = now + AUTH_CORE_SESSION_IDLE_US;
            slot->active = true;
            *out_session = slot->view;
        } else {
            memset(slot, 0, sizeof(*slot));
        }
    }
    const app_error_code_t unlock_result = auth_core_unlock(core);
    if (unlock_result != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        memset(out_session, 0, sizeof(*out_session));
        return unlock_result;
    }
    return result;
}

app_error_code_t auth_core_session_validate(auth_core_t *core,
                                            const char *session_token,
                                            const char *csrf_token)
{
    if (core == NULL || !auth_core_valid_hex_token(session_token) ||
        !auth_core_valid_hex_token(csrf_token)) {
        return APP_ERROR_AUTH_REQUIRED;
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
            if (auth_core_constant_time_equal(
                    (const uint8_t *)entry->view.session_token,
                    (const uint8_t *)session_token,
                    AUTH_TOKEN_HEX_BYTES - 1U) &&
                auth_core_constant_time_equal(
                    (const uint8_t *)entry->view.csrf_token,
                    (const uint8_t *)csrf_token,
                    AUTH_TOKEN_HEX_BYTES - 1U)) {
                if (UINT64_MAX - now < AUTH_CORE_SESSION_IDLE_US) {
                    result = APP_ERROR_INTERNAL;
                } else {
                    entry->view.expires_at_us = now + AUTH_CORE_SESSION_IDLE_US;
                    result = APP_ERROR_NONE;
                }
                break;
            }
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_session_logout(auth_core_t *core,
                                          const char *session_token)
{
    if (core == NULL || !auth_core_valid_hex_token(session_token)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    result = APP_ERROR_NOT_FOUND;
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        if (core->sessions[index].active &&
            auth_core_constant_time_equal(
                (const uint8_t *)core->sessions[index].view.session_token,
                (const uint8_t *)session_token,
                AUTH_TOKEN_HEX_BYTES - 1U)) {
            memset(&core->sessions[index], 0, sizeof(core->sessions[index]));
            result = APP_ERROR_NONE;
            break;
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return result;
}
