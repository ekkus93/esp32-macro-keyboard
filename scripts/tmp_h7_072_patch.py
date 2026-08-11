#!/usr/bin/env python3
from __future__ import annotations

import os
import sys
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"expected exactly one {label}, found {count}")
    return text.replace(old, new, 1)


def patch_code() -> None:
    core = Path("tests/host/test_web_send.c")
    text = core.read_text()
    marker = '''static void test_create_invalid_ops_or_null_body(void) {
'''
    addition = '''static void test_create_executor_unavailable_maps_to_internal(void) {
    fake_send_backend_t fake = {0};
    fake.submit_result = APP_ERROR_INTERNAL;
    const web_send_ops_t ops = fake_ops(&fake);
    size_t body_capacity = 0U;
    char *body =
        dup_body("{\\\"source\\\":\\\"first\\\",\\\"keyPressMs\\\":8,\\\"interKeyMs\\\":15}", &body_capacity);
    const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
    free(body);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_INTERNAL, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK(fake.submit_called);
}

''' + marker
    text = replace_once(text, marker, addition, "web_send invalid-ops marker")
    old_calls = '''    test_create_usb_not_ready();
    test_create_backend_internal_error();
    test_create_invalid_ops_or_null_body();
'''
    new_calls = '''    test_create_usb_not_ready();
    test_create_backend_internal_error();
    test_create_executor_unavailable_maps_to_internal();
    test_create_invalid_ops_or_null_body();
'''
    core.write_text(replace_once(text, old_calls, new_calls, "web_send main calls"))

    route = Path("tests/host/test_web_server_send_route.c")
    text = route.read_text()
    marker = '''/* -------------------------------------------------------------------------
 * GET /api/v1/send (send_get_handler)
 * ---------------------------------------------------------------------- */
'''
    addition = '''static void assert_internal_error_response(const fake_httpd_request_t *fake,
                                           const char *expected_message) {
    TEST_CHECK_EQ_STRING("500 Internal Server Error", fake->response_status);
    cJSON *root = parse_response(fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK(error != NULL);
    TEST_CHECK_EQ_STRING("internal",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING(expected_message,
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    TEST_CHECK(strstr(fake->response_body, "storage_unavailable") == NULL);
    cJSON_Delete(root);
}

static void test_send_create_executor_unavailable_is_internal(void) {
    reset_fakes();
    g_submit_result = APP_ERROR_INTERNAL;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\\\"source\\\":\\\"first\\\",\\\"keyPressMs\\\":8,\\\"interKeyMs\\\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK(g_submit_called);
    assert_internal_error_response(&fake, "send could not be accepted");
}

''' + marker
    text = replace_once(text, marker, addition, "send GET section marker")

    marker = '''static void test_send_get_never_sent(void) {
'''
    addition = '''static void test_send_get_generic_executor_unavailable_is_internal(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){
        .state = EXECUTION_RUNNING,
        .available = false,
        .release_error = APP_ERROR_NONE,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    assert_internal_error_response(&fake, "send status unavailable");
}

''' + marker
    text = replace_once(text, marker, addition, "send never-sent marker")

    marker = '''static void test_send_cancel_never_sent(void) {
'''
    addition = '''static void test_send_cancel_executor_unavailable_is_internal(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_RUNNING, .available = true};
    g_cancel_result = APP_ERROR_INTERNAL;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_cancel_handler(&request));
    TEST_CHECK(g_cancel_called);
    assert_internal_error_response(&fake, "cancellation could not be recorded");
}

''' + marker
    text = replace_once(text, marker, addition, "send cancel never-sent marker")

    old_calls = '''    test_send_create_busy();
    test_send_create_usb_not_ready();

    test_send_get_valid();
'''
    new_calls = '''    test_send_create_busy();
    test_send_create_usb_not_ready();
    test_send_create_executor_unavailable_is_internal();

    test_send_get_valid();
'''
    text = replace_once(text, old_calls, new_calls, "route create calls")
    old_calls = '''    test_send_get_release_fault_visible_when_executor_unavailable();
    test_send_get_never_sent();

    test_send_cancel_valid();
    test_send_cancel_unauthorized_without_cookie();
    test_send_cancel_never_sent();
'''
    new_calls = '''    test_send_get_release_fault_visible_when_executor_unavailable();
    test_send_get_generic_executor_unavailable_is_internal();
    test_send_get_never_sent();

    test_send_cancel_valid();
    test_send_cancel_unauthorized_without_cookie();
    test_send_cancel_executor_unavailable_is_internal();
    test_send_cancel_never_sent();
'''
    route.write_text(replace_once(text, old_calls, new_calls, "route get/cancel calls"))


