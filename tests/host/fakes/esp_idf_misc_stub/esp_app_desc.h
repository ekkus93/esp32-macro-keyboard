#ifndef ESP_APP_DESC_H
#define ESP_APP_DESC_H

/* Host-test stand-in for ESP-IDF's
 * components/esp_app_format/include/esp_app_desc.h. The real header pulls in
 * esp_assert.h/sdkconfig.h and is only meaningful against a real linked
 * app-description section, so it cannot be compiled or linked on the host --
 * see esp_http_server_stub/esp_http_server.h's header comment for the same
 * rationale applied there. web_server_status_limits.c and
 * web_server_diagnostics.c are the only first-party files that include this
 * on a host build; this supplies just the one struct field (`version`) and
 * the two entry points they call. */

#include <stddef.h>

typedef struct {
    char version[32];
} esp_app_desc_t;

const esp_app_desc_t *esp_app_get_description(void);
int esp_app_get_elf_sha256(char *dst, size_t size);

#endif
