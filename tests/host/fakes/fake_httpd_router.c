#include "fake_httpd_router.h"

#include <stddef.h>
#include <string.h>

/* See fake_httpd_router.h for the design rationale and the ESP-IDF source
 * this is ported from. */

typedef struct {
    httpd_uri_t entries[FAKE_HTTPD_ROUTER_MAX_HANDLERS];
    size_t count;
    uint16_t max_uri_handlers;
    httpd_uri_match_func_t uri_match_fn;
    bool started;
} fake_httpd_router_state_t;

static fake_httpd_router_state_t router;

void fake_httpd_router_reset(void) {
    memset(&router, 0, sizeof(router));
}

bool fake_httpd_router_started(void) {
    return router.started;
}

size_t fake_httpd_router_registered_count(void) {
    return router.count;
}

/* Ported from ESP-IDF v5.5.5's httpd_uri_match_wildcard()
 * (components/esp_http_server/src/httpd_uri.c) for exact fidelity --
 * web_server_lifecycle.c configures the real production route tables with
 * this exact matcher (configuration.uri_match_fn = httpd_uri_match_wildcard),
 * so a test relying on it needs the real matching semantics, not an
 * approximation. Every first-party route template in this codebase uses only
 * a trailing "*" or no wildcard at all -- never "?" -- but the "?" branch is
 * kept so this stays a faithful port rather than a narrowed one. */
bool httpd_uri_match_wildcard(const char *uri_template, const char *uri_to_match,
                              size_t match_upto) {
    const size_t template_length = strlen(uri_template);
    size_t exact_match_chars = template_length;

    const char last = template_length > 0U ? uri_template[template_length - 1U] : '\0';
    const char previous_last = template_length > 1U ? uri_template[template_length - 2U] : '\0';
    const bool has_asterisk = last == '*' || (previous_last == '*' && last == '?');
    const bool has_question = last == '?' || (previous_last == '?' && last == '*');

    const size_t special_char_count = (has_asterisk ? 1U : 0U) + (has_question ? 2U : 0U);
    if (exact_match_chars < special_char_count) {
        return false;
    }
    exact_match_chars -= special_char_count;

    if (match_upto < exact_match_chars) {
        return false;
    }

    if (!has_question) {
        if (!has_asterisk && match_upto != exact_match_chars) {
            return false;
        }
        return strncmp(uri_template, uri_to_match, exact_match_chars) == 0;
    }
    if (match_upto > exact_match_chars &&
        uri_template[exact_match_chars] != uri_to_match[exact_match_chars]) {
        return false;
    }
    if (strncmp(uri_template, uri_to_match, exact_match_chars) != 0) {
        return false;
    }
    return has_asterisk || match_upto <= exact_match_chars + 1U;
}

esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config) {
    if (handle == NULL || config == NULL || router.started) {
        return ESP_FAIL;
    }
    router.count = 0U;
    router.max_uri_handlers = config->max_uri_handlers;
    router.uri_match_fn = config->uri_match_fn;
    router.started = true;
    *handle = &router;
    return ESP_OK;
}

esp_err_t httpd_stop(httpd_handle_t handle) {
    if (handle != &router || !router.started) {
        return ESP_FAIL;
    }
    router.started = false;
    router.count = 0U;
    return ESP_OK;
}

esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri_handler) {
    if (handle != &router || !router.started || uri_handler == NULL || uri_handler->uri == NULL ||
        uri_handler->handler == NULL) {
        return ESP_FAIL;
    }
    if (router.count >= router.max_uri_handlers || router.count >= FAKE_HTTPD_ROUTER_MAX_HANDLERS) {
        return ESP_FAIL;
    }
    router.entries[router.count] = *uri_handler;
    ++router.count;
    return ESP_OK;
}

/* Ported from ESP-IDF v5.5.5's httpd_find_uri_handler() -- see the header
 * comment for why the 404-vs-405 distinction matters. */
fake_httpd_route_result_t fake_httpd_router_resolve(const char *uri, httpd_method_t method,
                                                    esp_err_t (**out_handler)(httpd_req_t *)) {
    if (uri == NULL || !router.started) {
        return FAKE_HTTPD_ROUTE_NOT_FOUND;
    }
    const size_t uri_length = strlen(uri);
    bool method_mismatch_found = false;
    for (size_t index = 0U; index < router.count; ++index) {
        const httpd_uri_t *entry = &router.entries[index];
        const bool uri_matches =
            router.uri_match_fn != NULL
                ? router.uri_match_fn(entry->uri, uri, uri_length)
                : (strlen(entry->uri) == uri_length && strncmp(entry->uri, uri, uri_length) == 0);
        if (!uri_matches) {
            continue;
        }
        if (entry->method == method) {
            if (out_handler != NULL) {
                *out_handler = entry->handler;
            }
            return FAKE_HTTPD_ROUTE_FOUND;
        }
        method_mismatch_found = true;
    }
    return method_mismatch_found ? FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED : FAKE_HTTPD_ROUTE_NOT_FOUND;
}