def patch_docs() -> None:
    product_sha = os.environ["PRODUCT_SHA"]
    run_id = os.environ["GITHUB_RUN_ID"]

    evidence = f'''# H7-072 — Executor error classification evidence

**Date:** 2026-08-11
**Task:** `H7-072 — Correct error classification`
**Regression SHA:** `{product_sha}`

## Result

The production correction was already present in commit `48d7714d9f48621e1876c4ef3d434826542c6710`: a latched-unavailable executor returns `APP_ERROR_INTERNAL` from submit/cancel/confirm instead of the unrelated `APP_ERROR_STORAGE_UNAVAILABLE`. Current source contains no `APP_ERROR_STORAGE_UNAVAILABLE` reference anywhere under `firmware/components/macro_executor`.

This task therefore did not invent a new executor error enum or change the API contract. It added explicit boundary regressions proving the existing executor/internal classification survives through the web-send core and live HTTP adapters.

## HTTP disposition

For a generic executor-unavailable condition, the existing API behavior is intentionally fail-closed and sanitized:

- `POST /api/v1/send` -> `500 Internal Server Error`, code `internal`, message `send could not be accepted`.
- `GET /api/v1/send` when status is unavailable for a non-release-fault reason -> `500 Internal Server Error`, code `internal`, message `send status unavailable`.
- `DELETE /api/v1/send` when cancellation cannot be recorded -> `500 Internal Server Error`, code `internal`, message `cancellation could not be recorded`.

The tests assert that `storage_unavailable` does not appear in those response bodies. H7-071's separate release-fault case remains reportable through the existing sanitized `releaseError` field rather than being hidden behind the generic unavailable response.

## Validation

Targeted workflow run **{run_id}** ran:

- `./scripts/run-tests.sh executor` — **2/2 passed**.
- `./scripts/run-tests.sh web` — **29/29 passed**.
- `./scripts/run-tests.sh --sanitizers executor` — **2/2 passed under ASan+UBSan**.
- `./scripts/run-tests.sh --sanitizers web` — **29/29 passed under ASan+UBSan**.
- A source guard that fails if `APP_ERROR_STORAGE_UNAVAILABLE` appears under `firmware/components/macro_executor`.
- `clang-format` and `git diff --check` on changed files.

## H7-072 disposition

Both checklist clauses are satisfied: unavailable executor state uses the executor/internal error domain, and the HTTP boundary maps it to generic fixed-vocabulary internal failures without leaking storage semantics or implementation detail.
'''
    Path("docs/implementation-v2/H7_072_EXECUTOR_ERROR_CLASSIFICATION_2026-08-11.md").write_text(evidence)

    todo = Path("docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md")
    text = todo.read_text()
    old = '''### H7-072 — Correct error classification

- [ ] Replace executor-unavailable -> `APP_ERROR_STORAGE_UNAVAILABLE` mappings with an executor/internal-appropriate error.
- [ ] Verify HTTP mapping remains sensible and does not leak internals.
'''
    new = f'''### H7-072 — Correct error classification

- [x] Replace executor-unavailable -> `APP_ERROR_STORAGE_UNAVAILABLE` mappings with an executor/internal-appropriate error.
- [x] Verify HTTP mapping remains sensible and does not leak internals.
- Evidence: production correction landed in `48d7714d9f48621e1876c4ef3d434826542c6710`; `{product_sha}` adds explicit web-core/live-route regression coverage for POST/GET/DELETE unavailable-executor mappings. Executor 2/2 and web 29/29 passed in normal and ASan+UBSan runs in targeted workflow `{run_id}`; source guard confirms no `APP_ERROR_STORAGE_UNAVAILABLE` remains in `firmware/components/macro_executor`. Full evidence: `docs/implementation-v2/H7_072_EXECUTOR_ERROR_CLASSIFICATION_2026-08-11.md`.
'''
    todo.write_text(replace_once(text, old, new, "H7-072 ledger block"))


def main() -> None:
    if len(sys.argv) != 2 or sys.argv[1] not in {"code", "docs"}:
        raise SystemExit("usage: tmp_h7_072_patch.py code|docs")
    if sys.argv[1] == "code":
        patch_code()
    else:
        patch_docs()


if __name__ == "__main__":
    main()
