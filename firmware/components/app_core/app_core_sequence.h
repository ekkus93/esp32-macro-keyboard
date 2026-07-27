#ifndef APP_CORE_SEQUENCE_H
#define APP_CORE_SEQUENCE_H

#include <stdbool.h>

#include "app_core_ops.h"

typedef struct {
    bool manufacturing_provisioning_enabled;
} app_core_policy_t;

app_error_code_t app_core_map_nvs_result(app_core_nvs_result_t result);
app_error_code_t app_core_sequence_start(const app_core_ops_t *operations,
                                         const app_core_policy_t *policy);

#endif
