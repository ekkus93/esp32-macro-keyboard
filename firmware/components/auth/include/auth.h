#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "macro_limits.h"

#define AUTH_SALT_BYTES 16U
#define AUTH_HASH_BYTES 32U
/* Provisional PBKDF2-HMAC-SHA-256 iteration count. V2-041 has not yet run the
 * ESP32-S3R8 hardware benchmark that must select the frozen value (target
 * approximately 250-500 ms derivation time); this number is NOT that frozen
 * result and must be replaced once the benchmark lands before V2-040/V2-041
 * are claimed complete. */
#define AUTH_PBKDF2_ITERATIONS 120000U
/* Explicit V2 alias for call sites (setup/provisioning) that derive password
 * material for the V2 device-settings record, so it is unambiguous which
 * value they depend on and why it is not yet final. */
#define AUTH_V2_PBKDF2_ITERATIONS_PROVISIONAL AUTH_PBKDF2_ITERATIONS
#define AUTH_PASSWORD_MIN_BYTES 12U
#define AUTH_PASSWORD_MAX_BYTES 128U
#define AUTH_TOKEN_HEX_BYTES ((APP_SESSION_TOKEN_BYTES * 2U) + 1U)

typedef struct {
    uint8_t salt[AUTH_SALT_BYTES];
    uint8_t hash[AUTH_HASH_BYTES];
    uint32_t iterations;
} auth_password_record_t;

typedef struct {
    char session_token[AUTH_TOKEN_HEX_BYTES];
    uint64_t expires_at_us;
    uint64_t absolute_expires_at_us;
} auth_session_view_t;

app_error_code_t auth_init(void);
app_error_code_t auth_deinit(void);
app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record);
app_error_code_t auth_password_verify(const char *password, size_t password_length,
                                      const auth_password_record_t *record, bool *out_matches);
app_error_code_t auth_session_create(auth_session_view_t *out_session);
app_error_code_t auth_session_validate(const char *session_token);
app_error_code_t auth_session_logout(const char *session_token);
app_error_code_t auth_login_attempt_allowed(uint32_t source_ipv4,
                                            uint32_t *out_retry_after_seconds);
app_error_code_t auth_login_record_failure(uint32_t source_ipv4);
app_error_code_t auth_login_record_success(uint32_t source_ipv4);

#endif
