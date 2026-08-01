#include "http_health.h"

#include "app_error.h"
#include "subsystem_health.h"

typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} http_health_state_t;

static http_health_state_t g_state;

void http_health_reset(void) {
    g_state.primary_error = APP_ERROR_NONE;
    g_state.cleanup_error = APP_ERROR_NONE;
    g_state.cleanup_incomplete = false;
}

void http_health_record_primary(app_error_code_t error) {
    if (error != APP_ERROR_NONE && g_state.primary_error == APP_ERROR_NONE) {
        g_state.primary_error = error;
    }
}

void http_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete) {
    if (cleanup_error != APP_ERROR_NONE && g_state.cleanup_error == APP_ERROR_NONE) {
        g_state.cleanup_error = cleanup_error;
    }
    g_state.cleanup_incomplete = g_state.cleanup_incomplete || cleanup_incomplete;
}

http_health_t http_health_snapshot(void) {
    subsystem_health_state_t state = SUBSYSTEM_HEALTH_HEALTHY;
    if (g_state.cleanup_incomplete || g_state.cleanup_error != APP_ERROR_NONE ||
        g_state.primary_error != APP_ERROR_NONE) {
        state = SUBSYSTEM_HEALTH_FAILED;
    }
    return (http_health_t){
        .state = state,
        .primary_error = g_state.primary_error,
        .cleanup_error = g_state.cleanup_error,
        .cleanup_incomplete = g_state.cleanup_incomplete,
    };
}
