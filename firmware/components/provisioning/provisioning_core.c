#include "provisioning_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "auth.h"
#include "macro_limits.h"
#include "provisioning.h"
#include "wifi_ap.h"

#define WIRE_MAGIC_OFFSET 0U
#define WIRE_SCHEMA_OFFSET 4U
#define WIRE_REVISION_OFFSET 8U
#define WIRE_CREDENTIAL_VERSION_OFFSET 12U
#define WIRE_FLAGS_OFFSET 16U
#define WIRE_SSID_OFFSET 20U
#define WIRE_PASSPHRASE_OFFSET (WIRE_SSID_OFFSET + WIFI_AP_SSID_MAX_BYTES + 1U)
#define WIRE_SALT_OFFSET (WIRE_PASSPHRASE_OFFSET + WIFI_AP_PASSPHRASE_MAX_BYTES + 1U)
#define WIRE_HASH_OFFSET (WIRE_SALT_OFFSET + AUTH_SALT_BYTES)
#define WIRE_ITERATIONS_OFFSET (WIRE_HASH_OFFSET + AUTH_HASH_BYTES)
#define WIRE_ACTIVE_SET_OFFSET (WIRE_ITERATIONS_OFFSET + 4U)

static const uint8_t RECORD_MAGIC[4] = {'P', 'R', 'O', 'V'};

static bool operations_valid(const provisioning_ops_t *operations) {
    return operations != NULL && operations->read_blob != NULL &&
           operations->write_blob != NULL && operations->erase_blob != NULL &&
           operations->commit != NULL && operations->secure_zero != NULL;
}

static void put_u32(uint8_t *output, size_t offset, uint32_t value) {
    output[offset] = (uint8_t)(value & 0xffU);
    output[offset + 1U] = (uint8_t)((value >> 8U) & 0xffU);
    output[offset + 2U] = (uint8_t)((value >> 16U) & 0xffU);
    output[offset + 3U] = (uint8_t)((value >> 24U) & 0xffU);
}

static uint32_t get_u32(const uint8_t *input, size_t offset) {
    return (uint32_t)input[offset] |
           ((uint32_t)input[offset + 1U] << 8U) |
           ((uint32_t)input[offset + 2U] << 16U) |
           ((uint32_t)input[offset + 3U] << 24U);
}

