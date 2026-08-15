#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include "app_error.h"

/* The v2 setup code is an ephemeral, per-unprovisioned-boot secret. This is
 * the only production plaintext disclosure boundary: trusted UART0 console
 * output required by SPEC_V2. It validates the 8-digit wire format and fails
 * if the console stream cannot be flushed. */
app_error_code_t serial_console_show_setup_code(const char *setup_code);

/* Starts the interactive command console on UART0, exposed by the devkit
 * USB-to-UART bridge. Physical access to this console is the trusted boundary
 * for setup-code disclosure plus confirmation/cancellation commands. Registers
 * every serial-console command and starts the REPL task; returns once the REPL
 * task is running. */
app_error_code_t serial_console_start(void);

#endif
