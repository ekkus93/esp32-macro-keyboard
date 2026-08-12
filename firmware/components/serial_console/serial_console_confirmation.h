#ifndef SERIAL_CONSOLE_CONFIRMATION_H
#define SERIAL_CONSOLE_CONFIRMATION_H

#include "app_error.h"

typedef struct {
    void *context;
    app_error_code_t (*confirm_send)(void *context);
    app_error_code_t (*confirm_device_action)(void *context);
} serial_console_confirmation_ops_t;

/* Route one generic `confirm` command to exactly one pending confirmation
 * domain. A pending send has precedence so one command can never authorize
 * both a send and an unrelated administrative operation. If the send domain
 * reports a real failure, fail closed rather than falling through. */
app_error_code_t serial_console_route_confirmation(const serial_console_confirmation_ops_t *ops);

#endif
