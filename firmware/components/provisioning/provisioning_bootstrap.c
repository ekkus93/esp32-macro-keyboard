#include "provisioning_bootstrap.h"

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "esp_err.h"
#include "esp_hmac.h"
#include "esp_mac.h"
#include "provisioning_bootstrap_core.h"
#include "sdkconfig.h"

static app_error_code_t adapter_read_device_id(
    void *context,
    uint8_t output[PROVISIONING_DEVICE_ID_BYTES]) {
    (void)context;
    return esp_read_mac(output, ESP_MAC_WIFI_SOFTAP) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_calculate_hmac(
    void *context,
    const uint8_t *message,
    size_t message_size,
    uint8_t output[PROVISIONING_HMAC_BYTES]) {
    (void)context;
    return esp_hmac_calculate((hmac_key_id_t)CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID,
                              message,
                              message_size,
                              output) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static void adapter_secure_zero(void *context, void *memory, size_t size) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

app_error_code_t provisioning_bootstrap_derive(provisioning_bootstrap_t *out_bootstrap) {
    const provisioning_bootstrap_ops_t operations = {
        .context = NULL,
        .read_device_id = adapter_read_device_id,
        .calculate_hmac = adapter_calculate_hmac,
        .secure_zero = adapter_secure_zero,
    };
    return provisioning_bootstrap_derive_with_ops(&operations, out_bootstrap);
}

void provisioning_bootstrap_clear(provisioning_bootstrap_t *bootstrap) {
    if (bootstrap != NULL) {
        adapter_secure_zero(NULL, bootstrap, sizeof(*bootstrap));
    }
}
