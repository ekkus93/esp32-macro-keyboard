#ifndef ESP_SYSTEM_H
#define ESP_SYSTEM_H

#include <stdint.h>

/* Host-test stand-in for ESP-IDF's components/esp_system/include/esp_system.h.
 * The real header pulls in esp_bit_defs.h/esp_err.h/sdkconfig.h/soc headers and
 * declares dozens of chip-management entry points meaningful only against a
 * real target -- see esp_http_server_stub/esp_http_server.h's header comment
 * for the same rationale applied there. web_server_api.c is the only
 * first-party file compiled for a host test target that includes this header
 * to call esp_restart() after a successful device/restart or
 * device/factory-reset response; this supplies just that one entry point.
 * Deliberately no [[noreturn]]/__attribute__((noreturn)) here, unlike the
 * real esp_restart(): the host-test fake definition in
 * test_web_server_administration_route.c returns normally so the caller's
 * own code after the call site still executes under test.
 *
 * web_server_diagnostics.c additionally needs esp_reset_reason_t and three
 * more entry points (esp_reset_reason(), esp_get_free_heap_size(),
 * esp_get_minimum_free_heap_size()) for GET /api/v1/diagnostics -- values
 * copied from the real header's enum (components/esp_system/include/
 * esp_system.h, ESP-IDF v5.5.5) so a host-test fake's reset-reason mapping
 * exercises the exact same enumerators reset_reason_string() switches on. */

typedef enum {
    ESP_RST_UNKNOWN,
    ESP_RST_POWERON,
    ESP_RST_EXT,
    ESP_RST_SW,
    ESP_RST_PANIC,
    ESP_RST_INT_WDT,
    ESP_RST_TASK_WDT,
    ESP_RST_WDT,
    ESP_RST_DEEPSLEEP,
    ESP_RST_BROWNOUT,
    ESP_RST_SDIO,
    ESP_RST_USB,
    ESP_RST_JTAG,
    ESP_RST_EFUSE,
    ESP_RST_PWR_GLITCH,
    ESP_RST_CPU_LOCKUP,
} esp_reset_reason_t;

void esp_restart(void);
esp_reset_reason_t esp_reset_reason(void);
uint32_t esp_get_free_heap_size(void);
uint32_t esp_get_minimum_free_heap_size(void);

#endif
