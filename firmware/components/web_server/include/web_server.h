#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>

#include "app_error.h"
#include "auth.h"
#include "provisioning_bootstrap.h"
#include "wifi_ap.h"

typedef enum { WEB_SERVER_MODE_NORMAL = 0, WEB_SERVER_MODE_SETUP } web_server_mode_t;

typedef struct {
    web_server_mode_t mode;
    bool login_enabled;
    auth_password_record_t password_record;
    char setup_device_id[PROVISIONING_DEVICE_ID_HEX_BYTES + 1U];
    char setup_ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char setup_code[PROVISIONING_SETUP_SECRET_BUFFER_BYTES];
    bool setup_physical_confirmation_required;
    bool setup_manufacturing_bypass;
} web_server_config_t;

app_error_code_t web_server_start(const web_server_config_t *configuration);
app_error_code_t web_server_stop(void);
bool web_server_owns_resources(void);

#endif
