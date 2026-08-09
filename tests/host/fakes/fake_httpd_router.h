#ifndef FAKE_HTTPD_ROUTER_H
#define FAKE_HTTPD_ROUTER_H

#include "esp_http_server.h"

/* Fake backend for esp_http_server_stub's httpd_start()/httpd_stop()/
 * httpd_register_uri_handler()/httpd_uri_match_wildcard() -- lets host tests
 * drive the real web_server_lifecycle.c (web_server_start()/web_server_stop())
 * and then ask "which handler would a real esp_http_server answer this
 * uri/method with, in this provisioning mode?" without a real socket or
 * URI-trie.
 *
 * This is a different fake from fake_httpd.h/fake_httpd.c on purpose:
 * fake_httpd.c is a per-request body/response double that lets a test call
 * one fixed-URI handler function directly (blob/status/limits/send/setup/
 * administration route tests all point httpd_req_t.aux at one of its
 * instances). fake_httpd_router.c is a registration/dispatch double one
 * level up the stack -- it never touches httpd_req_t.aux and is only linked
 * into web_server_lifecycle_tests, the one target that compiles
 * web_server_lifecycle.c and therefore needs httpd_start()/
 * httpd_register_uri_handler() to have real bodies to link against.
 *
 * The dispatch algorithm below is ported from ESP-IDF v5.5.5's
 * httpd_find_uri_handler() (components/esp_http_server/src/httpd_uri.c):
 * scan every registered entry in registration order; the first entry whose
 * uri matches (via the config's uri_match_fn) AND whose method matches wins;
 * a uri match with a different method is remembered as a possible 405 but
 * scanning continues (a later entry can register the same uri under a
 * different method, e.g. GET and POST /api/v1/blob are two separate
 * httpd_uri_t entries); if nothing matched at all, it is a 404. This exact
 * distinction (405 vs. 404) matters for the setup-route matrix: DELETE
 * /api/v1/setup must resolve as "uri registered, method not allowed", not
 * "uri absent", because only GET and POST are registered for it. */

#define FAKE_HTTPD_ROUTER_MAX_HANDLERS 32U

typedef enum {
    FAKE_HTTPD_ROUTE_FOUND,
    FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED,
    FAKE_HTTPD_ROUTE_NOT_FOUND,
} fake_httpd_route_result_t;

/* Clears all registered routes and the started/stopped bookkeeping.
 * web_server_start()/web_server_stop() (via httpd_start()/httpd_stop()) drive
 * the started/stopped state themselves; call this between test cases so a
 * prior case's registrations never leak into the next one even if a test
 * forgets to call web_server_stop(). */
void fake_httpd_router_reset(void);

/* True once httpd_start() has been called without a matching httpd_stop().
 * Lets a test assert web_server_stop() actually released the fake server. */
bool fake_httpd_router_started(void);

size_t fake_httpd_router_registered_count(void);

/* Resolves uri/method against every route registered so far, using the same
 * scan ESP-IDF's real httpd_find_uri_handler() performs. *out_handler is set
 * to the matched handler on FAKE_HTTPD_ROUTE_FOUND and left untouched
 * otherwise (callers should not read it unless the result is FOUND). */
fake_httpd_route_result_t fake_httpd_router_resolve(const char *uri, httpd_method_t method,
                                                    esp_err_t (**out_handler)(httpd_req_t *));

#endif
