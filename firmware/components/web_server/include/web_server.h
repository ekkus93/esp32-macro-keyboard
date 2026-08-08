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
} web_server_config_t;

app_error_code_t web_server_start(const web_server_config_t *configuration);
app_error_code_t web_server_stop(void);
bool web_server_owns_resources(void);

#endif
