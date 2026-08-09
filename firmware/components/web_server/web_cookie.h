#ifndef WEB_COOKIE_H
#define WEB_COOKIE_H

#include <stddef.h>

#include "app_error.h"

app_error_code_t web_cookie_extract_session(const char *cookie, char *out_token, size_t token_size);

/* Builds the exact Set-Cookie header value login_handler sends on a
 * successful login (SPEC_V2 13.5): "MKSESSION=<token>; HttpOnly;
 * SameSite=Strict; Path=/". No esp_http_server dependency, so this half of
 * the cookie logic is host-testable like web_cookie_extract_session() above;
 * the httpd adapter (web_server_login.c::send_login_accepted) calls it
 * directly and sets the returned value verbatim as the "Set-Cookie" header.
 * Fails (without writing to `out_header`) when `session_token` is NULL, does
 * not fit AUTH_TOKEN_HEX_BYTES, or the formatted header does not fit
 * `out_header_size`. */
app_error_code_t web_cookie_build_session_header(const char *session_token, char *out_header,
                                                 size_t out_header_size);

#endif
