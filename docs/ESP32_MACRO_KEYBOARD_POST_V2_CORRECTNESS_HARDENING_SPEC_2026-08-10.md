# ESP32 Macro Keyboard — Post-v2 Correctness and Hardening Specification

**Document status:** Implementation specification  
**Date:** 2026-08-10  
**Repository:** `ekkus93/esp32-macro-keyboard`  
**Baseline reviewed:** `99b86777d14cbb5b3655e0facad80651d29e8fa0`  
**Applies to:** v2 firmware, v2 React application, tests, release evidence, and `docs/TODO_V2.md`

## 1. Purpose

The v2 rebuild established the correct product architecture: the ESP32 is primarily a USB HID keyboard plus durable opaque storage and device configuration, while the React application owns repository, package, and macro semantics. This specification does **not** replace that architecture.

This specification defines the correctness and hardening work required after the v2 implementation review. Its purpose is to eliminate failure modes where the system:

- reports success after only part of an operation succeeded,
- silently degrades a security or safety control,
- loses the primary failure behind a cleanup failure,
- hides an active send or removes the visible cancellation path,
- silently converts persistence failures into apparent success,
- falls back to blocking or unsafe behavior when a required subsystem is unavailable,
- or marks validation complete when the committed evidence does not prove the checkbox as written.

The dominant requirement is simple:

> **Security, HID safety, destructive operations, active execution state, and persistence integrity must never use invisible best-effort behavior.**

A degraded path is allowed only when all of the following are true:

1. the degraded behavior remains safe,
2. the state is unambiguous,
3. the user or diagnostics can see that degradation occurred,
4. retry or recovery semantics are defined,
5. and the API does not claim stronger success than actually occurred.

## 2. Authority and relationship to existing specifications

The product requirements remain defined by:

- `docs/SPEC_V2.md`
- `docs/UI_UX_SPEC_V2.md`

`docs/TODO_V2.md` remains the historical implementation sequence and evidence ledger for the v2 rebuild.

This document is a **post-v2 correctness specification**. It may clarify implementation and failure semantics needed to satisfy the existing v2 intent, but it must not silently redefine product behavior.

If this document exposes a genuine conflict with `SPEC_V2.md` or `UI_UX_SPEC_V2.md`, implementation must stop at that conflict and the authoritative specification must be reconciled explicitly.

## 3. Non-goals

This work must not:

- reintroduce firmware-owned package or macro repositories,
- move repository semantics back into firmware,
- add compatibility shims for retired v1 behavior,
- add autosave or automatic snapshot deletion,
- weaken gzip-only repository persistence,
- introduce a cloud dependency,
- create a second execution engine,
- create a second authentication/session implementation,
- or redesign the UI merely for cosmetic reasons.

This is a correctness and integrity pass over the current v2 system.

## 4. Mandatory engineering principles

### 4.1 No silent best-effort for critical state

The following operations may not swallow failures and continue as though the requested invariant is satisfied:

- password changes,
- session invalidation associated with credential changes,
- factory reset,
- reset-settings when session invalidation is required,
- physical/serial confirmation gating,
- active-send recovery,
- executor key release,
- durable storage commit,
- cancellation visibility,
- and destructive blob operations.

### 4.2 Preserve the primary failure

When an operation fails and cleanup also fails:

- the primary failure must remain identifiable,
- cleanup failure must be recorded separately,
- cleanup failure must not overwrite the initiating failure,
- and diagnostics/tests must be able to assert both values when both occur.

Where a public API exposes only one error code, it must expose the primary error and retain the cleanup error in structured internal status/diagnostics.

### 4.3 Never claim unqualified success for partial commit

If an operation mutates durable or security-sensitive state before a later stage fails, the result must not be represented as simple success or simple rollback unless that statement is true.

The implementation must choose one of:

- atomic commit,
- rollback with verified rollback result,
- or an explicit recovery/commit-uncertain state.

### 4.4 Fail closed when a safety subsystem is unavailable

When a feature explicitly depends on a safety subsystem, loss of that subsystem must not silently bypass the control.

Examples:

- physical confirmation enabled but confirmation routing unavailable,
- async confirmation worker unavailable,
- executor fault-latched unavailable,
- credential cache refresh not coherent with durable credential state.

### 4.5 Tests must prove boundary behavior

Unit tests are necessary but insufficient for requirements crossing subsystem boundaries.

Examples:

