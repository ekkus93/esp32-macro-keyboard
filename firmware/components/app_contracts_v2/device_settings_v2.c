#include "device_settings_v2.h"

#include <stdint.h>
#include <string.h>

#include "app_limits_v2.h"

#define APP_V2_SETTINGS_LAST_PACKAGE_FIELD_BYTES 37U
#define APP_V2_SETTINGS_DEVICE_NAME_FIELD_BYTES 33U
#define APP_V2_SETTINGS_SSID_FIELD_BYTES 33U
#define APP_V2_SETTINGS_PASSPHRASE_FIELD_BYTES 64U
#define APP_V2_SETTINGS_DEFAULT_SNAPSHOT_RETENTION_TARGET 5U
#define APP_V2_SETTINGS_UTF8_CONTINUATION_SHIFT 6U
#define APP_V2_SETTINGS_UUID_HYPHEN_1_INDEX 8U
#define APP_V2_SETTINGS_UUID_HYPHEN_2_INDEX 13U
#define APP_V2_SETTINGS_UUID_HYPHEN_3_INDEX 18U
#define APP_V2_SETTINGS_UUID_HYPHEN_4_INDEX 23U
#define APP_V2_SETTINGS_UUID_VERSION_INDEX 14U
#define APP_V2_SETTINGS_UUID_VARIANT_INDEX 19U

_Static_assert(APP_V2_SETTINGS_OFFSET_STATION_PASSPHRASE + APP_V2_SETTINGS_PASSPHRASE_FIELD_BYTES ==
                   APP_V2_SETTINGS_RECORD_BYTES,
               "v2 settings layout must end at the declared record length");

static void write_u16_le(uint8_t *destination, uint16_t value) {
    destination[0] = (uint8_t)(value & UINT16_C(0xff));
    destination[1] = (uint8_t)((value >> 8U) & UINT16_C(0xff));
}

static void write_u32_le(uint8_t *destination, uint32_t value) {
    for (uint32_t index = 0U; index < 4U; ++index) {
        destination[index] = (uint8_t)((value >> (index * 8U)) & UINT32_C(0xff));
    }
}

static void write_u64_le(uint8_t *destination, uint64_t value) {
    for (uint32_t index = 0U; index < 8U; ++index) {
        destination[index] = (uint8_t)((value >> (index * 8U)) & UINT64_C(0xff));
    }
}

static uint16_t read_u16_le(const uint8_t *source) {
    return (uint16_t)((uint16_t)source[0] | (uint16_t)((uint16_t)source[1] << 8U));
}

static uint32_t read_u32_le(const uint8_t *source) {
    uint32_t value = 0U;
    for (uint32_t index = 0U; index < 4U; ++index) {
        value |= (uint32_t)source[index] << (index * 8U);
    }
    return value;
}

static uint64_t read_u64_le(const uint8_t *source) {
    uint64_t value = 0U;
    for (uint32_t index = 0U; index < 8U; ++index) {
        value |= (uint64_t)source[index] << (index * 8U);
    }
    return value;
}

static bool bytes_are_zero(const uint8_t *bytes, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        if (bytes[index] != 0U) {
            return false;
        }
    }
    return true;
}

static bool bounded_length(const char *text, size_t capacity, size_t *out_length) {
    if (text == NULL || out_length == NULL) {
        return false;
    }
    for (size_t index = 0U; index < capacity; ++index) {
        if (text[index] == '\0') {
            *out_length = index;
            return true;
        }
    }
    return false;
}

