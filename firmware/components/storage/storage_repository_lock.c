#include "storage_repository_lock.h"

#include <stddef.h>

#include "app_error.h"

#ifdef ESP_PLATFORM

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t repository_mutex;

static app_error_code_t backend_init(void *context) {
    (void)context;
    if (repository_mutex != NULL) {
        return APP_ERROR_CONFLICT;
    }
    repository_mutex = xSemaphoreCreateMutex();
    return repository_mutex != NULL ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t backend_take(void *context) {
    (void)context;
    if (repository_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return xSemaphoreTake(repository_mutex, portMAX_DELAY) == pdTRUE ? APP_ERROR_NONE
                                                                     : APP_ERROR_INTERNAL;
}

static app_error_code_t backend_give(void *context) {
    (void)context;
    if (repository_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return xSemaphoreGive(repository_mutex) == pdTRUE ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t backend_deinit(void *context) {
    (void)context;
    if (repository_mutex == NULL) {
        return APP_ERROR_NONE;
    }
    vSemaphoreDelete(repository_mutex);
    repository_mutex = NULL;
    return APP_ERROR_NONE;
}

#else

#include <stdbool.h>

/* Host default: a single-threaded flag lock. It has no cross-thread guarantees --
 * host tests are single-threaded -- but it detects a re-entrant take, which on the
 * production non-recursive FreeRTOS mutex would deadlock. That turns any accidental
 * lock-while-held (a missing `_locked` seam) into a visible test failure. */
static bool lock_initialized;
static bool lock_held;

static app_error_code_t backend_init(void *context) {
    (void)context;
    if (lock_initialized) {
        return APP_ERROR_CONFLICT;
    }
    lock_initialized = true;
    lock_held = false;
    return APP_ERROR_NONE;
}

static app_error_code_t backend_take(void *context) {
    (void)context;
    if (!lock_initialized || lock_held) {
        return APP_ERROR_INTERNAL;
    }
    lock_held = true;
    return APP_ERROR_NONE;
}

static app_error_code_t backend_give(void *context) {
    (void)context;
    if (!lock_initialized || !lock_held) {
        return APP_ERROR_INTERNAL;
    }
    lock_held = false;
    return APP_ERROR_NONE;
}

static app_error_code_t backend_deinit(void *context) {
    (void)context;
    lock_initialized = false;
    lock_held = false;
    return APP_ERROR_NONE;
}

#endif

static const storage_repository_lock_ops_t default_ops = {
    .context = NULL,
    .init = backend_init,
    .take = backend_take,
    .give = backend_give,
    .deinit = backend_deinit,
};

static const storage_repository_lock_ops_t *active_ops = &default_ops;

void storage_repository_lock_set_ops(const storage_repository_lock_ops_t *ops) {
    active_ops = ops != NULL ? ops : &default_ops;
}

app_error_code_t storage_repository_lock_init(void) {
    if (active_ops == NULL || active_ops->init == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return active_ops->init(active_ops->context);
}

app_error_code_t storage_repository_lock_take(void) {
    if (active_ops == NULL || active_ops->take == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return active_ops->take(active_ops->context);
}

app_error_code_t storage_repository_lock_give(void) {
    if (active_ops == NULL || active_ops->give == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return active_ops->give(active_ops->context);
}

app_error_code_t storage_repository_lock_deinit(void) {
    if (active_ops == NULL || active_ops->deinit == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return active_ops->deinit(active_ops->context);
}
