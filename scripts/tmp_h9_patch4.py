from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one anchor, found {count}: {old[:100]!r}")
    target.write_text(text.replace(old, new, 1))


# Host send-route target now compiles the real settings-backed confirmation adapter.
replace_once(
    "tests/host/CMakeLists.txt",
    "            ../../firmware/components/auth/include\n            ../../firmware/components/storage/include\n            ../../firmware/components/wifi_ap/include\n",
    "            ../../firmware/components/auth/include\n            ../../firmware/components/device_settings/include\n            ../../firmware/components/storage/include\n            ../../firmware/components/wifi_ap/include\n",
)

replace_once(
    "tests/host/test_web_server_send_route.c",
    '#include "cJSON.h"\n#include "fake_httpd.h"\n',
    '#include "cJSON.h"\n#include "device_settings.h"\n#include "fake_httpd.h"\n',
)
replace_once(
    "tests/host/test_web_server_send_route.c",
    'static app_error_code_t g_submit_result;\n',
    '''static app_error_code_t g_settings_read_result;\nstatic bool g_require_serial_confirmation;\n\napp_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {\n    if (out_settings == NULL) {\n        return APP_ERROR_INVALID_ARGUMENT;\n    }\n    if (g_settings_read_result != APP_ERROR_NONE) {\n        return g_settings_read_result;\n    }\n    *out_settings = (app_v2_device_settings_t){\n        .require_serial_confirmation = g_require_serial_confirmation,\n    };\n    return APP_ERROR_NONE;\n}\n\nstatic app_error_code_t g_submit_result;\n''',
)
replace_once(
    "tests/host/test_web_server_send_route.c",
    '    g_submit_result = APP_ERROR_NONE;\n',
    '    g_settings_read_result = APP_ERROR_NONE;\n    g_require_serial_confirmation = false;\n    g_submit_result = APP_ERROR_NONE;\n',
)
replace_once(
    "tests/host/test_web_server_send_route.c",
    '    TEST_CHECK_EQ_U64(15U, g_last_submitted_request.inter_key_ms);\n\n    cJSON *root = parse_response(&fake);\n',
    '    TEST_CHECK_EQ_U64(15U, g_last_submitted_request.inter_key_ms);\n    TEST_CHECK(!g_last_submitted_request.require_confirmation);\n\n    cJSON *root = parse_response(&fake);\n',
)
insert_anchor = '''static void test_send_create_valid_matches_example(void) {\n'''
insert = '''static void test_send_create_binds_authoritative_confirmation_setting(void) {\n    reset_fakes();\n    g_require_serial_confirmation = true;\n    fake_httpd_request_t fake;\n    httpd_req_t request;\n    fake_httpd_reset(&fake);\n    authenticate(&fake);\n    bind_json_body(&request, &fake,\n                   "{\\\"source\\\":\\\"first\\\",\\\"keyPressMs\\\":8,\\\"interKeyMs\\\":15}");\n\n    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));\n    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);\n    TEST_CHECK(g_submit_called);\n    TEST_CHECK(g_last_submitted_request.require_confirmation);\n}\n\nstatic void test_send_create_confirmation_policy_read_failure_fails_closed(void) {\n    reset_fakes();\n    g_settings_read_result = APP_ERROR_STORAGE_UNAVAILABLE;\n    fake_httpd_request_t fake;\n    httpd_req_t request;\n    fake_httpd_reset(&fake);\n    authenticate(&fake);\n    bind_json_body(&request, &fake,\n                   "{\\\"source\\\":\\\"first\\\",\\\"keyPressMs\\\":8,\\\"interKeyMs\\\":15}");\n\n    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));\n    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);\n    TEST_CHECK(!g_submit_called);\n    cJSON *root = parse_response(&fake);\n    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");\n    TEST_CHECK_EQ_STRING("storage_unavailable",\n                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);\n    TEST_CHECK_EQ_STRING("send backend unavailable",\n                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);\n    cJSON_Delete(root);\n}\n\nstatic void test_send_create_valid_matches_example(void) {\n'''
replace_once("tests/host/test_web_server_send_route.c", insert_anchor, insert)
replace_once(
    "tests/host/test_web_server_send_route.c",
    '    test_send_create_valid();\n    test_send_create_valid_matches_example();\n',
    '    test_send_create_valid();\n    test_send_create_binds_authoritative_confirmation_setting();\n    test_send_create_confirmation_policy_read_failure_fails_closed();\n    test_send_create_valid_matches_example();\n',
)

# H9-091: test failures must not echo compared secret strings into CI logs.
replace_once(
    "tests/host/support/test_assert.h",
    '            test_fail_string(__FILE__, __LINE__, #actual_value " == " #expected_value,             \\\n                             test_expected_, test_actual_);                                        \\\n',
    '            test_fail_string(__FILE__, __LINE__, "string equality", test_expected_, test_actual_); \\\n',
)
replace_once(
    "tests/host/support/test_assert.h",
    '            test_fail_buffer(__FILE__, __LINE__, #actual_value " == " #expected_value,             \\\n                             test_expected_, test_actual_, test_length_);                          \\\n',
    '            test_fail_buffer(__FILE__, __LINE__, "buffer equality", test_expected_,               \\\n                             test_actual_, test_length_);                                          \\\n',
)
replace_once(
    "tests/host/support/test_assert.c",
    '''void test_fail_string(const char *file, int line, const char *expression, const char *expected,\n                      const char *actual) {\n    (void)fprintf(stderr, "test failure at %s:%d: %s; expected=\\\"%s\\\", actual=\\\"%s\\\"\\n",\n                  safe_string(file), line, safe_string(expression), safe_string(expected),\n                  safe_string(actual));\n    exit(EXIT_FAILURE);\n}\n''',
    '''void test_fail_string(const char *file, int line, const char *expression, const char *expected,\n                      const char *actual) {\n    (void)expected;\n    (void)actual;\n    (void)fprintf(stderr, "test failure at %s:%d: %s; string values differ\\n", safe_string(file),\n                  line, safe_string(expression));\n    exit(EXIT_FAILURE);\n}\n''',
)

