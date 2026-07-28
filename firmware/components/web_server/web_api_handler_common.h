#ifndef WEB_API_HANDLER_COMMON_H
#define WEB_API_HANDLER_COMMON_H

#include <stddef.h>

#include "app_error.h"
#include "macro_model.h"
#include "provisioning.h"
#include "storage.h"
#include "storage_repository.h"
#include "web_api_response.h"

app_error_code_t web_api_handler_error(web_api_response_t *response, app_error_code_t error,
                                       const char *message, const char *details_json);
app_error_code_t web_api_handler_success_json(web_api_response_t *response, unsigned int status,
                                              const char *data_json);
app_error_code_t web_api_handler_session_json(const char *csrf_token, char **out_json);
app_error_code_t web_api_handler_set_json(const macro_set_t *set, char **out_json);
app_error_code_t web_api_handler_set_list_json(const storage_set_list_t *list, char **out_json);
app_error_code_t web_api_handler_macro_json(const macro_t *macro, char **out_json);
app_error_code_t web_api_handler_macro_list_json(const storage_macro_list_t *list, char **out_json);
app_error_code_t web_api_handler_procedure_json(const procedure_t *procedure, char **out_json);
app_error_code_t web_api_handler_procedure_list_json(const storage_procedure_list_t *list,
                                                     char **out_json);
app_error_code_t web_api_handler_progress_json(const storage_progress_snapshot_t *snapshot,
                                               char **out_json);
app_error_code_t web_api_handler_settings_json(const provisioning_settings_t *settings,
                                               char **out_json);
app_error_code_t web_api_handler_quarantine_json(const storage_quarantine_list_t *list,
                                                 char **out_json);
void web_api_handler_json_free(char *json);

#endif
