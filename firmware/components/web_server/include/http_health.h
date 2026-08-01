#ifndef HTTP_HEALTH_H
#define HTTP_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* HTTP server health for Phase 19 diagnostics (FIX1 handoff §7.1). Portable C
 * with no ESP-IDF dependency, so it is host-testable directly. Recorded from
 * app_core.c's existing http_start/http_stop call sites. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} http_health_t;

void http_health_reset(void);
void http_health_record_primary(app_error_code_t error);
void http_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
http_health_t http_health_snapshot(void);

#endif
