#include "executor_health.h"

#include "app_error.h"
#include "freertos/FreeRTOS.h"
#include "subsystem_health.h"

typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} executor_health_state_t;

static executor_health_state_t g_state;
/* Diagnostics are read from the HTTP task while startup rollback can still
 * mutate health if stopping HTTP itself fails. Keep each snapshot/update in a
 * single FreeRTOS critical section so readers never observe a torn state. */
static portMUX_TYPE g_state_lock = portMUX_INITIALIZER_UNLOCKED;

void executor_health_reset(void) {
    portENTER_CRITICAL(&g_state_lock);
    g_state.primary_error = APP_ERROR_NONE;
    g_state.cleanup_error = APP_ERROR_NONE;
    g_state.cleanup_incomplete = false;
    portEXIT_CRITICAL(&g_state_lock);
}

void executor_health_record_primary(app_error_code_t error) {
    portENTER_CRITICAL(&g_state_lock);
    if (error != APP_ERROR_NONE && g_state.primary_error == APP_ERROR_NONE) {
        g_state.primary_error = error;
    }
    portEXIT_CRITICAL(&g_state_lock);
}

void executor_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete) {
    portENTER_CRITICAL(&g_state_lock);
    if (cleanup_error != APP_ERROR_NONE && g_state.cleanup_error == APP_ERROR_NONE) {
        g_state.cleanup_error = cleanup_error;
    }
    g_state.cleanup_incomplete = g_state.cleanup_incomplete || cleanup_incomplete;
    portEXIT_CRITICAL(&g_state_lock);
}

executor_health_t executor_health_snapshot(void) {
    portENTER_CRITICAL(&g_state_lock);
    const executor_health_state_t snapshot = g_state;
    portEXIT_CRITICAL(&g_state_lock);

    subsystem_health_state_t state = SUBSYSTEM_HEALTH_HEALTHY;
    if (snapshot.cleanup_incomplete || snapshot.cleanup_error != APP_ERROR_NONE ||
        snapshot.primary_error != APP_ERROR_NONE) {
        state = SUBSYSTEM_HEALTH_FAILED;
    }
    return (executor_health_t){
        .state = state,
        .primary_error = snapshot.primary_error,
        .cleanup_error = snapshot.cleanup_error,
        .cleanup_incomplete = snapshot.cleanup_incomplete,
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
