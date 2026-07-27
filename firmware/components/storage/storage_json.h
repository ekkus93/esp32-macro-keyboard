#ifndef STORAGE_JSON_H
#define STORAGE_JSON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"

app_error_code_t storage_json_parse_object_fields(const char *data, size_t length,
                                                  const char *const *field_names,
                                                  size_t field_count, size_t required_count,
                                                  cJSON **out_root);
app_error_code_t storage_json_parse_exact_object(const char *data, size_t length,
                                                 const char *const *field_names, size_t field_count,
                                                 cJSON **out_root);
app_error_code_t storage_json_get_string(const cJSON *object, const char *name, char *destination,
                                         size_t destination_size, bool require_nonempty);
app_error_code_t storage_json_get_allocated_string(const cJSON *object, const char *name,
                                                   size_t maximum, bool require_nonempty,
                                                   char **out_value, size_t *out_length);
app_error_code_t storage_json_get_u32(const cJSON *object, const char *name, uint32_t minimum,
                                      uint32_t maximum, uint32_t *out_value);
app_error_code_t storage_json_get_i32(const cJSON *object, const char *name, int32_t *out_value);
app_error_code_t storage_json_get_bool(const cJSON *object, const char *name, bool *out_value);
app_error_code_t storage_json_get_uuid(const cJSON *object, const char *name,
                                       app_uuid_t *out_value);
bool storage_json_has_field(const cJSON *object, const char *name);

#endif
