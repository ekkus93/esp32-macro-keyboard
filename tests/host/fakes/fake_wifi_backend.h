#ifndef FAKE_WIFI_BACKEND_H
#define FAKE_WIFI_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#include "fake_call_log.h"

typedef enum {
    FAKE_WIFI_NETIF_INIT = 0,
    FAKE_WIFI_EVENT_LOOP,
    FAKE_WIFI_CREATE_AP,
    FAKE_WIFI_INIT,
    FAKE_WIFI_REGISTER_HANDLER,
    FAKE_WIFI_SET_MODE,
    FAKE_WIFI_SET_CONFIG,
    FAKE_WIFI_START,
    FAKE_WIFI_STOP,
    FAKE_WIFI_UNREGISTER_HANDLER,
    FAKE_WIFI_DEINIT,
    FAKE_WIFI_DESTROY_AP,
    FAKE_WIFI_OPERATION_COUNT
} fake_wifi_operation_t;

typedef struct {
    int results[FAKE_WIFI_OPERATION_COUNT];
    uint8_t configured_ssid[33U];
    uint8_t configured_password[64U];
    size_t configured_max_clients;
    fake_call_log_t calls;
} fake_wifi_backend_t;

void fake_wifi_backend_reset(fake_wifi_backend_t *wifi);
void fake_wifi_backend_set_result(fake_wifi_backend_t *wifi,
                                  fake_wifi_operation_t operation,
                                  int result);
int fake_wifi_backend_call(fake_wifi_backend_t *wifi, fake_wifi_operation_t operation);
void fake_wifi_backend_capture_config(fake_wifi_backend_t *wifi,
                                      const char *ssid,
                                      const char *password,
                                      size_t max_clients);

#endif
