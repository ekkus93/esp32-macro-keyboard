#ifndef APP_CORE_HEALTH_H
#define APP_CORE_HEALTH_H

#include <stdbool.h>

#include "app_core.h"
#include "app_error.h"

/* Portable C with no ESP-IDF dependency, so it is host-testable directly,
 * independent of app_core.c's real adapters. app_lifecycle_health_t (declared
 * in app_core.h) is always derived fresh from the recorded fields here, never
 * stored independently, so it can never drift out of sync with them. */
void app_core_health_reset(void);
void app_core_health_record_stage_failure(app_error_code_t error);
void app_core_health_record_degraded(void);
void app_core_health_record_cleanup_failed(app_error_code_t cleanup_error, bool cleanup_incomplete);
app_lifecycle_health_t app_core_health_snapshot(void);

#endif
