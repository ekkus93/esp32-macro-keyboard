#include "http_health.h"

#include "app_error.h"
#include "subsystem_health.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include <pthread.h>
#include <stdlib.h>
#endif

typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    http_async_failure_stage_t async_failure_stage;
    app_error_code_t async_error;
} http_health_state_t;

static http_health_state_t g_state;

/* Production uses an ESP-IDF portMUX. Host builds use a real pthread
 * mutex rather than a fake no-op lock, so stress tests exercise the same
 * exclusion invariant. This is compile-time test portability, not a
 * runtime fallback. */
#ifdef ESP_PLATFORM
static portMUX_TYPE g_state_lock = portMUX_INITIALIZER_UNLOCKED;

static void health_lock(void) {
    portENTER_CRITICAL(&g_state_lock);
}

static void health_unlock(void) {
    portEXIT_CRITICAL(&g_state_lock);
}
#else
static pthread_mutex_t g_state_lock = PTHREAD_MUTEX_INITIALIZER;

static void health_lock(void) {
    if (pthread_mutex_lock(&g_state_lock) != 0) {
        abort();
    }
}

static void health_unlock(void) {
    if (pthread_mutex_unlock(&g_state_lock) != 0) {
        abort();
    }
}
#endif

static bool async_failure_stage_valid(http_async_failure_stage_t stage) {
    return stage >= HTTP_ASYNC_FAILURE_WORKER_START && stage <= HTTP_ASYNC_FAILURE_COMPLETION;
}

void http_health_reset(void) {
    health_lock();
    g_state.primary_error = APP_ERROR_NONE;
    g_state.cleanup_error = APP_ERROR_NONE;
    g_state.cleanup_incomplete = false;
    g_state.async_failure_stage = HTTP_ASYNC_FAILURE_NONE;
    g_state.async_error = APP_ERROR_NONE;
    health_unlock();
}

void http_health_record_primary(app_error_code_t error) {
    health_lock();
    if (error != APP_ERROR_NONE && g_state.primary_error == APP_ERROR_NONE) {
        g_state.primary_error = error;
    }
    health_unlock();
}

void http_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete) {
    health_lock();
    if (cleanup_error != APP_ERROR_NONE && g_state.cleanup_error == APP_ERROR_NONE) {
        g_state.cleanup_error = cleanup_error;
    }
    g_state.cleanup_incomplete = g_state.cleanup_incomplete || cleanup_incomplete;
    health_unlock();
}

void http_health_record_async_failure(http_async_failure_stage_t stage, app_error_code_t error) {
    if (!async_failure_stage_valid(stage) || error == APP_ERROR_NONE) {
        return;
    }
    health_lock();
    /* First async failure wins. Completion/shutdown fallout cannot erase
     * the earliest actionable async cause. */
    if (g_state.async_failure_stage == HTTP_ASYNC_FAILURE_NONE) {
        g_state.async_failure_stage = stage;
        g_state.async_error = error;
    }
    health_unlock();
}

http_health_t http_health_snapshot(void) {
    health_lock();
    const http_health_state_t snapshot = g_state;
    health_unlock();

    subsystem_health_state_t state = SUBSYSTEM_HEALTH_HEALTHY;
    if (snapshot.cleanup_incomplete || snapshot.cleanup_error != APP_ERROR_NONE ||
        snapshot.primary_error != APP_ERROR_NONE ||
        snapshot.async_failure_stage != HTTP_ASYNC_FAILURE_NONE ||
        snapshot.async_error != APP_ERROR_NONE) {
        state = SUBSYSTEM_HEALTH_FAILED;
    }
    return (http_health_t){
        .state = state,
        .primary_error = snapshot.primary_error,
        .cleanup_error = snapshot.cleanup_error,
        .cleanup_incomplete = snapshot.cleanup_incomplete,
        .async_failure_stage = snapshot.async_failure_stage,
        .async_error = snapshot.async_error,
    };
}
