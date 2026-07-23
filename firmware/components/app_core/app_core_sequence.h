#ifndef APP_CORE_SEQUENCE_H
#define APP_CORE_SEQUENCE_H

#include <stdbool.h>

#include "app_core_ops.h"

#define APP_CORE_DEVELOPMENT_PASSWORD_BYTES 20U
#define APP_CORE_CREDENTIAL_RETRY_LIMIT 4U

typedef struct {
    bool development_provisioning_enabled;
    const char *development_ssid;
} app_core_policy_t;

app_error_code_t app_core_map_nvs_result(app_core_nvs_result_t result);
app_error_code_t app_core_sequence_start(const app_core_ops_t *operations,
                                         const app_core_policy_t *policy);

#endif
