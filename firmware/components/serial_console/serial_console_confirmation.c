#include "serial_console_confirmation.h"

#include <stddef.h>

#include "app_error.h"

app_error_code_t serial_console_route_confirmation(const serial_console_confirmation_ops_t *ops) {
    if (ops == NULL || ops->confirm_send == NULL || ops->confirm_device_action == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const app_error_code_t send_result = ops->confirm_send(ops->context);
    if (send_result == APP_ERROR_NONE || send_result == APP_ERROR_CONFLICT) {
        return APP_ERROR_NONE;
    }
    if (send_result != APP_ERROR_NOT_FOUND) {
        return send_result;
    }

    return ops->confirm_device_action(ops->context);
}
