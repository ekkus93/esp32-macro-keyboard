#include "web_server_password_record.h"

#include <string.h>

/* Narrow host-test substitute for targets that exercise routing/handler
 * composition rather than concurrency. The dedicated
 * web_server_password_record test target links the real synchronized module
 * instead. */
static bool password_transition_in_progress;

void web_server_configuration_store(const web_server_config_t *configuration) {
    server_configuration = *configuration;
    password_transition_in_progress = false;
}

void web_server_configuration_clear(void) {
    memset(&server_configuration, 0, sizeof(server_configuration));
    password_transition_in_progress = false;
}

void web_server_password_record_snapshot(auth_password_record_t *out_record) {
    *out_record = server_configuration.password_record;
}

app_error_code_t web_server_password_record_snapshot_for_login(auth_password_record_t *out_record) {
    if (out_record == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (password_transition_in_progress) {
        return APP_ERROR_AUTH_STATE_INCOMPLETE;
    }
    *out_record = server_configuration.password_record;
    return APP_ERROR_NONE;
}

void web_server_password_record_replace(const auth_password_record_t *record) {
    server_configuration.password_record = *record;
}

app_error_code_t web_server_password_transition_begin(void) {
    if (password_transition_in_progress) {
        return APP_ERROR_CONFLICT;
    }
    password_transition_in_progress = true;
    return APP_ERROR_NONE;
}

void web_server_password_transition_end(void) {
    password_transition_in_progress = false;
}
