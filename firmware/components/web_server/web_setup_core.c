#include "web_setup_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "macro_limits.h"
#include "provisioning.h"
#include "provisioning_bootstrap.h"
#include "wifi_ap.h"

static bool operations_valid(const web_setup_ops_t *operations) {
    return operations != NULL && operations->provisioning_load != NULL &&
           operations->password_create != NULL && operations->provisioning_commit != NULL &&
           operations->wait_for_confirmation != NULL && operations->schedule_restart != NULL &&
           operations->secure_zero != NULL;
}

static bool all_zero(const void *memory, size_t size) {
    const uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        if (bytes[index] != 0U) {
            return false;
        }
    }
    return true;
}

static bool canonical_string(const char *text, size_t capacity, size_t *out_length) {
    if (text == NULL || out_length == NULL || capacity == 0U) {
        return false;
    }
    for (size_t index = 0U; index < capacity; ++index) {
        if (text[index] == '\0') {
            const size_t trailing = capacity - index - 1U;
            if (!all_zero(text + index + 1U, trailing)) {
                return false;
            }
            *out_length = index;
            return true;
        }
    }
    return false;
}

static bool uppercase_hex(const char *text, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        const char value = text[index];
        if (!((value >= '0' && value <= '9') || (value >= 'A' && value <= 'F'))) {
            return false;
        }
    }
    return true;
}

static bool configuration_valid(const web_setup_configuration_t *configuration) {
    if (configuration == NULL) {
        return false;
    }
    size_t device_length = 0U;
    size_t ssid_length = 0U;
    size_t code_length = 0U;
    return canonical_string(configuration->device_id, sizeof(configuration->device_id),
                            &device_length) &&
           device_length == PROVISIONING_DEVICE_ID_HEX_BYTES &&
           uppercase_hex(configuration->device_id, device_length) &&
           canonical_string(configuration->ap_ssid, sizeof(configuration->ap_ssid), &ssid_length) &&
           ssid_length > 0U &&
           canonical_string(configuration->setup_code, sizeof(configuration->setup_code),
                            &code_length) &&
           code_length == PROVISIONING_SETUP_SECRET_HEX_BYTES &&
           uppercase_hex(configuration->setup_code, code_length);
}

static bool submission_valid(const web_setup_submission_t *submission,
                             size_t *out_password_length) {
    if (submission == NULL || out_password_length == NULL) {
        return false;
    }
    size_t code_length = 0U;
    size_t ssid_length = 0U;
    size_t passphrase_length = 0U;
    size_t password_length = 0U;
    if (!canonical_string(submission->setup_code, sizeof(submission->setup_code), &code_length) ||
        code_length != PROVISIONING_SETUP_SECRET_HEX_BYTES ||
        !uppercase_hex(submission->setup_code, code_length) ||
        !canonical_string(submission->ap_ssid, sizeof(submission->ap_ssid), &ssid_length) ||
        ssid_length == 0U ||
        !canonical_string(submission->ap_passphrase, sizeof(submission->ap_passphrase),
                          &passphrase_length) ||
        passphrase_length < WIFI_AP_PASSPHRASE_MIN_BYTES ||
        !canonical_string(submission->administrator_password,
                          sizeof(submission->administrator_password), &password_length) ||
        password_length < AUTH_PASSWORD_MIN_BYTES || password_length > AUTH_PASSWORD_MAX_BYTES) {
        return false;
    }
    *out_password_length = password_length;
    return true;
}

static bool constant_time_equal(const char *left, const char *right, size_t length) {
    uint8_t difference = 0U;
    for (size_t index = 0U; index < length; ++index) {
        difference |= (uint8_t)left[index] ^ (uint8_t)right[index];
    }
    return difference == 0U;
}

