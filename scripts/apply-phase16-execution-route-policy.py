#!/usr/bin/env python3
"""Remove the no-op confirmation route and wire pure execution route policy."""

from pathlib import Path
import re


def replace_once(path_text: str, old: str, new: str) -> None:
    path = Path(path_text)
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path_text}: expected one match, found {count}: {old!r}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def regex_once(path_text: str, pattern: str, replacement: str) -> None:
    path = Path(path_text)
    text = path.read_text(encoding="utf-8")
    text, count = re.subn(pattern, replacement, text, count=1, flags=re.DOTALL)
    if count != 1:
        raise SystemExit(f"{path_text}: regex matched {count}: {pattern!r}")
    path.write_text(text, encoding="utf-8")


replace_once(
    "firmware/components/web_server/web_api_core.h",
    "    WEB_API_ROUTE_EXECUTION_CONFIRM,\n",
    "",
)
replace_once(
    "firmware/components/web_server/web_api_core.h",
    "bool web_api_route_requires_physical_confirmation(web_api_route_t route);\n",
    "bool web_api_route_requires_physical_confirmation(web_api_route_t route);\n"
    "bool web_api_physical_confirmation_required(web_api_route_t route,\n"
    "                                            bool execution_confirmation_enabled);\n",
)

replace_once(
    "firmware/components/web_server/web_api_core.c",
    '''    if (segments->count == 3U && text_equal(segments->items[2], "cancel")) {
        out_path->route = WEB_API_ROUTE_EXECUTION_CANCEL;
    } else if (segments->count == 3U && text_equal(segments->items[2], "confirm")) {
        out_path->route = WEB_API_ROUTE_EXECUTION_CONFIRM;
    } else {
        return APP_ERROR_NOT_FOUND;
    }
''',
    '''    if (segments->count == 3U && text_equal(segments->items[2], "cancel")) {
        out_path->route = WEB_API_ROUTE_EXECUTION_CANCEL;
    } else {
        return APP_ERROR_NOT_FOUND;
    }
''',
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    case WEB_API_ROUTE_EXECUTION_CONFIRM:\n",
    "",
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    '''bool web_api_route_requires_physical_confirmation(web_api_route_t route) {
    return route == WEB_API_ROUTE_EXECUTIONS || route == WEB_API_ROUTE_EXECUTION_CONFIRM ||
           route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
           route == WEB_API_ROUTE_DEVICE_RESTART || route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_RESTORE;
}
''',
    '''bool web_api_physical_confirmation_required(web_api_route_t route,
                                            bool execution_confirmation_enabled) {
    if (route == WEB_API_ROUTE_EXECUTIONS) {
        return execution_confirmation_enabled;
    }
    return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
           route == WEB_API_ROUTE_DEVICE_RESTART || route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_RESTORE;
}

bool web_api_route_requires_physical_confirmation(web_api_route_t route) {
    return web_api_physical_confirmation_required(route, true);
}
''',
)

replace_once(
    "firmware/components/web_server/web_api_dispatch.c",
    "    case WEB_API_ROUTE_EXECUTION_CONFIRM:\n",
    "",
)

replace_once(
    "firmware/components/web_server/web_api_execution.c",
    '#include "web_execution_submit.h"\n',
    '#include "web_execution_route_policy.h"\n#include "web_execution_submit.h"\n',
)
regex_once(
    "firmware/components/web_server/web_api_execution.c",
    r"static bool execution_matches_path\(.*?\n\}\n\nstatic bool execution_terminal\(.*?\n\}\n\n",
    "",
)
regex_once(
    "firmware/components/web_server/web_api_execution.c",
    r"static app_error_code_t handle_cancel\(.*?\n\}\n\nstatic app_error_code_t handle_confirm\(.*?\n\}\n",
    '''static app_error_code_t handle_cancel(const web_api_call_t *call, web_api_response_t *response) {
    const macro_execution_status_t status = macro_executor_get_status();
    web_execution_cancel_policy_t policy = {0};
    app_error_code_t result =
        web_execution_cancel_policy_evaluate(&status, &call->path, &policy);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!policy.permitted) {
        return explicit_error(response, policy.status, policy.error, policy.message);
    }
    result = macro_executor_cancel();
    const unsigned int http_status = web_api_cancel_http_status(&status, result);
    if (result != APP_ERROR_NONE) {
        return explicit_error(response, http_status, result, "cancellation request failed");
    }
    return web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED,
                                        "{\\"cancelRequested\\":true}");
}
''',
)
replace_once(
    "firmware/components/web_server/web_api_execution.c",
    '''    case WEB_API_ROUTE_EXECUTION_CONFIRM:
        return handle_confirm(call, response);
''',
    "",
)

