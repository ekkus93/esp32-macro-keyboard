#ifndef AUTH_CORE_H
#define AUTH_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "auth.h"
#include "auth_ops.h"

#define AUTH_RATE_LIMIT_SOURCE_MAX 8U
#define AUTH_RATE_LIMIT_FAILURE_MAX 5U

typedef struct {
    bool active;
    auth_session_view_t view;
    uint64_t last_used_us;
} auth_session_entry_t;

typedef struct {
    bool active;
    uint32_t source_ipv4;
    uint64_t failures_us[AUTH_RATE_LIMIT_FAILURE_MAX];
    size_t failure_count;
    uint64_t lockout_until_us;
    uint64_t last_used_us;
} auth_rate_limit_entry_t;

typedef struct {
    auth_ops_t ops;
    auth_session_entry_t sessions[APP_SESSION_TABLE_MAX];
    auth_rate_limit_entry_t rate_limits[AUTH_RATE_LIMIT_SOURCE_MAX];
    uint64_t last_now_us;
    bool clock_initialized;
} auth_core_t;

app_error_code_t auth_core_init(auth_core_t *core, const auth_ops_t *operations);
app_error_code_t auth_core_password_create(auth_core_t *core, const char *password,
                                           size_t password_length,
                                           auth_password_record_t *out_record);
app_error_code_t auth_core_password_verify(auth_core_t *core, const char *password,
                                           size_t password_length,
                                           const auth_password_record_t *record, bool *out_matches);
app_error_code_t auth_core_session_create(auth_core_t *core, auth_session_view_t *out_session);
app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token);
app_error_code_t auth_core_session_logout(auth_core_t *core, const char *session_token);
app_error_code_t auth_core_login_attempt_allowed(auth_core_t *core, uint32_t source_ipv4,
                                                 uint32_t *out_retry_after_seconds);
app_error_code_t auth_core_login_record_failure(auth_core_t *core, uint32_t source_ipv4);
app_error_code_t auth_core_login_record_success(auth_core_t *core, uint32_t source_ipv4);

#endif
