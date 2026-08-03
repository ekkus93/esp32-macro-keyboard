#ifndef WEB_SERVER_INTERNAL_H
#define WEB_SERVER_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "esp_http_server.h"
#include "macro_executor.h"
#include "usb_keyboard.h"
#include "web_api_response.h"
#include "web_server.h"
#include "web_server_adapter.h"
#include "web_setup_core.h"
#include "wifi_ap.h"

/* Component-private declarations shared across the web_server translation units.
 * These were file-scope statics in the former source amalgamation; splitting the
 * amalgamation into normal .c units requires them to have external linkage, so
 * they are declared here (private to the component) rather than exposed publicly. */

#define HTTP_HEADER_MAX_BYTES 256U
#define LOGIN_BODY_MAX_BYTES 256U
#define SESSION_COOKIE_NAME "MKSESSION"

extern web_server_config_t server_configuration;
extern web_adapter_lifecycle_t server_lifecycle;
extern web_setup_core_t server_setup_core;

const char *usb_state_string(usb_keyboard_state_t state);
const char *wifi_state_string(wifi_ap_state_t state);
const char *execution_state_string(execution_state_t state);
esp_err_t send_json(httpd_req_t *request, const char *json, const char *status);
esp_err_t send_error(httpd_req_t *request, const char *status, app_error_code_t code,
                     const char *message);
app_error_code_t read_bounded_body(httpd_req_t *request, char *buffer, size_t buffer_size,
                                   size_t maximum_length);
app_error_code_t web_server_get_header(httpd_req_t *request, const char *name, char *buffer,
                                       size_t buffer_size);
app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token);

esp_err_t status_handler(httpd_req_t *request);
esp_err_t limits_handler(httpd_req_t *request);
esp_err_t login_handler(httpd_req_t *request);
esp_err_t logout_handler(httpd_req_t *request);
esp_err_t execution_handler(httpd_req_t *request);
esp_err_t cancel_handler(httpd_req_t *request);
esp_err_t api_handler(httpd_req_t *request);

/* Runs a parsed API call to completion and sends its response. Separated from
 * api_handler so the async worker can run the same path off the httpd task.
 * Sets *out_should_restart when the caller must esp_restart() after the
 * response has been sent (and, for async callers, after the request has been
 * marked complete). */
esp_err_t web_api_handle_call(httpd_req_t *request, bool *out_should_restart);

/* Handle a call whose body was already read, taking ownership of it.
 *
 * The async worker needs this. esp_http_server hands an async handler the
 * request but not its unread payload, so a body not read before
 * httpd_req_async_handler_begin() cannot be recovered afterwards -- restore and
 * import both reached their handlers with body_length 0 and answered 422.
 * Passing NULL reads the body here, which is what the ordinary path does. */
esp_err_t web_api_handle_call_with_body(httpd_req_t *request, char *preread_body,
                                        size_t preread_length, bool *out_should_restart);

/* Read a request body on the httpd task, bounded by the route's own limit.
 * Returns APP_ERROR_NOT_FOUND when the route does not parse, so the caller can
 * fall through to the normal path and let it produce the proper error. */
app_error_code_t web_api_read_route_body(httpd_req_t *request, char **out_body, size_t *out_length);

/* Sends a bare JSON error envelope. Used by the async layer, which has to
 * answer before a call is ever parsed. */
esp_err_t web_api_send_status_error(httpd_req_t *request, unsigned int status,
                                    app_error_code_t code, const char *message);

/* True when this request would block the handler in the physical-confirmation
 * wait, and so must not run on the httpd task. */
/* True when this request must be handed to the async worker rather than served
 * on the httpd task -- because it waits for physical confirmation, because its
 * own work is long enough to trip the task watchdog, or both (SPEC 13.5). */
bool web_api_request_requires_worker(httpd_req_t *request);

/* Async offload for confirmation-gated requests. web_server_async_dispatch
 * takes ownership of the request on success; the caller must return its result
 * to httpd without touching the request again. */
app_error_code_t web_server_async_start(void);
app_error_code_t web_server_async_stop(void);
esp_err_t web_server_async_dispatch(httpd_req_t *request);
esp_err_t static_handler(httpd_req_t *request);
esp_err_t setup_state_handler(httpd_req_t *request);
esp_err_t setup_credentials_handler(httpd_req_t *request);
esp_err_t setup_complete_handler(httpd_req_t *request);
esp_err_t setup_restart_handler(httpd_req_t *request);

app_error_code_t web_server_setup_init(const web_server_config_t *configuration);
app_error_code_t web_server_setup_deinit(void);
bool web_server_setup_owns_resources(void);

app_error_code_t web_diagnostics_handle(web_api_response_t *response);

#endif
