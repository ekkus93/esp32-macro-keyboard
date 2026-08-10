#include "web_server_password_record.h"

#include <string.h>

#include "auth.h"
#include "freertos/FreeRTOS.h"
#include "web_server.h"

/* A portMUX is sufficient here because the protected region is only a
 * bounded structure copy. In particular, neither password hashing nor any
 * request/confirmation wait is allowed while this lock is held. */
static portMUX_TYPE password_record_lock = portMUX_INITIALIZER_UNLOCKED;

void web_server_configuration_store(const web_server_config_t *configuration) {
    portENTER_CRITICAL(&password_record_lock);
    server_configuration = *configuration;
    portEXIT_CRITICAL(&password_record_lock);
}

void web_server_configuration_clear(void) {
    portENTER_CRITICAL(&password_record_lock);
    memset(&server_configuration, 0, sizeof(server_configuration));
    portEXIT_CRITICAL(&password_record_lock);
}

void web_server_password_record_snapshot(auth_password_record_t *out_record) {
    portENTER_CRITICAL(&password_record_lock);
    *out_record = server_configuration.password_record;
    portEXIT_CRITICAL(&password_record_lock);
}

void web_server_password_record_replace(const auth_password_record_t *record) {
    portENTER_CRITICAL(&password_record_lock);
    server_configuration.password_record = *record;
    portEXIT_CRITICAL(&password_record_lock);
}
