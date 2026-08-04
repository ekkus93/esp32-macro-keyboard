#ifndef WEB_API_JSON_H
#define WEB_API_JSON_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "provisioning.h"

app_error_code_t web_api_json_parse_expected_revision(const char *body, size_t body_length,
                                                      uint32_t *out_expected_revision);
app_error_code_t web_api_json_parse_settings_update(const char *body, size_t body_length,
                                                    provisioning_settings_t *out_settings,
                                                    uint32_t *out_expected_revision);

#endif
