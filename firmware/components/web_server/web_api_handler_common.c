#include "web_api_handler_common.h"

#include "app_error.h"
#include "cJSON.h"
#include "web_api_core.h"
#include "web_api_response.h"

app_error_code_t web_api_handler_error(web_api_response_t *response, app_error_code_t error,
                                       const char *message, const char *field) {
    return web_api_response_error(response, &(web_api_error_spec_t){
                                                .status = web_api_http_status_for_error(error),
                                                .code = error,
                                                .message = message,
                                                .field = field,
                                            });
}

app_error_code_t web_api_handler_success_json(web_api_response_t *response, unsigned int status,
                                              const char *data_json) {
    return web_api_response_success(response, status, data_json);
}

app_error_code_t web_api_handler_no_content(web_api_response_t *response, unsigned int status) {
    return web_api_response_no_content(response, status);
}

void web_api_handler_json_free(char *json) {
    cJSON_free(json);
}
