#ifndef WEB_COOKIE_H
#define WEB_COOKIE_H

#include <stddef.h>

#include "app_error.h"

app_error_code_t web_cookie_extract_session(const char *cookie,
                                            char *out_token,
                                            size_t token_size);

#endif
