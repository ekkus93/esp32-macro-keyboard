# Phase H7 exit — Executor/HID release-safety evidence

**Date:** 2026-08-11
**Phase:** `H7 — Executor/HID release safety`
**Current source audited:** `62d2969b17bc44d090982e64e77e88156bbf9ad0`
**Behavior-test SHA:** `896ddce7173e83f73a0113fd6ba2a16cf45039c1`
**Targeted validation:** workflow run `31538914176`, job `93936574189`

## Result

Both Phase H7 exit gates are satisfied by the current source and the permanent executor regressions already closed under H7-070 through H7-073. No additional runtime change is required for phase closure.

The H7-073 behavior-test SHA and the audited current source have identical production and test code for this scope. GitHub comparison `896ddce7173e83f73a0113fd6ba2a16cf45039c1..62d2969b17bc44d090982e64e77e88156bbf9ad0` contains only removal of the temporary H7-073 workflow, the H7-073 evidence file, and the H7-073 TODO checkoff.

## Exit gate 1 — Every safety-relevant key-release attempt has an observed result

The current production call graph has no ignored safety-relevant release result.

### Executor engine release boundaries

`firmware/components/macro_executor/macro_executor_engine.c` has three runtime release boundaries through `ops.usb_release_all`:

1. `execute_action()` captures the result in `release_result`, passes it to `record_release_failure()`, and makes it the action result when there was no earlier action error.
2. `finish_execution()` captures the terminal defensive release result and passes it to `record_release_failure()` while preserving the primary execution result separately.
3. `finish_submission_failure()` captures defensive submission-cleanup release, records it separately from the primary submit failure, and publishes the resulting unavailable status.

`record_release_failure()` retains the first fixed-vocabulary release failure, sets `status.release_error` where a status object is available, atomically latches the engine unavailable, and records the cleanup fault into executor health.

### Concrete USB adapter and deinit boundary

`firmware/components/macro_executor/macro_executor.c` has the concrete adapter `adapter_usb_release_all()`, which directly returns `usb_keyboard_release_all()` to the engine, so the three engine observations above receive the actual lower-layer result rather than a fabricated success.

The separate shutdown release in `macro_executor_deinit()` also captures `usb_keyboard_release_all()` into `release_result`; a failure contributes to the returned deinit error when no earlier shutdown error already owns primary-error precedence. Through `app_core.c`, the deinit result is also recorded as executor cleanup health. Shutdown has already closed submissions before this release is attempted.

`firmware/components/usb_keyboard/usb_keyboard.c` directly returns `usb_keyboard_state_release_all()`, and `usb_keyboard_state.c` returns readiness, timeout, not-ready, and report-send failures to its caller rather than swallowing them.

This satisfies SPEC §12.1: every attempted release-all in the reviewed production call graph has its result consumed by the caller; no safety-relevant release uses a void discard or unconditional-success wrapper.

## Exit gate 2 — Failed release cannot be silently followed by accepting new sends

For all active executor submission/execution release failures, `record_release_failure()` latches `engine.unavailable=true` before returning to the caller. `macro_executor_engine_submit()` checks that atomic latch before validation, USB readiness checks, lock acquisition, queueing, or ownership transfer and returns `APP_ERROR_INTERNAL` while fault-latched. `cancel()` and `confirm()` also reject a fault-latched engine, and status retains the sanitized release error with `available=false`.

Permanent regressions prove the required behavior:

- `test_submission_cleanup_release_errors_are_published()` covers unlock failure + failed release and queue-send failure + failed release, then proves a subsequent send is rejected before plan ownership transfers.
- `test_press_release_and_final_release_errors()` covers a mid-action press failure plus failed defensive release while retaining primary and release errors separately.
- `test_cancel_timeout_release_failure_latches_unavailable()` covers cancellation and timeout release failures.
- `test_release_fault_recovery_requires_reinit_and_ready_usb()` proves ordinary submission cannot clear the latch; executor reinit with USB still not ready continues to reject sends with `APP_ERROR_USB_NOT_READY`; only after the reinitialized HID transport reports ready may submission resume.

At the full-module lifecycle boundary, `macro_executor_deinit()` calls `executor_shutdown_state_begin()` before worker cancellation/stop, so submissions are closed during teardown. In the application teardown ordering, HTTP and Wi-Fi are stopped before executor deinit and USB is deinitialized afterward. A later normal startup initializes USB before the executor, and executor submission independently requires USB `READY`. Thus a deinit-time release failure is returned/health-visible while no live HTTP send path remains, and normal send acceptance is restored only through the defined USB/executor lifecycle.

## Validation and source continuity

Targeted H7-073 workflow `31538914176`, job `93936574189`, passed:

- `./scripts/run-tests.sh executor` — **2/2 passed** (`macro_executor`, `executor_health`).
- `./scripts/run-tests.sh --sanitizers executor` — **2/2 passed under ASan + UBSan**.
- The source-to-runner guard verified that the release-fault regressions are actually invoked by the executor suite.

Comparison from the behavior-test SHA to current audited source shows no production or test change in the H7 scope, so those regression results remain code-equivalent to the current source.

## Phase H7 disposition

- **PASS:** Every safety-relevant key-release attempt in the reviewed production call graph has an observed result.
- **PASS:** A failed active executor release cannot be silently followed by accepting a new send; the fault is latched unavailable until the defined reinitialization path and USB readiness gate establish a usable HID transport.

No hardware claim is made by this phase-exit evidence beyond the pre-existing hardware evidence referenced by earlier v2 work.