from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one anchor, found {count}: {old[:80]!r}")
    target.write_text(text.replace(old, new, 1))


# H9-091: the setup code is a manufacturing-label secret. Never print it to UART.
replace_once(
    "firmware/components/app_core/app_core.c",
    '    case APP_CORE_LOG_SETUP_CODE:\n        ESP_LOGW(TAG, "setup code: %s", event->setup_code);\n        break;\n',
    '    case APP_CORE_LOG_SETUP_CODE:\n        ESP_LOGI(TAG, "setup credentials are available from the manufacturing label");\n        break;\n',
)
replace_once(
    "firmware/components/app_core/app_core_sequence.c",
    'static void log_setup_code(const app_core_ops_t *operations, const char *setup_code) {\n',
    'static void log_setup_code(const app_core_ops_t *operations, const char *setup_code) {\n    (void)setup_code;\n',
)
replace_once(
    "firmware/components/app_core/app_core_sequence.c",
    '        .setup_code = setup_code,\n',
    '        .setup_code = NULL,\n',
)

# H9-090: if async-worker startup fails and httpd_stop also fails, preserve the live
# server handle so lifecycle cleanup/retry can still own it instead of creating a ghost server.
replace_once(
    "firmware/components/web_server/web_server_lifecycle.c",
    '    if (web_server_async_start() != APP_ERROR_NONE) {\n        (void)httpd_stop(handle);\n        return -1;\n    }\n',
    '    if (web_server_async_start() != APP_ERROR_NONE) {\n        if (httpd_stop(handle) == ESP_OK) {\n            return -1;\n        }\n        *out_handle = handle;\n        return -1;\n    }\n',
)
replace_once(
    "firmware/components/web_server/web_server_adapter_lifecycle.c",
    '    void *handle = NULL;\n    if (ops->start(ops->context, &handle) != 0 || handle == NULL) {\n        return APP_ERROR_INTERNAL;\n    }\n    lifecycle->handle = handle;\n',
    '    void *handle = NULL;\n    const int start_result = ops->start(ops->context, &handle);\n    if (start_result != 0) {\n        if (handle == NULL) {\n            return APP_ERROR_INTERNAL;\n        }\n        lifecycle->handle = handle;\n        lifecycle->registered_routes = 0U;\n        lifecycle->cleanup_error = APP_ERROR_IO;\n        return APP_ERROR_IO;\n    }\n    if (handle == NULL) {\n        return APP_ERROR_INTERNAL;\n    }\n    lifecycle->handle = handle;\n',
)

# Regression seam: allow a start operation to fail after creating a live handle.
replace_once(
    "tests/host/test_web_server_adapter_stream_lifecycle.inc",
    '    int start_result;\n    size_t failed_route;\n',
    '    int start_result;\n    bool handle_on_start_failure;\n    size_t failed_route;\n',
)
replace_once(
    "tests/host/test_web_server_adapter_stream_lifecycle.inc",
    '    if (fake->start_result != 0) {\n        return -1;\n    }\n    *out_handle = &fake->handle;\n',
    '    if (fake->start_result != 0) {\n        if (fake->handle_on_start_failure) {\n            *out_handle = &fake->handle;\n        }\n        return -1;\n    }\n    *out_handle = &fake->handle;\n',
)
anchor = '''    TEST_CHECK_EQ_U64(0U, fake.stop_calls);\n\n    /* Registration failure plus a successful stop: fully cleaned up. */\n'''
insert = '''    TEST_CHECK_EQ_U64(0U, fake.stop_calls);\n\n    /* Start failure after the adapter created a live handle: ownership must be\n     * retained so cleanup is retryable and another start is rejected. */\n    fake = (lifecycle_fake_t){\n        .start_result = -1,\n        .handle_on_start_failure = true,\n        .failed_route = SIZE_MAX,\n    };\n    ops = lifecycle_ops(&fake);\n    TEST_CHECK_APP_ERROR(APP_ERROR_IO, web_adapter_lifecycle_start(&lifecycle, &ops, 3U));\n    TEST_CHECK(lifecycle.handle != NULL);\n    TEST_CHECK_EQ_U64(0U, lifecycle.registered_routes);\n    TEST_CHECK_APP_ERROR(APP_ERROR_IO, lifecycle.cleanup_error);\n    TEST_CHECK(web_adapter_lifecycle_owns_resources(&lifecycle));\n    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,\n                         web_adapter_lifecycle_start(&lifecycle, &ops, 3U));\n    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_adapter_lifecycle_stop(&lifecycle, &ops));\n    TEST_CHECK(!web_adapter_lifecycle_owns_resources(&lifecycle));\n\n    /* Registration failure plus a successful stop: fully cleaned up. */\n'''
replace_once("tests/host/test_web_server_adapter_stream_lifecycle.inc", anchor, insert)

