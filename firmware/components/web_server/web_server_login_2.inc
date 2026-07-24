    }
    cJSON *root = cJSON_ParseWithLength(body, strlen(body));
    if (root == NULL) {
        memset(body, 0, sizeof(body));
        return send_error(request,
                          "400 Bad Request",
                          APP_ERROR_INVALID_ARGUMENT,
                          "invalid JSON");
    }
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    const bool valid_password = cJSON_IsString(password) && password->valuestring != NULL &&
                                auth_password_verify(password->valuestring,
                                                     strlen(password->valuestring),
                                                     &server_configuration.password_record);
    cJSON_Delete(root);
    memset(body, 0, sizeof(body));
    if (!valid_password) {
        const app_error_code_t failure_result = auth_login_record_failure();
        if (failure_result != APP_ERROR_NONE) {
            return send_error(request,
                              "500 Internal Server Error",
                              failure_result,
                              "could not record login failure");
        }
        return send_error(request,
                          "401 Unauthorized",
                          APP_ERROR_AUTH_FAILED,
                          "invalid credentials");
    }
    const app_error_code_t success_result = auth_login_record_success();
    if (success_result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          success_result,
                          "could not reset login throttle");
    }
    auth_session_view_t session = {0};