static bool valid_utf8(const uint8_t *text, size_t length) {
    size_t index = 0U;
    while (index < length) {
        const uint8_t first = text[index];
        if (first <= UINT8_C(0x7f)) {
            ++index;
            continue;
        }

        uint32_t code_point = 0U;
        size_t continuation_count = 0U;
        uint32_t minimum = 0U;
        if (first >= UINT8_C(0xc2) && first <= UINT8_C(0xdf)) {
            code_point = (uint32_t)(first & UINT8_C(0x1f));
            continuation_count = 1U;
            minimum = UINT32_C(0x80);
        } else if (first >= UINT8_C(0xe0) && first <= UINT8_C(0xef)) {
            code_point = (uint32_t)(first & UINT8_C(0x0f));
            continuation_count = 2U;
            minimum = UINT32_C(0x800);
        } else if (first >= UINT8_C(0xf0) && first <= UINT8_C(0xf4)) {
            code_point = (uint32_t)(first & UINT8_C(0x07));
            continuation_count = 3U;
            minimum = UINT32_C(0x10000);
        } else {
            return false;
        }

        if (continuation_count > length - index - 1U) {
            return false;
        }
        for (size_t offset = 1U; offset <= continuation_count; ++offset) {
            const uint8_t continuation = text[index + offset];
            if ((continuation & UINT8_C(0xc0)) != UINT8_C(0x80)) {
                return false;
            }
            code_point = (code_point << APP_V2_SETTINGS_UTF8_CONTINUATION_SHIFT) |
                         (uint32_t)(continuation & UINT8_C(0x3f));
        }
        if (code_point < minimum || code_point > UINT32_C(0x10ffff) ||
            (code_point >= UINT32_C(0xd800) && code_point <= UINT32_C(0xdfff))) {
            return false;
        }
        index += continuation_count + 1U;
    }
    return true;
}

static bool valid_text(const char *text, size_t capacity, size_t minimum, size_t maximum) {
    size_t length = 0U;
    return bounded_length(text, capacity, &length) && length >= minimum && length <= maximum &&
           valid_utf8((const uint8_t *)text, length);
}

static bool valid_uuid_or_empty(const char *uuid) {
    size_t length = 0U;
    if (!bounded_length(uuid, APP_V2_UUID_BUFFER_BYTES, &length)) {
        return false;
    }
    if (length == 0U) {
        return true;
    }
    if (length != APP_V2_UUID_TEXT_BYTES) {
        return false;
    }
    for (size_t index = 0U; index < APP_V2_UUID_TEXT_BYTES; ++index) {
        const char value = uuid[index];
        if (index == APP_V2_SETTINGS_UUID_HYPHEN_1_INDEX ||
            index == APP_V2_SETTINGS_UUID_HYPHEN_2_INDEX ||
            index == APP_V2_SETTINGS_UUID_HYPHEN_3_INDEX ||
            index == APP_V2_SETTINGS_UUID_HYPHEN_4_INDEX) {
            if (value != '-') {
                return false;
            }
            continue;
        }
        const bool decimal = value >= '0' && value <= '9';
        const bool lowercase_hex = value >= 'a' && value <= 'f';
        if (!decimal && !lowercase_hex) {
            return false;
        }
    }
    return uuid[APP_V2_SETTINGS_UUID_VERSION_INDEX] == '4' &&
           (uuid[APP_V2_SETTINGS_UUID_VARIANT_INDEX] == '8' ||
            uuid[APP_V2_SETTINGS_UUID_VARIANT_INDEX] == '9' ||
            uuid[APP_V2_SETTINGS_UUID_VARIANT_INDEX] == 'a' ||
            uuid[APP_V2_SETTINGS_UUID_VARIANT_INDEX] == 'b');
}

static bool credential_bytes_present(const app_v2_device_settings_t *settings) {
    return settings->password_iterations != 0U &&
           !bytes_are_zero(settings->password_salt, APP_V2_PASSWORD_SALT_BYTES) &&
           !bytes_are_zero(settings->password_verifier, APP_V2_PASSWORD_VERIFIER_BYTES);
}

static bool credential_bytes_absent(const app_v2_device_settings_t *settings) {
    return settings->password_iterations == 0U &&
           bytes_are_zero(settings->password_salt, APP_V2_PASSWORD_SALT_BYTES) &&
           bytes_are_zero(settings->password_verifier, APP_V2_PASSWORD_VERIFIER_BYTES);
}

