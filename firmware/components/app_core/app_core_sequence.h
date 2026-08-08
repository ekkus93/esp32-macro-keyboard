#ifndef APP_CORE_SEQUENCE_H
#define APP_CORE_SEQUENCE_H

#include "app_core_ops.h"

app_error_code_t app_core_map_nvs_result(app_core_nvs_result_t result);
app_error_code_t app_core_sequence_start(const app_core_ops_t *operations);

#endif
