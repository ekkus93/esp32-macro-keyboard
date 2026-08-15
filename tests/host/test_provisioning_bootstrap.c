#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "provisioning_bootstrap_core.h"
#include "test_assert.h"

#define BOOTSTRAP_MESSAGE_BYTES 26U
#define BOOTSTRAP_DOMAIN_BYTES 20U

typedef struct {
    uint8_t device_id[PROVISIONING_DEVICE_ID_BYTES];
    app_error_code_t device_error;
    app_error_code_t hmac_error;
    unsigned int hmac_calls;
    uint8_t messages[1][BOOTSTRAP_MESSAGE_BYTES];
    size_t message_sizes[1];
    unsigned int zero_calls;
    size_t zero_bytes;
} fake_bootstrap_t;

static app_error_code_t fake_read_device_id(void *context,
                                            uint8_t output[PROVISIONING_DEVICE_ID_BYTES]) {
    fake_bootstrap_t *fake = context;
    if (fake->device_error != APP_ERROR_NONE) {
        return fake->device_error;
    }
    memcpy(output, fake->device_id, sizeof(fake->device_id));
    return APP_ERROR_NONE;
}

static app_error_code_t fake_calculate_hmac(void *context, const uint8_t *message,
                                            size_t message_size,
                                            uint8_t output[PROVISIONING_HMAC_BYTES]) {
    fake_bootstrap_t *fake = context;
    if (fake->hmac_calls < 1U) {
        memcpy(fake->messages[fake->hmac_calls], message, message_size);
        fake->message_sizes[fake->hmac_calls] = message_size;
    }
    ++fake->hmac_calls;
    if (fake->hmac_error != APP_ERROR_NONE) {
        return fake->hmac_error;
    }
    for (size_t index = 0U; index < PROVISIONING_HMAC_BYTES; ++index) {
        output[index] = (uint8_t)(index + fake->hmac_calls);
    }
    return APP_ERROR_NONE;
}

static void fake_secure_zero(void *context, void *memory, size_t size) {
    fake_bootstrap_t *fake = context;
    memset(memory, 0, size);
    ++fake->zero_calls;
    fake->zero_bytes += size;
}

static provisioning_bootstrap_ops_t operations(fake_bootstrap_t *fake) {
    return (provisioning_bootstrap_ops_t){
        .context = fake,
        .read_device_id = fake_read_device_id,
        .calculate_hmac = fake_calculate_hmac,
        .secure_zero = fake_secure_zero,
    };
}

static void test_success_derives_only_stable_ap_credentials(void) {
    fake_bootstrap_t fake = {
        .device_id = {0x10U, 0x20U, 0x30U, 0xa0U, 0xb0U, 0xc0U},
    };
    const provisioning_bootstrap_ops_t ops = operations(&fake);
    provisioning_bootstrap_t bootstrap;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, provisioning_bootstrap_derive_with_ops(&ops, &bootstrap));
    TEST_CHECK_EQ_STRING("102030A0B0C0", bootstrap.device_id);
    TEST_CHECK_EQ_STRING("ESP32-Macro-A0B0C0", bootstrap.ap_ssid);
    TEST_CHECK_EQ_STRING("0102030405060708090A0B0C", bootstrap.ap_passphrase);
    TEST_CHECK_EQ_U64(1U, fake.hmac_calls);
    TEST_CHECK_EQ_U64(BOOTSTRAP_MESSAGE_BYTES, fake.message_sizes[0]);
    TEST_CHECK_EQ_BUFFER(fake.device_id, fake.messages[0] + BOOTSTRAP_DOMAIN_BYTES,
                         sizeof(fake.device_id));
    TEST_CHECK_EQ_U64(3U, fake.zero_calls);
    TEST_CHECK_EQ_U64(64U, fake.zero_bytes);
}

static void test_argument_validation(void) {
    fake_bootstrap_t fake = {0};
    provisioning_bootstrap_ops_t ops = operations(&fake);
    provisioning_bootstrap_t bootstrap;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_bootstrap_derive_with_ops(NULL, &bootstrap));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_bootstrap_derive_with_ops(&ops, NULL));
    ops.read_device_id = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_bootstrap_derive_with_ops(&ops, &bootstrap));
    ops = operations(&fake);
    ops.calculate_hmac = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_bootstrap_derive_with_ops(&ops, &bootstrap));
    ops = operations(&fake);
    ops.secure_zero = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         provisioning_bootstrap_derive_with_ops(&ops, &bootstrap));
}

static void test_device_failure_clears_output(void) {
    fake_bootstrap_t fake = {
        .device_error = APP_ERROR_INTERNAL,
    };
    const provisioning_bootstrap_ops_t ops = operations(&fake);
    provisioning_bootstrap_t bootstrap;
    memset(&bootstrap, 0xaa, sizeof(bootstrap));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         provisioning_bootstrap_derive_with_ops(&ops, &bootstrap));
    const uint8_t zero[sizeof(bootstrap)] = {0};
    TEST_CHECK_EQ_BUFFER(zero, &bootstrap, sizeof(bootstrap));
    TEST_CHECK_EQ_U64(0U, fake.hmac_calls);
}

static void test_hmac_failure_clears_output(void) {
    fake_bootstrap_t fake = {
        .device_id = {1U, 2U, 3U, 4U, 5U, 6U},
        .hmac_error = APP_ERROR_INTERNAL,
    };
    const provisioning_bootstrap_ops_t ops = operations(&fake);
    provisioning_bootstrap_t bootstrap;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         provisioning_bootstrap_derive_with_ops(&ops, &bootstrap));
    const uint8_t zero[sizeof(bootstrap)] = {0};
    TEST_CHECK_EQ_BUFFER(zero, &bootstrap, sizeof(bootstrap));
    TEST_CHECK_EQ_U64(1U, fake.hmac_calls);
}

int main(void) {
    test_success_derives_only_stable_ap_credentials();
    test_argument_validation();
    test_device_failure_clears_output();
    test_hmac_failure_clears_output();
    puts("provisioning bootstrap tests passed");
    return EXIT_SUCCESS;
}
