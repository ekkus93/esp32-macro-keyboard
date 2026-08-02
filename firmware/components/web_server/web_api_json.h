#ifndef WEB_API_JSON_H
#define WEB_API_JSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "provisioning.h"
#include "storage_object_json.h"
#include "web_execution_submit.h"

typedef struct {
    uint32_t expected_revision;
    char *resource_json;
    size_t resource_length;
} web_api_resource_mutation_t;

typedef struct {
    size_t body_length;
    size_t maximum_resource_length;
} web_api_resource_parse_limits_t;

typedef struct {
    size_t body_length;
    size_t maximum_count;
} web_api_order_parse_limits_t;

app_error_code_t web_api_json_parse_expected_revision(const char *body, size_t body_length,
                                                      uint32_t *out_expected_revision);
app_error_code_t web_api_json_parse_resource_mutation(const char *body,
                                                      const web_api_resource_parse_limits_t *limits,
                                                      web_api_resource_mutation_t *out_mutation);
app_error_code_t web_api_json_parse_set_resource(const char *body, size_t body_length,
                                                 macro_set_t *out_set);
app_error_code_t web_api_json_parse_macro_resource(const char *body, size_t body_length,
                                                   macro_t *out_macro);
void web_api_json_free_resource_mutation(web_api_resource_mutation_t *mutation);
app_error_code_t web_api_json_parse_uuid_order(const char *body,
                                               const web_api_order_parse_limits_t *limits,
                                               storage_uuid_order_t *out_order);
app_error_code_t web_api_json_parse_execution_submit(const char *body, size_t body_length,
                                                     web_execution_submit_request_t *out_request);
app_error_code_t web_api_json_parse_settings_update(const char *body, size_t body_length,
                                                    provisioning_settings_t *out_settings,
                                                    uint32_t *out_expected_revision);

#endif
