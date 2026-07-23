#ifndef AUTH_CORE_INTERNAL_H
#define AUTH_CORE_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "auth_core.h"

#define AUTH_CORE_PBKDF2_ITERATIONS 120000U
#define AUTH_CORE_SESSION_IDLE_US (15ULL * 60ULL * 1000000ULL)
#define AUTH_CORE_MAX_FAILURES 5U
#define AUTH_CORE_FAILURE_WINDOW_US (60ULL * 1000000ULL)
#define AUTH_CORE_PASSWORD_MIN_BYTES 12U
#define AUTH_CORE_PASSWORD_MAX_BYTES 128U
#define AUTH_CORE_TOKEN_GENERATION_ATTEMPTS 4U

typedef struct {
    auth_session_entry_t sessions[APP_SESSION_TABLE_MAX];
    uint32_t failure_count;
    uint64_t failure_window_start_us;
    bool failure_window_active;
    uint64_t last_now_us;
    bool clock_initialized;
} auth_core_state_snapshot_t;

bool auth_core_constant_time_equal(const uint8_t *left,
                                   const uint8_t *right,
                                   size_t length);
bool auth_core_valid_hex_token(const char *token);
app_error_code_t auth_core_lock(auth_core_t *core);
app_error_code_t auth_core_unlock(auth_core_t *core);
app_error_code_t auth_core_read_now(auth_core_t *core, uint64_t *out_now);
app_error_code_t auth_core_generate_session_tokens(auth_core_t *core,
                                                   auth_session_view_t *view);
void auth_core_snapshot_state(const auth_core_t *core,
                              auth_core_state_snapshot_t *snapshot);
void auth_core_restore_state(auth_core_t *core,
                             const auth_core_state_snapshot_t *snapshot);

#endif