/* An empty passphrase is valid here: it represents a station network that
 * associated with no password (an open network), which
 * wifi_ap_connect_station() already permits when actually connecting.
 * SPEC_V2 12.4 requires persistence to follow any successful association
 * unconditionally, with no open-network exception, so this field cannot
 * reuse the AP-passphrase minimum (APP_V2_WIFI_PASSPHRASE_MIN_BYTES) below
 * that governs the device's own SoftAP, which per project policy must never
 * be open. A non-empty passphrase still has to meet the ordinary WPA-style
 * bounds, since a 1-7 byte value could never have produced a real
 * association. */
static bool station_passphrase_valid(const app_v2_device_settings_t *settings) {
    size_t length = 0U;
    if (!bounded_length(settings->station_passphrase, sizeof(settings->station_passphrase),
                        &length)) {
        return false;
    }
    if (length == 0U) {
        return true;
    }
    return length >= APP_V2_WIFI_PASSPHRASE_MIN_BYTES &&
           length <= APP_V2_WIFI_PASSPHRASE_MAX_BYTES &&
           valid_utf8((const uint8_t *)settings->station_passphrase, length);
}

static bool station_fields_valid(const app_v2_device_settings_t *settings) {
    if (settings->station_configured) {
        return valid_text(settings->station_ssid, sizeof(settings->station_ssid), 1U,
                          APP_V2_WIFI_SSID_MAX_BYTES) &&
               station_passphrase_valid(settings);
    }
    return valid_text(settings->station_ssid, sizeof(settings->station_ssid), 0U, 0U) &&
           valid_text(settings->station_passphrase, sizeof(settings->station_passphrase), 0U, 0U);
}

static bool copy_record_text(const uint8_t *source, size_t source_length, char *destination,
                             size_t destination_length) {
    size_t terminator = source_length;
    for (size_t index = 0U; index < source_length; ++index) {
        if (source[index] == 0U) {
            terminator = index;
            break;
        }
    }
    if (terminator == source_length || terminator >= destination_length ||
        !valid_utf8(source, terminator) ||
        !bytes_are_zero(source + terminator + 1U, source_length - terminator - 1U)) {
        return false;
    }
    memset(destination, 0, destination_length);
    memcpy(destination, source, terminator);
    return true;
}

static void write_record_text(uint8_t *destination, size_t destination_length, const char *text) {
    size_t length = 0U;
    (void)bounded_length(text, destination_length, &length);
    memcpy(destination, text, length);
}

void app_v2_device_settings_init_unprovisioned(app_v2_device_settings_t *settings) {
    if (settings == NULL) {
        return;
    }
    memset(settings, 0, sizeof(*settings));
    settings->credential_version = APP_V2_CREDENTIAL_VERSION;
    settings->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings->send_mode = APP_V2_SEND_MODE_QUICK;
    settings->snapshot_retention_target = APP_V2_SETTINGS_DEFAULT_SNAPSHOT_RETENTION_TARGET;
    (void)memcpy(settings->device_name, "ESP32 Macro Keyboard", sizeof("ESP32 Macro Keyboard"));
}

app_v2_settings_result_t app_v2_device_settings_validate(const app_v2_device_settings_t *settings) {
    if (settings == NULL) {
        return APP_V2_SETTINGS_INVALID_ARGUMENT;
    }
    if (settings->credential_version != APP_V2_CREDENTIAL_VERSION ||
        settings->password_algorithm_version != APP_V2_PASSWORD_ALGORITHM_VERSION ||
        (settings->send_mode != APP_V2_SEND_MODE_QUICK &&
         settings->send_mode != APP_V2_SEND_MODE_PREVIEW) ||
        settings->snapshot_retention_target > APP_V2_SNAPSHOT_RETENTION_TARGET_MAX ||
        !valid_uuid_or_empty(settings->last_selected_package_id) ||
        !valid_text(settings->device_name, sizeof(settings->device_name), 1U,
                    APP_V2_DEVICE_NAME_MAX_BYTES)) {
        return APP_V2_SETTINGS_CORRUPT;
    }

    if (!settings->provisioned) {
        if (!credential_bytes_absent(settings) ||
            !valid_text(settings->ap_ssid, sizeof(settings->ap_ssid), 0U, 0U) ||
            !valid_text(settings->ap_passphrase, sizeof(settings->ap_passphrase), 0U, 0U) ||
            !station_fields_valid(settings)) {
            return APP_V2_SETTINGS_CORRUPT;
        }
        return APP_V2_SETTINGS_OK;
    }

    if (!credential_bytes_present(settings) ||
        !valid_text(settings->ap_ssid, sizeof(settings->ap_ssid), 1U, APP_V2_WIFI_SSID_MAX_BYTES) ||
        !valid_text(settings->ap_passphrase, sizeof(settings->ap_passphrase),
                    APP_V2_WIFI_PASSPHRASE_MIN_BYTES, APP_V2_WIFI_PASSPHRASE_MAX_BYTES)) {
        return APP_V2_SETTINGS_CORRUPT;
    }

    if (!station_fields_valid(settings)) {
        return APP_V2_SETTINGS_CORRUPT;
    }
    return APP_V2_SETTINGS_OK;
}

