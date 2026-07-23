#include "auth.h"

#include <stdint.h>

#include "auth_core.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"

static SemaphoreHandle_t auth_mutex;
static auth_core_t auth_core;

static bool adapter_lock(void *context)
{
    (void)context;
    return auth_mutex != NULL && xSemaphoreTake(auth_mutex, portMAX_DELAY) == pdTRUE;
}

static bool adapter_unlock(void *context)
{
    (void)context;
    return auth_mutex != NULL && xSemaphoreGive(auth_mutex) == pdTRUE;
}

static bool adapter_random_fill(void *context, uint8_t *output, size_t length)
{
    (void)context;
    if (output == NULL && length != 0U) {
        return false;
    }
    esp_fill_random(output, length);
    return true;
}

static uint64_t adapter_now_us(void *context)
{
    (void)context;
    const int64_t now = esp_timer_get_time();
    return now < 0 ? 0U : (uint64_t)now;
}

static int adapter_derive(void *context,
                          const char *password,
                          size_t password_length,
                          const uint8_t *salt,
                          uint32_t iterations,
                          uint8_t *output)
{
    (void)context;
    return mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
                                         (const unsigned char *)password,
                                         password_length,
                                         salt,
                                         AUTH_SALT_BYTES,
                                         iterations,
                                         AUTH_HASH_BYTES,
                                         output);
}

static void adapter_secure_zero(void *context, void *memory, size_t length)
{
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

app_error_code_t auth_init(void)
{
    if (auth_mutex != NULL) {
        return APP_ERROR_CONFLICT;
    }
    auth_mutex = xSemaphoreCreateMutex();
    if (auth_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const auth_ops_t ops = {
        .context = NULL,
        .lock = adapter_lock,
        .unlock = adapter_unlock,
        .random_fill = adapter_random_fill,
        .now_us = adapter_now_us,
        .derive = adapter_derive,
        .secure_zero = adapter_secure_zero,
    };
    const app_error_code_t result = auth_core_init(&auth_core, &ops);
    if (result != APP_ERROR_NONE) {
        vSemaphoreDelete(auth_mutex);
        auth_mutex = NULL;
    }
    return result;
}

app_error_code_t auth_password_create(const char *password,
                                      size_t password_length,
                                      auth_password_record_t *out_record)
{
    return auth_core_password_create(&auth_core, password, password_length, out_record);
}

bool auth_password_verify(const char *password,
                          size_t password_length,
                          const auth_password_record_t *record)
{
    return auth_core_password_verify(&auth_core, password, password_length, record);
}

app_error_code_t auth_session_create(auth_session_view_t *out_session)
{
    return auth_core_session_create(&auth_core, out_session);
}

app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token)
{
    return auth_core_session_validate(&auth_core, session_token, csrf_token);
}

app_error_code_t auth_session_logout(const char *session_token)
{
    return auth_core_session_logout(&auth_core, session_token);
}

app_error_code_t auth_login_attempt_allowed(uint32_t *out_retry_after_seconds)
{
    return auth_core_login_attempt_allowed(&auth_core, out_retry_after_seconds);
}

app_error_code_t auth_login_record_failure(void)
{
    return auth_core_login_record_failure(&auth_core);
}

app_error_code_t auth_login_record_success(void)
{
    return auth_core_login_record_success(&auth_core);
}
