#ifndef WEB_API_ADMIN_BOUNDARY_H
#define WEB_API_ADMIN_BOUNDARY_H

#include "app_error.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

app_error_code_t web_api_admin_boundary_handle(const web_api_call_t *call,
                                               web_api_response_t *response);

#endif
