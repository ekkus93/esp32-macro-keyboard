#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>

#include "app_error.h"
#include "auth.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"

typedef enum { WEB_SERVER_MODE_NORMAL = 0, WEB_SERVER_MODE_SETUP } web_server_mode_t;

typedef struct {
    web_server_mode_t mode;
    bool login_enabled;
    auth_password_record_t password_record;
    bool require_physical_confirmation;
    char setup_device_name[APP_V2_DEVICE_NAME_MAX_BYTES + 1U];
    char setup_code[APP_V2_SETUP_CODE_BUFFER_BYTES];
    void *setup_code_clear_context;
    void (*setup_code_clear)(void *context);
} web_server_config_t;

app_error_code_t web_server_start(const web_server_config_t *configuration);
app_error_code_t web_server_stop(void);
bool web_server_owns_resources(void);

/* The single running server's configuration, set by web_server_start() and
 * read by every handler that needs it (e.g. web_server_login.c::login_handler()
 * checks login_enabled; web_api_administration.c's change-password handler
 * refreshes password_record after a successful change). Declared here
 * (rather than web_server_internal.h, which pulls in esp_http_server.h
 * transitively) so consumers that take no httpd_req_t of their own, like
 * web_api_administration.c, do not acquire an ESP-IDF dependency they
 * otherwise avoid -- see web_diagnostics_handle()'s identical rationale in
 * web_diagnostics.h. Defined in web_server_common.c. */
extern web_server_config_t server_configuration;

#endif
