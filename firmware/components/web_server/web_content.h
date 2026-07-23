#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

#include <stdbool.h>

const char *web_content_type(const char *path);
bool web_accept_encoding_gzip(const char *header);

#endif
