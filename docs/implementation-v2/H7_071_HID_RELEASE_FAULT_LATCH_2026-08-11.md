# H7-071 — HID release fault latch evidence

**Date:** 2026-08-11
**Task:** `H7-071 — Fault latch unsafe HID state`
**Implementation/test SHA:** `5d5f042a3567476fe54e4e623462037819ea25da`

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

Targeted workflow run **31537774806**, job **validate-and-record** ran:

- `./scripts/run-tests.sh executor` — **2/2 passed**.
- `./scripts/run-tests.sh web` — **29/29 passed**.
- `./scripts/run-tests.sh --sanitizers executor` — **2/2 passed under ASan+UBSan**.
- `./scripts/run-tests.sh --sanitizers web` — **29/29 passed under ASan+UBSan**.
- `clang-format` on all changed C test/production files and `git diff --check`.

## H7-071 disposition

All three H7-071 clauses are satisfied: failed release makes executor/HID execution unavailable, new sends remain rejected until safe lifecycle reinitialization restores USB readiness, and the fault is visible through both the existing send-status `releaseError` field and the existing diagnostics `executor=failed` subsystem state.
