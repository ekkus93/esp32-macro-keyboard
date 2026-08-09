#ifndef ESP_SYSTEM_H
#define ESP_SYSTEM_H

/* Host-test stand-in for ESP-IDF's components/esp_system/include/esp_system.h.
 * The real header pulls in esp_bit_defs.h/esp_err.h/sdkconfig.h/soc headers and
 * declares dozens of chip-management entry points meaningful only against a
 * real target -- see esp_http_server_stub/esp_http_server.h's header comment
 * for the same rationale applied there. web_server_api.c is the only
 * first-party file compiled for a host test target that includes this header
 * (to call esp_restart() after a successful device/restart or
 * device/factory-reset response); this supplies just that one entry point.
 * Deliberately no [[noreturn]]/__attribute__((noreturn)) here, unlike the
 * real esp_restart(): the host-test fake definition in
 * test_web_server_administration_route.c returns normally so the caller's
 * own code after the call site still executes under test. */

void esp_restart(void);

#endif
