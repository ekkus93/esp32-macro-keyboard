#!/usr/bin/env python3
"""Apply Phase 16 request-policy corrections with fail-closed source assertions."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def write(relative: str, text: str) -> None:
    (ROOT / relative).write_text(text, encoding="utf-8")


def replace_once(relative: str, old: str, new: str) -> None:
    text = read(relative)
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{relative}: expected one match, found {count}: {old[:100]!r}")
    write(relative, text.replace(old, new, 1))


def replace_regex_once(relative: str, pattern: str, replacement: str) -> None:
    text = read(relative)
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.DOTALL)
    if count != 1:
        raise SystemExit(f"{relative}: expected one regex match, found {count}: {pattern[:100]!r}")
    write(relative, updated)


# Expose a session-cookie-only validation path for safe/read-only requests.
replace_once(
    "firmware/components/auth/include/auth.h",
    "app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token);\n",
    "app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token);\n"
    "app_error_code_t auth_session_validate_read_only(const char *session_token);\n",
)
replace_once(
    "firmware/components/auth/auth_core.h",
    "app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,\n"
    "                                             const char *csrf_token);\n",
    "app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,\n"
    "                                             const char *csrf_token);\n"
    "app_error_code_t auth_core_session_validate_read_only(auth_core_t *core,\n"
    "                                                       const char *session_token);\n",
)
replace_once(
    "firmware/components/auth/auth.c",
    "app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token) {\n"
    "    return auth_core_session_validate(&auth_core, session_token, csrf_token);\n"
    "}\n",
    "app_error_code_t auth_session_validate(const char *session_token, const char *csrf_token) {\n"
    "    return auth_core_session_validate(&auth_core, session_token, csrf_token);\n"
    "}\n\n"
    "app_error_code_t auth_session_validate_read_only(const char *session_token) {\n"
    "    return auth_core_session_validate_read_only(&auth_core, session_token);\n"
    "}\n",
)
replace_once(
    "firmware/components/auth/auth_core_session.c",
    "#include <stdint.h>\n",
    "#include <stdbool.h>\n#include <stdint.h>\n",
)
replace_regex_once(
    "firmware/components/auth/auth_core_session.c",
    r"app_error_code_t auth_core_session_validate\(auth_core_t \*core, const char \*session_token,.*?\n\}\n\napp_error_code_t auth_core_session_logout",
    """static app_error_code_t validate_session(auth_core_t *core, const char *session_token,
                                               const char *csrf_token, bool require_csrf) {
    if (core == NULL || !auth_core_valid_hex_token(session_token) ||
        (require_csrf && !auth_core_valid_hex_token(csrf_token))) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    app_error_code_t result = auth_core_lock(core);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    auth_core_state_snapshot_t snapshot;
    auth_core_snapshot_state(core, &snapshot);
    uint64_t now = 0U;
    result = auth_core_read_now(core, &now);
    if (result == APP_ERROR_NONE) {
        result = APP_ERROR_AUTH_REQUIRED;
        for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
            auth_session_entry_t *entry = &core->sessions[index];
            if (!entry->active) {
                continue;
            }
            if (entry->view.expires_at_us <= now) {
                memset(entry, 0, sizeof(*entry));
                continue;
            }
            const bool session_matches = auth_core_constant_time_equal(
                (const uint8_t *)entry->view.session_token, (const uint8_t *)session_token,
                AUTH_TOKEN_HEX_BYTES - 1U);
            const bool csrf_matches =
                !require_csrf ||
                auth_core_constant_time_equal((const uint8_t *)entry->view.csrf_token,
                                              (const uint8_t *)csrf_token,
                                              AUTH_TOKEN_HEX_BYTES - 1U);
            if (session_matches && csrf_matches) {
                if (UINT64_MAX - now < AUTH_CORE_SESSION_IDLE_US) {
                    result = APP_ERROR_INTERNAL;
                } else {
                    entry->view.expires_at_us = now + AUTH_CORE_SESSION_IDLE_US;
                    result = APP_ERROR_NONE;
                }
                break;
            }
        }
    }
    if (auth_core_unlock(core) != APP_ERROR_NONE) {
        auth_core_restore_state(core, &snapshot);
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t auth_core_session_validate(auth_core_t *core, const char *session_token,
                                             const char *csrf_token) {
    return validate_session(core, session_token, csrf_token, true);
}

app_error_code_t auth_core_session_validate_read_only(auth_core_t *core,
                                                       const char *session_token) {
    return validate_session(core, session_token, NULL, false);
}

app_error_code_t auth_core_session_logout""",
)

# GET requests validate only the session cookie; mutations still require CSRF.
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "    (void)context;\n"
    "    return auth_session_validate(session_token, csrf_token);\n",
    "    (void)context;\n"
    "    return csrf_token == NULL ? auth_session_validate_read_only(session_token)\n"
    "                              : auth_session_validate(session_token, csrf_token);\n",
)
replace_regex_once(
    "firmware/components/web_server/web_request_policy.c",
    r"    char csrf\[AUTH_TOKEN_HEX_BYTES\] = \{0\};\n    if \(read_required_header\(operations, \"X-CSRF-Token\", csrf, sizeof\(csrf\)\) != APP_ERROR_NONE\) \{.*?\n    const app_error_code_t validation =\n        operations->validate_session\(operations->context, out_result->session_token, csrf\);",
    """    char csrf[AUTH_TOKEN_HEX_BYTES] = {0};
    const char *csrf_token = NULL;
    if (web_api_route_requires_csrf(input->route, input->method)) {
        if (read_required_header(operations, "X-CSRF-Token", csrf, sizeof(csrf)) !=
            APP_ERROR_NONE) {
            return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_CSRF,
                        APP_ERROR_AUTH_REQUIRED);
        }
        csrf_token = csrf;
    }
    const app_error_code_t validation = operations->validate_session(
        operations->context, out_result->session_token, csrf_token);""",
)

# Restrict methods accurately and ensure execution submission enters confirmation policy.
replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    case WEB_API_ROUTE_PROCEDURE_PROGRESS:\n"
    "    case WEB_API_ROUTE_SETTINGS:\n"
    "        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT ||\n"
    "               method == WEB_API_METHOD_DELETE;\n",
    "    case WEB_API_ROUTE_PROCEDURE_PROGRESS:\n"
    "        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT ||\n"
    "               method == WEB_API_METHOD_DELETE;\n"
    "    case WEB_API_ROUTE_SETTINGS:\n"
    "        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT;\n",
)
replace_once(
    "firmware/components/web_server/web_api_core.c",
    "    return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||\n",
    "    return route == WEB_API_ROUTE_EXECUTIONS ||\n"
    "           route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||\n",
)

# Core route regression coverage.
replace_once(
    "tests/host/test_web_api_core.c",
    "    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));\n"
    "    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_MACRO));\n",
    "    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));\n"
    "    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_EXECUTIONS));\n"
    "    TEST_CHECK(!web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_MACRO));\n"
    "    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET));\n"
    "    TEST_CHECK(web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT));\n"
    "    TEST_CHECK(!web_api_route_allows_method(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_DELETE));\n",
)

# Request-policy regression coverage records nullable CSRF behavior.
replace_once(
    "tests/host/test_web_request_policy.c",
    "    size_t validation_calls;\n"
    "    size_t confirmation_calls;\n",
    "    size_t validation_calls;\n"
    "    size_t confirmation_calls;\n"
    "    bool saw_null_csrf;\n",
)
replace_once(
    "tests/host/test_web_request_policy.c",
    "    if (csrf_token != NULL) {\n"
    "        TEST_CHECK_EQ_STRING(TOKEN, csrf_token);\n"
    "    }\n"
    "    ++fixture->validation_calls;\n",
    "    fixture->saw_null_csrf = csrf_token == NULL;\n"
    "    if (csrf_token != NULL) {\n"
    "        TEST_CHECK_EQ_STRING(TOKEN, csrf_token);\n"
    "    }\n"
    "    ++fixture->validation_calls;\n",
)
replace_once(
    "tests/host/test_web_request_policy.c",
    "static void test_failure_statuses(void) {\n",
    "static void test_get_does_not_require_csrf(void) {\n"
    "    fixture_t fixture = {\n"
    "        .missing = \"X-CSRF-Token\",\n"
    "        .validation_result = APP_ERROR_NONE,\n"
    "    };\n"
    "    const web_request_policy_ops_t ops = operations(&fixture);\n"
    "    const web_request_policy_input_t policy = input(WEB_API_ROUTE_SETS, WEB_API_METHOD_GET);\n"
    "    web_request_policy_result_t result = {0};\n"
    "    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;\n"
    "    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,\n"
    "                         web_request_policy_evaluate(&policy, &ops, &result, &failure));\n"
    "    TEST_CHECK(fixture.saw_null_csrf);\n"
    "    TEST_CHECK_EQ_U64(1U, fixture.validation_calls);\n"
    "}\n\n"
    "static void test_failure_statuses(void) {\n",
)
replace_once(
    "tests/host/test_web_request_policy.c",
    "    fixture = (fixture_t){\n"
    "        .content_type = \"application/json\",\n"
    "        .origin = \"http://192.168.4.1\",\n"
    "        .validation_result = APP_ERROR_AUTH_FAILED,\n"
    "    };\n",
    "    fixture = (fixture_t){\n"
    "        .content_type = \"application/json\",\n"
    "        .origin = \"http://192.168.4.1\",\n"
    "        .missing = \"X-CSRF-Token\",\n"
    "    };\n"
    "    ops = operations(&fixture);\n"
    "    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,\n"
    "                         web_request_policy_evaluate(&policy, &ops, &result, &failure));\n"
    "    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_CSRF, failure);\n\n"
    "    fixture = (fixture_t){\n"
    "        .content_type = \"application/json\",\n"
    "        .origin = \"http://192.168.4.1\",\n"
    "        .validation_result = APP_ERROR_AUTH_FAILED,\n"
    "    };\n",
)
replace_once(
    "tests/host/test_web_request_policy.c",
    "    test_success_and_generated_request_id();\n"
    "    test_failure_statuses();\n",
    "    test_success_and_generated_request_id();\n"
    "    test_get_does_not_require_csrf();\n"
    "    test_failure_statuses();\n",
)

# Authentication-core coverage for read-only validation and token rejection.
replace_once(
    "tests/host/auth_existing_tests.inc",
    "    TEST_CHECK(auth_core_session_validate(&core,\n"
    "                                           session.session_token,\n"
    "                                           session.csrf_token) == APP_ERROR_NONE);\n\n"
    "    fake.now_us += 1000000U;\n",
    "    TEST_CHECK(auth_core_session_validate(&core,\n"
    "                                           session.session_token,\n"
    "                                           session.csrf_token) == APP_ERROR_NONE);\n"
    "    TEST_CHECK(auth_core_session_validate_read_only(&core, session.session_token) ==\n"
    "               APP_ERROR_NONE);\n\n"
    "    fake.now_us += 1000000U;\n",
)
replace_once(
    "tests/host/auth_existing_tests.inc",
    "    TEST_CHECK(auth_core_session_validate(&core, \"short\", wrong) == APP_ERROR_AUTH_REQUIRED);\n",
    "    TEST_CHECK(auth_core_session_validate(&core, \"short\", wrong) == APP_ERROR_AUTH_REQUIRED);\n"
    "    TEST_CHECK(auth_core_session_validate_read_only(&core, wrong) == APP_ERROR_AUTH_REQUIRED);\n"
    "    TEST_CHECK(auth_core_session_validate_read_only(NULL, session.session_token) ==\n"
    "               APP_ERROR_AUTH_REQUIRED);\n",
)

print("Phase 16 request-policy hardening applied")
