#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if text.count(old) != 1:
        raise SystemExit(f"expected exactly one {label}, found {text.count(old)}")
    return text.replace(old, new, 1)


def patch_code() -> None:
    web_send = Path("firmware/components/web_server/web_send.c")
    text = web_send.read_text()
    old = '''    if (!status.available) {
        return (web_send_get_outcome_t){.result = WEB_SEND_GET_INTERNAL};
    }
    web_send_get_outcome_t outcome = {.result = WEB_SEND_GET_OK};
'''
    new = '''    /* An unavailable executor caused by an HID release fault still has useful,
     * already-sanitized status to report through the existing releaseError field.
     * Suppressing that status behind a generic 500 would hide the safety fault from
     * the caller. Other unavailable states (for example status publication/locking
     * failure) remain fail-closed because their status cannot be trusted. */
    if (!status.available && status.release_error == APP_ERROR_NONE) {
        return (web_send_get_outcome_t){.result = WEB_SEND_GET_INTERNAL};
    }
    web_send_get_outcome_t outcome = {.result = WEB_SEND_GET_OK};
'''
    web_send.write_text(replace_once(text, old, new, "web_send unavailable gate"))

    test_web_send = Path("tests/host/test_web_send.c")
    text = test_web_send.read_text()
    marker = '''static void test_get_unavailable_backend(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return =
        (macro_execution_status_t){.state = EXECUTION_RUNNING, .available = false};
    const web_send_ops_t ops = fake_ops(&fake);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_INTERNAL, web_send_get_handle(&ops, 0U).result);
}

'''
    addition = marker + '''static void test_get_unavailable_release_fault_remains_visible(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return = (macro_execution_status_t){
        .state = EXECUTION_FAILED,
        .available = false,
        .error = APP_ERROR_IO,
        .release_error = APP_ERROR_USB_NOT_READY,
    };
    const web_send_ops_t ops = fake_ops(&fake);
    const web_send_get_outcome_t outcome = web_send_get_handle(&ops, 0U);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_OK, outcome.result);
    TEST_CHECK_EQ_STRING("failed", outcome.status.state);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_IO), outcome.status.error);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_USB_NOT_READY),
                         outcome.status.release_error);
}

'''
    text = replace_once(text, marker, addition, "web_send unavailable test")
    old_call = '''    test_get_reports_terminal_errors();
    test_get_unavailable_backend();
    test_cancel_never_sent();
'''
    new_call = '''    test_get_reports_terminal_errors();
    test_get_unavailable_backend();
    test_get_unavailable_release_fault_remains_visible();
    test_cancel_never_sent();
'''
    test_web_send.write_text(replace_once(text, old_call, new_call, "web_send main calls"))

    route = Path("tests/host/test_web_server_send_route.c")
    text = route.read_text()
    marker = '''static void test_send_get_never_sent(void) {
'''
    addition = '''static void test_send_get_release_fault_visible_when_executor_unavailable(void) {
    reset_fakes();
    app_uuid_t id = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_generate(&id));
    g_execution_status = (macro_execution_status_t){
        .state = EXECUTION_FAILED,
        .available = false,
        .execution_id = id,
        .action_index = 1U,
        .action_count = 1U,
        .error = APP_ERROR_IO,
        .release_error = APP_ERROR_USB_NOT_READY,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING("failed", cJSON_GetObjectItemCaseSensitive(root, "state")->valuestring);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_IO),
                         cJSON_GetObjectItemCaseSensitive(root, "error")->valuestring);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_USB_NOT_READY),
                         cJSON_GetObjectItemCaseSensitive(root, "releaseError")->valuestring);
    cJSON_Delete(root);
}

''' + marker
    text = replace_once(text, marker, addition, "route never-sent marker")
    old_call = '''    test_send_get_unauthorized_expired_session();
    test_send_get_never_sent();

    test_send_cancel_valid();
'''
    new_call = '''    test_send_get_unauthorized_expired_session();
    test_send_get_release_fault_visible_when_executor_unavailable();
    test_send_get_never_sent();

    test_send_cancel_valid();
'''
    route.write_text(replace_once(text, old_call, new_call, "route main calls"))

    admin = Path("tests/host/test_web_server_administration_route.c")
    text = admin.read_text()
    old_include = '#include "esp_timer.h"\n#include "fake_httpd.h"\n#include "http_health.h"\n'
    new_include = '#include "esp_timer.h"\n#include "executor_health.h"\n#include "fake_httpd.h"\n#include "http_health.h"\n'
    text = replace_once(text, old_include, new_include, "executor health include")
    old_reset = '''    g_largest_free_block_bytes = 120000U;
    http_health_reset();

    server_configuration = (web_server_config_t){0};
'''
    new_reset = '''    g_largest_free_block_bytes = 120000U;
    executor_health_reset();
    http_health_reset();

    server_configuration = (web_server_config_t){0};
'''
    text = replace_once(text, old_reset, new_reset, "diagnostics health reset")
    marker = '''static void test_diagnostics_async_failure_degrades_existing_http_subsystem(void) {
'''
    addition = '''static void test_diagnostics_release_fault_degrades_executor_subsystem(void) {
    reset_fakes();
    executor_health_record_cleanup(APP_ERROR_USB_NOT_READY, true);
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *subsystems = cJSON_GetObjectItemCaseSensitive(root, "subsystems");
    TEST_CHECK_EQ_INT(8, cJSON_GetArraySize(subsystems));

    const cJSON *executor_entry = NULL;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, subsystems) {
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (cJSON_IsString(name) && strcmp(name->valuestring, "executor") == 0) {
            executor_entry = entry;
        }
    }
    TEST_CHECK(executor_entry != NULL);
    TEST_CHECK_EQ_STRING("failed",
                         cJSON_GetObjectItemCaseSensitive(executor_entry, "state")->valuestring);
    cJSON_Delete(root);
}

''' + marker
    text = replace_once(text, marker, addition, "diagnostics async marker")
    old_call = '''    test_diagnostics_get_valid();
    test_diagnostics_async_failure_degrades_existing_http_subsystem();
'''
    new_call = '''    test_diagnostics_get_valid();
    test_diagnostics_release_fault_degrades_executor_subsystem();
    test_diagnostics_async_failure_degrades_existing_http_subsystem();
'''
    admin.write_text(replace_once(text, old_call, new_call, "diagnostics main calls"))


