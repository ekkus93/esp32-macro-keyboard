#ifndef WEB_ORIGIN_H
#define WEB_ORIGIN_H

#include <stdbool.h>

bool web_origin_matches_host(const char *origin, const char *host);

#endif
