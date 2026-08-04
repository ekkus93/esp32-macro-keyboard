#ifndef WEB_API_CORE_H
#define WEB_API_CORE_H

#include <stdbool.h>

#include "app_error.h"

typedef enum {
    WEB_API_METHOD_GET = 0,
    WEB_API_METHOD_POST,
    WEB_API_METHOD_PUT,
    WEB_API_METHOD_DELETE
} web_api_method_t;

typedef enum {
    WEB_API_ROUTE_UNKNOWN = 0,
    WEB_API_ROUTE_AUTH_SESSION,
    WEB_API_ROUTE_SETTINGS,
    WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD,
    WEB_API_ROUTE_DEVICE_RESTART,
    WEB_API_ROUTE_DEVICE_RESET_SETTINGS,
    WEB_API_ROUTE_DEVICE_FACTORY_RESET,
    WEB_API_ROUTE_DIAGNOSTICS_FULL
} web_api_route_t;

typedef struct {
    web_api_route_t route;
} web_api_path_t;

app_error_code_t web_api_parse_path(const char *uri, web_api_path_t *out_path);
bool web_api_route_allows_method(web_api_route_t route, web_api_method_t method);
bool web_api_route_requires_body(web_api_route_t route, web_api_method_t method);
bool web_api_route_requires_session(web_api_route_t route);
bool web_api_route_requires_physical_confirmation(web_api_route_t route);
bool web_api_physical_confirmation_required(web_api_route_t route, bool confirmation_enabled);
bool web_api_route_requires_worker(web_api_route_t route);
bool web_api_content_type_is_json(const char *content_type);
bool web_api_request_id_is_valid(const char *request_id);
unsigned int web_api_http_status_for_error(app_error_code_t error);

#endif