regex_once(
    "firmware/components/web_server/web_server_api.c",
    r"static app_error_code_t policy_confirm\(void \*context\) \{.*?\n\}\n\nstatic app_error_code_t method_from_request",
    '''static app_error_code_t policy_confirm(void *context) {
    web_http_request_context_t *request_context = context;
    if (request_context == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!web_api_physical_confirmation_required(
            request_context->route, server_configuration.require_physical_confirmation)) {
        return APP_ERROR_NONE;
    }
    return device_controls_wait_for_confirmation(APP_PHYSICAL_CONFIRM_TIMEOUT_MS);
}

static app_error_code_t method_from_request''',
)

replace_once(
    "firmware/components/web_server/CMakeLists.txt",
    '    "web_execution_submit.c"\n',
    '    "web_execution_submit.c"\n    "web_execution_route_policy.c"\n',
)

cmake_path = Path("tests/host/CMakeLists.txt")
cmake_text = cmake_path.read_text(encoding="utf-8")
if "web_execution_route_policy_tests" in cmake_text:
    raise SystemExit("web_execution_route_policy_tests already registered")
marker = 'set_tests_properties(web_execution_submit PROPERTIES LABELS "web")\n'
if cmake_text.count(marker) != 1:
    raise SystemExit("web execution submit registration marker changed")
block = r'''

add_executable(
    web_execution_route_policy_tests
    test_web_execution_route_policy.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_execution_route_policy.c
)
target_include_directories(
    web_execution_route_policy_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/web_server
)
target_link_libraries(web_execution_route_policy_tests PRIVATE test_support)
target_compile_options(web_execution_route_policy_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_execution_route_policy COMMAND web_execution_route_policy_tests)
set_tests_properties(web_execution_route_policy PROPERTIES LABELS "web")
'''
cmake_path.write_text(cmake_text.replace(marker, marker + block, 1), encoding="utf-8")

replace_once(
    "tests/host/test_web_request_policy.c",
    "    {WEB_API_ROUTE_EXECUTION_CONFIRM, WEB_API_METHOD_POST},\n",
    "",
)
replace_once(
    "tests/host/test_web_api_dispatch.c",
    "    {WEB_API_ROUTE_EXECUTION_CONFIRM, HANDLER_EXECUTION},\n",
    "",
)
replace_once(
    "tests/host/test_web_api_core.c",
    '''    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/%2fetc", &path));
''',
    '''    TEST_CHECK_APP_ERROR(
        APP_ERROR_NOT_FOUND,
        web_api_parse_path("/api/v1/executions/" EXECUTION_ID "/confirm", &path));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_parse_path("/api/v1/sets/%2fetc", &path));
''',
)
replace_once(
    "tests/host/test_web_api_core.c",
    '''    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_EXECUTIONS));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_MACRO));
''',
    '''    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_EXECUTIONS));
    TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, true));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_EXECUTIONS, false));
    TEST_CHECK(web_api_physical_confirmation_required(WEB_API_ROUTE_DEVICE_FACTORY_RESET, false));
    TEST_CHECK(!web_api_physical_confirmation_required(WEB_API_ROUTE_SET_MACRO, true));
    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_MACRO));
''',
)
print("Phase 16 execution route policy applied")
