#include "app_error.h"

const char *app_error_code_string(app_error_code_t code)
{
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
