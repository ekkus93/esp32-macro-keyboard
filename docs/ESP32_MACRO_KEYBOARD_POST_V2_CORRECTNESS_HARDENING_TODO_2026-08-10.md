# ESP32 Macro Keyboard — Post-v2 Correctness and Hardening TODO

**Document status:** Authoritative implementation sequence for post-v2 hardening  
**Date:** 2026-08-10  
**Baseline reviewed:** `99b86777d14cbb5b3655e0facad80651d29e8fa0`  
**Companion specification:** `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_SPEC_2026-08-10.md`

## 0. Authority and execution rules

This TODO implements the post-v2 correctness specification above while preserving the product requirements in:

- `docs/SPEC_V2.md`
- `docs/UI_UX_SPEC_V2.md`

The existing `docs/TODO_V2.md` remains the historical v2 implementation/evidence ledger. This TODO is the ordered repair and hardening sequence produced by the post-v2 code review.

### 0.1 Completion rules

- [ ] Work in task order unless a dependency exception is documented in committed evidence.
- [ ] Do not mark a checkbox complete from comments or implementation reports alone; inspect and test the current code.
- [ ] Do not silently change `SPEC_V2.md` or `UI_UX_SPEC_V2.md` to make implementation easier.
- [ ] Do not reintroduce firmware-owned package/macro repositories or v1 compatibility layers.
- [ ] Do not use `catch {}`, ignored return codes, `|| true`, warning suppression, analyzer exclusions, test skips, or lower thresholds to hide failures in the hardened paths.
- [ ] Preserve the primary failure separately from cleanup/release/durability failures.
- [ ] Do not call a partial commit a rollback.
- [ ] Do not claim hardware completion from host/fake tests.
- [ ] Do not check off a compound validation item when any behavior named by the checkbox remains unverified.
- [ ] Every bug fixed from this TODO gets a regression test that fails before the fix and passes after it, unless a hardware-only behavior cannot be reproduced off-device; in that case record the reason and hardware reproduction.
- [ ] Every task that changes runtime behavior gets an implementation/evidence report under `docs/implementation-v2/` or a successor hardening evidence directory before completion.
- [ ] Keep the working tree and exact tested SHA recorded in evidence.

### 0.2 Ralph-loop task discipline

Each task should be small enough to complete, test, document, commit, and validate independently.

For each task:

1. inspect the current source and tests,
2. reproduce or encode the failure,
3. implement the minimum coherent fix,
4. add/adjust tests,
5. run the narrow relevant gate,
6. run sanitizer/static-analysis gates when C/C++ changes,
7. commit implementation and evidence intentionally,
8. only then check off the task.

Do not batch unrelated safety fixes into one opaque commit.

---

## Phase H0 — Baseline, evidence reconciliation, and failure inventory

### H0-001 — Record exact starting state

- [ ] Record current `master` SHA after these two planning documents land.
- [ ] Record working-tree cleanliness.
- [ ] Record ESP-IDF, Node, Python, compiler/static-analysis, browser, and board versions used for this hardening pass.
- [ ] Run and record the current baseline result of:

  ```text
  ./scripts/check-all.sh
  ./scripts/run-tests.sh --sanitizers
  ./scripts/generate-native-coverage.sh
  ```

- [ ] If any baseline gate fails before product changes, diagnose it separately and do not conflate it with a hardening task.

### H0-002 — Correct known `TODO_V2.md` overclaims before new acceptance work

- [ ] Re-read V2-154's compound checkbox for login/logout/idle expiry/absolute expiry/lockout.
- [ ] Uncheck or split it because the current committed evidence explicitly says idle and absolute expiry were not independently hardware-verified.
- [ ] Re-read V2-154's “no secret appears” checkbox.
- [ ] Uncheck or narrow it because the current evidence describes a spot-check rather than a complete audit.
- [ ] Add an explicit note to V2-061/Phase 6 evidence that executor confirmation exists but real HTTP sends do not yet wire the setting end-to-end.
- [ ] Re-evaluate V2-055/V2-154 password-change completion against the stale-RAM-verifier and partial-session-invalidation failure modes.
- [ ] Preserve historical evidence; do not rewrite old reports to pretend they proved more than they did.

