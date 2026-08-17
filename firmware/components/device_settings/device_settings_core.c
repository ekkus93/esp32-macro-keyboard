#include "device_settings_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "device_settings_v2.h"

static bool operations_valid(const device_settings_core_ops_t *operations) {
    return operations != NULL && operations->read_record != NULL &&
           operations->replace_record_atomic != NULL && operations->secure_zero != NULL;
}

static app_error_code_t map_decode_result(app_v2_settings_result_t result) {
    switch (result) {
    case APP_V2_SETTINGS_OK:
        return APP_ERROR_NONE;
    case APP_V2_SETTINGS_INVALID_ARGUMENT:
        return APP_ERROR_INVALID_ARGUMENT;
    case APP_V2_SETTINGS_INVALID_LENGTH:
    case APP_V2_SETTINGS_CORRUPT:
    case APP_V2_SETTINGS_UNSUPPORTED_VERSION:
        return APP_ERROR_STORAGE_CORRUPT;
    default:
        return APP_ERROR_INTERNAL;
    }
}

static app_error_code_t map_candidate_result(app_v2_settings_result_t result) {
    switch (result) {
    case APP_V2_SETTINGS_OK:
        return APP_ERROR_NONE;
    case APP_V2_SETTINGS_INVALID_ARGUMENT:
    case APP_V2_SETTINGS_CORRUPT:
        return APP_ERROR_INVALID_ARGUMENT;
    case APP_V2_SETTINGS_INVALID_LENGTH:
    case APP_V2_SETTINGS_UNSUPPORTED_VERSION:
    default:
        return APP_ERROR_INTERNAL;
    }
}

static void clear_record(device_settings_core_t *core, uint8_t *record) {
    core->ops.secure_zero(core->ops.context, record, APP_V2_SETTINGS_RECORD_BYTES);
}

static void invalidate_loaded_state(device_settings_core_t *core) {
    core->ops.secure_zero(core->ops.context, &core->current, sizeof(core->current));
    core->loaded = false;
    core->record_present = false;
}

static app_error_code_t encode_settings(const app_v2_device_settings_t *settings, uint8_t *record) {
    const app_v2_settings_result_t result =
        app_v2_device_settings_encode(settings, record, APP_V2_SETTINGS_RECORD_BYTES);
    return map_candidate_result(result);
}

static bool copy_bounded_text(const char *source, char *destination, size_t capacity) {
    if (source == NULL || destination == NULL || capacity == 0U) {
        return false;
    }
    size_t length = 0U;
    while (length < capacity && source[length] != '\0') {
        ++length;
    }
    if (length == capacity) {
        return false;
    }
    memset(destination, 0, capacity);
    memcpy(destination, source, length);
    return true;
}

