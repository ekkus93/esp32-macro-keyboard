#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include "app_error.h"

/* Starts the trusted physical UART0 command console described by
 * SPEC_V2.md. Unlike the HTTP API, commands issued here require no session or
 * additional confirmation because physical/USB possession is the authorization
 * boundary. Registers every serial-console command and starts the REPL task;
 * returns once the REPL task is running. */
app_error_code_t serial_console_start(void);
/* Make one validated one-time setup code available to the explicit physical
 * UART `setup-code` command. The value is never emitted automatically. Use
 * serial_console_clear_setup_code() to retire it after setup or on failure. */
app_error_code_t serial_console_publish_setup_code(const char *setup_code);
void serial_console_clear_setup_code(void);

#endif
