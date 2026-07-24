#include "web_origin.h"

#include <string.h>

static bool valid_authority(const char *authority)
{
    if (authority == NULL || authority[0] == '\0') {
        return false;
    }
    for (const unsigned char *cursor = (const unsigned char *)authority;
         *cursor != '\0';
         ++cursor) {
        if (*cursor <= 0x20U || *cursor >= 0x7fU || *cursor == '/' || *cursor == '?' ||
            *cursor == '#' || *cursor == '@') {
            return false;
        }
    }
    return true;
}

bool web_origin_matches_host(const char *origin, const char *host)
{
    static const char scheme[] = "http://";
    if (origin == NULL || host == NULL ||
        strncmp(origin, scheme, sizeof(scheme) - 1U) != 0) {
        return false;
    }
    const char *authority = origin + sizeof(scheme) - 1U;
    return valid_authority(authority) && valid_authority(host) && strcmp(authority, host) == 0;
}
