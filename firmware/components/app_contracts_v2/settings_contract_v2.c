#include "settings_contract_v2.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_limits_v2.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"

#define SETTINGS_UTF8_CONTINUATION_SHIFT 6U
#define SETTINGS_UUID_TEXT_BYTES 36U
#define SETTINGS_UUID_HYPHEN_1_INDEX 8U
#define SETTINGS_UUID_HYPHEN_2_INDEX 13U
#define SETTINGS_UUID_HYPHEN_3_INDEX 18U
#define SETTINGS_UUID_HYPHEN_4_INDEX 23U
#define SETTINGS_UUID_VERSION_INDEX 14U
#define SETTINGS_UUID_VARIANT_INDEX 19U

static void secure_zero(void *memory, size_t size) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
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
            code_point = (code_point << SETTINGS_UTF8_CONTINUATION_SHIFT) |
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

static bool valid_text(app_v2_string_view_t value, size_t minimum, size_t maximum) {
    if (value.data == NULL || value.length < minimum || value.length > maximum) {
        return false;
    }
    for (size_t index = 0U; index < value.length; ++index) {
        if ((uint8_t)value.data[index] == UINT8_C(0)) {
            return false;
        }
    }
    return valid_utf8((const uint8_t *)value.data, value.length);
}

static bool valid_uuid_v4(app_v2_string_view_t value) {
    if (value.data == NULL || value.length != SETTINGS_UUID_TEXT_BYTES) {
        return false;
    }
    for (size_t index = 0U; index < SETTINGS_UUID_TEXT_BYTES; ++index) {
        const char character = value.data[index];
        if (index == SETTINGS_UUID_HYPHEN_1_INDEX || index == SETTINGS_UUID_HYPHEN_2_INDEX ||
            index == SETTINGS_UUID_HYPHEN_3_INDEX || index == SETTINGS_UUID_HYPHEN_4_INDEX) {
            if (character != '-') {
                return false;
            }
            continue;
        }
        const bool decimal = character >= '0' && character <= '9';
        const bool lowercase_hex = character >= 'a' && character <= 'f';
        if (!decimal && !lowercase_hex) {
            return false;
        }
    }
    return value.data[SETTINGS_UUID_VERSION_INDEX] == '4' &&
           (value.data[SETTINGS_UUID_VARIANT_INDEX] == '8' ||
            value.data[SETTINGS_UUID_VARIANT_INDEX] == '9' ||
            value.data[SETTINGS_UUID_VARIANT_INDEX] == 'a' ||
            value.data[SETTINGS_UUID_VARIANT_INDEX] == 'b');
}

static bool copy_view(char *destination, size_t destination_size, app_v2_string_view_t source) {
    if (destination == NULL || destination_size == 0U || source.data == NULL ||
        source.length >= destination_size) {
        return false;
    }
    memset(destination, 0, destination_size);
    memcpy(destination, source.data, source.length);
    return true;
}

static app_v2_string_view_t view_of(const char *text) {
    return (app_v2_string_view_t){.data = text, .length = strlen(text)};
}

static app_v2_optional_string_view_t optional_view_of(const char *text) {
    const size_t length = strlen(text);
    if (length == 0U) {
        return (app_v2_optional_string_view_t){.present = false};
    }
    return (app_v2_optional_string_view_t){
        .present = true,
        .value = (app_v2_string_view_t){.data = text, .length = length},
    };
}

app_v2_settings_update_result_t
app_v2_settings_response_from_settings(const app_v2_device_settings_t *settings,
                                       app_v2_settings_response_t *out_response) {
    if (out_response != NULL) {
        memset(out_response, 0, sizeof(*out_response));
    }
    if (settings == NULL || out_response == NULL) {
        return APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT;
    }
    if (app_v2_device_settings_validate(settings) != APP_V2_SETTINGS_OK || !settings->provisioned) {
        return APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS;
    }

    out_response->device_name = view_of(settings->device_name);
    out_response->require_serial_confirmation = settings->require_serial_confirmation;
    out_response->send_mode = settings->send_mode;
    out_response->snapshot_retention_target = settings->snapshot_retention_target;
    out_response->show_macro_source_previews = settings->show_macro_source_previews;
    out_response->last_selected_package_id = optional_view_of(settings->last_selected_package_id);
    out_response->ap_ssid = view_of(settings->ap_ssid);
    out_response->station_configured = settings->station_configured;
    out_response->station_ssid = settings->station_configured
                                     ? optional_view_of(settings->station_ssid)
                                     : (app_v2_optional_string_view_t){0};
    return APP_V2_SETTINGS_UPDATE_OK;
}

static bool settings_update_request_is_empty(const app_v2_settings_update_request_t *request) {
    return !request->has_device_name && !request->has_require_serial_confirmation &&
           !request->has_send_mode && !request->has_snapshot_retention_target &&
           !request->has_show_macro_source_previews && !request->has_last_selected_package_id &&
           !request->has_access_point && !request->has_station;
}

