#include "setup_contract_v2.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_limits_v2.h"
#include "device_settings_v2.h"

#define APP_V2_SETUP_UTF8_CONTINUATION_SHIFT 6U

static void secure_zero(void *memory, size_t size) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

static bool bytes_are_zero(const uint8_t *bytes, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        if (bytes[index] != 0U) {
            return false;
        }
    }
    return true;
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
            code_point = (code_point << APP_V2_SETUP_UTF8_CONTINUATION_SHIFT) |
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

static bool valid_setup_code(app_v2_string_view_t code) {
    if (code.data == NULL || code.length != (size_t)APP_V2_SETUP_CODE_DIGITS) {
        return false;
    }
    for (size_t index = 0U; index < code.length; ++index) {
        if (code.data[index] < '0' || code.data[index] > '9') {
            return false;
        }
    }
    return true;
}

static bool constant_time_code_equal(const char expected[APP_V2_SETUP_CODE_BUFFER_BYTES],
                                     app_v2_string_view_t supplied) {
    uint8_t difference = 0U;
    for (size_t index = 0U; index < (size_t)APP_V2_SETUP_CODE_DIGITS; ++index) {
        difference |= (uint8_t)expected[index] ^ (uint8_t)supplied.data[index];
    }
    return difference == 0U;
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

static void format_setup_code(uint32_t value, char output[APP_V2_SETUP_CODE_BUFFER_BYTES]) {
    for (size_t index = (size_t)APP_V2_SETUP_CODE_DIGITS; index > 0U; --index) {
        output[index - 1U] = (char)('0' + (value % UINT32_C(10)));
        value /= UINT32_C(10);
    }
    output[APP_V2_SETUP_CODE_DIGITS] = '\0';
}

static bool password_material_valid(const app_v2_setup_password_material_t *material) {
    return material != NULL && material->credential_version == APP_V2_CREDENTIAL_VERSION &&
           material->password_algorithm_version == APP_V2_PASSWORD_ALGORITHM_VERSION &&
           material->password_iterations > 0U &&
           !bytes_are_zero(material->password_salt, sizeof(material->password_salt)) &&
           !bytes_are_zero(material->password_verifier, sizeof(material->password_verifier));
}

app_v2_setup_result_t app_v2_setup_session_generate(const app_v2_setup_random_ops_t *operations,
                                                    app_v2_setup_session_t *session) {
    if (session != NULL) {
        secure_zero(session, sizeof(*session));
    }
    if (operations == NULL || operations->random_u32 == NULL || session == NULL) {
        return APP_V2_SETUP_INVALID_ARGUMENT;
    }

    const uint64_t source_space = (uint64_t)UINT32_MAX + UINT64_C(1);
    const uint64_t accepted_space =
        source_space - (source_space % (uint64_t)APP_V2_SETUP_CODE_SPACE);
    for (uint32_t attempt = 0U; attempt < APP_V2_SETUP_RANDOM_ATTEMPTS_MAX; ++attempt) {
        uint32_t random_value = 0U;
        if (!operations->random_u32(operations->context, &random_value)) {
            secure_zero(session, sizeof(*session));
            return APP_V2_SETUP_RANDOM_FAILURE;
        }
        if ((uint64_t)random_value >= accepted_space) {
            continue;
        }
        format_setup_code(random_value % APP_V2_SETUP_CODE_SPACE, session->code);
        session->consumed = false;
        return APP_V2_SETUP_OK;
    }

    secure_zero(session, sizeof(*session));
    return APP_V2_SETUP_RANDOM_FAILURE;
}

app_v2_setup_result_t app_v2_setup_session_init(app_v2_setup_session_t *session,
                                                app_v2_string_view_t code) {
    if (session != NULL) {
        secure_zero(session, sizeof(*session));
    }
    if (session == NULL) {
        return APP_V2_SETUP_INVALID_ARGUMENT;
    }
    if (!valid_setup_code(code)) {
        return APP_V2_SETUP_MALFORMED_CODE;
    }
    memcpy(session->code, code.data, (size_t)APP_V2_SETUP_CODE_DIGITS);
    session->code[APP_V2_SETUP_CODE_DIGITS] = '\0';
    session->consumed = false;
    return APP_V2_SETUP_OK;
}

app_v2_setup_result_t app_v2_setup_session_consume(app_v2_setup_session_t *session) {
    if (session == NULL) {
        return APP_V2_SETUP_INVALID_ARGUMENT;
    }
    if (session->consumed) {
        return APP_V2_SETUP_CODE_CONSUMED;
    }
    secure_zero(session->code, sizeof(session->code));
    session->consumed = true;
    return APP_V2_SETUP_OK;
}

app_v2_setup_result_t
app_v2_setup_state_from_settings(const app_v2_device_settings_t *settings,
                                 app_v2_setup_state_response_t *out_response) {
    if (out_response != NULL) {
        memset(out_response, 0, sizeof(*out_response));
    }
    if (settings == NULL || out_response == NULL) {
        return APP_V2_SETUP_INVALID_ARGUMENT;
    }
    if (app_v2_device_settings_validate(settings) != APP_V2_SETTINGS_OK) {
        return APP_V2_SETUP_INVALID_CURRENT_SETTINGS;
    }
    if (settings->provisioned) {
        return APP_V2_SETUP_ALREADY_PROVISIONED;
    }

    size_t device_name_length = 0U;
    while (device_name_length < sizeof(settings->device_name) &&
           settings->device_name[device_name_length] != '\0') {
        ++device_name_length;
    }
    out_response->provisioned = false;
    out_response->device_name = (app_v2_string_view_t){
        .data = settings->device_name,
        .length = device_name_length,
    };
    return APP_V2_SETUP_OK;
}

app_v2_setup_result_t
app_v2_setup_prepare_candidate(const app_v2_setup_session_t *session,
                               const app_v2_setup_request_t *request,
                               const app_v2_device_settings_t *current,
                               const app_v2_setup_password_material_t *password_material,
                               app_v2_device_settings_t *out_candidate) {
    if (out_candidate != NULL) {
        memset(out_candidate, 0, sizeof(*out_candidate));
    }
    if (session == NULL || request == NULL || current == NULL || password_material == NULL ||
        out_candidate == NULL) {
        return APP_V2_SETUP_INVALID_ARGUMENT;
    }
    if (app_v2_device_settings_validate(current) != APP_V2_SETTINGS_OK) {
        return APP_V2_SETUP_INVALID_CURRENT_SETTINGS;
    }
    if (current->provisioned) {
        return APP_V2_SETUP_ALREADY_PROVISIONED;
    }
    if (session->consumed) {
        return APP_V2_SETUP_CODE_CONSUMED;
    }
    if (!valid_setup_code(request->setup_code)) {
        return APP_V2_SETUP_MALFORMED_CODE;
    }
    if (!constant_time_code_equal(session->code, request->setup_code)) {
        return APP_V2_SETUP_CODE_MISMATCH;
    }
    if (!valid_text(request->device_name, 1U, (size_t)APP_V2_DEVICE_NAME_MAX_BYTES)) {
        return APP_V2_SETUP_INVALID_DEVICE_NAME;
    }
    if (!valid_text(request->ap_ssid, 1U, (size_t)APP_V2_WIFI_SSID_MAX_BYTES)) {
        return APP_V2_SETUP_INVALID_AP_SSID;
    }
    if (!valid_text(request->ap_passphrase, (size_t)APP_V2_WIFI_PASSPHRASE_MIN_BYTES,
                    (size_t)APP_V2_WIFI_PASSPHRASE_MAX_BYTES)) {
        return APP_V2_SETUP_INVALID_AP_PASSPHRASE;
    }
    if (!valid_text(request->admin_password, (size_t)APP_V2_ADMIN_PASSWORD_MIN_BYTES,
                    (size_t)APP_V2_ADMIN_PASSWORD_MAX_BYTES)) {
        return APP_V2_SETUP_INVALID_ADMIN_PASSWORD;
    }
    if (!password_material_valid(password_material)) {
        return APP_V2_SETUP_INVALID_PASSWORD_MATERIAL;
    }

    app_v2_device_settings_t candidate = *current;
    candidate.provisioned = true;
    candidate.credential_version = password_material->credential_version;
    candidate.password_algorithm_version = password_material->password_algorithm_version;
    candidate.password_iterations = password_material->password_iterations;
    memcpy(candidate.password_salt, password_material->password_salt,
           sizeof(candidate.password_salt));
    memcpy(candidate.password_verifier, password_material->password_verifier,
           sizeof(candidate.password_verifier));
    candidate.require_serial_confirmation = request->require_serial_confirmation;

    if (!copy_view(candidate.device_name, sizeof(candidate.device_name), request->device_name) ||
        !copy_view(candidate.ap_ssid, sizeof(candidate.ap_ssid), request->ap_ssid) ||
        !copy_view(candidate.ap_passphrase, sizeof(candidate.ap_passphrase),
                   request->ap_passphrase)) {
        secure_zero(&candidate, sizeof(candidate));
        return APP_V2_SETUP_INVALID_ARGUMENT;
    }

    if (app_v2_device_settings_validate(&candidate) != APP_V2_SETTINGS_OK) {
        secure_zero(&candidate, sizeof(candidate));
        return APP_V2_SETUP_INVALID_CURRENT_SETTINGS;
    }

    *out_candidate = candidate;
    secure_zero(&candidate, sizeof(candidate));
    return APP_V2_SETUP_OK;
}

void app_v2_setup_accepted_response_init(app_v2_setup_accepted_t *response) {
    if (response == NULL) {
        return;
    }
    *response = (app_v2_setup_accepted_t){
        .action =
            {
                .accepted = true,
                .connection_will_close = true,
                .reprovisioning_required = false,
            },
        .restart_required = true,
    };
}