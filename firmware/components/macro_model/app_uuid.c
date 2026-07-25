#include "app_uuid.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "app_error.h"
#include "macro_limits.h"

#ifdef ESP_PLATFORM
#include "esp_random.h"
#endif

/* Positions and bit patterns for an RFC 4122 version-4 UUID. */
#define UUID_VERSION_CHAR_INDEX 14U
#define UUID_VARIANT_CHAR_INDEX 19U
#define UUID_VERSION_BYTE_INDEX 6U
#define UUID_VARIANT_BYTE_INDEX 8U
#define UUID_VERSION_NIBBLE_MASK 0x0fU
#define UUID_VERSION_4 0x40U
#define UUID_VARIANT_BITS_MASK 0x3fU
#define UUID_VARIANT_RFC4122 0x80U

static bool is_lower_hex(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f');
}

bool app_uuid_is_valid_string(const char *text) {
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

    return text[UUID_VERSION_CHAR_INDEX] == '4' &&
           (text[UUID_VARIANT_CHAR_INDEX] == '8' || text[UUID_VARIANT_CHAR_INDEX] == '9' ||
            text[UUID_VARIANT_CHAR_INDEX] == 'a' || text[UUID_VARIANT_CHAR_INDEX] == 'b');
}

app_error_code_t app_uuid_parse(const char *text, app_uuid_t *out_uuid) {
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

app_error_code_t app_uuid_generate(app_uuid_t *out_uuid) {
    if (out_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t bytes[16] = {0U};
#ifdef ESP_PLATFORM
    esp_fill_random(bytes, sizeof(bytes));
#else
    int descriptor = open("/dev/urandom", O_RDONLY);
    if (descriptor < 0) {
        return APP_ERROR_INTERNAL;
    }
    size_t offset = 0U;
    while (offset < sizeof(bytes)) {
        const ssize_t count = read(descriptor, bytes + offset, sizeof(bytes) - offset);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            const int close_result = close(descriptor);
            if (close_result != 0) {
                return APP_ERROR_INTERNAL;
            }
            return APP_ERROR_INTERNAL;
        }
        if (count == 0) {
            const int close_result = close(descriptor);
            if (close_result != 0) {
                return APP_ERROR_INTERNAL;
            }
            return APP_ERROR_INTERNAL;
        }
        offset += (size_t)count;
    }
    if (close(descriptor) != 0) {
        return APP_ERROR_INTERNAL;
    }
#endif
    bytes[UUID_VERSION_BYTE_INDEX] =
        (uint8_t)((bytes[UUID_VERSION_BYTE_INDEX] & UUID_VERSION_NIBBLE_MASK) | UUID_VERSION_4);
    bytes[UUID_VARIANT_BYTE_INDEX] =
        (uint8_t)((bytes[UUID_VARIANT_BYTE_INDEX] & UUID_VARIANT_BITS_MASK) | UUID_VARIANT_RFC4122);

    const int written =
        snprintf(out_uuid->value, sizeof(out_uuid->value),
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", bytes[0],
                 bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7], bytes[8],
                 bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    if (written != (int)APP_UUID_STRING_LENGTH) {
        out_uuid->value[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

bool app_uuid_equal(const app_uuid_t *left, const app_uuid_t *right) {
    return left != NULL && right != NULL &&
           memcmp(left->value, right->value, sizeof(left->value)) == 0;
}
