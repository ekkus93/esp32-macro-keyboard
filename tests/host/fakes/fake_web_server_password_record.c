#include "web_server_password_record.h"

#include <string.h>

/* Narrow host-test substitute for targets that exercise routing/handler
 * composition rather than concurrency. The dedicated
 * web_server_password_record test target links the real synchronized module
 * instead. */
void web_server_configuration_store(const web_server_config_t *configuration) {
    server_configuration = *configuration;
}

void web_server_configuration_clear(void) {
    memset(&server_configuration, 0, sizeof(server_configuration));
}

void web_server_password_record_snapshot(auth_password_record_t *out_record) {
    *out_record = server_configuration.password_record;
}

void web_server_password_record_replace(const auth_password_record_t *record) {
    server_configuration.password_record = *record;
}
