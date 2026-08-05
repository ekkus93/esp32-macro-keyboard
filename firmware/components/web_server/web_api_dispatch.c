#include "web_api_handlers.h"

#include <stddef.h>

#include "app_error.h"
#include "web_api_core.h"
#include "web_api_response.h"

app_error_code_t web_api_dispatch(const web_api_call_t *call, web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (call->path.route == WEB_API_ROUTE_UNKNOWN) {
        return APP_ERROR_NOT_FOUND;
    }
    return web_api_handle_administration(call, response);
}
