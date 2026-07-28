#!/usr/bin/env python3
"""Apply the Phase 16 HTTP integration edits with fail-closed source assertions."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def replace_once(relative: str, old: str, new: str) -> None:
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{relative}: expected one match, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def append_once(relative: str, marker: str, addition: str) -> None:
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    if marker in text:
        raise SystemExit(f"{relative}: marker already present")
    path.write_text(text.rstrip() + "\n\n" + addition.strip() + "\n", encoding="utf-8")


replace_once(
    "firmware/components/web_server/include/web_server.h",
    "    bool login_enabled;\n    auth_password_record_t password_record;\n",
    "    bool login_enabled;\n"
    "    auth_password_record_t password_record;\n"
    "    bool require_physical_confirmation;\n",
)

replace_once(
    "firmware/components/app_core/app_core_sequence.c",
    "        .mode = WEB_SERVER_MODE_NORMAL,\n"
    "        .login_enabled = true,\n"
    "        .password_record = provisioning->password_record,\n",
    "        .mode = WEB_SERVER_MODE_NORMAL,\n"
    "        .login_enabled = true,\n"
    "        .password_record = provisioning->password_record,\n"
    "        .require_physical_confirmation =\n"
    "            provisioning->require_physical_confirmation,\n",
)

replace_once(
    "firmware/components/web_server/web_server_internal.h",
    "app_error_code_t read_bounded_body(httpd_req_t *request, char *buffer, size_t buffer_size,\n"
    "                                   size_t maximum_length);\n"
    "app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token);\n",
    "app_error_code_t read_bounded_body(httpd_req_t *request, char *buffer, size_t buffer_size,\n"
    "                                   size_t maximum_length);\n"
    "app_error_code_t web_server_get_header(httpd_req_t *request, const char *name, char *buffer,\n"
    "                                       size_t buffer_size);\n"
    "app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token);\n",
)
replace_once(
    "firmware/components/web_server/web_server_internal.h",
    "esp_err_t cancel_handler(httpd_req_t *request);\n",
    "esp_err_t cancel_handler(httpd_req_t *request);\n"
    "esp_err_t api_handler(httpd_req_t *request);\n",
)

replace_once(
    "firmware/components/web_server/web_server_common.c",
    "static app_error_code_t get_header_adapter(void *context, const char *name, char *buffer,\n"
    "                                           size_t buffer_size) {\n"
    "    httpd_req_t *request = context;\n",
    "app_error_code_t web_server_get_header(httpd_req_t *request, const char *name, char *buffer,\n"
    "                                       size_t buffer_size) {\n",
)
replace_once(
    "firmware/components/web_server/web_server_common.c",
    "    return web_adapter_authorize_mutation(get_header_adapter, validate_session_adapter, request,\n",
    "    return web_adapter_authorize_mutation((web_adapter_get_header_fn)web_server_get_header,\n"
    "                                          validate_session_adapter, request,\n",
)

replace_once(
    "firmware/components/web_server/web_server_lifecycle.c",
    "#define WEB_MAX_URI_HANDLERS 24U\n",
    "#define WEB_MAX_URI_HANDLERS 28U\n",
)
replace_once(
    "firmware/components/web_server/web_server_lifecycle.c",
    "    {.uri = \"/api/v1/executions/current/cancel\", .method = HTTP_POST, .handler = cancel_handler},\n"
    "    {.uri = \"/*\", .method = HTTP_GET, .handler = static_handler},\n",
    "    {.uri = \"/api/v1/executions/current/cancel\", .method = HTTP_POST, .handler = cancel_handler},\n"
    "    {.uri = \"/api/v1/*\", .method = HTTP_GET, .handler = api_handler},\n"
    "    {.uri = \"/api/v1/*\", .method = HTTP_POST, .handler = api_handler},\n"
    "    {.uri = \"/api/v1/*\", .method = HTTP_PUT, .handler = api_handler},\n"
    "    {.uri = \"/api/v1/*\", .method = HTTP_DELETE, .handler = api_handler},\n"
    "    {.uri = \"/*\", .method = HTTP_GET, .handler = static_handler},\n",
)

replace_once(
    "firmware/components/web_server/CMakeLists.txt",
    "    \"web_server_lifecycle.c\"\n",
    "    \"web_server_lifecycle.c\"\n"
    "    \"web_server_api.c\"\n",
)
replace_once(
    "firmware/components/web_server/CMakeLists.txt",
    "    \"web_api_core.c\"\n"
    "    \"web_request_policy.c\"\n"
    "    \"web_execution_submit.c\"\n",
    "    \"web_api_core.c\"\n"
    "    \"web_api_response.c\"\n"
    "    \"web_api_json.c\"\n"
    "    \"web_api_handler_common.c\"\n"
    "    \"web_api_sets.c\"\n"
    "    \"web_api_macros.c\"\n"
    "    \"web_api_procedures.c\"\n"
    "    \"web_api_execution.c\"\n"
    "    \"web_api_administration.c\"\n"
    "    \"web_api_dispatch.c\"\n"
    "    \"web_request_policy.c\"\n"
    "    \"web_execution_submit.c\"\n",
)

replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    case WEB_API_ROUTE_SET_PROCEDURE:\n"
    "    case WEB_API_ROUTE_PROCEDURE_PROGRESS:\n"
    "    case WEB_API_ROUTE_SETTINGS:\n"
    "        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT ||\n"
    "               method == WEB_API_METHOD_DELETE;\n",
    "    case WEB_API_ROUTE_SET_PROCEDURE:\n"
    "    case WEB_API_ROUTE_PROCEDURE_PROGRESS:\n"
    "        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT ||\n"
    "               method == WEB_API_METHOD_DELETE;\n"
    "    case WEB_API_ROUTE_SETTINGS:\n"
    "        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT;\n",
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||\n",
    "    return route == WEB_API_ROUTE_EXECUTIONS || route == WEB_API_ROUTE_EXECUTION_CONFIRM ||\n"
    "           route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||\n",
)

replace_once(
    "firmware/components/macro_executor/macro_executor_engine.c",
    "    engine->status.state = EXECUTION_IDLE;\n",
    "    engine->status.state = EXECUTION_IDLE;\n"
    "    engine->status.available = true;\n",
)
replace_once(
    "firmware/components/macro_executor/macro_executor_engine.c",
    "    if (engine == NULL) {\n"
    "        return APP_ERROR_INVALID_ARGUMENT;\n"
    "    }\n"
    "    app_error_code_t result = lock_engine(engine);\n",
    "    if (engine == NULL) {\n"
    "        return APP_ERROR_INVALID_ARGUMENT;\n"
    "    }\n"
    "    if (engine->unavailable) {\n"
    "        return APP_ERROR_STORAGE_UNAVAILABLE;\n"
    "    }\n"
    "    app_error_code_t result = lock_engine(engine);\n",
)
replace_once(
    "firmware/components/macro_executor/macro_executor_engine.c",
    "    if (!engine->busy) {\n"
    "        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_NOT_FOUND : APP_ERROR_INTERNAL;\n"
    "    }\n"
    "    engine->cancellation_requested = true;\n",
    "    if (!engine->busy) {\n"
    "        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_NOT_FOUND : APP_ERROR_INTERNAL;\n"
    "    }\n"
    "    if (engine->cancellation_requested) {\n"
    "        return unlock_engine(engine) == APP_ERROR_NONE ? APP_ERROR_CONFLICT : APP_ERROR_INTERNAL;\n"
    "    }\n"
    "    engine->cancellation_requested = true;\n",
)
replace_once(
    "firmware/components/macro_executor/macro_executor_engine.c",
    "    result = engine->status;\n"
    "    if (unlock_engine(engine) != APP_ERROR_NONE) {\n",
    "    result = engine->status;\n"
    "    result.available = !engine->unavailable;\n"
    "    result.cancellation_requested = engine->cancellation_requested;\n"
    "    if (unlock_engine(engine) != APP_ERROR_NONE) {\n",
)
replace_once(
    "firmware/components/macro_executor/macro_executor_engine.c",
    "        .release_error = APP_ERROR_NONE,\n"
    "        .execution_id = request->execution_id,\n"
    "        .action_index = 0U,\n",
    "        .release_error = APP_ERROR_NONE,\n"
    "        .execution_id = request->execution_id,\n"
    "        .set_id = request->set_id,\n"
    "        .macro_id = request->macro_id,\n"
    "        .macro_revision = request->macro_revision,\n"
    "        .available = true,\n"
    "        .action_index = 0U,\n",
)

replace_once(
    "firmware/components/web_server/web_api_handler_common.c",
    "    cJSON *item = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);\n"
    "    cJSON_free(json);\n"
    "    if (item == NULL || parse_end == NULL || (size_t)(parse_end - json) != length) {\n",
    "    cJSON *item = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);\n"
    "    const bool complete =\n"
    "        item != NULL && parse_end != NULL && (size_t)(parse_end - json) == length;\n"
    "    cJSON_free(json);\n"
    "    if (!complete) {\n",
)

replace_once(
    "firmware/components/web_server/web_api_execution.c",
    "#include \"web_http_status.h\"\n",
    "#include \"web_http_status.h\"\n"
    "#include \"web_server_internal.h\"\n",
)

replace_once(
    "firmware/components/web_server/web_server_api.c",
    "#include <string.h>\n",
    "#include <string.h>\n"
    "#include <sys/types.h>\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "    char *body = NULL;\n\n"
    "    app_error_code_t result = method_from_request(request, &call.method);\n",
    "    char *body = NULL;\n"
    "    bool response_ready = false;\n\n"
    "    app_error_code_t result = method_from_request(request, &call.method);\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "        result = set_error_response(&response, status, result,\n"
    "                                    status == WEB_HTTP_STATUS_NOT_FOUND ? \"route not found\"\n"
    "                                                                         : \"invalid API path\");\n"
    "    } else if (!web_api_route_allows_method(call.path.route, call.method)) {\n"
    "        result = set_error_response(&response, WEB_HTTP_STATUS_METHOD_NOT_ALLOWED,\n"
    "                                    APP_ERROR_INVALID_ARGUMENT, \"method not allowed\");\n"
    "    }\n\n"
    "    const size_t body_limit = route_body_limit(call.path.route);\n"
    "    if (result == APP_ERROR_NONE &&\n",
    "        result = set_error_response(&response, status, result,\n"
    "                                    status == WEB_HTTP_STATUS_NOT_FOUND ? \"route not found\"\n"
    "                                                                         : \"invalid API path\");\n"
    "        response_ready = result == APP_ERROR_NONE;\n"
    "    } else if (!web_api_route_allows_method(call.path.route, call.method)) {\n"
    "        result = set_error_response(&response, WEB_HTTP_STATUS_METHOD_NOT_ALLOWED,\n"
    "                                    APP_ERROR_INVALID_ARGUMENT, \"method not allowed\");\n"
    "        response_ready = result == APP_ERROR_NONE;\n"
    "    }\n\n"
    "    const size_t body_limit = route_body_limit(call.path.route);\n"
    "    if (!response_ready && result == APP_ERROR_NONE &&\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "    if (result == APP_ERROR_NONE) {\n"
    "        const app_error_code_t policy_result =\n",
    "    if (!response_ready && result == APP_ERROR_NONE) {\n"
    "        const app_error_code_t policy_result =\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "            result = set_error_response(&response,\n"
    "                                        web_request_policy_http_status(policy_failure, policy_result),\n"
    "                                        policy_result, policy_failure_message(policy_failure));\n"
    "        }\n"
    "    }\n"
    "    if (result == APP_ERROR_NONE) {\n",
    "            result = set_error_response(&response,\n"
    "                                        web_request_policy_http_status(policy_failure, policy_result),\n"
    "                                        policy_result, policy_failure_message(policy_failure));\n"
    "            response_ready = result == APP_ERROR_NONE;\n"
    "        }\n"
    "    }\n"
    "    if (!response_ready && result == APP_ERROR_NONE) {\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "            result = set_error_response(&response, web_api_http_status_for_error(result), result,\n"
    "                                        \"could not read request body\");\n"
    "        }\n"
    "    }\n"
    "    if (result == APP_ERROR_NONE) {\n",
    "            result = set_error_response(&response, web_api_http_status_for_error(result), result,\n"
    "                                        \"could not read request body\");\n"
    "            response_ready = result == APP_ERROR_NONE;\n"
    "        }\n"
    "    }\n"
    "    if (!response_ready && result == APP_ERROR_NONE) {\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "        if (result != APP_ERROR_NONE && response.body == NULL) {\n"
    "            result = set_error_response(&response, web_api_http_status_for_error(result), result,\n"
    "                                        \"API operation failed\");\n"
    "        }\n"
    "    }\n\n"
    "    const bool should_restart = response.body != NULL && restart_after_response(&call, &response);\n",
    "        if (result != APP_ERROR_NONE && response.body == NULL) {\n"
    "            result = set_error_response(&response, web_api_http_status_for_error(result), result,\n"
    "                                        \"API operation failed\");\n"
    "        }\n"
    "        response_ready = result == APP_ERROR_NONE && response.body != NULL;\n"
    "    }\n"
    "    if (!response_ready) {\n"
    "        web_api_response_free(&response);\n"
    "        (void)set_error_response(&response, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR,\n"
    "                                 APP_ERROR_INTERNAL, \"response encoding failed\");\n"
    "    }\n\n"
    "    const bool should_restart = response.body != NULL && restart_after_response(&call, &response);\n",
)

append_once(
    "tests/host/CMakeLists.txt",
    "# Phase 16 strict JSON and response envelope tests.",
    r'''
# Phase 16 strict JSON and response envelope tests.
add_executable(
    web_api_json_tests
    test_web_api_json.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_api_json.c
)
target_include_directories(
    web_api_json_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include
            ../../firmware/components/storage/include
            ../../firmware/components/web_server
)
target_link_libraries(web_api_json_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_json_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_json COMMAND web_api_json_tests)
set_tests_properties(web_api_json PROPERTIES LABELS "web")

add_executable(
    web_api_response_tests
    test_web_api_response.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_api_response.c
)
target_include_directories(
    web_api_response_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/web_server
)
target_link_libraries(web_api_response_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_response_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_response COMMAND web_api_response_tests)
set_tests_properties(web_api_response PROPERTIES LABELS "web")
''',
)
