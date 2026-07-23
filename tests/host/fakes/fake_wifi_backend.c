#include "fake_wifi_backend.h"

#include <stdlib.h>
#include <string.h>

static const char *operation_name(fake_wifi_operation_t operation)
{
    static const char *const names[FAKE_WIFI_OPERATION_COUNT] = {
        "wifi_netif_init",
        "wifi_event_loop",
        "wifi_create_ap",
        "wifi_init",
        "wifi_register_handler",
        "wifi_set_mode",
        "wifi_set_config",
        "wifi_start",
        "wifi_stop",
        "wifi_unregister_handler",
        "wifi_deinit",
        "wifi_destroy_ap",
    };
    if (operation < 0 || operation >= FAKE_WIFI_OPERATION_COUNT) {
        abort();
    }
    return names[(size_t)operation];
}

void fake_wifi_backend_reset(fake_wifi_backend_t *wifi)
{
    if (wifi == NULL) {
        abort();
    }
    memset(wifi, 0, sizeof(*wifi));
    fake_call_log_reset(&wifi->calls);
}

void fake_wifi_backend_set_result(fake_wifi_backend_t *wifi,
                                  fake_wifi_operation_t operation,
                                  int result)
{
    if (wifi == NULL || operation < 0 || operation >= FAKE_WIFI_OPERATION_COUNT) {
        abort();
    }
    wifi->results[(size_t)operation] = result;
}

int fake_wifi_backend_call(fake_wifi_backend_t *wifi, fake_wifi_operation_t operation)
{
    if (wifi == NULL) {
        abort();
    }
    const char *const name = operation_name(operation);
    if (fake_call_log_record(&wifi->calls, name, 0U, 0U)) {
        return -1;
    }
    return wifi->results[(size_t)operation];
}

void fake_wifi_backend_capture_config(fake_wifi_backend_t *wifi,
                                      const char *ssid,
                                      const char *password,
                                      size_t max_clients)
{
    if (wifi == NULL || ssid == NULL || password == NULL || strlen(ssid) > 32U ||
        strlen(password) > 63U) {
        abort();
    }
    memcpy(wifi->configured_ssid, ssid, strlen(ssid) + 1U);
    memcpy(wifi->configured_password, password, strlen(password) + 1U);
    wifi->configured_max_clients = max_clients;
    (void)fake_call_log_record(&wifi->calls, "wifi_capture_config", max_clients, 0U);
}