# H9-092 / H1 safety boundary: send acceptance must read the authoritative device
# confirmation setting and bind it into the execution request. Failure to read policy
# fails closed instead of silently sending without confirmation.
replace_once(
    "firmware/components/web_server/web_send.h",
    '    app_error_code_t (*submit)(void *context, macro_execution_request_t *request);\n',
    '    app_error_code_t (*submit)(void *context, macro_execution_request_t *request);\n    /* Reads the authoritative device-wide confirmation policy at send-accept time. */\n    app_error_code_t (*get_require_confirmation)(void *context, bool *out_required);\n',
)
replace_once(
    "firmware/components/web_server/web_send.c",
    'static bool ops_valid(const web_send_ops_t *ops) {\n    return ops != NULL && ops->submit != NULL && ops->get_status != NULL && ops->cancel != NULL;\n}\n',
    'static bool ops_valid(const web_send_ops_t *ops) {\n    return ops != NULL && ops->submit != NULL && ops->get_require_confirmation != NULL &&\n           ops->get_status != NULL && ops->cancel != NULL;\n}\n',
)
replace_once(
    "firmware/components/web_server/web_send.c",
    '    app_uuid_t send_id = {0};\n    if (app_uuid_generate(&send_id) != APP_ERROR_NONE) {\n',
    '    bool require_confirmation = false;\n    const app_error_code_t policy_result =\n        ops->get_require_confirmation(ops->context, &require_confirmation);\n    if (policy_result != APP_ERROR_NONE) {\n        macro_plan_v2_free(&plan);\n        return outcome_with_detail(WEB_SEND_CREATE_BACKEND_UNAVAILABLE, policy_result);\n    }\n\n    app_uuid_t send_id = {0};\n    if (app_uuid_generate(&send_id) != APP_ERROR_NONE) {\n',
)
replace_once(
    "firmware/components/web_server/web_send.c",
    '        .inter_key_ms = request.inter_key_ms,\n        .plan = plan,\n',
    '        .inter_key_ms = request.inter_key_ms,\n        .plan = plan,\n        .require_confirmation = require_confirmation,\n',
)
replace_once(
    "firmware/components/web_server/web_server_send.c",
    '#include "auth.h"\n#include "macro_executor.h"\n',
    '#include "auth.h"\n#include "device_settings.h"\n#include "macro_executor.h"\n',
)
replace_once(
    "firmware/components/web_server/web_server_send.c",
    'static macro_execution_status_t executor_status_adapter(void *context) {\n',
    '''static app_error_code_t confirmation_policy_adapter(void *context, bool *out_required) {\n    (void)context;\n    if (out_required == NULL) {\n        return APP_ERROR_INVALID_ARGUMENT;\n    }\n    app_v2_device_settings_t settings = {0};\n    const app_error_code_t result = device_settings_read(&settings);\n    if (result == APP_ERROR_NONE) {\n        *out_required = settings.require_serial_confirmation;\n    }\n    memset(&settings, 0, sizeof(settings));\n    return result;\n}\n\nstatic macro_execution_status_t executor_status_adapter(void *context) {\n''',
)
replace_once(
    "firmware/components/web_server/web_server_send.c",
    '        .submit = executor_submit_adapter,\n        .get_status = executor_status_adapter,\n',
    '        .submit = executor_submit_adapter,\n        .get_require_confirmation = confirmation_policy_adapter,\n        .get_status = executor_status_adapter,\n',
)

