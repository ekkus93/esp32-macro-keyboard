#ifndef PROVISIONING_BOOTSTRAP_CORE_H
#define PROVISIONING_BOOTSTRAP_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "provisioning_bootstrap.h"

#define PROVISIONING_DEVICE_ID_BYTES 6U
#define PROVISIONING_HMAC_BYTES 32U

typedef struct {
    void *context;
    app_error_code_t (*read_device_id)(void *context, uint8_t output[PROVISIONING_DEVICE_ID_BYTES]);
    app_error_code_t (*calculate_hmac)(void *context, const uint8_t *message, size_t message_size,
                                       uint8_t output[PROVISIONING_HMAC_BYTES]);
    void (*secure_zero)(void *context, void *memory, size_t size);
} provisioning_bootstrap_ops_t;

app_error_code_t
provisioning_bootstrap_derive_with_ops(const provisioning_bootstrap_ops_t *operations,
                                       provisioning_bootstrap_t *out_bootstrap);

#endif
