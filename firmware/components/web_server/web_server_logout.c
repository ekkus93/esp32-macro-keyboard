#include "web_server_internal.h"

#include <string.h>

#include "app_error.h"
#include "auth.h"

esp_err_t logout_handler(httpd_req_t *request) {
    char session_token[AUTH_TOKEN_HEX_BYTES];
    const app_error_code_t authorization = authorize_mutation(request, session_token);
    if (authorization != APP_ERROR_NONE) {
        memset(session_token, 0, sizeof(session_token));
        return send_error(request, "401 Unauthorized", authorization, "authentication required");
    }
    const app_error_code_t result = auth_session_logout(session_token);
    memset(session_token, 0, sizeof(session_token));
    if (result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", result, "logout failed");
    }
    /* SPEC_V2 13.5: logout returns 204 No Content with no response body. */
    if (httpd_resp_set_hdr(request, "Set-Cookie",
                           SESSION_COOKIE_NAME
                           "=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0") != ESP_OK ||
        httpd_resp_set_status(request, "204 No Content") != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send(request, NULL, 0);
}