static app_v2_settings_update_result_t
apply_device_name(const app_v2_settings_update_request_t *request,
                  app_v2_device_settings_t *candidate) {
    if (!request->has_device_name) {
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (!valid_text(request->device_name, 1U, (size_t)APP_V2_DEVICE_NAME_MAX_BYTES) ||
        !copy_view(candidate->device_name, sizeof(candidate->device_name), request->device_name)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME;
    }
    return APP_V2_SETTINGS_UPDATE_OK;
}

static app_v2_settings_update_result_t
apply_snapshot_retention_target(const app_v2_settings_update_request_t *request,
                                app_v2_device_settings_t *candidate) {
    if (!request->has_snapshot_retention_target) {
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (request->snapshot_retention_target > APP_V2_SNAPSHOT_RETENTION_TARGET_MAX) {
        return APP_V2_SETTINGS_UPDATE_INVALID_SNAPSHOT_RETENTION_TARGET;
    }
    candidate->snapshot_retention_target = request->snapshot_retention_target;
    return APP_V2_SETTINGS_UPDATE_OK;
}

static app_v2_settings_update_result_t
apply_last_selected_package_id(const app_v2_settings_update_request_t *request,
                               app_v2_device_settings_t *candidate) {
    if (!request->has_last_selected_package_id) {
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (!request->last_selected_package_id.present) {
        memset(candidate->last_selected_package_id, 0, sizeof(candidate->last_selected_package_id));
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (!valid_uuid_v4(request->last_selected_package_id.value) ||
        !copy_view(candidate->last_selected_package_id, sizeof(candidate->last_selected_package_id),
                   request->last_selected_package_id.value)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID;
    }
    return APP_V2_SETTINGS_UPDATE_OK;
}

/* Bundles the two independent output flags apply_access_point()/
 * apply_station() report so neither function takes two adjacent `bool *`
 * parameters (bugprone-easily-swappable-parameters). */
typedef struct {
    bool restart_required;
    bool reconnect_required;
} settings_update_flags_t;

/* Sets out_flags->restart_required and out_flags->reconnect_required only on
 * success -- the caller initializes both to false and never clears them, so
 * a later field's failure cannot un-set an earlier field's true. */
static app_v2_settings_update_result_t
apply_access_point(const app_v2_settings_update_request_t *request,
                   app_v2_device_settings_t *candidate, settings_update_flags_t *out_flags) {
    if (!request->has_access_point) {
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (!valid_text(request->access_point.ssid, 1U, (size_t)APP_V2_WIFI_SSID_MAX_BYTES)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_SSID;
    }
    if (!valid_text(request->access_point.passphrase, (size_t)APP_V2_WIFI_PASSPHRASE_MIN_BYTES,
                    (size_t)APP_V2_WIFI_PASSPHRASE_MAX_BYTES)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_PASSPHRASE;
    }
    if (!copy_view(candidate->ap_ssid, sizeof(candidate->ap_ssid), request->access_point.ssid) ||
        !copy_view(candidate->ap_passphrase, sizeof(candidate->ap_passphrase),
                   request->access_point.passphrase)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_PASSPHRASE;
    }
    out_flags->restart_required = true;
    out_flags->reconnect_required = true;
    return APP_V2_SETTINGS_UPDATE_OK;
}

/* Station changes never touch out_flags->reconnect_required: they do not
 * disturb the browser's own access-point session. See
 * settings_contract_v2.h's app_v2_settings_prepare_update() doc comment for
 * why they still require a restart. */
static app_v2_settings_update_result_t
apply_station(const app_v2_settings_update_request_t *request, app_v2_device_settings_t *candidate,
              settings_update_flags_t *out_flags) {
    if (!request->has_station) {
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (request->remove_station) {
        candidate->station_configured = false;
        memset(candidate->station_ssid, 0, sizeof(candidate->station_ssid));
        memset(candidate->station_passphrase, 0, sizeof(candidate->station_passphrase));
        out_flags->restart_required = true;
        return APP_V2_SETTINGS_UPDATE_OK;
    }
    if (!valid_text(request->station.ssid, 1U, (size_t)APP_V2_WIFI_SSID_MAX_BYTES)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_STATION_SSID;
    }
    if (!valid_text(request->station.passphrase, (size_t)APP_V2_WIFI_PASSPHRASE_MIN_BYTES,
                    (size_t)APP_V2_WIFI_PASSPHRASE_MAX_BYTES)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_STATION_PASSPHRASE;
    }
    if (!copy_view(candidate->station_ssid, sizeof(candidate->station_ssid),
                   request->station.ssid) ||
        !copy_view(candidate->station_passphrase, sizeof(candidate->station_passphrase),
                   request->station.passphrase)) {
        return APP_V2_SETTINGS_UPDATE_INVALID_STATION_PASSPHRASE;
    }
    candidate->station_configured = true;
    out_flags->restart_required = true;
    return APP_V2_SETTINGS_UPDATE_OK;
}

app_v2_settings_update_result_t
app_v2_settings_prepare_update(const app_v2_device_settings_t *current,
                               const app_v2_settings_update_request_t *request,
                               app_v2_device_settings_t *out_candidate, bool *out_restart_required,
                               bool *out_reconnect_required) {
    if (out_candidate != NULL) {
        memset(out_candidate, 0, sizeof(*out_candidate));
    }
    if (out_restart_required != NULL) {
        *out_restart_required = false;
    }
    if (out_reconnect_required != NULL) {
        *out_reconnect_required = false;
    }
    if (current == NULL || request == NULL || out_candidate == NULL ||
        out_restart_required == NULL || out_reconnect_required == NULL) {
        return APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT;
    }
    if (app_v2_device_settings_validate(current) != APP_V2_SETTINGS_OK || !current->provisioned) {
        return APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS;
    }
    if (settings_update_request_is_empty(request)) {
        return APP_V2_SETTINGS_UPDATE_EMPTY;
    }

    app_v2_device_settings_t candidate = *current;
    settings_update_flags_t flags = {0};

    app_v2_settings_update_result_t result = apply_device_name(request, &candidate);
    if (result == APP_V2_SETTINGS_UPDATE_OK && request->has_require_serial_confirmation) {
        candidate.require_serial_confirmation = request->require_serial_confirmation;
    }
    if (result == APP_V2_SETTINGS_UPDATE_OK && request->has_send_mode) {
        candidate.send_mode = request->send_mode;
    }
    if (result == APP_V2_SETTINGS_UPDATE_OK) {
        result = apply_snapshot_retention_target(request, &candidate);
    }
    if (result == APP_V2_SETTINGS_UPDATE_OK && request->has_show_macro_source_previews) {
        candidate.show_macro_source_previews = request->show_macro_source_previews;
    }
    if (result == APP_V2_SETTINGS_UPDATE_OK) {
        result = apply_last_selected_package_id(request, &candidate);
    }
    if (result == APP_V2_SETTINGS_UPDATE_OK) {
        result = apply_access_point(request, &candidate, &flags);
    }
    if (result == APP_V2_SETTINGS_UPDATE_OK) {
        result = apply_station(request, &candidate, &flags);
    }
    if (result != APP_V2_SETTINGS_UPDATE_OK) {
        return result;
    }

    if (app_v2_device_settings_validate(&candidate) != APP_V2_SETTINGS_OK) {
        return APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS;
    }

    *out_candidate = candidate;
    *out_restart_required = flags.restart_required;
    *out_reconnect_required = flags.reconnect_required;
    return APP_V2_SETTINGS_UPDATE_OK;
}

app_v2_password_change_result_t
app_v2_password_change_validate(const app_v2_device_settings_t *current,
                                app_v2_string_view_t new_password) {
    if (current == NULL) {
        return APP_V2_PASSWORD_CHANGE_INVALID_ARGUMENT;
    }
    if (app_v2_device_settings_validate(current) != APP_V2_SETTINGS_OK || !current->provisioned) {
        return APP_V2_PASSWORD_CHANGE_INVALID_CURRENT_SETTINGS;
    }
    if (!valid_text(new_password, (size_t)APP_V2_ADMIN_PASSWORD_MIN_BYTES,
                    (size_t)APP_V2_ADMIN_PASSWORD_MAX_BYTES)) {
        return APP_V2_PASSWORD_CHANGE_INVALID_NEW_PASSWORD;
    }
    return APP_V2_PASSWORD_CHANGE_OK;
}

bool app_v2_password_change_prepare_candidate(const app_v2_device_settings_t *current,
                                              const app_v2_setup_password_material_t *material,
                                              app_v2_device_settings_t *out_candidate) {
    if (out_candidate != NULL) {
        memset(out_candidate, 0, sizeof(*out_candidate));
    }
    if (current == NULL || material == NULL || out_candidate == NULL) {
        return false;
    }
    app_v2_device_settings_t candidate = *current;
    candidate.credential_version = material->credential_version;
    candidate.password_algorithm_version = material->password_algorithm_version;
    candidate.password_iterations = material->password_iterations;
    memcpy(candidate.password_salt, material->password_salt, sizeof(candidate.password_salt));
    memcpy(candidate.password_verifier, material->password_verifier,
           sizeof(candidate.password_verifier));
    if (app_v2_device_settings_validate(&candidate) != APP_V2_SETTINGS_OK) {
        secure_zero(&candidate, sizeof(candidate));
        return false;
    }
    *out_candidate = candidate;
    secure_zero(&candidate, sizeof(candidate));
    return true;
}