# Core send regressions for both policy values and fail-closed policy read.
replace_once(
    "tests/host/test_web_send.c",
    '    app_error_code_t submit_result;\n    macro_execution_request_t last_request;\n',
    '    app_error_code_t submit_result;\n    app_error_code_t confirmation_policy_result;\n    bool require_confirmation;\n    macro_execution_request_t last_request;\n',
)
replace_once(
    "tests/host/test_web_send.c",
    'static macro_execution_status_t fake_get_status(void *context) {\n',
    '''static app_error_code_t fake_get_require_confirmation(void *context, bool *out_required) {\n    fake_send_backend_t *fake = context;\n    if (fake->confirmation_policy_result != APP_ERROR_NONE) {\n        return fake->confirmation_policy_result;\n    }\n    *out_required = fake->require_confirmation;\n    return APP_ERROR_NONE;\n}\n\nstatic macro_execution_status_t fake_get_status(void *context) {\n''',
)
replace_once(
    "tests/host/test_web_send.c",
    '        .submit = fake_submit,\n        .get_status = fake_get_status,\n',
    '        .submit = fake_submit,\n        .get_require_confirmation = fake_get_require_confirmation,\n        .get_status = fake_get_status,\n',
)
replace_once(
    "tests/host/test_web_send.c",
    '    TEST_CHECK_EQ_U64(1U, fake.last_request.macro_revision);\n}\n\nstatic void test_create_rejects_malformed_bodies(void) {\n',
    '''    TEST_CHECK_EQ_U64(1U, fake.last_request.macro_revision);\n    TEST_CHECK(!fake.last_request.require_confirmation);\n}\n\nstatic void test_create_binds_confirmation_policy_and_fails_closed(void) {\n    fake_send_backend_t fake = {0};\n    fake.submit_result = APP_ERROR_NONE;\n    fake.require_confirmation = true;\n    web_send_ops_t ops = fake_ops(&fake);\n    size_t body_capacity = 0U;\n    char *body =\n        dup_body("{\\\"source\\\":\\\"first\\\",\\\"keyPressMs\\\":8,\\\"interKeyMs\\\":15}", &body_capacity);\n    web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);\n    free(body);\n    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_OK, outcome.result);\n    TEST_CHECK(fake.submit_called);\n    TEST_CHECK(fake.last_request.require_confirmation);\n\n    fake = (fake_send_backend_t){0};\n    fake.confirmation_policy_result = APP_ERROR_STORAGE_UNAVAILABLE;\n    ops = fake_ops(&fake);\n    body =\n        dup_body("{\\\"source\\\":\\\"first\\\",\\\"keyPressMs\\\":8,\\\"interKeyMs\\\":15}", &body_capacity);\n    outcome = web_send_create_handle(body, body_capacity, &ops);\n    free(body);\n    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_BACKEND_UNAVAILABLE, outcome.result);\n    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);\n    TEST_CHECK(!fake.submit_called);\n}\n\nstatic void test_create_rejects_malformed_bodies(void) {\n''',
)
replace_once(
    "tests/host/test_web_send.c",
    '    test_create_success();\n    test_create_rejects_malformed_bodies();\n',
    '    test_create_success();\n    test_create_binds_confirmation_policy_and_fails_closed();\n    test_create_rejects_malformed_bodies();\n',
)

# H9-092: statically scan every production V2 frontend directory, including AppV2
# and all feature families, not just auth/startup/data-layer directories.
replace_once(
    "webapp/tests/v2-browser-storage-prohibition.test.tsx",
    ' * The scan also covers `src/features/auth/v2/` (the Phase 8 V2-080/V2-081\n * setup and sign-in pages): they hold a device setup code, a Wi-Fi\n * passphrase, and an administrator password in React state, so the same\n * "never touches browser storage" invariant applies to them, not just the\n * Phase 7 `src/v2/` data layer.\n *\n * It also covers `src/features/startup/v2/` (the Phase 8 V2-082/V2-083/\n * V2-084 repository startup state machine): its first-package form holds a\n * package name and its recovered `RepositoryWorkingCopyStore` holds the\n * entire loaded (or freshly created) repository, all in React state.\n',
    ' * The static scan covers `src/AppV2.tsx`, the complete `src/v2/` data layer,\n * and every `src/features/**/v2/` production feature directory. This includes\n * authentication/setup secrets, loaded repository state, macros, snapshots,\n * settings, shell state, and future V2 feature families added under that\n * directory convention.\n',
)
replace_once(
    "webapp/tests/v2-browser-storage-prohibition.test.tsx",
    '  [\n    "../src/v2/**/*.{ts,tsx}",\n    "../src/features/auth/v2/**/*.{ts,tsx}",\n    "../src/features/startup/v2/**/*.{ts,tsx}",\n  ],\n',
    '  [\n    "../src/AppV2.tsx",\n    "../src/v2/**/*.{ts,tsx}",\n    "../src/features/**/v2/**/*.{ts,tsx}",\n  ],\n',
)
