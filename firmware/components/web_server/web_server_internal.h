#ifndef WEB_SERVER_INTERNAL_H
#define WEB_SERVER_INTERNAL_H

#include <stddef.h>

#include "app_error.h"
#include "esp_http_server.h"
#include "macro_executor.h"
#include "usb_keyboard.h"
#include "web_server.h"
#include "web_server_adapter.h"
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

const char *usb_state_string(usb_keyboard_state_t state);
const char *wifi_state_string(wifi_ap_state_t state);
const char *execution_state_string(execution_state_t state);
esp_err_t send_json(httpd_req_t *request, const char *json, const char *status);
esp_err_t send_error(httpd_req_t *request, const char *status, app_error_code_t code,
                     const char *message);
app_error_code_t read_bounded_body(httpd_req_t *request, char *buffer, size_t buffer_size,
                                   size_t maximum_length);
app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token);

esp_err_t status_handler(httpd_req_t *request);
esp_err_t limits_handler(httpd_req_t *request);
esp_err_t login_handler(httpd_req_t *request);
esp_err_t logout_handler(httpd_req_t *request);
esp_err_t execution_handler(httpd_req_t *request);
esp_err_t cancel_handler(httpd_req_t *request);
esp_err_t static_handler(httpd_req_t *request);

#endif
