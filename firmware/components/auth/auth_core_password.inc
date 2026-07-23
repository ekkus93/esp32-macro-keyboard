#include "auth_core.h"

#include <string.h>

#include "auth_core_internal.h"

app_error_code_t auth_core_password_create(auth_core_t *core,
                                           const char *password,
                                           size_t password_length,
                                           auth_password_record_t *out_record)
{
    if (core == NULL || password == NULL || out_record == NULL ||
        password_length < AUTH_CORE_PASSWORD_MIN_BYTES ||
        password_length > AUTH_CORE_PASSWORD_MAX_BYTES ||
        memchr(password, '\0', password_length) != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_record, 0, sizeof(*out_record));
    if (!core->ops.random_fill(core->ops.context,
                               out_record->salt,
                               sizeof(out_record->salt))) {
        return APP_ERROR_INTERNAL;
    }
    out_record->iterations = AUTH_CORE_PBKDF2_ITERATIONS;
    if (core->ops.derive(core->ops.context,
                         password,
                         password_length,
                         out_record->salt,
                         out_record->iterations,
                         out_record->hash) != 0) {
        core->ops.secure_zero(core->ops.context, out_record, sizeof(*out_record));
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

bool auth_core_password_verify(auth_core_t *core,
                               const char *password,
                               size_t password_length,
                               const auth_password_record_t *record)
{
    if (core == NULL || password == NULL || record == NULL ||
        password_length < AUTH_CORE_PASSWORD_MIN_BYTES ||
        password_length > AUTH_CORE_PASSWORD_MAX_BYTES ||
        record->iterations < AUTH_CORE_PBKDF2_ITERATIONS ||
        memchr(password, '\0', password_length) != NULL) {
        return false;
    }
    uint8_t derived[AUTH_HASH_BYTES];
    if (core->ops.derive(core->ops.context,
                         password,
                         password_length,
                         record->salt,
                         record->iterations,
                         derived) != 0) {
        core->ops.secure_zero(core->ops.context, derived, sizeof(derived));
        return false;
    }
    const bool matches =
        auth_core_constant_time_equal(derived, record->hash, sizeof(derived));
    core->ops.secure_zero(core->ops.context, derived, sizeof(derived));
    return matches;
}
