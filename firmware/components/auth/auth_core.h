#ifndef AUTH_CORE_H
#define AUTH_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "auth.h"
#include "auth_ops.h"

typedef struct {
    bool active;
    auth_session_view_t view;
} auth_session_entry_t;

typedef struct {
    auth_ops_t ops;
    auth_session_entry_t sessions[APP_SESSION_TABLE_MAX];
    uint32_t failure_count;
    uint64_t failure_window_start_us;
    bool failure_window_active;
    uint64_t last_now_us;
    bool clock_initialized;
} auth_core_t;

app_error_code_t auth_core_init(auth_core_t *core, const auth_ops_t *ops);
app_error_code_t auth_core_password_create(auth_core_t *core,
                                           const char *password,
                                           size_t password_length,
                                           auth_password_record_t *out_record);
bool auth_core_password_verify(auth_core_t *core,
                               const char *password,
                               size_t password_length,
                               const auth_password_record_t *record);
app_error_code_t auth_core_session_create(auth_core_t *core,
                                          auth_session_view_t *out_session);
app_error_code_t auth_core_session_validate(auth_core_t *core,
                                            const char *session_token,
                                            const char *csrf_token);
app_error_code_t auth_core_session_logout(auth_core_t *core, const char *session_token);
app_error_code_t auth_core_login_attempt_allowed(auth_core_t *core,
                                                 uint32_t *out_retry_after_seconds);
app_error_code_t auth_core_login_record_failure(auth_core_t *core);
app_error_code_t auth_core_login_record_success(auth_core_t *core);

#endif
