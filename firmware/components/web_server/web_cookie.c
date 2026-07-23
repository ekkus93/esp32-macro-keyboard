#include "web_cookie.h"

#include <stdbool.h>
#include <string.h>

#include "auth.h"

#define SESSION_COOKIE_NAME "MKSESSION"

static bool valid_token(const char *token, size_t length)
{
    if (token == NULL || length != AUTH_TOKEN_HEX_BYTES - 1U) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        if (!((token[index] >= '0' && token[index] <= '9') ||
              (token[index] >= 'a' && token[index] <= 'f'))) {
            return false;
        }
    }
    return true;
}

app_error_code_t web_cookie_extract_session(const char *cookie,
                                            char *out_token,
                                            size_t token_size)
{
    if (out_token != NULL && token_size > 0U) {
        out_token[0] = '\0';
    }
    if (cookie == NULL || out_token == NULL || token_size < AUTH_TOKEN_HEX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    static const char prefix[] = SESSION_COOKIE_NAME "=";
    const size_t prefix_length = sizeof(prefix) - 1U;
    const char *cursor = cookie;
    const char *match = NULL;
    size_t match_length = 0U;
    while (*cursor != '\0') {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == ';') {
            ++cursor;
        }
        const char *end = strchr(cursor, ';');
        const size_t segment_length = end == NULL ? strlen(cursor) : (size_t)(end - cursor);
        size_t trimmed_length = segment_length;
        while (trimmed_length > 0U &&
               (cursor[trimmed_length - 1U] == ' ' || cursor[trimmed_length - 1U] == '\t')) {
            --trimmed_length;
        }
        if (trimmed_length >= prefix_length && memcmp(cursor, prefix, prefix_length) == 0) {
            if (match != NULL) {
                return APP_ERROR_AUTH_REQUIRED;
            }
            match = cursor + prefix_length;
            match_length = trimmed_length - prefix_length;
        }
        if (end == NULL) {
            break;
        }
        cursor = end + 1;
    }

    if (match == NULL || !valid_token(match, match_length) || match_length >= token_size) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    memcpy(out_token, match, match_length);
    out_token[match_length] = '\0';
    return APP_ERROR_NONE;
}