- executor confirmation tests do not prove `POST /api/v1/send` honors the setting,
- password-store tests do not prove login immediately uses the new verifier,
- storage happy-path tests do not prove partial factory-reset recovery,
- host HTTP handler tests do not prove ESP-IDF routing behavior,
- browser helper tests do not prove cancellation remains visible after reload/network faults.

## 5. Review findings that this specification resolves

### F-001 — Physical confirmation is not wired into real sends

The executor supports `require_confirmation`, `EXECUTION_AWAITING_CONFIRMATION`, confirmation, cancellation, and timeout. The real HTTP send path does not currently populate the request from `requireSerialConfirmation`.

**Required outcome:** when the configured confirmation setting is enabled, every accepted real HTTP send must enter the confirmation-gated execution path before any key is emitted.

### F-002 — Password-change RAM verifier refresh is best-effort

The current implementation persists the new verifier and then re-reads settings to refresh the server's in-memory password record. Failure of that read is ignored while returning `204`.

**Required outcome:** a successful password change must make the new verifier authoritative in RAM immediately without a fallible post-commit re-read being required for correctness.

### F-003 — Password change can partially commit before session invalidation fails

The durable password may change before `logout_all` succeeds.

**Required outcome:** the API must have defined semantics for the complete credential/session transaction. It may not return an ordinary failure that leaves the caller unable to know whether the password changed.

### F-004 — Factory reset can partially commit and still restart

Settings erase occurs before session invalidation/blob deletion, and restart is scheduled even if later cleanup fails.

**Required outcome:** factory reset must be restart-safe, power-loss-safe, and retry-safe. A failure after destructive work begins must enter a recoverable reset-in-progress state, not ambiguous normal/setup operation.

### F-005 — Active-send recovery errors are silently converted to “no send”

Startup currently treats failure to recover send state as `null`.

**Required outcome:** inability to determine active execution state must be visible. The UI must retain or present a cancellation/recovery path rather than implying that no send exists.

### F-006 — Storage cleanup can mask the primary error

Several storage paths return cleanup failure instead of the original write/rename/mount failure.

**Required outcome:** retain primary and cleanup errors separately.

### F-007 — Post-rename parent-sync failure creates ambiguous commit state

After atomic rename succeeds, parent-directory sync can fail. The canonical file may already contain the new data even though the operation returns failure.

**Required outcome:** define and expose a commit-uncertain durability state or otherwise make retry semantics explicitly idempotent and safe.

### F-008 — Async confirmation falls back to blocking the HTTP server

If the async worker is unavailable, the code executes confirmation synchronously on the ESP-IDF HTTP task.

**Required outcome:** required async infrastructure must fail closed with a visible service-unavailable/health error. It must not revert to a known whole-server blocking mode.

### F-009 — Executor submission cleanup discards `release_all` errors

Certain submission failure paths cast away the result of `usb_release_all()`.

**Required outcome:** release failures must always be retained and surfaced in executor health/status, even when no action for the new request ran.

### F-010 — Polling failures can remain invisible indefinitely

USB status and send-status polling retain stale state and retry silently.

**Required outcome:** transient retry remains allowed, but a continuing inability to refresh must become visible as stale/degraded state while preserving the last known value.

### F-011 — Package-selection persistence failures are invisible

The UI opens a package even if persisting `lastSelectedPackageId` fails, without informing the user.

**Required outcome:** opening locally may continue, but persistence failure must be visible and retryable.

### F-012 — Snapshot export can reject without a user-visible error

Export resets its busy flag in `finally` but does not report compression/download failure.

**Required outcome:** export failures must use the same visible error model as load/download/delete/import.

### F-013 — `TODO_V2.md` contains overclaimed validation state

At least the V2-154 checkbox covering login/logout/**idle expiry/absolute expiry**/lockout is checked while the evidence explicitly states idle/absolute expiry were not independently hardware-verified. The “no secret appears” checkbox is likewise checked on spot-check evidence while the same report says a full audit remains open.

**Required outcome:** checkbox text, state, and evidence must agree literally.

## 6. Physical confirmation end-to-end requirements

### 6.1 Single source of truth

The configured `requireSerialConfirmation`/physical-confirmation setting must be read through the existing settings subsystem. Do not add an independent global flag.

### 6.2 Send acceptance

Before `macro_executor_submit()` receives a request, the send pipeline must set `request.require_confirmation` from the authoritative setting.

The value must be captured for the accepted send so a settings change after acceptance cannot alter the semantics of an already accepted request.

