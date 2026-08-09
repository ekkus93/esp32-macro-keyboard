#ifndef ESP_TIMER_H
#define ESP_TIMER_H

/* Host-test stand-in for ESP-IDF's components/esp_timer/include/esp_timer.h
 * -- see esp_app_desc.h in this directory for why the real header cannot be
 * compiled on a host build. web_server_status_limits.c and
 * web_server_diagnostics.c are the only first-party files that include this
 * on a host build; this supplies just the one entry point they call. */

#include <stdint.h>

int64_t esp_timer_get_time(void);

#endif
