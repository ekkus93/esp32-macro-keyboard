#ifndef WEB_EXECUTION_ROUTE_POLICY_H
#define WEB_EXECUTION_ROUTE_POLICY_H

#include <stdbool.h>

#include "app_error.h"
#include "macro_executor.h"
#include "web_api_core.h"

typedef struct {
    bool permitted;
    unsigned int status;
    app_error_code_t error;
    const char *message;
} web_execution_cancel_policy_t;

app_error_code_t
web_execution_cancel_policy_evaluate(const macro_execution_status_t *execution_status,
                                     const web_api_path_t *request_path,
                                     web_execution_cancel_policy_t *out_policy);

#endif
