#include "web_settings_production_ops.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"
#include "web_server_password_record.h"
#include "web_settings.h"

static void secure_zero_local(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

static app_error_code_t production_settings_read(void *context,
                                                 app_v2_device_settings_t *out_settings) {
    (void)context;
    return device_settings_read(out_settings);
}

static app_error_code_t production_settings_replace(void *context,
                                                    const app_v2_device_settings_t *settings,
                                                    bool *out_changed) {
    (void)context;
    return device_settings_replace(settings, out_changed);
}

static app_error_code_t production_password_verify(void *context, const char *password,
                                                   size_t password_length,
                                                   const app_v2_device_settings_t *settings,
                                                   bool *out_matches) {
    (void)context;
    auth_password_record_t record = {.iterations = settings->password_iterations};
    memcpy(record.salt, settings->password_salt, sizeof(record.salt));
    memcpy(record.hash, settings->password_verifier, sizeof(record.hash));
    const app_error_code_t result =
        auth_password_verify(password, password_length, &record, out_matches);
    secure_zero_local(&record, sizeof(record));
    return result;
}

/* Bridges the V2 password-verifier path in the auth component to the setup
 * contract's material shape, mirroring web_server_setup.c's
 * setup_password_create() -- used here for change-password rather than
 * initial setup. AUTH_PBKDF2_ITERATIONS is the hardware-measured value
 * frozen by V2-041 (see auth.h). */
static app_error_code_t production_password_create(void *context, const char *password,
                                                   size_t password_length,
                                                   app_v2_setup_password_material_t *out_material) {
    (void)context;
    memset(out_material, 0, sizeof(*out_material));
    auth_password_record_t record = {0};
    const app_error_code_t result = auth_password_create(password, password_length, &record);
    if (result != APP_ERROR_NONE) {
        secure_zero_local(&record, sizeof(record));
        return result;
    }
    if (record.iterations != AUTH_PBKDF2_ITERATIONS) {
        /* Defensive: the auth component's default iteration count drifted
         * from what V2 change-password expects. Fail closed rather than
         * store material the login floor check would reject later. */
        secure_zero_local(&record, sizeof(record));
        return APP_ERROR_INTERNAL;
    }
    out_material->credential_version = APP_V2_CREDENTIAL_VERSION;
    out_material->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    out_material->password_iterations = record.iterations;
    memcpy(out_material->password_salt, record.salt, sizeof(out_material->password_salt));
    memcpy(out_material->password_verifier, record.hash, sizeof(out_material->password_verifier));
    secure_zero_local(&record, sizeof(record));
    return APP_ERROR_NONE;
}

static app_error_code_t production_password_transition_begin(void *context) {
    (void)context;
    return web_server_password_transition_begin();
}

static void production_password_activate(void *context, const app_v2_device_settings_t *settings) {
    (void)context;
    auth_password_record_t record = {.iterations = settings->password_iterations};
    memcpy(record.salt, settings->password_salt, sizeof(record.salt));
    memcpy(record.hash, settings->password_verifier, sizeof(record.hash));
    web_server_password_record_replace(&record);
    secure_zero_local(&record, sizeof(record));
}

static void production_password_transition_end(void *context) {
    (void)context;
    web_server_password_transition_end();
}

static app_error_code_t production_invalidate_all_sessions(void *context) {
    (void)context;
    return auth_session_logout_all();
}

web_settings_ops_t web_settings_production_ops(void) {
    return (web_settings_ops_t){
        .context = NULL,
        .settings_read = production_settings_read,
        .settings_replace = production_settings_replace,
        .password_verify = production_password_verify,
        .password_create = production_password_create,
        .password_transition_begin = production_password_transition_begin,
        .password_activate = production_password_activate,
        .password_transition_end = production_password_transition_end,
        .invalidate_all_sessions = production_invalidate_all_sessions,
    };
}
