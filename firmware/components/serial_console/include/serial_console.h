#ifndef SERIAL_CONSOLE_H
#define SERIAL_CONSOLE_H

#include "app_error.h"

/* Starts an interactive command console over the USB-Serial-JTAG debug
 * port - the same port used for flashing and `idf.py monitor`. This is a
 * debug/development feature added at the repository owner's explicit
 * request: it is deliberately outside SPEC.md's reviewed production
 * security model. Unlike the HTTP API, commands issued here require no
 * session or physical confirmation - physical/USB access to
 * this port is implicitly trusted. Registers every serial-console command
 * and starts the REPL task; returns once the REPL task is running. */
app_error_code_t serial_console_start(void);

#endif