### 6.3 No keystroke before confirmation

When confirmation is required:

- the accepted send must publish `awaiting_confirmation`,
- zero key/chord reports may be emitted before confirmation,
- cancellation must remain available,
- expiry must produce `timed_out`,
- expiry must type nothing,
- confirmation must transition exactly once into running,
- duplicate confirmation must remain an idempotent/conflict-style no-op,
- and a failed confirmation subsystem must fail the send rather than bypass confirmation.

### 6.4 API/UI visibility

The send-status API and React UI must represent `awaiting_confirmation` explicitly. The UI must explain what action is required and keep Cancel available.

### 6.5 Required tests

At minimum:

- host unit test proving settings=true sets `require_confirmation`,
- host integration test across real send handler/core + executor seam,
- browser test showing awaiting-confirmation state and Cancel,
- device test proving zero HID key-down reports before confirmation,
- device test proving confirmation allows execution,
- device test proving timeout/cancel types nothing.

## 7. Password-change transaction requirements

### 7.1 Remove fallible post-success verifier refresh

Do not depend on a second NVS read after credential persistence.

The operation that creates the new password material must make the newly committed verifier available to the login subsystem as part of the same transaction state transition.

Acceptable designs include:

- update the RAM password record from the exact material/candidate that was durably committed,
- or centralize credential ownership so the auth subsystem reads one authoritative coherent state object.

A separate best-effort re-read is not acceptable.

### 7.2 Session invalidation semantics

A password-change request is not fully successful until:

- new credential material is durably committed,
- the in-memory verifier is coherent with that durable state,
- and existing sessions are invalidated.

If those steps cannot be made atomically, introduce an explicit transitional/fault state that prevents authentication against ambiguous credential state until recovery completes.

### 7.3 Failure behavior

After any returned `204`:

- old password must fail immediately,
- new password must succeed immediately,
- prior sessions must fail immediately,
- no reboot may be required.

For every injected failure point, tests must prove exactly which password is authoritative and whether old sessions survive.

### 7.4 Secret handling

New transaction state must not log:

- plaintext password,
- salt,
- verifier,
- session token,
- or setup code.

Temporary credential buffers must be securely zeroed on every path.

## 8. Factory-reset integrity requirements

### 8.1 Reset is a state machine, not a sequence of best-effort calls

Factory reset must have explicit durable state, for example:

- `none`,
- `factory_reset_pending`,
- `factory_reset_cleanup_required`.

The exact representation may differ, but after reset begins the device must be able to determine on reboot that cleanup is incomplete.

### 8.2 Required reset invariant

The device may return to ordinary provisioning/normal operation only after all required reset effects are complete:

- device settings/credentials erased,
- all sessions invalidated or made unusable,
- all repository blobs deleted,
- temporary upload debris handled,
- and the reset-in-progress marker cleared only after the cleanup is complete.

### 8.3 Retry and reboot behavior

Every reset step must be idempotent.

If power is lost or restart occurs after any intermediate step, boot must resume/complete the reset instead of exposing partially reset state.

### 8.4 Error and response semantics

Do not send `202 accepted` unless the device has durably committed to completing the reset across restart.

Once a reset request is accepted into the durable reset state machine, later cleanup difficulty is not a reason to revert to normal operation. Diagnostics/setup state must make recovery status visible.

### 8.5 Required tests

Inject failure after every reset stage and prove:

- restart does not expose old credentials as usable,
- surviving blobs cannot become silently available to a newly provisioned owner,
- reset resumes on reboot,
- repeated cleanup is safe,
- and the marker is cleared only after complete cleanup.

Real hardware must include at least one forced interruption during reset cleanup if practical; if direct fault injection is required, document the instrumentation and restore production behavior afterward.

## 9. Active-send recovery and cancellation visibility

### 9.1 “Unknown” is not “none”

The frontend must distinguish:

- confirmed no active/recent send,
- known active send,
- known terminal send,
- and send state currently unavailable/unknown.

A network or response failure may not be converted to `null`.

### 9.2 Startup behavior

Failure to recover send state must not discard the successfully loaded repository. The app may continue to the shell, but it must show a prominent degraded execution-state banner/surface and a Retry action.

When safe cancellation can still be requested, Cancel must remain available. If cancellation cannot currently be delivered, the UI must say so rather than hide the control silently.

### 9.3 Polling behavior

For both send status and USB status:

