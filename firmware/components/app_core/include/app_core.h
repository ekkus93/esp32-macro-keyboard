#ifndef APP_CORE_H
#define APP_CORE_H

#include "app_error.h"
#include "app_lifecycle_health.h"

app_error_code_t app_core_start(void);
app_lifecycle_health_t app_core_get_health(void);

#endif
