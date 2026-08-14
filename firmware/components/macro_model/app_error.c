#include "app_error.h"

const char *app_error_code_string(app_error_code_t code) {
    switch (code) {
    case APP_ERROR_NONE:
        return "none";
    case APP_ERROR_INVALID_ARGUMENT:
        return "invalid_argument";
    case APP_ERROR_NOT_FOUND:
        return "not_found";
    case APP_ERROR_CONFLICT:
        return "conflict";
    case APP_ERROR_STORAGE_UNAVAILABLE:
        return "storage_unavailable";
    case APP_ERROR_STORAGE_FULL:
        return "storage_full";
    case APP_ERROR_STORAGE_CORRUPT:
        return "storage_corrupt";
    case APP_ERROR_MACRO_SYNTAX:
        return "macro_syntax";
    case APP_ERROR_MACRO_LIMIT:
        return "macro_limit";
    case APP_ERROR_USB_NOT_READY:
        return "usb_not_ready";
    case APP_ERROR_EXECUTOR_BUSY:
        return "executor_busy";
    case APP_ERROR_EXECUTION_CANCELLED:
        return "execution_cancelled";
    case APP_ERROR_AUTH_REQUIRED:
        return "auth_required";
    case APP_ERROR_AUTH_FAILED:
        return "auth_failed";
    case APP_ERROR_AUTH_STATE_INCOMPLETE:
        return "auth_state_incomplete";
    case APP_ERROR_RESET_RECOVERY_REQUIRED:
        return "reset_recovery_required";
    case APP_ERROR_RESET_SETTINGS_INCOMPLETE:
        return "reset_settings_incomplete";
    case APP_ERROR_COMMIT_UNCERTAIN:
        return "commit_uncertain";
    case APP_ERROR_RATE_LIMITED:
        return "rate_limited";
    case APP_ERROR_TIMEOUT:
        return "timeout";
    case APP_ERROR_IO:
        return "io";
    case APP_ERROR_INTERNAL:
        return "internal";
    default:
        return "unknown";
    }
}

bool app_error_is_object_fault(app_error_code_t error) {
    switch (error) {
    case APP_ERROR_STORAGE_CORRUPT:
    case APP_ERROR_MACRO_SYNTAX:
    case APP_ERROR_MACRO_LIMIT:
    case APP_ERROR_INVALID_ARGUMENT:
    case APP_ERROR_NOT_FOUND:
        return true;
    default:
        /* APP_ERROR_INTERNAL (allocation), APP_ERROR_IO, storage unavailable,
         * timeouts, commit uncertainty: the device is failing, not one object. */
        return false;
    }
}