- retain the last known value,
- record freshness time or consecutive failure state,
- expose a stale/degraded state after a bounded threshold,
- recover automatically after a successful poll,
- do not announce every successful poll through accessibility live regions,
- and do not create duplicate POST sends while reconnecting.

## 10. Error provenance and cleanup requirements

Introduce or reuse a structured operation-result type whenever an operation can have both a primary and cleanup/durability failure.

Minimum conceptual fields:

```text
primary_error
cleanup_error
commit_state
```

`commit_state` should distinguish at least:

- not committed,
- committed,
- commit uncertain / durability uncertain.

Do not force this exact C struct if an existing type can express the same semantics.

### 10.1 Storage

Update atomic-write, blob-create, mount/unmount, and cleanup paths so:

- write/verify/rename failure remains primary,
- cleanup failure is retained separately,
- rename-success + parent-sync-failure is not presented as ordinary “not saved”,
- and retry behavior is tested.

### 10.2 API mapping

Where commit state is uncertain, do not emit a generic error that invites unsafe blind retry. Use an explicit internal/API error code or response message indicating the caller must refresh/reconcile state before retrying.

## 11. Async confirmation infrastructure requirements

### 11.1 Worker is required infrastructure

If confirmation-gated routes depend on the async worker to preserve server responsiveness, worker absence is a subsystem failure.

Do not execute the confirmation wait synchronously on the main httpd task as fallback.

### 11.2 Failure response

Return an explicit service-unavailable/internal error and mark web/confirmation health degraded.

### 11.3 Completion errors

Do not discard failures from:

- `web_api_handle_call_with_body`,
- `httpd_req_async_handler_complete`,
- worker stop signaling,
- or queue completion.

If the HTTP response itself cannot be repaired after it has been sent, retain the error in health/diagnostics/logs using sanitized structured data.

## 12. Executor and HID cleanup requirements

### 12.1 Release-all is safety-critical

Every attempted `usb_release_all()` must have an observed result.

Do not cast the result to void in a failure-cleanup path.

### 12.2 Submission failures

If submission fails after the system chooses to issue release-all as defensive cleanup:

- retain the original submission failure as primary,
- retain release failure separately,
- latch executor/HID health unavailable when key state cannot be proven released,
- and reject new sends until recovery/reinitialization establishes safe HID state.

### 12.3 Correct error domain

Executor unavailability must not be reported as `APP_ERROR_STORAGE_UNAVAILABLE`. Use the closest existing internal/executor error or add an explicit code if necessary.

## 13. Frontend persistence/error visibility requirements

### 13.1 Package selection

If package selection is changed locally but `lastSelectedPackageId` cannot be persisted:

- open the selected package,
- do not dirty the repository,
- show a non-destructive warning,
- explain that the selection may not survive reload,
- provide Retry where practical,
- clear the warning after successful persistence.

### 13.2 Snapshot export

All export failures must be caught and shown with the existing error-banner vocabulary.

The export busy state must always clear.

A failed export must not mutate dirty state, loaded snapshot association, or repository content.

## 14. `TODO_V2.md` evidence reconciliation requirements

Before final post-v2 sign-off:

1. audit every checked item whose text contains multiple behaviors,
2. ensure every behavior named by the checkbox is actually proven,
3. split compound checkboxes when only part is independently verifiable,
4. uncheck any item whose own evidence says a named portion was not tested,
5. never use host tests as hardware evidence,
6. never call a spot-check a complete no-secret audit,
7. keep unavailable optional platforms explicitly unavailable without claiming validation.

At minimum, re-evaluate:

- V2-154 login/logout/idle expiry/absolute expiry/lockout,
- V2-154 no-secret validation,
- physical-confirmation completion across V2-054/V2-061/Phase 6,
- password-change validation in V2-055/V2-154,
- and final sign-off statements about silent failure/dangerous fallback.

## 15. Observability requirements

Diagnostics should expose subsystem health without secrets.

Where applicable expose sanitized state such as:

- executor available/unavailable,
- last executor primary/release error,
- async confirmation worker healthy/unhealthy,
- reset recovery pending/not pending,
- storage commit uncertainty count/last occurrence,
- active-send status freshness/degraded state where this belongs on the client rather than firmware.

Do not expose passwords, salts, verifier bytes, session tokens, setup codes, macro source, key usages, or repository contents in diagnostics.

## 16. Required regression-test strategy

### 16.1 Native host tests

Add deterministic failure injection for every modified C state machine.

