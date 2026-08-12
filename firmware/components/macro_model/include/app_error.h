#ifndef APP_ERROR_H
#define APP_ERROR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_ERROR_NONE = 0,
    APP_ERROR_INVALID_ARGUMENT,
    APP_ERROR_NOT_FOUND,
    APP_ERROR_CONFLICT,
    APP_ERROR_STORAGE_UNAVAILABLE,
    APP_ERROR_STORAGE_FULL,
    APP_ERROR_STORAGE_CORRUPT,
    APP_ERROR_MACRO_SYNTAX,
    APP_ERROR_MACRO_LIMIT,
    APP_ERROR_USB_NOT_READY,
    APP_ERROR_EXECUTOR_BUSY,
    APP_ERROR_EXECUTION_CANCELLED,
    APP_ERROR_AUTH_REQUIRED,
    APP_ERROR_AUTH_FAILED,
    APP_ERROR_RATE_LIMITED,
    APP_ERROR_TIMEOUT,
    APP_ERROR_IO,
    APP_ERROR_INTERNAL,
    /* The authoritative credential changed, but authentication/session cleanup
     * is not yet fully coherent. Appended to preserve all pre-existing numeric
     * error values. */
    APP_ERROR_AUTH_STATE_INCOMPLETE,
    /* A durable factory-reset journal says destructive cleanup is incomplete.
     * Appended to preserve every existing numeric app-error value. */
    APP_ERROR_RESET_RECOVERY_REQUIRED
} app_error_code_t;

const char *app_error_code_string(app_error_code_t code);

#ifdef __cplusplus
}
#endif

/* True when an error means "this one object is unusable" rather than "the
 * device is failing". Only the former may be skipped by a tolerant bulk read:
 * skipping an out-of-memory or I/O error would silently drop objects that are
 * perfectly good, turning a device fault into invisible data loss. */
bool app_error_is_object_fault(app_error_code_t error);

#endif
