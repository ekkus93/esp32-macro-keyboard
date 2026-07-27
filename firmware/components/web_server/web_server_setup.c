#include "web_server_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "device_controls.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "provisioning.h"
#include "web_origin.h"
#include "web_setup_core.h"
#include "web_setup_json.h"

#define SETUP_BODY_MAX_BYTES 512U
#define SETUP_RESPONSE_MAX_BYTES 256U
#define SETUP_CONTENT_TYPE_BYTES 64U
#define SETUP_RESTART_DELAY_US 500000ULL

web_setup_core_t server_setup_core;
static esp_timer_handle_t setup_restart_timer;

static void secure_zero_bytes(void *memory, size_t size) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

static app_error_code_t setup_provisioning_load(void *context,
                                                provisioning_config_t *out_configuration) {
    (void)context;
    return provisioning_load(out_configuration);
}

static app_error_code_t setup_password_create(void *context, const char *password,
                                              size_t password_length,
                                              auth_password_record_t *out_record) {
    (void)context;
    return auth_password_create(password, password_length, out_record);
}

static app_error_code_t setup_provisioning_commit(void *context,
                                                  const provisioning_config_t *replacement,
                                                  uint32_t expected_revision,
                                                  provisioning_config_t *out_committed) {
    (void)context;
    return provisioning_commit(replacement, expected_revision, out_committed);
}

static app_error_code_t setup_wait_for_confirmation(void *context, uint32_t timeout_ms) {
    (void)context;
    return device_controls_wait_for_confirmation(timeout_ms);
}

static void setup_restart_callback(void *argument) {
    (void)argument;
    esp_restart();
}

static app_error_code_t setup_schedule_restart(void *context) {
    (void)context;
    if (setup_restart_timer != NULL) {
        return APP_ERROR_CONFLICT;
    }
    const esp_timer_create_args_t arguments = {
        .callback = setup_restart_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "setup_restart",
        .skip_unhandled_events = true,
    };
    if (esp_timer_create(&arguments, &setup_restart_timer) != ESP_OK) {
        setup_restart_timer = NULL;
        return APP_ERROR_INTERNAL;
    }
    if (esp_timer_start_once(setup_restart_timer, SETUP_RESTART_DELAY_US) != ESP_OK) {
        const esp_err_t delete_result = esp_timer_delete(setup_restart_timer);
        if (delete_result == ESP_OK) {
            setup_restart_timer = NULL;
            return APP_ERROR_INTERNAL;
        }
        return APP_ERROR_IO;
    }
    return APP_ERROR_NONE;
}

static void setup_secure_zero(void *context, void *memory, size_t size) {
    (void)context;
    secure_zero_bytes(memory, size);
}

static web_setup_ops_t setup_operations(void) {
    return (web_setup_ops_t){
        .context = NULL,
        .provisioning_load = setup_provisioning_load,
        .password_create = setup_password_create,
        .provisioning_commit = setup_provisioning_commit,
        .wait_for_confirmation = setup_wait_for_confirmation,
        .schedule_restart = setup_schedule_restart,
        .secure_zero = setup_secure_zero,
    };
}

app_error_code_t web_server_setup_init(const web_server_config_t *configuration) {
    if (configuration == NULL || configuration->mode != WEB_SERVER_MODE_SETUP ||
        web_server_setup_owns_resources()) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    web_setup_configuration_t setup_configuration = {
        .physical_confirmation_required = configuration->setup_physical_confirmation_required,
        .manufacturing_bypass = configuration->setup_manufacturing_bypass,
    };
    memcpy(setup_configuration.device_id, configuration->setup_device_id,
           sizeof(setup_configuration.device_id));
    memcpy(setup_configuration.ap_ssid, configuration->setup_ap_ssid,
           sizeof(setup_configuration.ap_ssid));
    memcpy(setup_configuration.setup_code, configuration->setup_code,
           sizeof(setup_configuration.setup_code));
    const web_setup_ops_t operations = setup_operations();
    const app_error_code_t result =
        web_setup_core_init(&server_setup_core, &operations, &setup_configuration);
    secure_zero_bytes(&setup_configuration, sizeof(setup_configuration));
    return result;
}

app_error_code_t web_server_setup_deinit(void) {
    app_error_code_t result = APP_ERROR_NONE;
    if (setup_restart_timer != NULL) {
        if (esp_timer_is_active(setup_restart_timer) &&
            esp_timer_stop(setup_restart_timer) != ESP_OK) {
            result = APP_ERROR_IO;
        }
        if (esp_timer_delete(setup_restart_timer) == ESP_OK) {
            setup_restart_timer = NULL;
        } else if (result == APP_ERROR_NONE) {
            result = APP_ERROR_IO;
        }
    }
    const app_error_code_t core_result = web_setup_core_deinit(&server_setup_core);
    return result != APP_ERROR_NONE ? result : core_result;
}

bool web_server_setup_owns_resources(void) {
    return server_setup_core.initialized || setup_restart_timer != NULL;
}

static bool get_header(httpd_req_t *request, const char *name, char *output, size_t output_size) {
    if (request == NULL || name == NULL || output == NULL || output_size == 0U) {
        return false;
    }
    const size_t length = httpd_req_get_hdr_value_len(request, name);
    return length > 0U && length < output_size &&
           httpd_req_get_hdr_value_str(request, name, output, output_size) == ESP_OK;
}

