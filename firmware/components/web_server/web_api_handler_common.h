#ifndef WEB_API_HANDLER_COMMON_H
#define WEB_API_HANDLER_COMMON_H

#include <stddef.h>

#include "app_error.h"
#include "provisioning.h"
#include "web_api_response.h"

/* `field` is the single offending request field, or NULL when none applies
 * (SPEC_V2 13.2). */
app_error_code_t web_api_handler_error(web_api_response_t *response, app_error_code_t error,
                                       const char *message, const char *field);
/* Macro-parser-error variant: always emits code "macro_parse_error" plus the
 * exact byteOffset/line/column triple (SPEC_V2 13.2). */
app_error_code_t web_api_handler_parser_error(web_api_response_t *response, const char *message,
                                              const char *field, size_t byte_offset, size_t line,
                                              size_t column);
app_error_code_t web_api_handler_success_json(web_api_response_t *response, unsigned int status,
                                              const char *data_json);
/* A body-less success response, e.g. the 204 SPEC_V2 13.9 requires for a
 * successful password change. */
app_error_code_t web_api_handler_no_content(web_api_response_t *response, unsigned int status);
app_error_code_t web_api_handler_settings_json(const provisioning_settings_t *settings,
                                               char **out_json);
void web_api_handler_json_free(char *json);

#endif
