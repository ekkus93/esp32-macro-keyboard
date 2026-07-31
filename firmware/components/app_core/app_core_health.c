#include "app_core_health.h"

typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    bool degraded;
} app_core_health_state_t;

static app_core_health_state_t g_state;

void app_core_health_reset(void) {
    g_state.primary_error = APP_ERROR_NONE;
    g_state.cleanup_error = APP_ERROR_NONE;
    g_state.cleanup_incomplete = false;
    g_state.degraded = false;
}

void app_core_health_record_stage_failure(app_error_code_t error) {
    if (error != APP_ERROR_NONE && g_state.primary_error == APP_ERROR_NONE) {
        g_state.primary_error = error;
    }
}

void app_core_health_record_degraded(void) {
    g_state.degraded = true;
}

void app_core_health_record_cleanup_failed(app_error_code_t cleanup_error,
                                           bool cleanup_incomplete) {
    if (cleanup_error != APP_ERROR_NONE && g_state.cleanup_error == APP_ERROR_NONE) {
        g_state.cleanup_error = cleanup_error;
    }
    g_state.cleanup_incomplete = g_state.cleanup_incomplete || cleanup_incomplete;
}

app_lifecycle_health_t app_core_health_snapshot(void) {
    subsystem_health_state_t state = SUBSYSTEM_HEALTH_HEALTHY;
    if (g_state.cleanup_incomplete || g_state.cleanup_error != APP_ERROR_NONE ||
        g_state.primary_error != APP_ERROR_NONE) {
        state = SUBSYSTEM_HEALTH_FAILED;
    } else if (g_state.degraded) {
        state = SUBSYSTEM_HEALTH_DEGRADED;
    }
    return (app_lifecycle_health_t){
        .state = state,
        .primary_error = g_state.primary_error,
        .cleanup_error = g_state.cleanup_error,
        .cleanup_incomplete = g_state.cleanup_incomplete,
    };
}
