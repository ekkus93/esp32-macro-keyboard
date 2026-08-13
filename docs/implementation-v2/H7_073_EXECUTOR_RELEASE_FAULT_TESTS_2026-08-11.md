# H7-073 — Executor release-fault test evidence

**Date:** 2026-08-11
**Task:** `H7-073 — Tests`
**Validated source tree:** `896ddce7173e83f73a0113fd6ba2a16cf45039c1`
**Product predecessor:** `96a7974038baf1503ea40995259d70c78e5bab9c`
**Targeted validation:** workflow run `31538914176`, job `93936574189`

## Result

No new production or test implementation was required for H7-073. The six requested fault cases were already present in the current executor host suite as part of the H7-070/H7-071 hardening. H7-073 work therefore reconciled each literal checklist item against the current source, verified that the relevant source fragments are wired into `test_macro_executor.c`, and reran the executor suite in normal and sanitizer configurations.

The validation SHA differs from the product predecessor only by the temporary read-only H7-073 validator workflow; the executor implementation and test sources being validated are unchanged from the product tree.

## Checklist mapping

1. **unlock failure + release-all failure**
   - `tests/host/executor_validation_tests.inc`
   - `test_submission_cleanup_release_errors_are_published()` injects the submit-path `unlock` failure and makes the corresponding defensive `release_all` return `APP_ERROR_USB_NOT_READY`.
   - It asserts the primary submit failure remains `APP_ERROR_INTERNAL`, the release failure is retained separately, the executor becomes unavailable, and the failed release is recorded.

2. **queue-send failure + release-all failure**
   - The second half of `test_submission_cleanup_release_errors_are_published()` injects `fake.queue_result = false` plus a failed defensive release.
   - It asserts the queue failure remains primary, the release failure remains separately visible, and availability is latched false.

3. **mid-action press failure + release-all failure**
   - `tests/host/executor_terminal_tests.inc`
   - `test_press_release_and_final_release_errors()` injects `APP_ERROR_IO` from the key press and `APP_ERROR_USB_NOT_READY` from that action's immediate defensive release.
   - It asserts the press failure remains primary, the release error is retained separately, the terminal release is still attempted, and the executor becomes unavailable.

4. **cancellation/timeout + release-all failure**
   - `test_cancel_timeout_release_failure_latches_unavailable()` covers both cancellation and timeout independently.
   - In both cases the requested terminal outcome remains primary while the failed release is retained separately and availability is latched false.

5. **fault latch rejects subsequent sends**
   - `test_submission_cleanup_release_errors_are_published()` attempts another send after the failed defensive release and requires `APP_ERROR_INTERNAL` before plan ownership transfers.
   - `test_release_fault_recovery_requires_reinit_and_ready_usb()` independently exercises the same fail-closed rejection after a terminal release fault.

6. **recovery/reinit clears only when safe**
   - `test_release_fault_recovery_requires_reinit_and_ready_usb()` proves ordinary submission cannot clear the latch.
   - A full executor reinitialization clears the old executor latch, but with `usb_is_ready = false` the next send still fails with `APP_ERROR_USB_NOT_READY` and retains caller ownership.
   - Only after the reinitialized HID transport reports ready does a new send execute successfully.

## Runner wiring

`tests/host/test_macro_executor.c` invokes both `executor_run_validation_tests()` and `executor_run_terminal_tests()`. The targeted validator also checked that the H7-073 test functions are called by those runners rather than merely existing as dead source.

## Validation

Targeted workflow `31538914176` / job `93936574189` passed:

- H7-073 source-to-runner regression-matrix guard.
- `./scripts/run-tests.sh executor` — **2/2 passed** (`macro_executor`, `executor_health`).
- `./scripts/run-tests.sh --sanitizers executor` — **2/2 passed under ASan + UBSan**.
- Ubuntu 24.04, GCC 13.3.0, libcjson 1.7.17.

## Disposition

All six H7-073 checklist cases are directly represented by executed host regressions and pass both normal and sanitizer executor suites. No hardware claim is made by this evidence.
