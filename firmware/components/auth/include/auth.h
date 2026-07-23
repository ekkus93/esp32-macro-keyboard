#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "macro_limits.h"

#define AUTH_SALT_BYTES 16U
#define AUTH_HASH_BYTES 32U
#define AUTH_TOKEN_HEX_BYTES ((APP_SESSION_TOKEN_BYTES * 2U) + 1U)

typedef struct {
    uint8_t salt[AUTH_SALT_BYTES];
    uint8_t hash[AUTH_HASH_BYTES];
    uint32_t iterations;
} auth_password_record_t;

typedef struct {
    char session_token[AUTH_TOKEN_HEX_BYTES];
    char csrf_token[AUTH_TOKEN_HEX_BYTES];
    uint64_t expires_at_us;
} auth_session_view_t;

app_error_code_t auth_init(void);
app_error_code_t auth_password_create(const char *password,
                                      size_t password_length,
                                      auth_password_record_t *out_record);
bool auth_password_verify(const char *password,
                          size_t password_length,
                          const auth_password_record_t *record);
app_error_code_t auth_session_create(auth_session_view_t *out_session);
app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token);
app_error_code_t auth_session_logout(const char *session_token);
app_error_code_t auth_login_attempt_allowed(uint32_t *out_retry_after_seconds);
app_error_code_t auth_login_record_failure(void);
app_error_code_t auth_login_record_success(void);

#endif