static bool bounded_length(const char *text, size_t maximum, size_t *out_length) {
    if (text == NULL || out_length == NULL) {
        return false;
    }
    for (size_t index = 0U; index <= maximum; ++index) {
        if (text[index] == '\0') {
            *out_length = index;
            return true;
        }
    }
    return false;
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

static bool password_record_valid(const auth_password_record_t *record) {
    return record->iterations == AUTH_PBKDF2_ITERATIONS &&
           !all_zero(record->salt, sizeof(record->salt)) &&
           !all_zero(record->hash, sizeof(record->hash));
}

static bool credentials_empty(const provisioning_config_t *configuration) {
    return configuration->ap_ssid[0] == '\0' &&
           configuration->ap_passphrase[0] == '\0' &&
           all_zero(&configuration->password_record,
                    sizeof(configuration->password_record));
}

bool provisioning_config_is_valid(const provisioning_config_t *configuration) {
    if (configuration == NULL || configuration->schema_version != APP_SCHEMA_VERSION) {
        return false;
    }
    size_t ssid_length = 0U;
    size_t passphrase_length = 0U;
    if (!bounded_length(configuration->ap_ssid,
                        WIFI_AP_SSID_MAX_BYTES,
                        &ssid_length) ||
        !bounded_length(configuration->ap_passphrase,
                        WIFI_AP_PASSPHRASE_MAX_BYTES,
                        &passphrase_length)) {
        return false;
    }
    if (configuration->has_active_set) {
        if (!app_uuid_is_valid_string(configuration->active_set_id.value)) {
            return false;
        }
    } else if (!all_zero(&configuration->active_set_id,
                         sizeof(configuration->active_set_id))) {
        return false;
    }
    if (!configuration->provisioned) {
        return credentials_empty(configuration);
    }
    return configuration->credential_version >=
               PROVISIONING_CREDENTIAL_VERSION_INITIAL &&
           ssid_length > 0U &&
           passphrase_length >= WIFI_AP_PASSPHRASE_MIN_BYTES &&
           password_record_valid(&configuration->password_record);
}

static provisioning_config_t default_configuration(void) {
    return (provisioning_config_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 0U,
        .credential_version = 0U,
        .provisioned = false,
        .require_physical_confirmation = true,
        .always_select_set = true,
        .has_active_set = false,
    };
}

static app_error_code_t encode_configuration(
    const provisioning_config_t *configuration,
    uint8_t output[PROVISIONING_RECORD_BYTES]) {
    if (!provisioning_config_is_valid(configuration)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(output, 0, PROVISIONING_RECORD_BYTES);
    memcpy(output + WIRE_MAGIC_OFFSET, RECORD_MAGIC, sizeof(RECORD_MAGIC));
    put_u32(output, WIRE_SCHEMA_OFFSET, configuration->schema_version);
    put_u32(output, WIRE_REVISION_OFFSET, configuration->revision);
    put_u32(output,
            WIRE_CREDENTIAL_VERSION_OFFSET,
            configuration->credential_version);
    output[WIRE_FLAGS_OFFSET] = configuration->provisioned ? 1U : 0U;
    output[WIRE_FLAGS_OFFSET + 1U] =
        configuration->require_physical_confirmation ? 1U : 0U;
    output[WIRE_FLAGS_OFFSET + 2U] = configuration->always_select_set ? 1U : 0U;
    output[WIRE_FLAGS_OFFSET + 3U] = configuration->has_active_set ? 1U : 0U;

    size_t length = 0U;
    if (!bounded_length(configuration->ap_ssid,
                        WIFI_AP_SSID_MAX_BYTES,
                        &length)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(output + WIRE_SSID_OFFSET, configuration->ap_ssid, length + 1U);
    if (!bounded_length(configuration->ap_passphrase,
                        WIFI_AP_PASSPHRASE_MAX_BYTES,
                        &length)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(output + WIRE_PASSPHRASE_OFFSET,
           configuration->ap_passphrase,
           length + 1U);
    memcpy(output + WIRE_SALT_OFFSET,
           configuration->password_record.salt,
           AUTH_SALT_BYTES);
    memcpy(output + WIRE_HASH_OFFSET,
           configuration->password_record.hash,
           AUTH_HASH_BYTES);
    put_u32(output,
            WIRE_ITERATIONS_OFFSET,
            configuration->password_record.iterations);
    if (configuration->has_active_set) {
        memcpy(output + WIRE_ACTIVE_SET_OFFSET,
               configuration->active_set_id.value,
               APP_UUID_BUFFER_LENGTH);
    }
    return APP_ERROR_NONE;
}

static bool flag_valid(uint8_t value) {
    return value == 0U || value == 1U;
}

static app_error_code_t decode_configuration(
    provisioning_core_t *core,
    const uint8_t input[PROVISIONING_RECORD_BYTES],
    provisioning_config_t *out_configuration) {
    if (memcmp(input + WIRE_MAGIC_OFFSET, RECORD_MAGIC, sizeof(RECORD_MAGIC)) != 0 ||
        !flag_valid(input[WIRE_FLAGS_OFFSET]) ||
        !flag_valid(input[WIRE_FLAGS_OFFSET + 1U]) ||
        !flag_valid(input[WIRE_FLAGS_OFFSET + 2U]) ||
        !flag_valid(input[WIRE_FLAGS_OFFSET + 3U])) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    provisioning_config_t configuration = {0};
    configuration.schema_version = get_u32(input, WIRE_SCHEMA_OFFSET);
    configuration.revision = get_u32(input, WIRE_REVISION_OFFSET);
    configuration.credential_version =
        get_u32(input, WIRE_CREDENTIAL_VERSION_OFFSET);
    configuration.provisioned = input[WIRE_FLAGS_OFFSET] != 0U;
    configuration.require_physical_confirmation =
        input[WIRE_FLAGS_OFFSET + 1U] != 0U;
    configuration.always_select_set = input[WIRE_FLAGS_OFFSET + 2U] != 0U;
    configuration.has_active_set = input[WIRE_FLAGS_OFFSET + 3U] != 0U;
    memcpy(configuration.ap_ssid,
           input + WIRE_SSID_OFFSET,
           sizeof(configuration.ap_ssid));
    memcpy(configuration.ap_passphrase,
           input + WIRE_PASSPHRASE_OFFSET,
           sizeof(configuration.ap_passphrase));
    memcpy(configuration.password_record.salt,
           input + WIRE_SALT_OFFSET,
           AUTH_SALT_BYTES);
    memcpy(configuration.password_record.hash,
           input + WIRE_HASH_OFFSET,
           AUTH_HASH_BYTES);
    configuration.password_record.iterations =
        get_u32(input, WIRE_ITERATIONS_OFFSET);
    memcpy(configuration.active_set_id.value,
           input + WIRE_ACTIVE_SET_OFFSET,
           APP_UUID_BUFFER_LENGTH);
    if (!provisioning_config_is_valid(&configuration)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    uint8_t canonical[PROVISIONING_RECORD_BYTES];
    const app_error_code_t encode =
        encode_configuration(&configuration, canonical);
    const bool exact = encode == APP_ERROR_NONE &&
                       memcmp(input, canonical, sizeof(canonical)) == 0;
    core->operations.secure_zero(core->operations.context,
                                 canonical,
                                 sizeof(canonical));
    if (!exact) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_configuration = configuration;
    return APP_ERROR_NONE;
}

app_error_code_t provisioning_core_init(provisioning_core_t *core,
                                        const provisioning_ops_t *operations) {
    if (core == NULL || !operations_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(core, 0, sizeof(*core));
    core->operations = *operations;
    core->current = default_configuration();
    core->initialized = true;
    return APP_ERROR_NONE;
}

static app_error_code_t read_configuration(provisioning_core_t *core,
                                           provisioning_config_t *out_configuration,
                                           bool *out_present) {
    uint8_t encoded[PROVISIONING_RECORD_BYTES];
    size_t encoded_size = 0U;
    const app_error_code_t read = core->operations.read_blob(
        core->operations.context, encoded, sizeof(encoded), &encoded_size);
    if (read == APP_ERROR_NOT_FOUND) {
        *out_configuration = default_configuration();
        *out_present = false;
        core->operations.secure_zero(core->operations.context,
                                     encoded,
                                     sizeof(encoded));
        return APP_ERROR_NONE;
    }
    if (read != APP_ERROR_NONE) {
        core->operations.secure_zero(core->operations.context,
                                     encoded,
                                     sizeof(encoded));
        return read;
    }
    if (encoded_size != sizeof(encoded)) {
        core->operations.secure_zero(core->operations.context,
                                     encoded,
                                     sizeof(encoded));
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const app_error_code_t decode =
        decode_configuration(core, encoded, out_configuration);
    core->operations.secure_zero(core->operations.context,
                                 encoded,
                                 sizeof(encoded));
    if (decode == APP_ERROR_NONE) {
        *out_present = true;
    }
    return decode;
}

app_error_code_t provisioning_core_load(provisioning_core_t *core,
                                        provisioning_config_t *out_config) {
    if (out_config != NULL) {
        memset(out_config, 0, sizeof(*out_config));
    }
    if (core == NULL || out_config == NULL || !core->initialized) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    provisioning_config_t loaded = {0};
    bool present = false;
    const app_error_code_t result =
        read_configuration(core, &loaded, &present);
    if (result != APP_ERROR_NONE) {
        core->operations.secure_zero(core->operations.context,
                                     &loaded,
                                     sizeof(loaded));
        return result;
    }
    core->operations.secure_zero(core->operations.context,
                                 &core->current,
                                 sizeof(core->current));
    core->current = loaded;
    core->record_present = present;
    core->loaded = true;
    *out_config = loaded;
    core->operations.secure_zero(core->operations.context,
                                 &loaded,
                                 sizeof(loaded));
    return APP_ERROR_NONE;
}

static app_error_code_t verify_committed(
    provisioning_core_t *core,
    const uint8_t expected[PROVISIONING_RECORD_BYTES],
    provisioning_config_t *out_configuration) {
    uint8_t readback[PROVISIONING_RECORD_BYTES];
    size_t readback_size = 0U;
    app_error_code_t result = core->operations.read_blob(
        core->operations.context, readback, sizeof(readback), &readback_size);
    if (result == APP_ERROR_NONE && readback_size != sizeof(readback)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE &&
        memcmp(expected, readback, sizeof(readback)) != 0) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = decode_configuration(core, readback, out_configuration);
    }
    core->operations.secure_zero(core->operations.context,
                                 readback,
                                 sizeof(readback));
    return result;
}

app_error_code_t provisioning_core_commit(provisioning_core_t *core,
                                          const provisioning_config_t *replacement,
                                          uint32_t expected_revision,
                                          provisioning_config_t *out_committed) {
    if (out_committed != NULL) {
        memset(out_committed, 0, sizeof(*out_committed));
    }
    if (core == NULL || replacement == NULL || out_committed == NULL ||
        !core->initialized || !core->loaded ||
        replacement->schema_version != APP_SCHEMA_VERSION) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (expected_revision != core->current.revision ||
        replacement->revision != expected_revision) {
        return APP_ERROR_CONFLICT;
    }
    if (expected_revision == UINT32_MAX) {
        return APP_ERROR_CONFLICT;
    }

    provisioning_config_t candidate = *replacement;
    candidate.schema_version = APP_SCHEMA_VERSION;
    candidate.revision = expected_revision + 1U;
    if (!provisioning_config_is_valid(&candidate)) {
        core->operations.secure_zero(core->operations.context,
                                     &candidate,
                                     sizeof(candidate));
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t encoded[PROVISIONING_RECORD_BYTES];
    app_error_code_t result = encode_configuration(&candidate, encoded);
    if (result == APP_ERROR_NONE) {
        result = core->operations.write_blob(
            core->operations.context, encoded, sizeof(encoded));
    }
    if (result == APP_ERROR_NONE) {
        result = core->operations.commit(core->operations.context);
    }
    provisioning_config_t verified = {0};
    if (result == APP_ERROR_NONE) {
        result = verify_committed(core, encoded, &verified);
    }
    core->operations.secure_zero(core->operations.context,
                                 encoded,
                                 sizeof(encoded));
    if (result != APP_ERROR_NONE) {
        core->operations.secure_zero(core->operations.context,
                                     &verified,
                                     sizeof(verified));
        core->operations.secure_zero(core->operations.context,
                                     &candidate,
                                     sizeof(candidate));
        return result;
    }

    core->operations.secure_zero(core->operations.context,
                                 &core->current,
                                 sizeof(core->current));
    core->current = verified;
    core->record_present = true;
    *out_committed = verified;
    core->operations.secure_zero(core->operations.context,
                                 &verified,
                                 sizeof(verified));
    core->operations.secure_zero(core->operations.context,
                                 &candidate,
                                 sizeof(candidate));
    return APP_ERROR_NONE;
}

app_error_code_t provisioning_core_clear_credentials(provisioning_core_t *core) {
    if (core == NULL || !core->initialized || !core->loaded) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (core->current.credential_version == UINT32_MAX) {
        return APP_ERROR_CONFLICT;
    }
    provisioning_config_t replacement = core->current;
    replacement.provisioned = false;
    replacement.credential_version += 1U;
    memset(replacement.ap_ssid, 0, sizeof(replacement.ap_ssid));
    memset(replacement.ap_passphrase, 0, sizeof(replacement.ap_passphrase));
    memset(&replacement.password_record, 0, sizeof(replacement.password_record));
    provisioning_config_t committed = {0};
    const app_error_code_t result = provisioning_core_commit(
        core, &replacement, core->current.revision, &committed);
    core->operations.secure_zero(core->operations.context,
                                 &replacement,
                                 sizeof(replacement));
    core->operations.secure_zero(core->operations.context,
                                 &committed,
                                 sizeof(committed));
    return result;
}

app_error_code_t provisioning_core_factory_reset(provisioning_core_t *core) {
    if (core == NULL || !core->initialized || !core->loaded) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result =
        core->operations.erase_blob(core->operations.context);
    if (result == APP_ERROR_NONE) {
        result = core->operations.commit(core->operations.context);
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }

    uint8_t readback[PROVISIONING_RECORD_BYTES];
    size_t readback_size = 0U;
    const app_error_code_t read = core->operations.read_blob(
        core->operations.context, readback, sizeof(readback), &readback_size);
    core->operations.secure_zero(core->operations.context,
                                 readback,
                                 sizeof(readback));
    if (read != APP_ERROR_NOT_FOUND) {
        return read == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : read;
    }

    core->operations.secure_zero(core->operations.context,
                                 &core->current,
                                 sizeof(core->current));
    core->current = default_configuration();
    core->record_present = false;
    return APP_ERROR_NONE;
}

app_error_code_t provisioning_core_deinit(provisioning_core_t *core) {
    if (core == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!core->initialized) {
        return APP_ERROR_NONE;
    }
    const provisioning_ops_t operations = core->operations;
    operations.secure_zero(operations.context, core, sizeof(*core));
    return APP_ERROR_NONE;
}
