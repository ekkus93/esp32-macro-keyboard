#ifndef WEB_STATIC_PATH_H
#define WEB_STATIC_PATH_H

#include <stdbool.h>
#include <stddef.h>

bool web_static_uri_normalize(const char *uri, char *normalized, size_t normalized_size);

#endif
