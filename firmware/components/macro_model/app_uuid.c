#include "app_uuid.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_random.h"
#endif

static bool is_lower_hex(char value)
{
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f');
}

bool app_uuid_is_valid_string(const char *text)
{
    static const size_t hyphen_positions[] = {8U, 13U, 18U, 23U};
    size_t hyphen_index = 0U;

    if (text == NULL || strlen(text) != APP_UUID_STRING_LENGTH) {
        return false;
    }

    for (size_t index = 0U; index < APP_UUID_STRING_LENGTH; ++index) {
        if (hyphen_index < (sizeof(hyphen_positions) / sizeof(hyphen_positions[0])) &&
            index == hyphen_positions[hyphen_index]) {
            if (text[index] != '-') {
                return false;
            }
            ++hyphen_index;
        } else if (!is_lower_hex(text[index])) {
            return false;
        }
    }

    return text[14] == '4' && (text[19] == '8' || text[19] == '9' ||
                              text[19] == 'a' || text[19] == 'b');
}

app_error_code_t app_uuid_parse(const char *text, app_uuid_t *out_uuid)
{
    if (out_uuid == NULL || !app_uuid_is_valid_string(text)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const int written = snprintf(out_uuid->value, sizeof(out_uuid->value), "%s", text);
    if (written != (int)APP_UUID_STRING_LENGTH) {
        out_uuid->value[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

app_error_code_t app_uuid_generate(app_uuid_t *out_uuid)
{
    if (out_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t bytes[16] = {0U};
#ifdef ESP_PLATFORM
    esp_fill_random(bytes, sizeof(bytes));
#else
    return APP_ERROR_INTERNAL;
#endif
    bytes[6] = (uint8_t)((bytes[6] & 0x0fU) | 0x40U);
    bytes[8] = (uint8_t)((bytes[8] & 0x3fU) | 0x80U);

    const int written = snprintf(
        out_uuid->value,
        sizeof(out_uuid->value),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
        bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    if (written != (int)APP_UUID_STRING_LENGTH) {
        out_uuid->value[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

bool app_uuid_equal(const app_uuid_t *left, const app_uuid_t *right)
{
    return left != NULL && right != NULL &&
           memcmp(left->value, right->value, sizeof(left->value)) == 0;
}
