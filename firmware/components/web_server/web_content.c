#include "web_content.h"

#include <stddef.h>
#include <string.h>

static bool is_space(char value)
{
    return value == ' ' || value == '\t';
}

static bool quality_is_zero(const char *parameters, size_t length)
{
    const char *cursor = parameters;
    const char *end = parameters + length;
    while (cursor < end) {
        while (cursor < end && (is_space(*cursor) || *cursor == ';')) {
            ++cursor;
        }
        if ((size_t)(end - cursor) >= 2U && cursor[0] == 'q' && cursor[1] == '=') {
            cursor += 2;
            while (cursor < end && is_space(*cursor)) {
                ++cursor;
            }
            if (cursor < end && *cursor == '0') {
                ++cursor;
                while (cursor < end && (*cursor == '0' || *cursor == '.')) {
                    ++cursor;
                }
                while (cursor < end && is_space(*cursor)) {
                    ++cursor;
                }
                return cursor == end || *cursor == ';';
            }
            return false;
        }
        while (cursor < end && *cursor != ';') {
            ++cursor;
        }
    }
    return false;
}

bool web_accept_encoding_gzip(const char *header)
{
    if (header == NULL) {
        return false;
    }
    const char *cursor = header;
    while (*cursor != '\0') {
        while (is_space(*cursor) || *cursor == ',') {
            ++cursor;
        }
        const char *end = strchr(cursor, ',');
        if (end == NULL) {
            end = cursor + strlen(cursor);
        }
        const char *token_end = cursor;
        while (token_end < end && *token_end != ';' && !is_space(*token_end)) {
            ++token_end;
        }
        if ((size_t)(token_end - cursor) == 4U && memcmp(cursor, "gzip", 4U) == 0) {
            return !quality_is_zero(token_end, (size_t)(end - token_end));
        }
        cursor = *end == '\0' ? end : end + 1;
    }
    return false;
}

const char *web_content_type(const char *path)
{
    if (path == NULL) {
        return "application/octet-stream";
    }
    const char *extension = strrchr(path, '.');
    if (extension == NULL) {
        return "application/octet-stream";
    }
    if (strcmp(extension, ".html") == 0) {
        return "text/html; charset=utf-8";
    }
    if (strcmp(extension, ".js") == 0) {
        return "text/javascript; charset=utf-8";
    }
    if (strcmp(extension, ".css") == 0) {
        return "text/css; charset=utf-8";
    }
    if (strcmp(extension, ".svg") == 0) {
        return "image/svg+xml";
    }
    if (strcmp(extension, ".json") == 0) {
        return "application/json";
    }
    if (strcmp(extension, ".png") == 0) {
        return "image/png";
    }
    return "application/octet-stream";
}
