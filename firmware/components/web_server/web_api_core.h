#ifndef WEB_API_CORE_H
#define WEB_API_CORE_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"

#define WEB_API_URI_MAX_BYTES 256U
#define WEB_API_REQUEST_ID_MAX_BYTES 64U

typedef enum {
    WEB_API_METHOD_GET = 0,
    WEB_API_METHOD_POST,
    WEB_API_METHOD_PUT,
    WEB_API_METHOD_DELETE,
} web_api_method_t;

typedef enum {
    WEB_API_ROUTE_UNKNOWN = 0,
    WEB_API_ROUTE_AUTH_SESSION,
    WEB_API_ROUTE_SETS,
    WEB_API_ROUTE_SETS_ORDER,
    WEB_API_ROUTE_SET,
    WEB_API_ROUTE_SET_DUPLICATE,
    WEB_API_ROUTE_SET_SELECT,
    WEB_API_ROUTE_SET_EXPORT,
    WEB_API_ROUTE_SET_IMPORT,
    WEB_API_ROUTE_SET_MACROS,
    WEB_API_ROUTE_SET_MACRO,
    WEB_API_ROUTE_SET_MACRO_VALIDATE,
    WEB_API_ROUTE_SET_MACRO_DUPLICATE,
    WEB_API_ROUTE_SET_MACROS_REORDER,
    WEB_API_ROUTE_GLOBAL_MACROS,
    WEB_API_ROUTE_GLOBAL_MACRO,
    WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE,
    WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE,
    WEB_API_ROUTE_GLOBAL_MACROS_REORDER,
    WEB_API_ROUTE_SET_PROCEDURES,
    WEB_API_ROUTE_SET_PROCEDURE,
    WEB_API_ROUTE_SET_PROCEDURES_REORDER,
    WEB_API_ROUTE_PROCEDURE_PROGRESS,
    WEB_API_ROUTE_PROGRESS_COMPLETE,
    WEB_API_ROUTE_PROGRESS_SKIP,
    WEB_API_ROUTE_EXECUTIONS,
    WEB_API_ROUTE_EXECUTION_CURRENT,
    WEB_API_ROUTE_EXECUTION_CANCEL,
    WEB_API_ROUTE_SETTINGS,
    WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD,
    WEB_API_ROUTE_DEVICE_RESTART,
    WEB_API_ROUTE_DEVICE_RESET_SETTINGS,
    WEB_API_ROUTE_DEVICE_FACTORY_RESET,
    WEB_API_ROUTE_DIAGNOSTICS_STORAGE,
    WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK,
    WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE,
    WEB_API_ROUTE_BACKUP,
    WEB_API_ROUTE_RESTORE,
} web_api_route_t;

typedef struct {
    web_api_route_t route;
    bool has_set_id;
    app_uuid_t set_id;
    bool has_macro_id;
    app_uuid_t macro_id;
    bool has_procedure_id;
    app_uuid_t procedure_id;
    bool has_execution_id;
    app_uuid_t execution_id;
} web_api_path_t;

bool web_api_content_type_is_json(const char *content_type);
bool web_api_request_id_is_valid(const char *request_id);
app_error_code_t web_api_parse_path(const char *uri, web_api_path_t *out_path);
bool web_api_route_allows_method(web_api_route_t route, web_api_method_t method);
bool web_api_route_requires_body(web_api_route_t route, web_api_method_t method);
bool web_api_route_requires_session(web_api_route_t route);
bool web_api_route_requires_csrf(web_api_route_t route, web_api_method_t method);
bool web_api_route_requires_physical_confirmation(web_api_route_t route);
bool web_api_physical_confirmation_required(web_api_route_t route,
                                            bool execution_confirmation_enabled);
unsigned int web_api_http_status_for_error(app_error_code_t error);
unsigned int web_api_cancel_http_status(const macro_execution_status_t *status,
                                        app_error_code_t cancel_result);

#endif
