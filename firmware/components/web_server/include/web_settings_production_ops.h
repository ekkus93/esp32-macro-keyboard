#ifndef WEB_SETTINGS_PRODUCTION_OPS_H
#define WEB_SETTINGS_PRODUCTION_OPS_H

#include "web_settings.h"

/* Constructs the production web_settings_ops_t, wired to the real auth/
 * device_settings/web_server_password_record backends. Deliberately kept out
 * of web_settings.c itself: that module is otherwise fully ops-injected and
 * has no concrete backend dependency, which is what makes it host-testable
 * in total isolation. This file is the one production adapter both
 * web_api_administration.c (the HTTP change-password route) and
 * serial_console.c (the set-admin-password command) call, so they drive the
 * exact same hardened wiring rather than each constructing their own copy. */
web_settings_ops_t web_settings_production_ops(void);

#endif
