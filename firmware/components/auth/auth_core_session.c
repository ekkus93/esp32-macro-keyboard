#include "auth_core.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "auth_core_internal.h"
#include "macro_limits.h"

app_error_code_t auth_core_session_create(auth_core_t *core, auth_session_view_t *out_session) {
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
        result = auth_core_generate_session_tokens(core, &slot->view);
        if (result == APP_ERROR_NONE && UINT64_MAX - now < AUTH_CORE_SESSION_IDLE_US) {
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

typedef struct {
    const char *session_token;
    const char *csrf_token;
    bool require_csrf;
    char *csrf_output;
    size_t csrf_output_size;
} auth_session_validation_t;

static void clear_csrf_output(const auth_session_validation_t *validation) {
    if (validation->csrf_output != NULL && validation->csrf_output_size > 0U) {
        validation->csrf_output[0] = '\0';
    }
}

static bool csrf_output_parameters_valid(const auth_session_validation_t *validation) {
    return (validation->csrf_output == NULL && validation->csrf_output_size == 0U) ||
           (validation->csrf_output != NULL &&
            validation->csrf_output_size >= AUTH_TOKEN_HEX_BYTES);
}

static bool validation_tokens_valid(const auth_core_t *core,
                                    const auth_session_validation_t *validation) {
    return core != NULL && auth_core_valid_hex_token(validation->session_token) &&
           (!validation->require_csrf || auth_core_valid_hex_token(validation->csrf_token));
}

static bool session_entry_matches(const auth_session_entry_t *entry,
                                  const auth_session_validation_t *validation) {
    if (!auth_core_constant_time_equal((const uint8_t *)entry->view.session_token,
                                       (const uint8_t *)validation->session_token,
                                       AUTH_TOKEN_HEX_BYTES - 1U)) {
        return false;
    }
    return !validation->require_csrf ||
           auth_core_constant_time_equal((const uint8_t *)entry->view.csrf_token,
                                         (const uint8_t *)validation->csrf_token,
                                         AUTH_TOKEN_HEX_BYTES - 1U);
}

static app_error_code_t refresh_session(auth_session_entry_t *entry, uint64_t now,
                                        const auth_session_validation_t *validation) {
    if (UINT64_MAX - now < AUTH_CORE_SESSION_IDLE_US) {
        return APP_ERROR_INTERNAL;
    }
    entry->view.expires_at_us = now + AUTH_CORE_SESSION_IDLE_US;
    if (validation->csrf_output != NULL) {
        memcpy(validation->csrf_output, entry->view.csrf_token, AUTH_TOKEN_HEX_BYTES);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t find_and_refresh_session(auth_core_t *core, uint64_t now,
                                                 const auth_session_validation_t *validation) {
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        auth_session_entry_t *entry = &core->sessions[index];
        if (!entry->active) {
            continue;
        }
        if (entry->view.expires_at_us <= now) {
            memset(entry, 0, sizeof(*entry));
            continue;
        }
        if (session_entry_matches(entry, validation)) {
            return refresh_session(entry, now, validation);
        }
    }
    return APP_ERROR_AUTH_REQUIRED;
}

static app_error_code_t validate_session(auth_core_t *core, const char *session_token,
                                         const char *csrf_token, bool require_csrf,
                                         char *out_csrf_token, size_t output_size) {
    const auth_session_validation_t validation = {
        .session_token = session_token,
        .csrf_token = csrf_token,
        .require_csrf = require_csrf,
        .csrf_output = out_csrf_token,
        .csrf_output_size = output_size,
    };
    clear_csrf_output(&validation);
    if (!csrf_output_parameters_valid(&validation)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!validation_tokens_valid(core, &validation)) {
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
        result = find_and_refresh_session(core, now, &validation);
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        clear_csrf_output(&validation);
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,
                                            const char *csrf_token) {
    return validate_session(core, session_token, csrf_token, true, NULL, 0U);
}

app_error_code_t auth_core_session_validate_read_only(auth_core_t *core,
                                                      const char *session_token) {
    return validate_session(core, session_token, NULL, false, NULL, 0U);
}

app_error_code_t auth_core_session_get_csrf_token(auth_core_t *core, const char *session_token,
                                                  char *out_csrf_token, size_t output_size) {
    if (out_csrf_token == NULL || output_size < AUTH_TOKEN_HEX_BYTES) {
        if (out_csrf_token != NULL && output_size > 0U) {
            out_csrf_token[0] = '\0';
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return validate_session(core, session_token, NULL, false, out_csrf_token, output_size);
}

app_error_code_t auth_core_session_logout(auth_core_t *core, const char *session_token) {
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
            auth_core_constant_time_equal((const uint8_t *)core->sessions[index].view.session_token,
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
