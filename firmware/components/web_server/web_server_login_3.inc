    const app_error_code_t session_result = auth_session_create(&session);
    if (session_result != APP_ERROR_NONE) {
        return send_error(request,
                          session_result == APP_ERROR_CONFLICT ? "503 Service Unavailable"
                                                               : "500 Internal Server Error",
                          session_result,
                          "could not create session");
    }
    char cookie[160U];
    const int cookie_length = snprintf(cookie,
                                       sizeof(cookie),
                                       SESSION_COOKIE_NAME "=%s; HttpOnly; SameSite=Strict; Path=/",
                                       session.session_token);
    if (cookie_length < 0 || (size_t)cookie_length >= sizeof(cookie) ||
        httpd_resp_set_hdr(request, "Set-Cookie", cookie) != ESP_OK) {
        const app_error_code_t logout_result = auth_session_logout(session.session_token);
        return send_error(request,
                          "500 Internal Server Error",
                          logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                          "could not create login response");
    }
    char response[192U];
    const int response_length = snprintf(response,
                                         sizeof(response),
                                         "{\"ok\":true,\"data\":{\"csrfToken\":\"%s\"}}",
                                         session.csrf_token);
    if (response_length < 0 || (size_t)response_length >= sizeof(response)) {
        const app_error_code_t logout_result = auth_session_logout(session.session_token);
        return send_error(request,
                          "500 Internal Server Error",
                          logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

