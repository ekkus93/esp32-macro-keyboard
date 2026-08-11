#ifndef HTTP_HEALTH_H
#define HTTP_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* Sanitized async-HTTP failure stages. These are internal health metadata;
 * the frozen v2 diagnostics contract continues to expose only the existing
 * "http" subsystem state. */
typedef enum {
    HTTP_ASYNC_FAILURE_NONE = 0,
    HTTP_ASYNC_FAILURE_WORKER_START,
    HTTP_ASYNC_FAILURE_WORKER_RUN,
    HTTP_ASYNC_FAILURE_WORKER_STOP,
    HTTP_ASYNC_FAILURE_QUEUE,
    HTTP_ASYNC_FAILURE_COMPLETION,
} http_async_failure_stage_t;

typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    http_async_failure_stage_t async_failure_stage;
    app_error_code_t async_error;
} http_health_t;

void http_health_reset(void);
void http_health_record_primary(app_error_code_t error);
void http_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
void http_health_record_async_failure(http_async_failure_stage_t stage, app_error_code_t error);
http_health_t http_health_snapshot(void);

#endif
