#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "macro_limits.h"

#define AUTH_SALT_BYTES 16U
#define AUTH_HASH_BYTES 32U
/* Measured on the reference ESP32-S3R8 (Aug 2026): 5500 iterations gave a
 * 436.6ms median / 496.8ms worst-case PBKDF2-HMAC-SHA-256 verify, the
 * highest of the measured candidates that stays under the 500ms target
 * ceiling. See docs/implementation-v2/V2_041_PBKDF2_BENCHMARK_2026-08-08.md
 * for the full candidate sweep and raw PBKDF2_BENCH data. */
#define AUTH_PBKDF2_ITERATIONS 5500U
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
/* Read-only: idle/absolute seconds remaining for an already-active,
 * unexpired session, without refreshing its idle deadline. Used by
 * GET /api/v1/auth/session (SPEC_V2 13.5), which is called only after the
 * request policy layer has already validated (and thereby refreshed) the
 * session, so a second refresh here would be misleading. */
app_error_code_t auth_session_remaining(const char *session_token,
                                        uint32_t *out_idle_seconds_remaining,
                                        uint32_t *out_absolute_seconds_remaining);
app_error_code_t auth_session_logout(const char *session_token);
/* Invalidates every active session. Used by the device restart/reset-settings/
 * factory-reset actions (SPEC_V2.md §13.12), which invalidate all sessions
 * before scheduling a reboot rather than requiring the caller's own token. */
app_error_code_t auth_session_logout_all(void);
app_error_code_t auth_login_attempt_allowed(uint32_t source_ipv4,
                                            uint32_t *out_retry_after_seconds);
app_error_code_t auth_login_record_failure(uint32_t source_ipv4);
app_error_code_t auth_login_record_success(uint32_t source_ipv4);

#endif
