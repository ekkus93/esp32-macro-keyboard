#include "wifi_ap_station.h"

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "wifi_ap.h"

bool wifi_ap_station_ops_is_valid(const wifi_ap_station_ops_t *operations) {
    return operations != NULL && operations->status_get != NULL && operations->status_set != NULL &&
           operations->connect_attempt != NULL;
}

static void publish(const wifi_ap_station_ops_t *operations, wifi_station_state_t state,
                    app_error_code_t error, uint32_t attempt_count) {
    const wifi_station_status_t status = {
        .state = state,
        .last_error = error,
        .attempt_count = attempt_count,
    };
    operations->status_set(operations->context, &status);
}

app_error_code_t wifi_ap_station_connect(const wifi_ap_station_ops_t *operations, const char *ssid,
                                         const char *password, uint32_t timeout_ms, char *out_ip,
                                         size_t out_ip_size) {
    if (!wifi_ap_station_ops_is_valid(operations) || ssid == NULL || password == NULL ||
        out_ip == NULL || out_ip_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_error_code_t last_error = APP_ERROR_NONE;
    for (uint32_t attempt = 1U; attempt <= WIFI_STATION_MAX_ATTEMPTS; ++attempt) {
        publish(operations, WIFI_STATION_CONNECTING, APP_ERROR_NONE, attempt);
        last_error = operations->connect_attempt(operations->context, ssid, password, timeout_ms,
                                                 out_ip, out_ip_size);
        if (last_error == APP_ERROR_NONE) {
            publish(operations, WIFI_STATION_CONNECTED, APP_ERROR_NONE, attempt);
            return APP_ERROR_NONE;
        }
    }
    publish(operations, WIFI_STATION_FAILED, last_error, WIFI_STATION_MAX_ATTEMPTS);
    return last_error;
}
