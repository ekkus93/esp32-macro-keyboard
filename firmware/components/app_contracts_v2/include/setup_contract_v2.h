#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "api_contracts_v2.h"
#include "device_settings_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_V2_SETUP_CODE_DIGITS UINT32_C(8)
#define APP_V2_SETUP_CODE_BUFFER_BYTES UINT32_C(9)
#define APP_V2_SETUP_CODE_SPACE UINT32_C(100000000)
#define APP_V2_SETUP_RANDOM_ATTEMPTS_MAX UINT32_C(32)

typedef enum {
    APP_V2_SETUP_OK = 0,
    APP_V2_SETUP_INVALID_ARGUMENT,
    APP_V2_SETUP_RANDOM_FAILURE,
    APP_V2_SETUP_MALFORMED_CODE,
    APP_V2_SETUP_CODE_MISMATCH,
    APP_V2_SETUP_CODE_CONSUMED,
    APP_V2_SETUP_ALREADY_PROVISIONED,
    APP_V2_SETUP_INVALID_CURRENT_SETTINGS,
    APP_V2_SETUP_INVALID_DEVICE_NAME,
    APP_V2_SETUP_INVALID_AP_SSID,
    APP_V2_SETUP_INVALID_AP_PASSPHRASE,
    APP_V2_SETUP_INVALID_ADMIN_PASSWORD,
    APP_V2_SETUP_INVALID_PASSWORD_MATERIAL,
} app_v2_setup_result_t;

typedef struct {
    void *context;
    bool (*random_u32)(void *context, uint32_t *out_value);
} app_v2_setup_random_ops_t;

typedef struct {
    char code[APP_V2_SETUP_CODE_BUFFER_BYTES];
    bool consumed;
} app_v2_setup_session_t;

typedef struct {
    uint16_t credential_version;
    uint16_t password_algorithm_version;
    uint32_t password_iterations;
    uint8_t password_salt[APP_V2_PASSWORD_SALT_BYTES];
    uint8_t password_verifier[APP_V2_PASSWORD_VERIFIER_BYTES];
} app_v2_setup_password_material_t;

app_v2_setup_result_t app_v2_setup_session_generate(const app_v2_setup_random_ops_t *operations,
                                                    app_v2_setup_session_t *session);
app_v2_setup_result_t app_v2_setup_session_init(app_v2_setup_session_t *session,
                                                app_v2_string_view_t code);
app_v2_setup_result_t app_v2_setup_session_consume(app_v2_setup_session_t *session);

app_v2_setup_result_t app_v2_setup_state_from_settings(const app_v2_device_settings_t *settings,
                                                       app_v2_setup_state_response_t *out_response);

app_v2_setup_result_t
app_v2_setup_prepare_candidate(const app_v2_setup_session_t *session,
                               const app_v2_setup_request_t *request,
                               const app_v2_device_settings_t *current,
                               const app_v2_setup_password_material_t *password_material,
                               app_v2_device_settings_t *out_candidate);

void app_v2_setup_accepted_response_init(app_v2_setup_accepted_t *response);

#ifdef __cplusplus
}
#endif
