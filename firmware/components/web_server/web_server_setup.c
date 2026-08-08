#include "web_server_internal.h"

#include <stddef.h>

#include "app_error.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "web_server.h"

static esp_err_t send_setup_state(httpd_req_t *request) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response allocation failed");
    }
    if (cJSON_AddBoolToObject(root, "provisioned", false) == NULL ||
        cJSON_AddStringToObject(root, "deviceName", server_configuration.setup_device_name) ==
            NULL) {
        cJSON_Delete(root);
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response allocation failed");
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response serialization failed");
    }
    const esp_err_t result = send_json(request, json, "200 OK");
    cJSON_free(json);
    return result;
}

esp_err_t setup_state_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    if (server_configuration.mode != WEB_SERVER_MODE_SETUP) {
        return send_error(request, "404 Not Found", APP_ERROR_NOT_FOUND, "route not found");
    }
    return send_setup_state(request);
}

esp_err_t setup_submit_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    if (server_configuration.mode != WEB_SERVER_MODE_SETUP) {
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT,
                          "device is already provisioned");
    }
    return send_error(request, "503 Service Unavailable", APP_ERROR_INTERNAL,
                      "V2 setup submission is not yet enabled");
}
