#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdbool.h>

#include "app_error.h"
#include "auth.h"

typedef struct {
    bool login_enabled;
    auth_password_record_t password_record;
} web_server_config_t;

app_error_code_t web_server_start(const web_server_config_t *configuration);
app_error_code_t web_server_stop(void);

#endif