app_error_code_t web_setup_core_init(web_setup_core_t *core, const web_setup_ops_t *operations,
                                     const web_setup_configuration_t *configuration) {
    if (core == NULL || !operations_valid(operations) || !configuration_valid(configuration)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(core, 0, sizeof(*core));
    core->operations = *operations;
    core->configuration = *configuration;
    core->initialized = true;
    return APP_ERROR_NONE;
}

web_setup_state_t web_setup_core_get_state(const web_setup_core_t *core) {
    web_setup_state_t state = {0};
    if (core == NULL || !core->initialized) {
        return state;
    }
    memcpy(state.device_id, core->configuration.device_id, sizeof(state.device_id));
    memcpy(state.ap_ssid, core->configuration.ap_ssid, sizeof(state.ap_ssid));
    state.completed = core->completed;
    state.physical_confirmation_required = core->configuration.physical_confirmation_required &&
                                           !core->configuration.manufacturing_bypass;
    return state;
}

static app_error_code_t authorize_submission(web_setup_core_t *core,
                                             const web_setup_submission_t *submission) {
    if (!constant_time_equal(submission->setup_code, core->configuration.setup_code,
                             PROVISIONING_SETUP_SECRET_HEX_BYTES)) {
        return APP_ERROR_AUTH_FAILED;
    }
    if (core->configuration.physical_confirmation_required &&
        !core->configuration.manufacturing_bypass) {
        return core->operations.wait_for_confirmation(core->operations.context,
                                                      WEB_SETUP_CONFIRMATION_TIMEOUT_MS);
    }
    return APP_ERROR_NONE;
}

static provisioning_config_t
replacement_configuration(const provisioning_config_t *current,
                          const web_setup_submission_t *submission,
                          const auth_password_record_t *password_record) {
    /* Start from the loaded record. Setup decides the AP credentials, the
     * administrator password, and two policy flags; every other field it does
     * not name belongs to whoever set it. Building a fresh struct here silently
     * discarded the stored station network (SPEC 15.2 clears that on an explicit
     * empty SSID and on nothing else), which on hardware meant a device that
     * came back from setup on its access point alone. */
    provisioning_config_t replacement = *current;
    replacement.schema_version = APP_SCHEMA_VERSION;
    replacement.revision = current->revision;
    replacement.credential_version = PROVISIONING_CREDENTIAL_VERSION_INITIAL;
    replacement.provisioned = true;
    replacement.password_record = *password_record;
    replacement.require_physical_confirmation = submission->require_physical_confirmation;
    replacement.always_select_package = submission->always_select_package;
    memcpy(replacement.ap_ssid, submission->ap_ssid, sizeof(replacement.ap_ssid));
    memcpy(replacement.ap_passphrase, submission->ap_passphrase, sizeof(replacement.ap_passphrase));
    return replacement;
}

app_error_code_t web_setup_core_submit(web_setup_core_t *core, web_setup_submission_t *submission,
                                       provisioning_config_t *out_committed) {
    if (out_committed != NULL) {
        memset(out_committed, 0, sizeof(*out_committed));
    }
    size_t password_length = 0U;
    if (core == NULL || !core->initialized || submission == NULL || out_committed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!submission_valid(submission, &password_length)) {
        core->operations.secure_zero(core->operations.context, submission, sizeof(*submission));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (core->completed) {
        core->operations.secure_zero(core->operations.context, submission, sizeof(*submission));
        return APP_ERROR_CONFLICT;
    }

    app_error_code_t result = authorize_submission(core, submission);
    provisioning_config_t current = {0};
    auth_password_record_t password_record = {0};
    provisioning_config_t replacement = {0};
    provisioning_config_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = core->operations.provisioning_load(core->operations.context, &current);
    }
    if (result == APP_ERROR_NONE && current.provisioned) {
        result = APP_ERROR_CONFLICT;
    }
    if (result == APP_ERROR_NONE) {
        result = core->operations.password_create(core->operations.context,
                                                  submission->administrator_password,
                                                  password_length, &password_record);
    }
    if (result == APP_ERROR_NONE) {
        replacement = replacement_configuration(&current, submission, &password_record);
        result = core->operations.provisioning_commit(core->operations.context, &replacement,
                                                      current.revision, &committed);
    }
    if (result == APP_ERROR_NONE) {
        core->completed = true;
        core->operations.secure_zero(core->operations.context, core->configuration.setup_code,
                                     sizeof(core->configuration.setup_code));
        *out_committed = committed;
    }

    core->operations.secure_zero(core->operations.context, submission, sizeof(*submission));
    core->operations.secure_zero(core->operations.context, &password_record,
                                 sizeof(password_record));
    core->operations.secure_zero(core->operations.context, &replacement, sizeof(replacement));
    core->operations.secure_zero(core->operations.context, &committed, sizeof(committed));
    core->operations.secure_zero(core->operations.context, &current, sizeof(current));
    return result;
}

app_error_code_t web_setup_core_restart(web_setup_core_t *core) {
    if (core == NULL || !core->initialized) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!core->completed) {
        return APP_ERROR_CONFLICT;
    }
    return core->operations.schedule_restart(core->operations.context);
}

app_error_code_t web_setup_core_deinit(web_setup_core_t *core) {
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!core->initialized) {
        return APP_ERROR_NONE;
    }
    const web_setup_ops_t operations = core->operations;
    operations.secure_zero(operations.context, core, sizeof(*core));
    return APP_ERROR_NONE;
}