Required coverage includes:

- settings read/write failure,
- session invalidation failure,
- credential RAM update failure if any fallible step remains,
- blob-delete failure during reset,
- restart/reboot between factory-reset stages,
- release-all failure on submit cleanup,
- async worker absent/queue failure/completion failure,
- atomic rename success + parent sync failure,
- write failure + cleanup failure preserving both errors.

Run under ASan + UBSan.

### 16.2 Contract/API tests

Prove exact HTTP status and body behavior for:

- confirmation-required send,
- confirmation backend unavailable,
- commit-uncertain storage outcome where externally visible,
- password-change failure injection,
- reset accepted/recovery state,
- and no-secret response checks.

### 16.3 React/Vitest tests

Prove:

- send recovery error is not mapped to no-send,
- stale/degraded polling state appears after bounded failure,
- state recovers after a successful poll,
- package-selection persistence warning appears/clears,
- export errors are visible,
- dirty working copy survives these errors,
- cancellation remains reachable while execution state is active or unknown.

### 16.4 Real browser tests

Add at least one workflow for each cross-layer UI behavior that jsdom cannot faithfully prove:

- reload during active send / failed first status recovery,
- stale status then recovery,
- package-selection persistence failure warning,
- export failure visibility if it can be deterministically induced.

### 16.5 Hardware evidence

Required before final sign-off:

- physical-confirmation HID test proving zero reports before confirmation,
- confirmation timeout/cancel proving no typing,
- password change proving immediate old/new/session behavior without reboot,
- factory reset including recovery/interruption semantics,
- release-all failure/disconnect behavior where injectable/practical,
- AP/station behavior remains correct after hardening.

## 17. CI and quality gates

All existing quality gates remain authoritative and must not be weakened.

The final exact product SHA must pass at least:

```text
./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
```

plus the full frontend test/browser suite and the appropriate device-test build/run.

No new analyzer exclusion, ignored exit code, `|| true`, warning suppression, test skip, or lower coverage threshold may be added merely to make the hardening work pass.

## 18. Implementation order

The recommended priority order is:

1. physical-confirmation end-to-end wiring,
2. password-change coherence and session transaction,
3. factory-reset recovery state machine,
4. active-send recovery/cancellation visibility,
5. storage error provenance and commit-uncertain semantics,
6. async confirmation fail-closed behavior,
7. executor/HID release-error propagation,
8. frontend persistence and export error visibility,
9. TODO/evidence reconciliation,
10. full host/browser/hardware validation and final clean-checkout gate.

The TODO companion document defines the exact task sequence.

## 19. Forbidden implementations

The following are specifically prohibited unless the authoritative specification is intentionally changed:

- `catch {}` or ignored return codes on critical operations listed in §4.1,
- returning `204` after password persistence while RAM authentication state is known or potentially stale,
- silently interpreting unknown send state as no active send,
- blocking the ESP-IDF main HTTP task because the async confirmation worker failed,
- replacing a primary storage error with a cleanup error,
- claiming an atomic rollback that did not occur,
- automatically deleting snapshots to recover storage pressure,
- automatically loading an older snapshot after newest-snapshot failure,
- reintroducing firmware package/macro CRUD,
- disabling physical confirmation because its supporting subsystem is unavailable,
- reporting factory-reset failure while allowing ordinary operation with ambiguous partially reset state,
- or checking off hardware-validation tasks from host-only evidence.

## 20. Final acceptance criteria

This hardening program is complete only when all of the following are true:

- real HTTP sends honor the configured confirmation requirement,
- no key can be emitted before required confirmation,
- password changes are immediately coherent in durable state, RAM verifier state, and session state,
- factory reset is resumable and unambiguous across failure/reboot,
- active execution is never silently lost from UI state,
- cancellation remains visible/recoverable when execution state is active or unknown,
- primary and cleanup failures remain distinguishable,
- post-rename durability uncertainty has defined semantics,
- async-worker failure fails closed rather than blocking the whole server,
- all defensive release-all failures are observed,
- persistent polling degradation becomes visible,
- package-selection persistence failure is visible without dirtying repository state,
- snapshot export errors are visible,
- `TODO_V2.md` no longer contains known evidence/checkbox contradictions,
- all required host, sanitizer, browser, contract, and hardware evidence is committed,
- the exact final clean product SHA passes the complete quality gate,
- and the final sign-off can truthfully state that no known silent failure or dangerous fallback remains in the reviewed scope.