### H0-003 — Create a hardening failure matrix

For each reviewed failure, record:

- [ ] initiating operation,
- [ ] primary failure point,
- [ ] durable state already changed or not changed,
- [ ] RAM/runtime state already changed or not changed,
- [ ] cleanup attempted,
- [ ] cleanup failure behavior,
- [ ] externally visible HTTP/UI result,
- [ ] retry semantics,
- [ ] reboot semantics,
- [ ] required regression test.

Include at least:

- [ ] physical-confirmation setting ignored by real send,
- [ ] password durable commit + RAM verifier refresh failure,
- [ ] password durable commit + session invalidation failure,
- [ ] factory reset + session invalidation failure,
- [ ] factory reset + blob deletion failure,
- [ ] factory reset + reboot/power loss between stages,
- [ ] active-send recovery request failure,
- [ ] storage primary failure + cleanup failure,
- [ ] rename success + parent sync failure,
- [ ] async worker unavailable,
- [ ] executor submit failure + release-all failure,
- [ ] package-selection persistence failure,
- [ ] snapshot export failure.

### Phase H0 exit gate

- [ ] `TODO_V2.md` contains no known checkbox/evidence contradiction identified by the review.
- [ ] Baseline gates and exact SHA are committed.
- [ ] Failure matrix is committed and becomes test-planning input for the following phases.

---

## Phase H1 — End-to-end physical confirmation for real sends

### H1-010 — Wire authoritative setting into send acceptance

- [ ] Identify the single authoritative runtime source for `requireSerialConfirmation`.
- [ ] Extend the send adapter/core dependency seam so the accepted request receives the setting without creating a second global source of truth.
- [ ] Set `macro_execution_request_t.require_confirmation` before `macro_executor_submit()`.
- [ ] Capture the setting at acceptance so a later settings change cannot mutate an already accepted send's semantics.
- [ ] Do not weaken behavior when settings read fails; reject the send with a visible backend-unavailable/internal error rather than defaulting confirmation off.

### H1-011 — Expose `awaiting_confirmation` through the v2 send API

- [ ] Ensure `EXECUTION_AWAITING_CONFIRMATION` maps to an explicit API state string.
- [ ] Update frozen/checked-in contract examples only if the authoritative spec already requires the state and the fixture is stale; otherwise flag a spec conflict before changing the contract.
- [ ] Ensure status serialization remains source/key-content redacted.
- [ ] Ensure cancellation is accepted while awaiting confirmation.

### H1-012 — React awaiting-confirmation UX

- [ ] Render a clear “awaiting confirmation” state for active sends.
- [ ] Explain the required physical/serial confirmation action without exposing secrets.
- [ ] Keep Cancel visible and operable.
- [ ] Do not issue a second send POST because of rerender, orientation change, retry, or confirmation-state polling.
- [ ] Preserve active-send state through the landscape blocker.

### H1-013 — Host regression tests

- [ ] Test settings=false -> request enters normal running path.
- [ ] Test settings=true -> request has `require_confirmation=true`.
- [ ] Test settings read failure -> no send accepted.
- [ ] Test no HID press occurs before confirmation.
- [ ] Test confirmation transitions to running exactly once.
- [ ] Test cancel-before-confirmation -> cancelled, types nothing.
- [ ] Test confirmation timeout -> timed_out, types nothing.
- [ ] Test release-all behavior on confirmation-phase terminal paths.
- [ ] Run executor/web host targets under ASan + UBSan.

### H1-014 — Browser regression tests

- [ ] Real/Vitest browser workflow displays awaiting-confirmation.
- [ ] Cancel remains reachable.
- [ ] Confirmed transition does not duplicate POST.
- [ ] Timeout is displayed distinctly from ordinary failure.

### H1-015 — Hardware evidence

- [ ] Enable the confirmation setting on the reference ESP32-S3R8.
- [ ] Capture HID reports proving zero key-down reports before confirmation.
- [ ] Confirm and capture the expected macro reports afterward.
- [ ] Run cancel-before-confirmation and prove zero typed reports.
- [ ] Run expiry/timeout path and prove zero typed reports.
- [ ] Record exact firmware SHA, board, host, settings, commands, and captured output.

