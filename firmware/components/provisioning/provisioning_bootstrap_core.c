#include "provisioning_bootstrap_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "provisioning_bootstrap.h"

#define DOMAIN_BYTES 20U
#define MESSAGE_BYTES (DOMAIN_BYTES + PROVISIONING_DEVICE_ID_BYTES)
#define SSID_PREFIX "ESP32-Macro-"
#define SSID_PREFIX_BYTES 12U
#define SSID_SUFFIX_BYTES 6U
#define HEX_NIBBLE_SHIFT 4U
#define HEX_NIBBLE_MASK UINT8_C(0x0f)

static const uint8_t AP_DOMAIN[DOMAIN_BYTES] = {
    'm', 'a', 'c', 'r', 'o', '-', 's', 'e', 't', 'u', 'p', '-', 'a', 'p', '-', 'v', '1', 0, 0, 0,
};
static const char HEX[] = "0123456789ABCDEF";

static bool operations_valid(const provisioning_bootstrap_ops_t *operations) {
    return operations != NULL && operations->read_device_id != NULL &&
           operations->calculate_hmac != NULL && operations->secure_zero != NULL;
}

static void encode_hex(const uint8_t *input, size_t input_size, char *output,
                       size_t output_hex_bytes) {
    const size_t bytes_to_encode = output_hex_bytes / 2U;
    for (size_t index = 0U; index < bytes_to_encode && index < input_size; ++index) {
        output[index * 2U] = HEX[input[index] >> HEX_NIBBLE_SHIFT];
        output[(index * 2U) + 1U] = HEX[input[index] & HEX_NIBBLE_MASK];
    }
    output[output_hex_bytes] = '\0';
}

static app_error_code_t derive_secret(const provisioning_bootstrap_ops_t *operations,
                                      const uint8_t domain[DOMAIN_BYTES],
                                      const uint8_t device_id[PROVISIONING_DEVICE_ID_BYTES],
                                      char output[PROVISIONING_SETUP_SECRET_BUFFER_BYTES]) {
    uint8_t message[MESSAGE_BYTES] = {0};
    uint8_t digest[PROVISIONING_HMAC_BYTES] = {0};
    memcpy(message, domain, DOMAIN_BYTES);
    memcpy(message + DOMAIN_BYTES, device_id, PROVISIONING_DEVICE_ID_BYTES);
    const app_error_code_t result =
        operations->calculate_hmac(operations->context, message, sizeof(message), digest);
    if (result == APP_ERROR_NONE) {
        encode_hex(digest, sizeof(digest), output, PROVISIONING_SETUP_SECRET_HEX_BYTES);
    }
    operations->secure_zero(operations->context, digest, sizeof(digest));
    operations->secure_zero(operations->context, message, sizeof(message));
    return result;
}

app_error_code_t
provisioning_bootstrap_derive_with_ops(const provisioning_bootstrap_ops_t *operations,
                                       provisioning_bootstrap_t *out_bootstrap) {
    if (out_bootstrap != NULL) {
        memset(out_bootstrap, 0, sizeof(*out_bootstrap));
    }
    if (!operations_valid(operations) || out_bootstrap == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t device_id[PROVISIONING_DEVICE_ID_BYTES] = {0};
    app_error_code_t result = operations->read_device_id(operations->context, device_id);
    if (result == APP_ERROR_NONE) {
        encode_hex(device_id, sizeof(device_id), out_bootstrap->device_id,
                   PROVISIONING_DEVICE_ID_HEX_BYTES);
        memcpy(out_bootstrap->ap_ssid, SSID_PREFIX, SSID_PREFIX_BYTES);
        memcpy(out_bootstrap->ap_ssid + SSID_PREFIX_BYTES,
               out_bootstrap->device_id + (PROVISIONING_DEVICE_ID_HEX_BYTES - SSID_SUFFIX_BYTES),
               SSID_SUFFIX_BYTES);
        out_bootstrap->ap_ssid[SSID_PREFIX_BYTES + SSID_SUFFIX_BYTES] = '\0';
        result = derive_secret(operations, AP_DOMAIN, device_id, out_bootstrap->ap_passphrase);
    }
    operations->secure_zero(operations->context, device_id, sizeof(device_id));
    if (result != APP_ERROR_NONE) {
        operations->secure_zero(operations->context, out_bootstrap, sizeof(*out_bootstrap));
    }
    return result;
}
