#include "executor_health.h"

#include "app_error.h"
#include "subsystem_health.h"

typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} executor_health_state_t;

static executor_health_state_t g_state;

void executor_health_reset(void) {
    g_state.primary_error = APP_ERROR_NONE;
    g_state.cleanup_error = APP_ERROR_NONE;
    g_state.cleanup_incomplete = false;
}

void executor_health_record_primary(app_error_code_t error) {
    if (error != APP_ERROR_NONE && g_state.primary_error == APP_ERROR_NONE) {
        g_state.primary_error = error;
    }
}

void executor_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete) {
    if (cleanup_error != APP_ERROR_NONE && g_state.cleanup_error == APP_ERROR_NONE) {
        g_state.cleanup_error = cleanup_error;
    }
    g_state.cleanup_incomplete = g_state.cleanup_incomplete || cleanup_incomplete;
}

executor_health_t executor_health_snapshot(void) {
    subsystem_health_state_t state = SUBSYSTEM_HEALTH_HEALTHY;
    if (g_state.cleanup_incomplete || g_state.cleanup_error != APP_ERROR_NONE ||
        g_state.primary_error != APP_ERROR_NONE) {
        state = SUBSYSTEM_HEALTH_FAILED;
    }
    return (executor_health_t){
        .state = state,
        .primary_error = g_state.primary_error,
        .cleanup_error = g_state.cleanup_error,
        .cleanup_incomplete = g_state.cleanup_incomplete,
    };
}

void executor_shutdown_state_reset(executor_shutdown_state_t *state) {
    state->shutting_down = false;
    state->stop_unconfirmed = false;
}

void executor_shutdown_state_begin(executor_shutdown_state_t *state) {
    state->shutting_down = true;
}

void executor_shutdown_state_complete(executor_shutdown_state_t *state, bool worker_stopped) {
    if (worker_stopped) {
        state->shutting_down = false;
        state->stop_unconfirmed = false;
        return;
    }
    state->shutting_down = true;
    state->stop_unconfirmed = true;
}

bool executor_shutdown_state_accepts_submissions(const executor_shutdown_state_t *state) {
    return !state->shutting_down;
}

bool executor_shutdown_state_fault_latched(const executor_shutdown_state_t *state) {
    return state->stop_unconfirmed;
}