# Strengthen credential-output scanner: sensitive variable identifiers are forbidden even
# behind a generic "%s" format, not only when the literal itself says "password" etc.
checker = Path("scripts/check-credential-logging.sh")
text = checker.read_text()
replace_once(
    "scripts/check-credential-logging.sh",
    '''SENSITIVE_WORD = re.compile(\n    r"(?:password|passphrase|setup[_ -]?code|(?:session|csrf|api|access)[_ -]?token|salt|verifier)",\n    re.IGNORECASE,\n)\nFORMAT_VALUE = re.compile(r"%(?:\\.\\*)?[a-zA-Z]")\n''',
    '''SENSITIVE_WORD = re.compile(\n    r"(?:password|passphrase|setup[_ -]?code|(?:session|csrf|api|access)[_ -]?token|salt|verifier)",\n    re.IGNORECASE,\n)\nSENSITIVE_IDENTIFIER = re.compile(\n    r"(?:password|passphrase|setup[_]?code|(?:session|csrf|api|access)[_]?token|salt|verifier)",\n    re.IGNORECASE,\n)\nFORMAT_VALUE = re.compile(r"%(?:\\.\\*)?[a-zA-Z]")\n''',
)
replace_once(
    "scripts/check-credential-logging.sh",
    '''            if SENSITIVE_WORD.search(message) and FORMAT_VALUE.search(message):\n                fail(f"{path}: credential-bearing output is forbidden")\n''',
    '''            if FORMAT_VALUE.search(message) and (\n                SENSITIVE_WORD.search(message) or SENSITIVE_IDENTIFIER.search(match.group(0))\n            ):\n                fail(f"{path}: credential-bearing output is forbidden")\n''',
)
replace_once(
    "tests/scripts/test-check-credential-logging.sh",
    '''write_valid_fixture\nprintf '%s\\n' 'CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG' \\\n''',
    '''write_valid_fixture\ncat >"${temporary_dir}/firmware/components/generic_leak.c" <<'SOURCE'\nvoid leak_setup(const char *setup_code) {\n    ESP_LOGW(TAG, "%s", setup_code);\n}\nSOURCE\nexpect_fail 'generic format sensitive identifier' 'credential-bearing output is forbidden'\n\nwrite_valid_fixture\nprintf '%s\\n' 'CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG' \\\n''',
)

# No production V2 code may print to the browser console; visible UI error handling is the
# supported failure surface, and this closes the H9 browser-console secret-output class.
replace_once(
    "webapp/tests/v2-browser-storage-prohibition.test.tsx",
    'const forbiddenApiPattern =\n  /\\blocalStorage\\b|\\bsessionStorage\\b|\\bindexedDB\\b|\\bcaches\\.\\w|\\bserviceWorker\\b|\\bopenDatabase\\b/;\n',
    'const forbiddenApiPattern =\n  /\\blocalStorage\\b|\\bsessionStorage\\b|\\bindexedDB\\b|\\bcaches\\.\\w|\\bserviceWorker\\b|\\bopenDatabase\\b/;\nconst browserConsolePattern = /\\bconsole\\.(?:log|info|warn|error|debug)\\s*\\(/;\n',
)
replace_once(
    "webapp/tests/v2-browser-storage-prohibition.test.tsx",
    '''  test("no file under src/v2 or src/features/auth/v2 references localStorage, sessionStorage, IndexedDB, Cache Storage, or service workers", () => {\n    const entries = Object.entries(v2SourceModules);\n    expect(entries.length).toBeGreaterThan(0);\n    const offenders = entries\n      .map(([path, source]) => ({ path, code: stripComments(source) }))\n      .filter(({ code }) => forbiddenApiPattern.test(code));\n    expect(offenders.map((offender) => offender.path)).toEqual([]);\n  });\n''',
    '''  test("no production V2 source references browser persistence APIs", () => {\n    const entries = Object.entries(v2SourceModules);\n    expect(entries.length).toBeGreaterThan(0);\n    const offenders = entries\n      .map(([path, source]) => ({ path, code: stripComments(source) }))\n      .filter(({ code }) => forbiddenApiPattern.test(code));\n    expect(offenders.map((offender) => offender.path)).toEqual([]);\n  });\n\n  test("no production V2 source writes application or secret state to the browser console", () => {\n    const offenders = Object.entries(v2SourceModules)\n      .map(([path, source]) => ({ path, code: stripComments(source) }))\n      .filter(({ code }) => browserConsolePattern.test(code));\n    expect(offenders.map((offender) => offender.path)).toEqual([]);\n  });\n''',
)

# Permanent H9 guard also protects redacted host-test failure output.
replace_once(
    "scripts/check-h9-architecture.py",
    'print("H9 architecture guard passed")\n',
    '''test_assert = read("tests/host/support/test_assert.c")\ntest_assert_h = read("tests/host/support/test_assert.h")\nif "expected=\\\"%s\\\"" in test_assert or "actual=\\\"%s\\\"" in test_assert:\n    fail("host test assertion failures can print compared string values")\nif '#actual_value " == " #expected_value' in test_assert_h:\n    fail("host string/buffer assertions can stringify secret-bearing macro arguments")\n\nprint("H9 architecture guard passed")\n''',
)
