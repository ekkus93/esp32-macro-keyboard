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
void web_server_password_record_replace(const auth_password_record_t *record);

#endif
