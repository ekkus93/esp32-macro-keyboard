#ifndef PROVISIONING_CORE_H
#define PROVISIONING_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "provisioning.h"

#define PROVISIONING_RECORD_BYTES 168U

typedef struct {
    void *context;
    app_error_code_t (*read_blob)(void *context, uint8_t *output, size_t capacity,
                                  size_t *out_size);
    app_error_code_t (*write_blob)(void *context, const uint8_t *data, size_t size);
    app_error_code_t (*erase_blob)(void *context);
    app_error_code_t (*commit)(void *context);
    void (*secure_zero)(void *context, void *memory, size_t size);
} provisioning_ops_t;

typedef struct {
    provisioning_ops_t operations;
    provisioning_config_t current;
    bool initialized;
    bool loaded;
    bool record_present;
} provisioning_core_t;

app_error_code_t provisioning_core_init(provisioning_core_t *core,
                                        const provisioning_ops_t *operations);
app_error_code_t provisioning_core_load(provisioning_core_t *core,
                                        provisioning_config_t *out_config);
app_error_code_t provisioning_core_commit(provisioning_core_t *core,
                                          const provisioning_config_t *replacement,
                                          uint32_t expected_revision,
                                          provisioning_config_t *out_committed);
app_error_code_t provisioning_core_settings_read(provisioning_core_t *core,
                                                 provisioning_settings_t *out_settings);
app_error_code_t provisioning_core_settings_update(provisioning_core_t *core,
                                                   const provisioning_settings_t *replacement,
                                                   uint32_t expected_revision,
                                                   provisioning_settings_t *out_committed);
app_error_code_t provisioning_core_clear_credentials(provisioning_core_t *core);
app_error_code_t provisioning_core_factory_reset(provisioning_core_t *core);
app_error_code_t provisioning_core_deinit(provisioning_core_t *core);
bool provisioning_config_is_valid(const provisioning_config_t *configuration);

#endif