app_v2_settings_result_t app_v2_device_settings_encode(const app_v2_device_settings_t *settings,
                                                       uint8_t *record, size_t record_length) {
    if (settings == NULL || record == NULL) {
        return APP_V2_SETTINGS_INVALID_ARGUMENT;
    }
    if (record_length != APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_V2_SETTINGS_INVALID_LENGTH;
    }
    const app_v2_settings_result_t validation = app_v2_device_settings_validate(settings);
    if (validation != APP_V2_SETTINGS_OK) {
        return validation;
    }

    memset(record, 0, record_length);
    write_u32_le(record + APP_V2_SETTINGS_OFFSET_MAGIC, APP_V2_SETTINGS_MAGIC);
    write_u16_le(record + APP_V2_SETTINGS_OFFSET_RECORD_VERSION, APP_V2_SETTINGS_RECORD_VERSION);
    write_u16_le(record + APP_V2_SETTINGS_OFFSET_RECORD_LENGTH, APP_V2_SETTINGS_RECORD_BYTES);
    write_u16_le(record + APP_V2_SETTINGS_OFFSET_CREDENTIAL_VERSION, settings->credential_version);
    write_u16_le(record + APP_V2_SETTINGS_OFFSET_PASSWORD_ALGORITHM,
                 settings->password_algorithm_version);
    write_u32_le(record + APP_V2_SETTINGS_OFFSET_PASSWORD_ITERATIONS,
                 settings->password_iterations);
    memcpy(record + APP_V2_SETTINGS_OFFSET_PASSWORD_SALT, settings->password_salt,
           APP_V2_PASSWORD_SALT_BYTES);
    memcpy(record + APP_V2_SETTINGS_OFFSET_PASSWORD_VERIFIER, settings->password_verifier,
           APP_V2_PASSWORD_VERIFIER_BYTES);
    write_u64_le(record + APP_V2_SETTINGS_OFFSET_NEXT_BLOB_ID, settings->next_blob_id);
    record[APP_V2_SETTINGS_OFFSET_SEND_MODE] = (uint8_t)settings->send_mode;
    record[APP_V2_SETTINGS_OFFSET_RETENTION_TARGET] = settings->snapshot_retention_target;
    /* APP_V2_SETTINGS_OFFSET_RESERVED_SHOW_SOURCE is left at the 0 the
     * memset above already gave it -- see the offset's own doc comment. */
    record[APP_V2_SETTINGS_OFFSET_REQUIRE_CONFIRMATION] =
        settings->require_serial_confirmation ? 1U : 0U;
    record[APP_V2_SETTINGS_OFFSET_PROVISIONED] = settings->provisioned ? 1U : 0U;
    record[APP_V2_SETTINGS_OFFSET_STATION_CONFIGURED] = settings->station_configured ? 1U : 0U;
    write_record_text(record + APP_V2_SETTINGS_OFFSET_LAST_SELECTED_PACKAGE,
                      APP_V2_SETTINGS_LAST_PACKAGE_FIELD_BYTES, settings->last_selected_package_id);
    write_record_text(record + APP_V2_SETTINGS_OFFSET_DEVICE_NAME,
                      APP_V2_SETTINGS_DEVICE_NAME_FIELD_BYTES, settings->device_name);
    write_record_text(record + APP_V2_SETTINGS_OFFSET_AP_SSID, APP_V2_SETTINGS_SSID_FIELD_BYTES,
                      settings->ap_ssid);
    write_record_text(record + APP_V2_SETTINGS_OFFSET_AP_PASSPHRASE,
                      APP_V2_SETTINGS_PASSPHRASE_FIELD_BYTES, settings->ap_passphrase);
    write_record_text(record + APP_V2_SETTINGS_OFFSET_STATION_SSID,
                      APP_V2_SETTINGS_SSID_FIELD_BYTES, settings->station_ssid);
    write_record_text(record + APP_V2_SETTINGS_OFFSET_STATION_PASSPHRASE,
                      APP_V2_SETTINGS_PASSPHRASE_FIELD_BYTES, settings->station_passphrase);
    return APP_V2_SETTINGS_OK;
}

