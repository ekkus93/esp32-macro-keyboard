#ifndef WEB_SERVER_PASSWORD_RECORD_H
#define WEB_SERVER_PASSWORD_RECORD_H

#include "auth.h"
#include "web_server.h"

/* These helpers are the only production path for copying the running
 * server's password record or for whole-configuration writes that include
 * it. The implementation holds its lock only for the bounded memory copy;
 * callers must perform PBKDF2, device-settings I/O, confirmation waits, and
 * every other potentially blocking operation outside the lock. */
void web_server_configuration_store(const web_server_config_t *configuration);
void web_server_configuration_clear(void);
void web_server_password_record_snapshot(auth_password_record_t *out_record);
/* Login-specific snapshot: while any password-change transaction owns the
 * transition gate, fail closed instead of authenticating against a credential
 * whose authority may be about to change. */
app_error_code_t web_server_password_record_snapshot_for_login(auth_password_record_t *out_record);
void web_server_password_record_replace(const auth_password_record_t *record);
/* The transition gate is independent from the narrow record-copy critical
 * section. begin/end only flip a bounded flag under that same portMUX; no NVS,
 * PBKDF2, confirmation wait, or session invalidation runs while the portMUX is
 * held. */
app_error_code_t web_server_password_transition_begin(void);
void web_server_password_transition_end(void);

#endif