### Phase H1 exit gate

- [ ] Real `POST /api/v1/send` honors physical/serial confirmation end-to-end.
- [ ] No tested path bypasses required confirmation when its support subsystem fails.
- [ ] Host, sanitizer, browser, and hardware evidence are committed.

---

## Phase H2 — Password-change atomicity and authentication coherence

### H2-020 — Remove best-effort verifier-cache refresh

- [ ] Delete the correctness dependency on `refresh_password_record_cache()` re-reading NVS after success.
- [ ] Update RAM login verifier directly from the exact credential candidate/material that is durably committed, or centralize auth credential ownership equivalently.
- [ ] Ensure no code path can return `204` while login still authenticates with the old verifier.
- [ ] Securely zero all transient credential material on every return path.

### H2-021 — Define password/session transaction semantics

**Correction (2026-08-10, verified against `web_settings.c:653-665`,
`web_change_password_handle()`):** the code does *not* silently return `204`
when session invalidation fails after the durable password write succeeds.
It already returns `WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE`, mapped to an
error response — the original F-003/H2 framing ("durable password may
change before `logout_all` succeeds" implying a silent-success path) does
not match current behavior and should not be used to look for that specific
symptom. The actual, verified gap is narrower: that error response is
indistinguishable from "nothing happened," while the true state is that the
password *has* durably changed and old sessions remain valid. A caller
cannot tell these apart from the response alone, so retrying, telling the
user "password change failed," or assuming the old password still works are
all wrong reactions to this specific failure.

- [ ] Specify the order/invariant for durable password commit, RAM verifier activation, and all-session invalidation.
- [ ] When session invalidation fails after the durable password commit already succeeded, do not represent the outcome as an ordinary/generic failure — the response must let the caller distinguish "nothing changed" from "password changed, invalidation incomplete."
- [ ] Ensure an error after durable password commit cannot leave the caller unable to determine which password is authoritative.
- [ ] Prefer a coherent transaction/fault state over rollback theater.
- [ ] If an explicit auth fault latch is needed, reject all login attempts until coherent recovery rather than guessing old/new credential state.
- [ ] Ensure reboot recovery has one deterministic authoritative password.

### H2-022 — Failure injection tests

Add tests for at least:

- [ ] password creation failure,
- [ ] settings replace failure,
- [ ] RAM verifier activation failure if any fallible step remains,
- [ ] session invalidation failure **after the durable password write already succeeded** (this is the verified current behavior, not a hypothetical — see H2-021's correction note),
- [ ] cleanup/zeroization paths,
- [ ] retry after each failure.

For every case assert:

- [ ] whether the old password succeeds,
- [ ] whether the new password succeeds,
- [ ] whether old sessions remain valid,
- [ ] exact API outcome, **and that the outcome is distinguishable from a "nothing changed" failure whenever the password durably changed**,
- [ ] no secret appears in diagnostics/log captures.

### H2-023 — Success invariant tests

After a returned `204` assert without reboot:

- [ ] old password fails immediately,
- [ ] new password succeeds immediately,
- [ ] the session used to change the password fails immediately,
- [ ] all other sessions fail immediately,
- [ ] a newly created session uses normal TTL/lockout semantics.

### H2-024 — Hardware validation

- [ ] Repeat successful password change on the reference board.
- [ ] Verify old/new/session behavior immediately without reboot.
- [ ] Verify a power cycle preserves the new password.
- [ ] Re-run PBKDF2 timing sanity check to confirm no unintended cost regression.

### Phase H2 exit gate

- [ ] No best-effort credential cache refresh remains.
- [ ] Returned success always means durable credential, RAM verifier, and session state agree.
- [ ] Injected failures have deterministic semantics.
- [ ] Host/sanitizer/hardware evidence is committed.

---

## Phase H3 — Crash-safe, resumable factory reset

### H3-030 — Design durable reset state

- [ ] Choose and document a minimal durable reset marker/state machine.
- [ ] Ensure the marker is committed before returning an accepted reset response.
- [ ] Define boot behavior for reset-pending/reset-cleanup-required state.
- [ ] Ensure reset mode cannot expose ordinary normal operation with ambiguous state.
- [ ] Keep the mechanism independent from repository semantics; blobs remain opaque firmware storage.

### H3-031 — Make reset stages idempotent

Stages must safely tolerate repetition:

- [ ] credential/settings erase,
- [ ] session invalidation,
- [ ] blob deletion,
- [ ] temporary upload/debris cleanup where applicable,
- [ ] reset-marker clear,
- [ ] restart/re-entry.

### H3-032 — Change accepted/error semantics

- [ ] Do not return `202` unless the durable reset state has been established.
- [ ] Once reset is durably accepted, later cleanup trouble must be represented as reset recovery, not ordinary failed normal operation.
- [ ] Do not clear the reset marker until every required destructive effect succeeds.
- [ ] Make setup/recovery state visible enough for the UI/diagnostics to avoid lying about readiness.

### H3-033 — Failure injection matrix

Inject failure:

- [ ] before marker commit,
- [ ] after marker commit/before settings erase,
- [ ] after settings erase,
- [ ] during session invalidation,
- [ ] during first/middle/final blob deletion,
- [ ] before marker clear,
- [ ] during restart scheduling,
- [ ] across simulated reboot between each stage.

Assert:

- [ ] reset resumes safely,
- [ ] old credentials never become usable again once accepted,
- [ ] old sessions cannot regain authority,
- [ ] surviving blobs are not exposed to a newly provisioned owner before cleanup completes,
- [ ] repeated cleanup is safe,
- [ ] final state is fully unprovisioned with zero repository blobs.

### H3-034 — Reset-settings semantics

- [ ] Re-audit reset-settings for the same partial-session-invalidation pattern.
- [ ] Preserve noncredential settings-reset intent while making session/restart semantics explicit.
- [ ] Add failure tests where invalidation/restart scheduling fails.

### H3-035 — Hardware interruption evidence

- [ ] Run normal factory reset and reprovisioning after the new state machine.
- [ ] Interrupt/reset/power-cycle during cleanup at least once using a controlled test seam or hardware procedure.
- [ ] Prove boot resumes reset rather than presenting ambiguous normal/setup state.
- [ ] Reprovision and prove old blobs are absent.

### Phase H3 exit gate

- [ ] Factory reset is idempotent, restart-safe, and interruption-safe.
- [ ] Partial destructive work can no longer produce ambiguous ordinary operation.
- [ ] Failure-injection and hardware recovery evidence is committed.

---

## Phase H4 — Active-send recovery and cancellation visibility

### H4-040 — Replace nullable recovery with explicit state

- [ ] Change startup/send recovery types to distinguish `none`, `known`, and `unavailable/unknown`.
- [ ] Do not map network, timeout, malformed response, or 5xx failures to `null`.
- [ ] Keep a true 404/no-send result distinct from recovery failure.

### H4-041 — Startup degraded-execution surface

- [ ] Allow repository startup to continue when repository/settings are valid but send recovery fails.
- [ ] Show a prominent execution-state-unavailable warning.
- [ ] Provide Retry.
- [ ] Preserve current working copy, route, dirty state, and selected package.
- [ ] Keep a cancellation/recovery affordance when safe cancellation can be attempted.
- [ ] If Cancel cannot be delivered, state that explicitly.

### H4-042 — Poll freshness/degradation model

For send and USB status polling:

- [ ] retain last known value,
- [ ] track success freshness or bounded consecutive failure state,
- [ ] expose stale/degraded UI after a defined threshold,
- [ ] automatically clear degraded state after successful refresh,
- [ ] avoid live-region announcements on every unchanged successful poll,
- [ ] avoid duplicate send POSTs.

### H4-043 — Regression tests

- [ ] reload while a send is active and first recovery request fails,
- [ ] verify UI does not display “no send” as if confirmed,
- [ ] verify Retry recovers the active send,
- [ ] verify Cancel remains reachable or explicitly unavailable,
- [ ] verify terminal recovery state renders correctly,
- [ ] verify transient one-poll failure does not produce noisy UI,
- [ ] verify persistent failure does become visible,
- [ ] verify recovery clears warning.

### H4-044 — Real browser workflow

- [ ] Add a real-Chrome scenario with an active send and intentionally failed status recovery.
- [ ] Confirm no duplicate POST and no loss of working-copy state.

### Phase H4 exit gate

- [ ] Unknown execution state is never represented as no execution.
- [ ] Cancellation visibility is preserved through reload/recovery failure.
- [ ] Browser tests prove degraded and recovered states.

---

## Phase H5 — Storage error provenance and commit certainty

### H5-050 — Audit all primary/cleanup error replacement patterns

Search and classify at least:

- [ ] atomic write staging cleanup,
- [ ] rename failure cleanup,
- [ ] blob upload temporary-file cleanup,
- [ ] mount/unmount rollback,
- [ ] shutdown/drain cleanup,
- [ ] any `cleanup == NONE ? primary : cleanup` pattern.

### H5-051 — Standardize structured operation results

- [ ] Reuse `app_operation_result` if sufficient; otherwise extend/create the minimal result type.
- [ ] Preserve `primary_error`.
- [ ] Preserve cleanup/release/durability error separately.
- [ ] Represent commit state where needed.
- [ ] Avoid invasive abstraction where a simple existing result structure already fits.

### H5-052 — Fix atomic-write error provenance

- [ ] Write failure remains primary if temporary cleanup also fails.
- [ ] Verify failure remains primary if cleanup also fails.
- [ ] Rename failure remains primary if temporary cleanup also fails.
- [ ] Parent sync failure after rename is represented as commit/durability uncertain rather than ordinary uncommitted failure.

### H5-053 — Define retry/reconciliation semantics for uncertain commit

- [ ] Determine how callers detect whether the canonical blob/settings value exists after a durability-uncertain result.
- [ ] Require refresh/reconcile before blind retry where duplicate creation could occur.
- [ ] Make blob-create retry behavior deterministic.
- [ ] Ensure UI/client does not create duplicate snapshots simply because the final durability acknowledgement was uncertain.

### H5-054 — Storage tests

- [ ] primary write failure + unlink failure -> both retained, primary preserved,
- [ ] verify failure + unlink failure -> both retained,
- [ ] rename failure + unlink failure -> both retained,
- [ ] rename success + parent sync failure -> uncertain commit state,
- [ ] retry/reconcile after uncertain commit does not silently duplicate data,
- [ ] mount rollback preserves initiating mount error and cleanup detail.

### H5-055 — Hardware durability sanity

- [ ] Re-run interrupted upload/power-cycle evidence after storage semantic changes.
- [ ] Confirm no formatting-on-mount-failure regression.
- [ ] Confirm byte identity and blob list behavior remain correct.

### Phase H5 exit gate

- [ ] No reviewed storage path overwrites the initiating error with cleanup failure.
- [ ] Post-rename durability uncertainty has explicit semantics.
- [ ] Retry cannot silently duplicate a snapshot because commit state was ambiguous.

---

## Phase H6 — Async HTTP confirmation fail-closed behavior

### H6-060 — Remove synchronous fallback

- [ ] Delete the fallback that runs confirmation-gated work on the main httpd task when the async worker is unavailable.
- [ ] Return explicit `503 Service Unavailable` or the contract-consistent equivalent.
- [ ] Do not bypass physical confirmation.
- [ ] Do not block the whole server for the confirmation timeout.

### H6-061 — Track async subsystem health

- [ ] Add sanitized health state for worker start/run/stop/queue/completion failures.
- [ ] Keep the first/most useful failure visible.
- [ ] Expose through existing diagnostics only if consistent with the authoritative diagnostics contract; otherwise keep internal/log evidence and flag required contract change.

### H6-062 — Stop ignoring completion results

- [ ] Observe `web_api_handle_call_with_body` result.
- [ ] Observe `httpd_req_async_handler_complete` result.
- [ ] Observe stop-signal result.
- [ ] Preserve socket/request cleanup safety even when an error cannot be returned to the original client.

### H6-063 — Regression tests

- [ ] worker unavailable -> fast visible failure, no synchronous wait,
- [ ] queue failure -> request answered/completed and health degraded,
- [ ] handler failure -> request completion still occurs,
- [ ] async completion failure -> health captures failure,
- [ ] stop while confirmation is pending remains bounded and safe,
- [ ] unrelated status request remains responsive while a confirmation request waits.

### Phase H6 exit gate

- [ ] No async-worker failure path silently degrades to whole-server blocking behavior.
- [ ] Confirmation-gated route failure remains fail-closed.

---

## Phase H7 — Executor/HID release safety

### H7-070 — Eliminate ignored release-all results

- [x] Find every `usb_release_all()`/`ops.usb_release_all()` call.
- [x] Remove `(void)` discards where the result is safety-relevant.
- [x] Preserve primary submit/execution error separately from release error.
- Evidence: implementation commit `e60e9d73c8ea9494957228c3a734f48aeec8566a` observes both submission-cleanup `usb_hid_release_all()` results, preserves the primary execution failure separately from `macro_execution_status_t.release_error`, and adds deterministic layer-release and press-cleanup regressions; formatter follow-up `44fc3f3be622ff49185419cd1de3b7b091a33740` is code-equivalent. Literal `./scripts/run-tests.sh executor` passed 2/2 (`macro_executor`, `executor_health`) against the uploaded matching master snapshot using only a sandbox-local cJSON 1.7.18 development-header/pkg-config shim; no repository file was changed by that shim. Exact-SHA Host run `31464144676`, Browser run `31464144657`, Device Test Build run `31464144658`, and `./scripts/check-all.sh` Quality run `31464144673`, job `93693418731`, passed on validation SHA `576fad519616844fb1d8ef6aa162e5ea6ac80d56`.

### H7-071 — Fault latch unsafe HID state

- [ ] If defensive release-all fails and key release cannot be proven, mark executor/HID unavailable.
- [ ] Reject new sends until a defined recovery/reinitialization path re-establishes ready state.
- [ ] Ensure status/diagnostics exposes sanitized release failure.

### H7-072 — Correct error classification

- [ ] Replace executor-unavailable -> `APP_ERROR_STORAGE_UNAVAILABLE` mappings with an executor/internal-appropriate error.
- [ ] Verify HTTP mapping remains sensible and does not leak internals.

### H7-073 — Tests

- [ ] unlock failure + release-all failure,
- [ ] queue-send failure + release-all failure,
- [ ] mid-action press failure + release-all failure,
- [ ] cancellation/timeout + release-all failure,
- [ ] fault latch rejects subsequent sends,
- [ ] recovery/reinit clears only when safe.

### Phase H7 exit gate

- [ ] Every safety-relevant key-release attempt has an observed result.
- [ ] Failed release cannot be silently followed by accepting new sends.

---

## Phase H8 — Frontend persistence and export failure visibility

### H8-080 — Package-selection persistence warning

Update every selection-changing call site:

- [ ] startup package chooser,
- [ ] first-package flow,
- [ ] package management Open,
- [ ] selected-package deletion resolution,
- [ ] snapshot load/import package resolution.

Required behavior:

- [ ] local package may still open,
- [ ] repository dirty state remains unchanged by selection,
- [ ] persistence failure is visible,
- [ ] warning explains selection may not survive reload,
- [ ] Retry is available where practical,
- [ ] warning clears after successful persistence.

### H8-081 — Snapshot export error handling

- [ ] Catch export/compression/file-save errors.
- [ ] Show `ErrorBanner` or the common equivalent.
- [ ] Always clear busy state.
- [ ] Do not modify repository, dirty flag, selected package, or loaded snapshot association on failure.
- [ ] Avoid unhandled rejected promises from event handlers.

### H8-082 — Frontend tests

- [ ] package selection persists successfully -> no warning,
- [ ] persistence fails -> local open + warning + non-dirty state,
- [ ] retry succeeds -> warning clears,
- [ ] export compression fails -> visible error,
- [ ] save-as-file fails -> visible error,
- [ ] state remains unchanged after export failure.

### Phase H8 exit gate

- [ ] No reviewed frontend persistence/export failure is intentionally invisible.
- [ ] Noncritical local continuation remains possible without falsely implying persistence succeeded.

---

## Phase H9 — Cross-cutting secret, fallback, and regression audit

### H9-090 — Search for silent critical catches/ignored results

Audit production firmware and v2 frontend for:

- [ ] `catch {}`,
- [ ] `.catch(() => {})`,
- [ ] `(void)` on fallible calls,
- [ ] ignored `esp_err_t`/`app_error_code_t`,
- [ ] “best-effort” comments,
- [ ] “fallback” comments,
- [ ] stale-state retry loops,
- [ ] cleanup-result replacement patterns.

For every hit:

- [ ] classify as safe/visible, or
- [ ] fix it, or
- [ ] document why it is intentionally acceptable with tests.

Do not ban best-effort globally; the criterion is whether the ignored failure changes user-visible correctness, safety, security, persistence, or recoverability.

### H9-091 — Complete no-secret audit

Audit:

- [ ] serial logs,
- [ ] firmware logs,
- [ ] HTTP success/error bodies,
- [ ] diagnostics,
- [ ] browser console where applicable,
- [ ] repository export,
- [ ] snapshot export,
- [ ] test failure output.

Secret sentinels must cover:

- [ ] admin password,
- [ ] AP passphrase,
- [ ] setup code,
- [ ] session token,
- [ ] password salt/verifier bytes.

### H9-092 — Strengthen architectural guards

- [ ] Add/extend static checks preventing firmware package/macro repository ownership from returning.
- [ ] Extend browser-storage prohibition scan to every production v2 frontend directory.
- [ ] Add a guard against reintroducing synchronous confirmation wait fallback if practical.
- [ ] Add a guard/test ensuring real send construction considers the confirmation setting.

### Phase H9 exit gate

- [ ] Every production “best-effort”/ignored-error site has been classified.
- [ ] No known critical silent failure from the review remains.
- [ ] Complete secret audit passes with committed evidence.

---

## Phase H10 — Full contract, browser, sanitizer, and device regression pass

### H10-100 — Native/contract gates

- [ ] Run complete host suite.
- [ ] Run complete host suite under ASan + UBSan.
- [ ] Run v2 contract corpus/native checks.
- [ ] Run route-manifest and setup isolation checks.
- [ ] Run static-analysis/clang-tidy with warnings fatal.
- [ ] Run native coverage policy and record exact values.

### H10-101 — Frontend gates

- [ ] `npm ci` from the pinned Node version.
- [ ] format check.
- [ ] typecheck.
- [ ] ESLint with zero warnings.
- [ ] stylelint with zero warnings.
- [ ] Vitest full suite.
- [ ] frontend coverage gate.
- [ ] production build.
- [ ] local-only/static asset checks.
- [ ] real-Chrome full scenario suite including new hardening scenarios.

### H10-102 — Device Unity tests

- [ ] Build `firmware/test_app` from the exact candidate SHA.
- [ ] Flash the reference ESP32-S3R8.
- [ ] Run every Unity test case, not build-only.
- [ ] Record pass/fail/ignored count.
- [ ] Add device-test coverage for new low-level hardening behavior where appropriate and practical.

### H10-103 — Hardware matrix refresh

At minimum revalidate affected areas:

- [ ] Linux HID identity/text/chords/release/cancel,
- [ ] confirmation required/confirm/cancel/timeout,
- [ ] USB disconnect/reconnect,
- [ ] password change,
- [ ] factory reset/recovery,
- [ ] blob add/list/load/delete,
- [ ] interrupted upload/power cycle,
- [ ] AP survival after station failure,
- [ ] bounded reconnect.

Optional unavailable hosts remain honestly recorded:

- [ ] ChromeOS test status recorded without false completion.
- [ ] Windows test status recorded without false completion.

### Phase H10 exit gate

- [ ] All full software gates pass on the same exact candidate SHA.
- [ ] Required affected hardware behaviors are revalidated on the same product line.
- [ ] No evidence file relies on an older SHA for behavior changed by this hardening pass.

---

## Phase H11 — Final `TODO_V2.md` reconciliation and product sign-off

### H11-110 — Re-audit every affected v2 checkbox

At minimum re-audit:

- [ ] V2-055 settings/password/device actions,
- [ ] V2-061 confirmation,
- [ ] V2-062 release-all,
- [ ] V2-074 selection persistence semantics,
- [ ] V2-075 send recovery/helper semantics,
- [ ] V2-082 startup send recovery,
- [ ] V2-116 export/import,
- [ ] V2-153 reset/power matrix,
- [ ] V2-154 auth/network/no-secret matrix,
- [ ] Phase 4/5/6/7/8/11/15 exit gates affected by these behaviors,
- [ ] final sign-off checklist.

### H11-111 — Literal evidence audit

For every checked affected item:

- [ ] every named behavior is independently proven,
- [ ] hardware wording has hardware evidence,
- [ ] spot-check wording is not called a full audit,
- [ ] host fake behavior is not called real-httpd/device behavior,
- [ ] exact evidence SHA is present,
- [ ] evidence file exists at the referenced path.

### H11-112 — Documentation synchronization

- [ ] Update current implementation/status documentation to describe the hardened semantics.
- [ ] Document factory-reset recovery behavior.
- [ ] Document password-change guarantees.
- [ ] Document confirmation-required send behavior.
- [ ] Document active-send degraded recovery behavior.
- [ ] Document storage commit-uncertain behavior if externally relevant.
- [ ] Remove stale “best-effort” comments that no longer describe implementation.

### Phase H11 exit gate

- [ ] `docs/TODO_V2.md` and implementation evidence agree literally.
- [ ] No affected requirement is checked solely because its implementation exists in isolation.
- [ ] Product documentation describes actual current behavior.

---

## Phase H12 — Final clean-checkout release gate

### H12-120 — Clean checkout

- [ ] Create a fresh checkout of the exact final candidate SHA.
- [ ] Install dependencies only through documented reproducible commands.
- [ ] Confirm no generated/untracked source artifact is required for success.

### H12-121 — Run complete authoritative gate

From the clean checkout:

```text
./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
```

- [ ] All commands exit 0.
- [ ] No warnings are ignored or downgraded.
- [ ] Record timings and key counts/budget margins.

### H12-122 — Final hardware confirmation on exact release SHA

- [ ] Build production firmware from the same exact SHA.
- [ ] Flash reference device.
- [ ] Verify version/commit diagnostics identify the exact SHA and clean build.
- [ ] Perform a bounded final smoke sequence: login, active send, confirmation-required send, cancel, snapshot save/load, password change, restart, factory reset/reprovision.
- [ ] Confirm no production test image remains flashed at sign-off.

### H12-123 — Final post-v2 acceptance statement

Only check this when truthful:

- [ ] No known critical silent failure remains in password, reset, send recovery, HID cleanup, storage commit, or confirmation paths.
- [ ] No known dangerous fallback remains in the reviewed scope.
- [ ] No known secret leak remains in the reviewed scope.
- [ ] No automatic snapshot creation/deletion was introduced.
- [ ] No firmware package/macro repository ownership was reintroduced.
- [ ] Cancellation remains accessible while a send is active or execution state is temporarily unknown.
- [ ] Every required hardware item has real hardware evidence or is explicitly recorded as unavailable/open.
- [ ] `TODO_V2.md` no longer overclaims the reviewed validation items.
- [ ] Exact final SHA passes the complete clean-checkout gate.

---

## Final completion gate

The post-v2 hardening program is complete only when:

- [ ] all H0-H12 exit gates are complete,
- [ ] all P0/P1 findings from the post-v2 review are fixed rather than merely documented,
- [ ] all regression tests are permanent and wired into authoritative gates,
- [ ] all affected hardware evidence is committed,
- [ ] `docs/TODO_V2.md` is reconciled honestly,
- [ ] the exact final product SHA passes clean software and device validation,
- [ ] and the product owner can review the final evidence without relying on silent fallback assumptions.
