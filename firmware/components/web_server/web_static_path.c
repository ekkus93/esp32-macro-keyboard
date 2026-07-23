#include "web_static_path.h"

#include <string.h>

static bool allowed_character(char character)
{
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '/' ||
           character == '-' || character == '_' || character == '.';
}

bool web_static_uri_normalize(const char *uri, char *normalized, size_t normalized_size)
{
    if (normalized != NULL && normalized_size > 0U) {
        normalized[0] = '\0';
    }
    if (uri == NULL || normalized == NULL || normalized_size < 2U || uri[0] != '/') {
        return false;
    }
    if (strncmp(uri, "/api/", 5U) == 0 || strcmp(uri, "/api") == 0 ||
        strncmp(uri, "/api?", 5U) == 0) {
        return false;
    }

    size_t output = 0U;
    for (size_t input = 0U; uri[input] != '\0' && uri[input] != '?'; ++input) {
        const char character = uri[input];
        if (!allowed_character(character) || output + 1U >= normalized_size) {
            normalized[0] = '\0';
            return false;
        }
        normalized[output++] = character;
    }
    normalized[output] = '\0';
    if (output == 0U || strstr(normalized, "..") != NULL) {
        normalized[0] = '\0';
        return false;
    }
    return true;
}
