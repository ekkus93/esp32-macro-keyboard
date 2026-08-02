#ifndef STORAGE_REPOSITORY_LOCK_H
#define STORAGE_REPOSITORY_LOCK_H

#include "app_error.h"
#include "storage_repository.h" /* storage_repository_lock_init / _deinit */

/* The repository mutation lock (FIX1 §7.5 / §9). Every public repository function
 * -- set CRUD and startup recovery -- serializes its whole
 * read-check-write transaction behind this single non-recursive lock. Internal
 * helpers invoked while it is held use `_locked` naming and must not reacquire it.
 *
 * The backend is an operations seam so host tests can substitute a deterministic
 * fake for the production FreeRTOS mutex; the platform default is installed
 * automatically (FreeRTOS on device, a re-entrancy-detecting flag lock on host). */
typedef struct {
    void *context;
    app_error_code_t (*init)(void *context);
    app_error_code_t (*take)(void *context);
    app_error_code_t (*give)(void *context);
    app_error_code_t (*deinit)(void *context);
} storage_repository_lock_ops_t;

/* Install a custom lock backend. Passing NULL restores the platform default.
 * Intended for tests; call only while the lock is not initialized. */
void storage_repository_lock_set_ops(const storage_repository_lock_ops_t *ops);

app_error_code_t storage_repository_lock_take(void);
app_error_code_t storage_repository_lock_give(void);

#endif