app_v2_settings_result_t app_v2_device_settings_decode(const uint8_t *record, size_t record_length,
                                                       app_v2_device_settings_t *settings) {
    if (record == NULL || settings == NULL) {
        return APP_V2_SETTINGS_INVALID_ARGUMENT;
    }
    if (record_length != APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_V2_SETTINGS_INVALID_LENGTH;
    }
    if (read_u32_le(record + APP_V2_SETTINGS_OFFSET_MAGIC) != APP_V2_SETTINGS_MAGIC) {
        return APP_V2_SETTINGS_CORRUPT;
    }
    if (read_u16_le(record + APP_V2_SETTINGS_OFFSET_RECORD_VERSION) !=
        APP_V2_SETTINGS_RECORD_VERSION) {
        return APP_V2_SETTINGS_UNSUPPORTED_VERSION;
    }
    if (read_u16_le(record + APP_V2_SETTINGS_OFFSET_RECORD_LENGTH) !=
        APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_V2_SETTINGS_CORRUPT;
    }
    if (read_u16_le(record + APP_V2_SETTINGS_OFFSET_CREDENTIAL_VERSION) !=
            APP_V2_CREDENTIAL_VERSION ||
        read_u16_le(record + APP_V2_SETTINGS_OFFSET_PASSWORD_ALGORITHM) !=
            APP_V2_PASSWORD_ALGORITHM_VERSION) {
        return APP_V2_SETTINGS_UNSUPPORTED_VERSION;
    }
    /* The reserved byte accepts 0 or 1, not just 0: an already-provisioned
     * device may still hold either value from before the show-macro-source-
     * previews preference was removed (see the offset's own doc comment),
     * and this decoder has no migration path to normalize it. */
    if (!bytes_are_zero(record + APP_V2_SETTINGS_OFFSET_RESERVED, 2U) ||
        record[APP_V2_SETTINGS_OFFSET_SEND_MODE] > (uint8_t)APP_V2_SEND_MODE_PREVIEW ||
        record[APP_V2_SETTINGS_OFFSET_RETENTION_TARGET] > APP_V2_SNAPSHOT_RETENTION_TARGET_MAX ||
        record[APP_V2_SETTINGS_OFFSET_RESERVED_SHOW_SOURCE] > 1U ||
        record[APP_V2_SETTINGS_OFFSET_REQUIRE_CONFIRMATION] > 1U ||
        record[APP_V2_SETTINGS_OFFSET_PROVISIONED] > 1U ||
        record[APP_V2_SETTINGS_OFFSET_STATION_CONFIGURED] > 1U) {
        return APP_V2_SETTINGS_CORRUPT;
    }

    app_v2_device_settings_t decoded;
    memset(&decoded, 0, sizeof(decoded));
    decoded.credential_version = APP_V2_CREDENTIAL_VERSION;
    decoded.password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    decoded.password_iterations = read_u32_le(record + APP_V2_SETTINGS_OFFSET_PASSWORD_ITERATIONS);
    memcpy(decoded.password_salt, record + APP_V2_SETTINGS_OFFSET_PASSWORD_SALT,
           APP_V2_PASSWORD_SALT_BYTES);
    memcpy(decoded.password_verifier, record + APP_V2_SETTINGS_OFFSET_PASSWORD_VERIFIER,
           APP_V2_PASSWORD_VERIFIER_BYTES);
    decoded.next_blob_id = read_u64_le(record + APP_V2_SETTINGS_OFFSET_NEXT_BLOB_ID);
    decoded.send_mode = (app_v2_send_mode_t)record[APP_V2_SETTINGS_OFFSET_SEND_MODE];
    decoded.snapshot_retention_target = record[APP_V2_SETTINGS_OFFSET_RETENTION_TARGET];
    decoded.require_serial_confirmation = record[APP_V2_SETTINGS_OFFSET_REQUIRE_CONFIRMATION] != 0U;
    decoded.provisioned = record[APP_V2_SETTINGS_OFFSET_PROVISIONED] != 0U;
    decoded.station_configured = record[APP_V2_SETTINGS_OFFSET_STATION_CONFIGURED] != 0U;

    if (!copy_record_text(record + APP_V2_SETTINGS_OFFSET_LAST_SELECTED_PACKAGE,
                          APP_V2_SETTINGS_LAST_PACKAGE_FIELD_BYTES,
                          decoded.last_selected_package_id,
                          sizeof(decoded.last_selected_package_id)) ||
        !copy_record_text(record + APP_V2_SETTINGS_OFFSET_DEVICE_NAME,
                          APP_V2_SETTINGS_DEVICE_NAME_FIELD_BYTES, decoded.device_name,
                          sizeof(decoded.device_name)) ||
        !copy_record_text(record + APP_V2_SETTINGS_OFFSET_AP_SSID, APP_V2_SETTINGS_SSID_FIELD_BYTES,
                          decoded.ap_ssid, sizeof(decoded.ap_ssid)) ||
        !copy_record_text(record + APP_V2_SETTINGS_OFFSET_AP_PASSPHRASE,
                          APP_V2_SETTINGS_PASSPHRASE_FIELD_BYTES, decoded.ap_passphrase,
                          sizeof(decoded.ap_passphrase)) ||
        !copy_record_text(record + APP_V2_SETTINGS_OFFSET_STATION_SSID,
                          APP_V2_SETTINGS_SSID_FIELD_BYTES, decoded.station_ssid,
                          sizeof(decoded.station_ssid)) ||
        !copy_record_text(record + APP_V2_SETTINGS_OFFSET_STATION_PASSPHRASE,
                          APP_V2_SETTINGS_PASSPHRASE_FIELD_BYTES, decoded.station_passphrase,
                          sizeof(decoded.station_passphrase))) {
        return APP_V2_SETTINGS_CORRUPT;
    }

    const app_v2_settings_result_t validation = app_v2_device_settings_validate(&decoded);
    if (validation != APP_V2_SETTINGS_OK) {
        return validation;
    }
    *settings = decoded;
    return APP_V2_SETTINGS_OK;
}

app_v2_settings_result_t
app_v2_device_settings_reset_noncredential(app_v2_device_settings_t *settings) {
    const app_v2_settings_result_t validation = app_v2_device_settings_validate(settings);
    if (validation != APP_V2_SETTINGS_OK || settings == NULL || !settings->provisioned) {
        return validation == APP_V2_SETTINGS_OK ? APP_V2_SETTINGS_INVALID_ARGUMENT : validation;
    }

    memset(settings->device_name, 0, sizeof(settings->device_name));
    (void)memcpy(settings->device_name, "ESP32 Macro Keyboard", sizeof("ESP32 Macro Keyboard"));
    settings->require_serial_confirmation = false;
    settings->send_mode = APP_V2_SEND_MODE_QUICK;
    settings->snapshot_retention_target = APP_V2_SETTINGS_DEFAULT_SNAPSHOT_RETENTION_TARGET;
    memset(settings->last_selected_package_id, 0, sizeof(settings->last_selected_package_id));
    settings->station_configured = false;
    memset(settings->station_ssid, 0, sizeof(settings->station_ssid));
    memset(settings->station_passphrase, 0, sizeof(settings->station_passphrase));
    return app_v2_device_settings_validate(settings);
}