app_error_code_t device_settings_core_init(device_settings_core_t *core,
                                           const device_settings_core_ops_t *operations) {
    if (core == NULL || !operations_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(core, 0, sizeof(*core));
    core->ops = *operations;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_core_load(device_settings_core_t *core,
                                           app_v2_device_settings_t *out_settings) {
    if (core == NULL || out_settings == NULL || !operations_valid(&core->ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (core->loaded) {
        *out_settings = core->current;
        return APP_ERROR_NONE;
    }

    uint8_t record[APP_V2_SETTINGS_RECORD_BYTES] = {0};
    size_t record_length = 0U;
    const app_error_code_t read_result =
        core->ops.read_record(core->ops.context, record, sizeof(record), &record_length);
    if (read_result == APP_ERROR_NOT_FOUND) {
        app_v2_device_settings_init_unprovisioned(&core->current);
        core->loaded = true;
        core->record_present = false;
        *out_settings = core->current;
        clear_record(core, record);
        return APP_ERROR_NONE;
    }
    if (read_result != APP_ERROR_NONE) {
        clear_record(core, record);
        return read_result;
    }
    if (record_length != APP_V2_SETTINGS_RECORD_BYTES) {
        clear_record(core, record);
        return APP_ERROR_STORAGE_CORRUPT;
    }

    app_v2_device_settings_t decoded = {0};
    const app_v2_settings_result_t decode_result =
        app_v2_device_settings_decode(record, record_length, &decoded);
    clear_record(core, record);
    if (decode_result != APP_V2_SETTINGS_OK) {
        core->ops.secure_zero(core->ops.context, &decoded, sizeof(decoded));
        return map_decode_result(decode_result);
    }

    core->current = decoded;
    core->loaded = true;
    core->record_present = true;
    *out_settings = core->current;
    core->ops.secure_zero(core->ops.context, &decoded, sizeof(decoded));
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_core_replace(device_settings_core_t *core,
                                              const app_v2_device_settings_t *settings,
                                              bool *out_changed) {
    if (core == NULL || settings == NULL || out_changed == NULL || !operations_valid(&core->ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_changed = false;

    app_v2_device_settings_t loaded = {0};
    app_error_code_t result = device_settings_core_load(core, &loaded);
    core->ops.secure_zero(core->ops.context, &loaded, sizeof(loaded));
    if (result != APP_ERROR_NONE) {
        return result;
    }

    uint8_t current_record[APP_V2_SETTINGS_RECORD_BYTES] = {0};
    uint8_t candidate_record[APP_V2_SETTINGS_RECORD_BYTES] = {0};
    result = encode_settings(&core->current, current_record);
    if (result == APP_ERROR_NONE) {
        result = encode_settings(settings, candidate_record);
    }
    if (result != APP_ERROR_NONE) {
        clear_record(core, current_record);
        clear_record(core, candidate_record);
        return result;
    }

    if (memcmp(current_record, candidate_record, sizeof(current_record)) == 0) {
        clear_record(core, current_record);
        clear_record(core, candidate_record);
        return APP_ERROR_NONE;
    }

    result = core->ops.replace_record_atomic(core->ops.context, candidate_record,
                                             sizeof(candidate_record));
    clear_record(core, current_record);
    clear_record(core, candidate_record);
    if (result != APP_ERROR_NONE) {
        if (result == APP_ERROR_COMMIT_UNCERTAIN) {
            invalidate_loaded_state(core);
        }
        return result;
    }

    core->current = *settings;
    core->record_present = true;
    *out_changed = true;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_core_set_station(device_settings_core_t *core, const char *ssid,
                                                  const char *passphrase, bool *out_changed) {
    if (core == NULL || ssid == NULL || passphrase == NULL || out_changed == NULL ||
        !operations_valid(&core->ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_changed = false;

    app_v2_device_settings_t candidate = {0};
    app_error_code_t result = device_settings_core_load(core, &candidate);
    if (result == APP_ERROR_NONE &&
        (!copy_bounded_text(ssid, candidate.station_ssid, sizeof(candidate.station_ssid)) ||
         !copy_bounded_text(passphrase, candidate.station_passphrase,
                            sizeof(candidate.station_passphrase)))) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        candidate.station_configured = true;
        result = device_settings_core_replace(core, &candidate, out_changed);
    }
    core->ops.secure_zero(core->ops.context, &candidate, sizeof(candidate));
    return result;
}

/* SPEC_V2 12.4: the physical console's set-ap-passphrase command. Unlike
 * station credentials, the device's own access-point passphrase must never
 * be empty -- CLAUDE.md: "no open-AP" is a standing invariant, never a
 * degraded mode -- so this rejects an out-of-bounds value before ever
 * touching storage rather than relying on app_v2_device_settings_validate()
 * to reject it deep inside device_settings_core_replace(), giving the
 * console a specific, pre-storage invalid_argument instead of an opaque
 * backend failure. */
app_error_code_t device_settings_core_set_access_point_passphrase(device_settings_core_t *core,
                                                                  const char *passphrase,
                                                                  bool *out_changed) {
    if (core == NULL || passphrase == NULL || out_changed == NULL ||
        !operations_valid(&core->ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_changed = false;
    const size_t passphrase_length = strlen(passphrase);
    if (passphrase_length < APP_V2_WIFI_PASSPHRASE_MIN_BYTES ||
        passphrase_length > APP_V2_WIFI_PASSPHRASE_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_v2_device_settings_t candidate = {0};
    app_error_code_t result = device_settings_core_load(core, &candidate);
    if (result == APP_ERROR_NONE &&
        !copy_bounded_text(passphrase, candidate.ap_passphrase, sizeof(candidate.ap_passphrase))) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = device_settings_core_replace(core, &candidate, out_changed);
    }
    core->ops.secure_zero(core->ops.context, &candidate, sizeof(candidate));
    return result;
}

app_error_code_t device_settings_core_reset_noncredential(device_settings_core_t *core,
                                                          app_v2_device_settings_t *out_settings,
                                                          bool *out_changed) {
    if (core == NULL || out_settings == NULL || out_changed == NULL ||
        !operations_valid(&core->ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_v2_device_settings_t candidate = {0};
    app_error_code_t result = device_settings_core_load(core, &candidate);
    if (result == APP_ERROR_NONE) {
        result = map_candidate_result(app_v2_device_settings_reset_noncredential(&candidate));
    }
    if (result == APP_ERROR_NONE) {
        result = device_settings_core_replace(core, &candidate, out_changed);
    }
    if (result == APP_ERROR_NONE) {
        *out_settings = core->current;
    } else {
        memset(out_settings, 0, sizeof(*out_settings));
    }
    core->ops.secure_zero(core->ops.context, &candidate, sizeof(candidate));
    return result;
}

app_error_code_t device_settings_core_factory_reset(device_settings_core_t *core,
                                                    app_v2_device_settings_t *out_settings,
                                                    bool *out_changed) {
    if (core == NULL || out_settings == NULL || out_changed == NULL ||
        !operations_valid(&core->ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_v2_device_settings_t candidate = {0};
    app_v2_device_settings_init_unprovisioned(&candidate);
    const app_error_code_t result = device_settings_core_replace(core, &candidate, out_changed);
    if (result == APP_ERROR_NONE) {
        *out_settings = core->current;
    } else {
        memset(out_settings, 0, sizeof(*out_settings));
    }
    core->ops.secure_zero(core->ops.context, &candidate, sizeof(candidate));
    return result;
}

void device_settings_core_deinit(device_settings_core_t *core) {
    if (core == NULL) {
        return;
    }
    if (core->ops.secure_zero != NULL) {
        core->ops.secure_zero(core->ops.context, &core->current, sizeof(core->current));
    } else {
        memset(&core->current, 0, sizeof(core->current));
    }
    memset(core, 0, sizeof(*core));
}