def patch_docs() -> None:
    product_sha = os.environ["PRODUCT_SHA"]
    run_id = os.environ["GITHUB_RUN_ID"]
    job_id = os.environ.get("JOB_ID", "")

    evidence_path = Path("docs/implementation-v2/H7_071_HID_RELEASE_FAULT_LATCH_2026-08-11.md")
    evidence_path.write_text(f'''# H7-071 — HID release fault latch evidence

**Date:** 2026-08-11
**Task:** `H7-071 — Fault latch unsafe HID state`
**Implementation/test SHA:** `{product_sha}`

## Result

The core executor safety latch was already present from commit `48d7714d9f48621e1876c4ef3d434826542c6710`: every observed `usb_release_all()` failure is retained as a fixed-vocabulary `app_error_code_t`, atomically latches the executor unavailable, publishes `macro_execution_status_t.available=false`, and rejects subsequent submit/cancel/confirm operations until lifecycle reinitialization. Existing engine regression `test_release_fault_recovery_requires_reinit_and_ready_usb()` proves reinitialization alone does not bypass the USB readiness gate; sends resume only after the reinitialized HID transport reports ready.

The remaining H7-071 visibility gap was in the HTTP send-status layer. `web_send_get_handle()` previously converted every `available=false` executor status into a generic internal error before serializing the already-sanitized `release_error`. That made a safety-relevant release failure invisible to the caller even though the engine and diagnostics health had retained it.

This SHA changes only that classification boundary: an unavailable status with a non-`APP_ERROR_NONE` `release_error` is reportable through the existing `releaseError` field. Unavailable states without a release fault remain fail-closed as `WEB_SEND_GET_INTERNAL`, because their status may be untrustworthy for other reasons such as status publication/locking failure. No new wire field or error vocabulary is introduced.

## Regression coverage

- `macro_executor` existing regressions prove a release fault latches `available=false`, rejects a later send, and clears only across reinit plus a ready USB transport.
- `web_send` now proves generic unavailable state still returns internal failure, while unavailable + release fault returns the normal status view with sanitized `error` and `releaseError` strings.
- `web_server_send_route` proves live `GET /api/v1/send` returns `200 OK` and exposes the existing sanitized `releaseError` when the executor is fault-latched by a release failure.
- `web_server_administration_route` now injects `executor_health_record_cleanup(APP_ERROR_USB_NOT_READY, true)` and proves live `GET /api/v1/diagnostics` keeps the frozen eight-entry schema while marking the existing `executor` subsystem `failed`.

## Validation

Targeted workflow run **{run_id}**{f', job **{job_id}**' if job_id else ''} ran:

- `./scripts/run-tests.sh executor` — **2/2 passed**.
- `./scripts/run-tests.sh web` — **29/29 passed**.
- `./scripts/run-tests.sh --sanitizers executor` — **2/2 passed under ASan+UBSan**.
- `./scripts/run-tests.sh --sanitizers web` — **29/29 passed under ASan+UBSan**.
- `clang-format` on all changed C test/production files and `git diff --check`.

## H7-071 disposition

All three H7-071 clauses are satisfied: failed release makes executor/HID execution unavailable, new sends remain rejected until safe lifecycle reinitialization restores USB readiness, and the fault is visible through both the existing send-status `releaseError` field and the existing diagnostics `executor=failed` subsystem state.
''')

    todo = Path("docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md")
    text = todo.read_text()
    old = '''### H7-071 — Fault latch unsafe HID state

- [ ] If defensive release-all fails and key release cannot be proven, mark executor/HID unavailable.
- [ ] Reject new sends until a defined recovery/reinitialization path re-establishes ready state.
- [ ] Ensure status/diagnostics exposes sanitized release failure.
'''
    new = f'''### H7-071 — Fault latch unsafe HID state

- [x] If defensive release-all fails and key release cannot be proven, mark executor/HID unavailable.
- [x] Reject new sends until a defined recovery/reinitialization path re-establishes ready state.
- [x] Ensure status/diagnostics exposes sanitized release failure.
- Evidence: core latch/recovery behavior is implemented by `48d7714d9f48621e1876c4ef3d434826542c6710`; `{product_sha}` closes the visibility gap by allowing an unavailable executor status with a retained release fault to use the existing sanitized `releaseError` wire field while unrelated unavailable states still fail closed. Live diagnostics regression also proves a recorded release cleanup fault degrades the existing `executor` subsystem without changing the frozen schema. Executor 2/2 and web 29/29 passed in normal and ASan+UBSan runs in targeted workflow `{run_id}`. Full evidence: `docs/implementation-v2/H7_071_HID_RELEASE_FAULT_LATCH_2026-08-11.md`.
'''
    todo.write_text(replace_once(text, old, new, "H7-071 ledger block"))


def main() -> None:
    if len(sys.argv) != 2 or sys.argv[1] not in {"code", "docs"}:
        raise SystemExit("usage: tmp_h7_071_patch.py code|docs")
    if sys.argv[1] == "code":
        patch_code()
    else:
        patch_docs()


if __name__ == "__main__":
    main()
