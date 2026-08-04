#ifndef WEB_API_HANDLER_COMMON_H
#define WEB_API_HANDLER_COMMON_H

#include "app_error.h"
#include "provisioning.h"
#include "web_api_response.h"

app_error_code_t web_api_handler_error(web_api_response_t *response, app_error_code_t error,
                                       const char *message, const char *details_json);
app_error_code_t web_api_handler_success_json(web_api_response_t *response, unsigned int status,
                                              const char *data_json);
app_error_code_t web_api_handler_session_json(char **out_json);
app_error_code_t web_api_handler_settings_json(const provisioning_settings_t *settings,
                                               char **out_json);
void web_api_handler_json_free(char *json);

#endif
