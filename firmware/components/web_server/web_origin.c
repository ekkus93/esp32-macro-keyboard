#include "web_origin.h"

#include <string.h>

bool web_origin_matches_host(const char *origin, const char *host)
{
    static const char scheme[] = "http://";
    if (origin == NULL || host == NULL || host[0] == '\0' ||
        strncmp(origin, scheme, sizeof(scheme) - 1U) != 0) {
        return false;
    }
    const char *authority = origin + sizeof(scheme) - 1U;
    if (strchr(authority, '/') != NULL || strchr(authority, '?') != NULL ||
        strchr(authority, '#') != NULL || strchr(authority, '@') != NULL ||
        strchr(authority, ' ') != NULL || strchr(authority, '\t') != NULL) {
        return false;
    }
    return strcmp(authority, host) == 0;
}