static bool setup_origin_allowed(httpd_req_t *request) {
    char host[HTTP_HEADER_MAX_BYTES];
    char origin[HTTP_HEADER_MAX_BYTES];
    return get_header(request, "Host", host, sizeof(host)) &&
           get_header(request, "Origin", origin, sizeof(origin)) &&
           web_origin_matches_host(origin, host);
}

static bool setup_json_content_type(httpd_req_t *request) {
    char content_type[SETUP_CONTENT_TYPE_BYTES];
    if (!get_header(request, "Content-Type", content_type, sizeof(content_type))) {
        return false;
    }
    return strcmp(content_type, "application/json") == 0 ||
           strcmp(content_type, "application/json; charset=utf-8") == 0;
}

static const char *setup_http_status(app_error_code_t result) {
    switch (result) {
    case APP_ERROR_INVALID_ARGUMENT:
        return "400 Bad Request";
    case APP_ERROR_AUTH_FAILED:
    case APP_ERROR_AUTH_REQUIRED:
        return "401 Unauthorized";
    case APP_ERROR_CONFLICT:
        return "409 Conflict";
    case APP_ERROR_TIMEOUT:
        return "408 Request Timeout";
    case APP_ERROR_STORAGE_FULL:
        return "507 Insufficient Storage";
    case APP_ERROR_STORAGE_UNAVAILABLE:
    case APP_ERROR_STORAGE_CORRUPT:
    case APP_ERROR_IO:
    case APP_ERROR_INTERNAL:
    default:
        return "500 Internal Server Error";
    }
}

static esp_err_t send_setup_state(httpd_req_t *request, const web_setup_state_t *state,
                                  const char *status) {
    char response[SETUP_RESPONSE_MAX_BYTES];
    const int length =
        snprintf(response, sizeof(response),
                 "{\"ok\":true,\"data\":{\"deviceId\":\"%s\",\"apSsid\":\"%s\","
                 "\"completed\":%s,\"physicalConfirmationRequired\":%s}}",
                 state->device_id, state->ap_ssid, state->completed ? "true" : "false",
                 state->physical_confirmation_required ? "true" : "false");
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response overflow");
    }
    return send_json(request, response, status);
}

esp_err_t setup_state_handler(httpd_req_t *request) {
    const web_setup_state_t state = web_setup_core_get_state(&server_setup_core);
    if (!server_setup_core.initialized) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_CONFLICT,
                          "setup service unavailable");
    }
    return send_setup_state(request, &state, "200 OK");
}

esp_err_t setup_credentials_handler(httpd_req_t *request) {
    if (!server_setup_core.initialized) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_CONFLICT,
                          "setup service unavailable");
    }
    if (!setup_origin_allowed(request)) {
        return send_error(request, "401 Unauthorized", APP_ERROR_AUTH_REQUIRED,
                          "invalid setup origin");
    }
    if (!setup_json_content_type(request)) {
        return send_error(request, "415 Unsupported Media Type", APP_ERROR_INVALID_ARGUMENT,
                          "content type must be application/json");
    }

    char body[SETUP_BODY_MAX_BYTES + 1U];
    const app_error_code_t body_result =
        read_bounded_body(request, body, sizeof(body), SETUP_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        secure_zero_bytes(body, sizeof(body));
        return send_error(request, "400 Bad Request", body_result, "invalid setup request body");
    }

    web_setup_submission_t submission = {0};
    const web_setup_json_ops_t json_operations = {
        .context = NULL,
        .secure_zero = setup_secure_zero,
    };
    const app_error_code_t parse_result =
        web_setup_json_parse(body, sizeof(body), &json_operations, &submission);
    if (parse_result != APP_ERROR_NONE) {
        return send_error(request, "400 Bad Request", parse_result, "invalid setup request");
    }

    provisioning_config_t committed = {0};
    const app_error_code_t result =
        web_setup_core_submit(&server_setup_core, &submission, &committed);
    secure_zero_bytes(&committed, sizeof(committed));
    if (result != APP_ERROR_NONE) {
        return send_error(request, setup_http_status(result), result,
                          "setup could not be completed");
    }
    const web_setup_state_t state = web_setup_core_get_state(&server_setup_core);
    return send_setup_state(request, &state, "201 Created");
}

esp_err_t setup_complete_handler(httpd_req_t *request) {
    if (!server_setup_core.initialized) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_CONFLICT,
                          "setup service unavailable");
    }
    if (!setup_origin_allowed(request)) {
        return send_error(request, "401 Unauthorized", APP_ERROR_AUTH_REQUIRED,
                          "invalid setup origin");
    }
    const web_setup_state_t state = web_setup_core_get_state(&server_setup_core);
    if (!state.completed) {
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT,
                          "setup credentials have not been committed");
    }
    return send_setup_state(request, &state, "200 OK");
}

esp_err_t setup_restart_handler(httpd_req_t *request) {
    if (!server_setup_core.initialized) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_CONFLICT,
                          "setup service unavailable");
    }
    if (!setup_origin_allowed(request)) {
        return send_error(request, "401 Unauthorized", APP_ERROR_AUTH_REQUIRED,
                          "invalid setup origin");
    }
    const app_error_code_t result = web_setup_core_restart(&server_setup_core);
    if (result != APP_ERROR_NONE) {
        return send_error(request, setup_http_status(result), result,
                          "restart could not be scheduled");
    }
    return send_json(request, "{\"ok\":true,\"data\":{\"restartScheduled\":true}}", "202 Accepted");
}
