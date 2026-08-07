#include "auth_core.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "auth_core_internal.h"

#define MICROSECONDS_PER_SECOND UINT64_C(1000000)

static uint32_t retry_seconds(uint64_t remaining_us) {
    return (uint32_t)((remaining_us + MICROSECONDS_PER_SECOND - UINT64_C(1)) /
                      MICROSECONDS_PER_SECOND);
}

static void clear_expired_lockout(auth_rate_limit_entry_t *entry, uint64_t now) {
    if (entry->lockout_until_us != 0U && entry->lockout_until_us <= now) {
        const uint32_t source_ipv4 = entry->source_ipv4;
        memset(entry, 0, sizeof(*entry));
        entry->active = true;
        entry->source_ipv4 = source_ipv4;
        entry->last_used_us = now;
    }
}

static void prune_failures(auth_rate_limit_entry_t *entry, uint64_t now) {
    if (!entry->active) {
        return;
    }
    clear_expired_lockout(entry, now);
    if (entry->lockout_until_us > now) {
        entry->last_used_us = now;
        return;
    }

    size_t write_index = 0U;
    for (size_t index = 0U; index < entry->failure_count; ++index) {
        if (now - entry->failures_us[index] < AUTH_CORE_FAILURE_WINDOW_US) {
            entry->failures_us[write_index] = entry->failures_us[index];
            ++write_index;
        }
    }
    for (size_t index = write_index; index < AUTH_RATE_LIMIT_FAILURE_MAX; ++index) {
        entry->failures_us[index] = 0U;
    }
    entry->failure_count = write_index;
    entry->last_used_us = now;
}

static auth_rate_limit_entry_t *find_entry(auth_core_t *core, uint32_t source_ipv4) {
    for (size_t index = 0U; index < AUTH_RATE_LIMIT_SOURCE_MAX; ++index) {
        if (core->rate_limits[index].active &&
            core->rate_limits[index].source_ipv4 == source_ipv4) {
            return &core->rate_limits[index];
        }
    }
    return NULL;
}

static auth_rate_limit_entry_t *allocate_entry(auth_core_t *core, uint32_t source_ipv4,
                                               uint64_t now) {
    auth_rate_limit_entry_t *least_recent_unlocked = NULL;
    for (size_t index = 0U; index < AUTH_RATE_LIMIT_SOURCE_MAX; ++index) {
        auth_rate_limit_entry_t *entry = &core->rate_limits[index];
        if (!entry->active) {
            memset(entry, 0, sizeof(*entry));
            entry->active = true;
            entry->source_ipv4 = source_ipv4;
            entry->last_used_us = now;
            return entry;
        }
        prune_failures(entry, now);
        if (entry->lockout_until_us > now) {
            continue;
        }
        if (least_recent_unlocked == NULL ||
            entry->last_used_us < least_recent_unlocked->last_used_us) {
            least_recent_unlocked = entry;
        }
    }
    if (least_recent_unlocked == NULL) {
        return NULL;
    }
    memset(least_recent_unlocked, 0, sizeof(*least_recent_unlocked));
    least_recent_unlocked->active = true;
    least_recent_unlocked->source_ipv4 = source_ipv4;
    least_recent_unlocked->last_used_us = now;
    return least_recent_unlocked;
}

app_error_code_t auth_core_login_attempt_allowed(auth_core_t *core, uint32_t source_ipv4,
                                                 uint32_t *out_retry_after_seconds) {
    if (core == NULL || out_retry_after_seconds == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_retry_after_seconds = 0U;
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    uint64_t now = 0U;
    result = auth_core_read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        auth_rate_limit_entry_t *entry = find_entry(core, source_ipv4);
        if (entry != NULL) {
            prune_failures(entry, now);
            if (entry->lockout_until_us > now) {
                *out_retry_after_seconds = retry_seconds(entry->lockout_until_us - now);
                result = APP_ERROR_RATE_LIMITED;
            }
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        *out_retry_after_seconds = 0U;
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_login_record_failure(auth_core_t *core, uint32_t source_ipv4) {
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    uint64_t now = 0U;
    result = auth_core_read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        auth_rate_limit_entry_t *entry = find_entry(core, source_ipv4);
        if (entry == NULL) {
            entry = allocate_entry(core, source_ipv4, now);
        } else {
            prune_failures(entry, now);
        }
        if (entry == NULL || entry->lockout_until_us > now) {
            result = APP_ERROR_RATE_LIMITED;
        } else if (entry->failure_count >= AUTH_RATE_LIMIT_FAILURE_MAX) {
            result = APP_ERROR_INTERNAL;
        } else {
            entry->failures_us[entry->failure_count] = now;
            ++entry->failure_count;
            entry->last_used_us = now;
            if (entry->failure_count == AUTH_RATE_LIMIT_FAILURE_MAX) {
                if (UINT64_MAX - now < AUTH_CORE_LOCKOUT_US) {
                    result = APP_ERROR_INTERNAL;
                } else {
                    entry->lockout_until_us = now + AUTH_CORE_LOCKOUT_US;
                }
            }
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_login_record_success(auth_core_t *core, uint32_t source_ipv4) {
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    auth_rate_limit_entry_t *entry = find_entry(core, source_ipv4);
    if (entry != NULL) {
        memset(entry, 0, sizeof(*entry));
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}
