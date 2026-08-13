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

- [x] Record current `master` SHA after these two planning documents land.
- [x] Record working-tree cleanliness.
- [x] Record ESP-IDF, Node, Python, compiler/static-analysis, browser, and board versions used for this hardening pass.
- [x] Run and record the current baseline result of:

  ```text
  ./scripts/check-all.sh
  ./scripts/run-tests.sh --sanitizers
  ./scripts/generate-native-coverage.sh
  ```

- [x] If any baseline gate fails before product changes, diagnose it separately and do not conflate it with a hardening task.

- Evidence: historical baseline/failure matrix plus current-master command attempts and local 60/60 normal + 60/60 ASan/UBSan results are recorded in `docs/implementation-v2/H0_POST_V2_HARDENING_RECONCILIATION_2026-08-11.md`.

### H0-002 — Correct known `TODO_V2.md` overclaims before new acceptance work

**Reconciliation note (2026-08-11):** two planning assumptions in this task were
superseded by later completed work before H0 itself was formally closed. H9 now
provides a complete no-secret audit, so V2-154's no-secret item is retained as
checked with H9 evidence rather than being mechanically unchecked. H9 also
wired authoritative `requireSerialConfirmation` into real send construction, so
V2-061/Phase-6 wording is updated to current truth while H1 remains open for the
full end-to-end acceptance matrix. See
`docs/implementation-v2/H0_POST_V2_HARDENING_RECONCILIATION_2026-08-11.md`.

- [x] Re-read V2-154's compound checkbox for login/logout/idle expiry/absolute expiry/lockout.
- [x] Uncheck or split it because the current committed evidence explicitly says idle and absolute expiry were not independently hardware-verified.
- [x] Re-read V2-154's “no secret appears” checkbox.
- [x] Reconcile the spot-check-only no-secret evidence. H9 later completed the full audit, so retain the item as checked only with the stronger H9 evidence rather than the original spot-check alone.
- [x] Reconcile V2-061/Phase 6 confirmation evidence. The original H0 plan identified missing real-send wiring; H9 later added that wiring, so record the current implementation while leaving H1 end-to-end acceptance open.
- [x] Re-evaluate V2-055/V2-154 password-change completion against the stale-RAM-verifier and partial-session-invalidation failure modes.
- [x] Preserve historical evidence; do not rewrite old reports to pretend they proved more than they did.

- Evidence: `docs/TODO_V2.md` now contains the literal H0 reconciliation changes; historical evidence remains unmodified. Details are recorded in the H0 reconciliation report above.

### H0-003 — Create a hardening failure matrix

For each reviewed failure, record:

- [x] initiating operation,
- [x] primary failure point,
- [x] durable state already changed or not changed,
- [x] RAM/runtime state already changed or not changed,
- [x] cleanup attempted,
- [x] cleanup failure behavior,
- [x] externally visible HTTP/UI result,
- [x] retry semantics,
- [x] reboot semantics,
- [x] required regression test.

Include at least:

- [x] physical-confirmation setting ignored by real send,
- [x] password durable commit + RAM verifier refresh failure,
- [x] password durable commit + session invalidation failure,
- [x] factory reset + session invalidation failure,
- [x] factory reset + blob deletion failure,
- [x] factory reset + reboot/power loss between stages,
- [x] active-send recovery request failure,
- [x] storage primary failure + cleanup failure,
- [x] rename success + parent sync failure,
- [x] async worker unavailable,
- [x] executor submit failure + release-all failure,
- [x] package-selection persistence failure,
- [x] snapshot export failure.

- Evidence: the original 13-row failure matrix remains in the historical H0 report; the reconciliation report adds a current disposition table mapping each row to H1-H8 status without erasing the original finding.

### Phase H0 exit gate

- [x] `TODO_V2.md` contains no known checkbox/evidence contradiction identified by the review.
- [x] Baseline gates and exact SHA are committed.
- [x] Failure matrix is committed and becomes test-planning input for the following phases.

- Evidence: H0 closeout report: `docs/implementation-v2/H0_POST_V2_HARDENING_RECONCILIATION_2026-08-11.md`. Baseline command failures caused solely by unavailable sandbox tooling are recorded explicitly; no CI job was monitored for this closeout.

---

## Phase H1 — End-to-end physical confirmation for real sends

### H1-010 — Wire authoritative setting into send acceptance

- [x] Identify the single authoritative runtime source for `requireSerialConfirmation`.
- [x] Extend the send adapter/core dependency seam so the accepted request receives the setting without creating a second global source of truth.
- [x] Set `macro_execution_request_t.require_confirmation` before `macro_executor_submit()`.
- [x] Capture the setting at acceptance so a later settings change cannot mutate an already accepted send's semantics.
- [x] Do not weaken behavior when settings read fails; reject the send with a visible backend-unavailable/internal error rather than defaulting confirmation off.

- Evidence: H9 supplied the authoritative device-settings read/fail-closed seam; H1 adds explicit capture-by-value regression coverage. See `docs/implementation-v2/H1_END_TO_END_PHYSICAL_CONFIRMATION_2026-08-11.md`.

### H1-011 — Expose `awaiting_confirmation` through the v2 send API

- [x] Ensure `EXECUTION_AWAITING_CONFIRMATION` maps to an explicit API state string.
- [x] Update frozen/checked-in contract examples only if the authoritative spec already requires the state and the fixture is stale; otherwise flag a spec conflict before changing the contract.
- [x] Ensure status serialization remains source/key-content redacted.
- [x] Ensure cancellation is accepted while awaiting confirmation.

- Evidence: H1 fixes POST/GET serialization to `awaiting_confirmation`, preserves the valid confirmation-disabled canonical fixture, and adds host handler regressions for POST/GET/DELETE while awaiting. See the H1 evidence report.

### H1-012 — React awaiting-confirmation UX

- [x] Render a clear “awaiting confirmation” state for active sends.
- [x] Explain the required physical/serial confirmation action without exposing secrets.
- [x] Keep Cancel visible and operable.
- [x] Do not issue a second send POST because of rerender, orientation change, retry, or confirmation-state polling.
- [x] Preserve active-send state through the landscape blocker.

- Evidence: the existing inline send-state design is retained; H1 makes the serial `confirm` action explicit and adds no-repost/cancel assertions. Actual pinned-Node/real-Chrome execution remains separately open under H1-014 rather than being inferred from source inspection.

### H1-013 — Host regression tests

- [x] Test settings=false -> request enters normal running path.
- [x] Test settings=true -> request has `require_confirmation=true`.
- [x] Test settings read failure -> no send accepted.
- [x] Test no HID press occurs before confirmation.
- [x] Test confirmation transitions to running exactly once.
- [x] Test cancel-before-confirmation -> cancelled, types nothing.
- [x] Test confirmation timeout -> timed_out, types nothing.
- [x] Test release-all behavior on confirmation-phase terminal paths.
- [x] Run executor/web host targets under ASan + UBSan.

- Evidence: local H1 validation passed executor **3/3**, web **30/30**, full host **61/61**, and the same full **61/61** under ASan+UBSan. Confirmation terminal-path release assertions were strengthened. See the H1 evidence report.

### H1-014 — Browser regression tests

- [x] Real/Vitest browser workflow displays awaiting-confirmation.
- [x] Cancel remains reachable.
- [x] Confirmed transition does not duplicate POST.
- [x] Timeout is displayed distinctly from ordinary failure.

- Evidence (2026-08-13): H10-101 executed the current frontend under pinned
  Node.js 24.18.0 and the permanent real-Chrome suite. The Macros/Quick Send
  scenarios exercise `awaiting_confirmation`, keep Cancel available, verify the
  confirmation transition without a duplicate send POST, and render timeout
  separately from ordinary failure. Exact frontend-validation candidate:
  `d440be6c26174a26b5b62748161f59d8aa5c18c1`; Quality run `31675479517`,
  job `94369022215`; full mapping:
  `docs/implementation-v2/H10_101_FRONTEND_VALIDATION_2026-08-13.md`.

### H1-015 — Hardware evidence

- [ ] Enable the confirmation setting on the reference ESP32-S3R8.
- [ ] Capture HID reports proving zero key-down reports before confirmation.
- [ ] Confirm and capture the expected macro reports afterward.
- [ ] Run cancel-before-confirmation and prove zero typed reports.
- [ ] Run expiry/timeout path and prove zero typed reports.
- [ ] Record exact firmware SHA, board, host, settings, commands, and captured output.

- Evidence status: `tests/hardware/test_send_confirmation.py` now provides the exact-SHA acceptance procedure, including the real 60-second timeout and setting restoration, but it has not been run on the reference board in this sandbox.

### Phase H1 exit gate

- [ ] Real `POST /api/v1/send` honors physical/serial confirmation end-to-end.
- [x] No tested path bypasses required confirmation when its support subsystem fails.
- [ ] Host, sanitizer, browser, and hardware evidence are committed.

- Evidence status: software/native fail-closed behavior is proven locally; final end-to-end/browser/hardware claims remain open until H1-014/H1-015 evidence is supplied on an exact candidate SHA.

---

## Phase H2 — Password-change atomicity and authentication coherence

### H2-020 — Remove best-effort verifier-cache refresh

- [x] Delete the correctness dependency on `refresh_password_record_cache()` re-reading NVS after success.
- [x] Update RAM login verifier directly from the exact credential candidate/material that is durably committed, or centralize auth credential ownership equivalently.
- [x] Ensure no code path can return `204` while login still authenticates with the old verifier.
- [x] Securely zero all transient credential material on every return path.

### H2-021 — Define password/session transaction semantics

- Evidence (2026-08-11): `c9351d3ba2c862d50dd46c0b6f1827a3b3f40d92` establishes gate -> read/verify/create -> durable commit -> direct RAM activation -> session invalidation. Login fails closed while the gate is active; no separate auth fault latch is needed because durable and RAM credential authority cannot diverge. Post-commit invalidation failure is `409 auth_state_incomplete`, which explicitly names the new password as authoritative. Full evidence: `docs/implementation-v2/H2_PASSWORD_CHANGE_ATOMICITY_2026-08-11.md`.

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

- [x] Specify the order/invariant for durable password commit, RAM verifier activation, and all-session invalidation.
- [x] When session invalidation fails after the durable password commit already succeeded, do not represent the outcome as an ordinary/generic failure — the response must let the caller distinguish "nothing changed" from "password changed, invalidation incomplete."
- [x] Ensure an error after durable password commit cannot leave the caller unable to determine which password is authoritative.
- [x] Prefer a coherent transaction/fault state over rollback theater.
- [x] If an explicit auth fault latch is needed, reject all login attempts until coherent recovery rather than guessing old/new credential state.
- [x] Ensure reboot recovery has one deterministic authoritative password.

### H2-022 — Failure injection tests

- Evidence (2026-08-11): permanent host tests inject password-material creation, durable replace, transition-busy, and post-commit session-invalidation faults, plus cleanup/zeroization and retry semantics. RAM activation is deliberately an infallible bounded copy from the exact committed candidate, so no fallible activation step remains to inject. Focused local web/auth suites passed 30/30 and 5/5 normally and under ASan+UBSan.

Add tests for at least:

- [x] password creation failure,
- [x] settings replace failure,
- [x] RAM verifier activation failure if any fallible step remains,
- [x] session invalidation failure **after the durable password write already succeeded** (this is the verified current behavior, not a hypothetical — see H2-021's correction note),
- [x] cleanup/zeroization paths,
- [x] retry after each failure.

For every case assert:

- [x] whether the old password succeeds,
- [x] whether the new password succeeds,
- [x] whether old sessions remain valid,
- [x] exact API outcome, **and that the outcome is distinguishable from a "nothing changed" failure whenever the password durably changed**,
- [x] no secret appears in diagnostics/log captures.

### H2-023 — Success invariant tests

- Evidence (2026-08-11): the real-auth-core integration test proves old password rejection, immediate new-password acceptance, invalidation of both pre-existing sessions, normal idle/absolute TTL, and the normal five-failure/300-second login lockout after the success path.

After a returned `204` assert without reboot:

- [x] old password fails immediately,
- [x] new password succeeds immediately,
- [x] the session used to change the password fails immediately,
- [x] all other sessions fail immediately,
- [x] a newly created session uses normal TTL/lockout semantics.

### H2-024 — Hardware validation

- Evidence status: still open. Reference-board immediate auth/session behavior, power-cycle persistence, and PBKDF2 timing sanity require the physical ESP32-S3R8 and are not inferred from host tests.

- [ ] Repeat successful password change on the reference board.
- [ ] Verify old/new/session behavior immediately without reboot.
- [ ] Verify a power cycle preserves the new password.
- [ ] Re-run PBKDF2 timing sanity check to confirm no unintended cost regression.

### Phase H2 exit gate

- Evidence status: software transaction semantics, failure injection, and sanitizer evidence are complete; the final hardware-dependent evidence item remains open until H2-024 is executed.

- [x] No best-effort credential cache refresh remains.
- [x] Returned success always means durable credential, RAM verifier, and session state agree.
- [x] Injected failures have deterministic semantics.
- [ ] Host/sanitizer/hardware evidence is committed.

---

## Phase H3 — Crash-safe, resumable factory reset

### H3-030 — Design durable reset state

- Evidence (2026-08-11): H3-030 uses a dedicated two-state NVS journal (`NONE`/`PENDING`) in namespace `reset_journal`, key `factory_reset`. `PENDING` is committed before any destructive factory-reset effect; marker commit failure is nondestructive. Pending/corrupt/unreadable journal state blocks boot before device settings, storage, auth, USB, executor, controls, Wi-Fi, or HTTP startup. The journal component has no repository/storage dependency. See `docs/implementation-v2/H3_030_DURABLE_FACTORY_RESET_STATE_2026-08-11.md`.

- [x] Choose and document a minimal durable reset marker/state machine.
- [x] Ensure the marker is committed before returning an accepted reset response.
- [x] Define boot behavior for reset-pending/reset-cleanup-required state.
- [x] Ensure reset mode cannot expose ordinary normal operation with ambiguous state.
- [x] Keep the mechanism independent from repository semantics; blobs remain opaque firmware storage.

### H3-031 — Make reset stages idempotent

Stages must safely tolerate repetition:

- [x] credential/settings erase,
- [x] session invalidation,
- [x] blob deletion,
- [x] temporary upload/debris cleanup where applicable,
- [x] reset-marker clear,
- [x] restart/re-entry.

- Evidence (2026-08-12): every destructive/reset-reentry stage now converges under replay; pending boot recovery repeats settings erase and storage cleanup before setup, canonical upload debris is included, bulk blob unlink is parent-synced, and restart scheduling is latched. See `docs/implementation-v2/H3_031_IDEMPOTENT_FACTORY_RESET_STAGES_2026-08-12.md`.

### H3-032 — Change accepted/error semantics

- [x] Do not return `202` unless the durable reset state has been established.
- [x] Once reset is durably accepted, later cleanup trouble must be represented as reset recovery, not ordinary failed normal operation.
- [x] Do not clear the reset marker until every required destructive effect succeeds.
- [x] Make setup/recovery state visible enough for the UI/diagnostics to avoid lying about readiness.

- Evidence (2026-08-12): factory reset now distinguishes pre-marker rejection from post-marker recovery ownership with a structured device-controls outcome. Only pre-commit failure is mapped as an ordinary error; every post-commit cleanup failure remains `202 Accepted` with the unchanged authoritative v2 response while retaining the exact primary failure internally and leaving `PENDING` for reboot recovery. The complete `202` response is allocated/serialized before the destructive backend call, and status/diagnostics fail visibly with reset-recovery-required while `PENDING` is durable so reconnect logic cannot mistake the brief pre-reboot window for readiness. See `docs/implementation-v2/H3_032_FACTORY_RESET_ACCEPTED_ERROR_SEMANTICS_2026-08-12.md`.

### H3-033 — Failure injection matrix

Inject failure:

- [x] before marker commit,
- [x] after marker commit/before settings erase,
- [x] after settings erase,
- [x] during session invalidation,
- [x] during first/middle/final blob deletion,
- [x] before marker clear,
- [x] during restart scheduling,
- [x] across simulated reboot between each stage.

Assert:

- [x] reset resumes safely,
- [x] old credentials never become usable again once accepted,
- [x] old sessions cannot regain authority,
- [x] surviving blobs are not exposed to a newly provisioned owner before cleanup completes,
- [x] repeated cleanup is safe,
- [x] final state is fully unprovisioned with zero repository blobs.

- Evidence (2026-08-12): deterministic host fault injection covers the durable marker boundary, every destructive/recovery stage, first/middle/final blob unlink failure, restart-timer fallback, and reboot cuts between stages. H3-033 also closes the pre-reboot authority gap found during audit: every provisioned `/api/v1` route fails closed while the reset journal is `PENDING`, so stale credentials, sessions, and surviving blobs cannot regain authority before cleanup completes. See `docs/implementation-v2/H3_033_FACTORY_RESET_FAILURE_INJECTION_MATRIX_2026-08-12.md`.

### H3-034 — Reset-settings semantics

- [x] Re-audit reset-settings for the same partial-session-invalidation pattern.
- [x] Preserve noncredential settings-reset intent while making session/restart semantics explicit.
- [x] Add failure tests where invalidation/restart scheduling fails.
- Evidence (2026-08-12): reset-settings now distinguishes precommit failure from durably-applied partial completion, retains session and restart failures separately, prebuilds the exact SPEC 202 response before mutation, and fails normal API authority closed until reboot. A session-invalidation failure remains accepted only when reboot ownership is established; restart-ownership failure is an explicit 409 `reset_settings_incomplete`. AP/admin credentials, provisioning state, and repository blobs remain preserved. See `docs/implementation-v2/H3_034_RESET_SETTINGS_SEMANTICS_2026-08-12.md`.

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

- [x] Delete the fallback that runs confirmation-gated work on the main httpd task when the async worker is unavailable.
- [x] Return explicit `503 Service Unavailable` or the contract-consistent equivalent.
- [x] Do not bypass physical confirmation.
- [x] Do not block the whole server for the confirmation timeout.
- Evidence: implementation commit `30301a89cef655c9bf6420c1192c19bdb2f3a09c`; `./scripts/run-tests.sh web` passed 28/28 and `./scripts/run-tests.sh --sanitizers web` passed 28/28 in targeted run `31530721738`, job `93909820931`. Worker-unavailable confirmation-gated routes now return `503 Service Unavailable` without invoking the confirmation wait, protected handler, or restart path; a non-confirmation settings route remains usable. Full evidence: `docs/implementation-v2/H6_060_ASYNC_CONFIRMATION_FAIL_CLOSED_2026-08-11.md`.

### H6-061 — Track async subsystem health

- [x] Add sanitized health state for worker start/run/stop/queue/completion failures.
- [x] Keep the first/most useful failure visible.
- [x] Expose through existing diagnostics only if consistent with the authoritative diagnostics contract; otherwise keep internal/log evidence and flag required contract change.
- Evidence: implementation commit `8f4ccfae3abf3803f70fc487cb6471039d9d13ab` adds synchronized first-failure async HTTP health metadata for worker start/run/stop, queue, and completion failures. The frozen diagnostics schema remains unchanged: async failure degrades the existing `http` subsystem entry rather than adding fields/subsystems. `./scripts/run-tests.sh web` passed 28/28 and `./scripts/run-tests.sh --sanitizers web` passed 28/28 in targeted run `31533545046`, job `93919101444`. Full evidence: `docs/implementation-v2/H6_061_ASYNC_HTTP_HEALTH_2026-08-11.md`.

### H6-062 — Stop ignoring completion results

- [x] Observe `web_api_handle_call_with_body` result.
- [x] Observe `httpd_req_async_handler_complete` result.
- [x] Observe stop-signal result.
- [x] Preserve socket/request cleanup safety even when an error cannot be returned to the original client.
- Evidence: runtime observation landed with H6-061 commit `8f4ccfae3abf3803f70fc487cb6471039d9d13ab`; worker-capable cleanup proof `f20b470bb914d3b06bae259a45fb5311dea70cc6` executes the real async worker under handler, completion, queue-send, and stop-signal faults. `./scripts/run-tests.sh web` and `./scripts/run-tests.sh --sanitizers web` passed in targeted run `31534770326`. Full evidence: `docs/implementation-v2/H6_062_ASYNC_COMPLETION_RESULTS_2026-08-11.md`.

### H6-063 — Regression tests

- [x] worker unavailable -> fast visible failure, no synchronous wait,
- [x] queue failure -> request answered/completed and health degraded,
- [x] handler failure -> request completion still occurs,
- [x] async completion failure -> health captures failure,
- [x] stop while confirmation is pending remains bounded and safe,
- [x] unrelated status request remains responsive while a confirmation request waits.
- Evidence: H6-060 commit `30301a89cef655c9bf6420c1192c19bdb2f3a09c` proves worker-unavailable fail-closed behavior; H6-062 worker seam `f20b470bb914d3b06bae259a45fb5311dea70cc6` proves queue/handler/completion cases; `4e0355f33cb3ea025dbb6525479dc541f7e4af84` adds deterministic pending-confirmation stop safety and real status-handler responsiveness. `./scripts/run-tests.sh web` and `./scripts/run-tests.sh --sanitizers web` both passed 29/29 in targeted run `31536438755`. Full evidence: `docs/implementation-v2/H6_063_ASYNC_REGRESSION_MATRIX_2026-08-11.md`.

### Phase H6 exit gate

- [x] No async-worker failure path silently degrades to whole-server blocking behavior.
- [x] Confirmation-gated route failure remains fail-closed.
- Evidence: H6-060 through H6-063 collectively cover worker absence, queue/handler/completion/stop faults, request cleanup, bounded pending-confirmation shutdown, and unrelated status responsiveness; see `docs/implementation-v2/H6_060_ASYNC_CONFIRMATION_FAIL_CLOSED_2026-08-11.md` through `H6_063_ASYNC_REGRESSION_MATRIX_2026-08-11.md`.

---

## Phase H7 — Executor/HID release safety

### H7-070 — Eliminate ignored release-all results

- [x] Find every `usb_release_all()`/`ops.usb_release_all()` call.
- [x] Remove `(void)` discards where the result is safety-relevant.
- [x] Preserve primary submit/execution error separately from release error.
- Evidence: implementation commit `e60e9d73c8ea9494957228c3a734f48aeec8566a` observes both submission-cleanup `usb_hid_release_all()` results, preserves the primary execution failure separately from `macro_execution_status_t.release_error`, and adds deterministic layer-release and press-cleanup regressions; formatter follow-up `44fc3f3be622ff49185419cd1de3b7b091a33740` is code-equivalent. Literal `./scripts/run-tests.sh executor` passed 2/2 (`macro_executor`, `executor_health`) against the uploaded matching master snapshot using only a sandbox-local cJSON 1.7.18 development-header/pkg-config shim; no repository file was changed by that shim. Exact-SHA Host run `31464144676`, Browser run `31464144657`, Device Test Build run `31464144658`, and `./scripts/check-all.sh` Quality run `31464144673`, job `93693418731`, passed on validation SHA `576fad519616844fb1d8ef6aa162e5ea6ac80d56`.

### H7-071 — Fault latch unsafe HID state

- [x] If defensive release-all fails and key release cannot be proven, mark executor/HID unavailable.
- [x] Reject new sends until a defined recovery/reinitialization path re-establishes ready state.
- [x] Ensure status/diagnostics exposes sanitized release failure.
- Evidence: core latch/recovery behavior is implemented by `48d7714d9f48621e1876c4ef3d434826542c6710`; `5d5f042a3567476fe54e4e623462037819ea25da` closes the visibility gap by allowing an unavailable executor status with a retained release fault to use the existing sanitized `releaseError` wire field while unrelated unavailable states still fail closed. Live diagnostics regression also proves a recorded release cleanup fault degrades the existing `executor` subsystem without changing the frozen schema. Executor 2/2 and web 29/29 passed in normal and ASan+UBSan runs in targeted workflow `31537774806`. Full evidence: `docs/implementation-v2/H7_071_HID_RELEASE_FAULT_LATCH_2026-08-11.md`.

### H7-072 — Correct error classification

- [x] Replace executor-unavailable -> `APP_ERROR_STORAGE_UNAVAILABLE` mappings with an executor/internal-appropriate error.
- [x] Verify HTTP mapping remains sensible and does not leak internals.
- Evidence: production correction landed in `48d7714d9f48621e1876c4ef3d434826542c6710`; `db5ebc475be8a21a9f18932fdbd5827b706a9a70` adds explicit web-core/live-route regression coverage for POST/GET/DELETE unavailable-executor mappings. Executor 2/2 and web 29/29 passed in normal and ASan+UBSan runs in targeted workflow `31538431440`; source guard confirms no `APP_ERROR_STORAGE_UNAVAILABLE` remains in `firmware/components/macro_executor`. Full evidence: `docs/implementation-v2/H7_072_EXECUTOR_ERROR_CLASSIFICATION_2026-08-11.md`.

### H7-073 — Tests

- [x] unlock failure + release-all failure,
- [x] queue-send failure + release-all failure,
- [x] mid-action press failure + release-all failure,
- [x] cancellation/timeout + release-all failure,
- [x] fault latch rejects subsequent sends,
- [x] recovery/reinit clears only when safe.
- Evidence: all six cases were already implemented in the current executor regression suite; targeted workflow `31538914176` verified the source-to-runner mapping and passed `./scripts/run-tests.sh executor` 2/2 plus `./scripts/run-tests.sh --sanitizers executor` 2/2 under ASan+UBSan on validation SHA `896ddce7173e83f73a0113fd6ba2a16cf45039c1`. Full mapping: `docs/implementation-v2/H7_073_EXECUTOR_RELEASE_FAULT_TESTS_2026-08-11.md`.

### Phase H7 exit gate

- [x] Every safety-relevant key-release attempt has an observed result.
- [x] Failed release cannot be silently followed by accepting new sends.
- Evidence: current-source call-graph audit at `62d2969b17bc44d090982e64e77e88156bbf9ad0` confirms all executor action, terminal, submission-cleanup, concrete USB adapter, and executor-deinit release results are consumed rather than discarded. Active executor release failure atomically latches the engine unavailable before a later submit can transfer ownership; H7-073 regressions prove subsequent sends are rejected and recovery requires reinit plus USB readiness. `./scripts/run-tests.sh executor` and `./scripts/run-tests.sh --sanitizers executor` both passed 2/2 in targeted run `31538914176` on behavior-test SHA `896ddce7173e83f73a0113fd6ba2a16cf45039c1`; comparison through the audited source SHA contains no production/test change in this scope. Full evidence: `docs/implementation-v2/H7_PHASE_EXIT_RELEASE_SAFETY_2026-08-11.md` (evidence commit `cc682530fcf0a320d95802bf45769e058f79f9d2`).

---

## Phase H8 — Frontend persistence and export failure visibility

### H8-080 — Package-selection persistence warning

Update every selection-changing call site:

- [x] startup package chooser,
- [x] first-package flow,
- [x] package management Open,
- [x] selected-package deletion resolution,
- [x] snapshot load/import package resolution.

Required behavior:

- [x] local package may still open,
- [x] repository dirty state remains unchanged by selection,
- [x] persistence failure is visible,
- [x] warning explains selection may not survive reload,
- [x] Retry is available where practical,
- [x] warning clears after successful persistence.
- Evidence: existing implementation `1445ae6f35502ece15824a04805d050e7d7baa4f` provides explicit persistence outcomes, visible common warning, local continuation, and Retry. Reconciliation commit `4c2eab2d2c06609c4862fb4da82c8359de7f9045` fixes snapshot load/import to resolve from the last successfully persisted package ID rather than transient local selection, preventing false warning clearance without a successful settings write. Full evidence: `docs/implementation-v2/H8_FRONTEND_PERSISTENCE_EXPORT_VISIBILITY_2026-08-11.md`.

### H8-081 — Snapshot export error handling

- [x] Catch export/compression/file-save errors.
- [x] Show `ErrorBanner` or the common equivalent.
- [x] Always clear busy state.
- [x] Do not modify repository, dirty flag, selected package, or loaded snapshot association on failure.
- [x] Avoid unhandled rejected promises from event handlers.
- Evidence: `SnapshotsPage.exportWorkingCopy()` contains export/compression and file-save failure inside `try/catch/finally`, publishes the common `ErrorBanner`, and always clears busy. Permanent regressions prove dirty/clean state, repository identity, selection, and loaded-snapshot association are unchanged by export failure. Full evidence: `docs/implementation-v2/H8_FRONTEND_PERSISTENCE_EXPORT_VISIBILITY_2026-08-11.md`.

### H8-082 — Frontend tests

- [x] package selection persists successfully -> no warning,
- [x] persistence fails -> local open + warning + non-dirty state,
- [x] retry succeeds -> warning clears,
- [x] export compression fails -> visible error,
- [x] save-as-file fails -> visible error,
- [x] state remains unchanged after export failure.
- Evidence: targeted Node 24.18.0 workflow `31542963700`, job `93949257708`, passed format, typecheck, ESLint, stylelint, **46/46 Vitest files and 517/517 tests**, coverage (**87.43% statements / 83.35% branches / 91.43% functions / 87.51% lines**), production build, and `git diff --check`. H8 snapshot page itself passed 41 tests. Full matrix: `docs/implementation-v2/H8_FRONTEND_PERSISTENCE_EXPORT_VISIBILITY_2026-08-11.md`.

### Phase H8 exit gate

- [x] No reviewed frontend persistence/export failure is intentionally invisible.
- [x] Noncritical local continuation remains possible without falsely implying persistence succeeded.
- Evidence: complete H8 call-site/export audit plus the durable-vs-local snapshot-selection correction and permanent regressions above. The later H10 real-Chrome/final-candidate gate remains separate and is not claimed here.

---

## Phase H9 — Cross-cutting secret, fallback, and regression audit

### H9-090 — Search for silent critical catches/ignored results

Audit production firmware and v2 frontend for:

- [x] `catch {}`,
- [x] `.catch(() => {})`,
- [x] `(void)` on fallible calls,
- [x] ignored `esp_err_t`/`app_error_code_t`,
- [x] “best-effort” comments,
- [x] “fallback” comments,
- [x] stale-state retry loops,
- [x] cleanup-result replacement patterns.

For every hit:

- [x] classify as safe/visible, or
- [x] fix it, or
- [x] document why it is intentionally acceptable with tests.

Do not ban best-effort globally; the criterion is whether the ignored failure changes user-visible correctness, safety, security, persistence, or recoverability.

- Evidence: initial mechanical audit `31544238722` / `93953189964`, H9 correction `f36b48eef170b84085f1a978b25fb8c14de99574`, final audit `31546340375` / `93959559165`, and subsequent Ralph regression-hardening correction `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d`. Final inventory remains zero empty catches, zero empty Promise catches, and zero best-effort markers; every actual fallible discard/fallback/retry/cleanup class is fixed or explicitly classified in `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_2026-08-11.md`. The additive regression-control correction is recorded in `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_RALPH_CORRECTION_2026-08-11.md`. H5 remains open for generalized storage primary/cleanup provenance where failures are visible but exact provenance can still be compressed.

### H9-091 — Complete no-secret audit

Audit:

- [x] serial logs,
- [x] firmware logs,
- [x] HTTP success/error bodies,
- [x] diagnostics,
- [x] browser console where applicable,
- [x] repository export,
- [x] snapshot export,
- [x] test failure output.

Secret sentinels must cover:

- [x] admin password,
- [x] AP passphrase,
- [x] setup code,
- [x] session token,
- [x] password salt/verifier bytes.
- Evidence: H9 removes setup-code value logging, strengthens the firmware credential-output checker, prohibits production-V2 browser-console output, and redacts compared values from shared host-test failure helpers. Ralph correction `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d` closes dynamic-format, alias, stdio, and generic-assertion disclosure gaps; follow-up `ccedef8965cd249a03e212d1ed02ffed0860ff12` adds lower-level ESP-IDF/ROM logging sinks. The credential checker regression suite now covers **16 cases**, and the executable assertion-redaction regression passes **3/3**. Existing exact diagnostics/HTTP allowlists and typed repository/snapshot boundaries complete the surface audit. Session tokens remain intentionally confined to cookie transport and are not reflected into JSON/log/diagnostic/export output. Historical surface×sentinel matrix: `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_2026-08-11.md`; additive correction: `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_RALPH_CORRECTION_2026-08-11.md`.

### H9-092 — Strengthen architectural guards

- [x] Add/extend static checks preventing firmware package/macro repository ownership from returning.
- [x] Extend browser-storage prohibition scan to every production v2 frontend directory.
- [x] Add a guard against reintroducing synchronous confirmation wait fallback if practical.
- [x] Add a guard/test ensuring real send construction considers the confirmation setting.
- Evidence: Phase-2 ownership guard remains exhaustive over firmware components/main; the frontend scan now covers `AppV2.tsx`, all `src/v2`, and every `src/features/**/v2`; `scripts/check-h9-architecture.py` rejects synchronous worker-unavailable confirmation fallback and reintroduction of generic host assertion expression stringification; and real send construction reads authoritative device settings, binds `require_serial_confirmation`, and fails closed if policy read fails. Original host regressions passed in `31546096618` / `93958810321`; Ralph correction `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d` adds the stricter committed regression controls documented in `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_RALPH_CORRECTION_2026-08-11.md`.

### Phase H9 exit gate

- [x] Every production “best-effort”/ignored-error site has been classified.
- [x] No known critical silent failure from the review remains.
- [x] Complete secret audit passes with committed evidence.
- Evidence: historical closeout `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_2026-08-11.md`; original validated H9 product SHA `f36b48eef170b84085f1a978b25fb8c14de99574`; final mechanical audit `31546340375` / `93959559165`; regression-hardening SHA `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d`, lower-level sink-hardening SHA `ccedef8965cd249a03e212d1ed02ffed0860ff12`, and additive evidence `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_RALPH_CORRECTION_2026-08-11.md`. Local revalidation from the user-provided master archive passed H9 architecture/secret guards, credential regressions **16/16**, assertion-redaction regressions **3/3**, web **29/29**, startup **2/2**, and both web/startup labels under ASan+UBSan.

---

## Phase H10 — Full contract, browser, sanitizer, and device regression pass

### H10-100 — Native/contract gates

- [x] Run complete host suite.
- [x] Run complete host suite under ASan + UBSan.
- [x] Run v2 contract corpus/native checks.
- [x] Run route-manifest and setup isolation checks.
- [x] Run static-analysis/clang-tidy with warnings fatal.
- [x] Run native coverage policy and record exact values.

- Evidence: exact validation checkpoint `910a8fd461fc8f9079cc99ffa450c1c4f76589eb`; Host Tests run `31671332886` passed normal host, ASan+UBSan, and pinned `gcovr==8.6` Native Coverage. Coverage-instrumented host tests passed **66/66**; the policy-enforced set measured **96.2% lines (2566/2668)**, **100.0% functions (227/227)**, and **82.8% branches (1870/2259)**. Quality run `31671332867` passed H10-100's static-analysis policy, setup/route synchronization, native contracts (**6/6**), production and device-test firmware build plus fatal first-party clang-tidy gates, then continued through stack/release/frontend/browser gates before failing later on `shfmt` diffs in two script regression tests. That later script-format failure is outside H10-100 and does not claim the H10 phase exit gate. See `docs/implementation-v2/H10_100_NATIVE_CONTRACT_VALIDATION_2026-08-12.md` and `docs/implementation-v2/H10_100_AUTHORITATIVE_REVALIDATION_2026-08-12.md`.

### H10-101 — Frontend gates

- [x] `npm ci` from the pinned Node version.
- [x] format check.
- [x] typecheck.
- [x] ESLint with zero warnings.
- [x] stylelint with zero warnings.
- [x] Vitest full suite.
- [x] frontend coverage gate.
- [x] production build.
- [x] local-only/static asset checks.
- [x] real-Chrome full scenario suite including new hardening scenarios.

- Evidence: exact frontend-validation candidate `d440be6c26174a26b5b62748161f59d8aa5c18c1`; permanent Quality run `31675479517`, job `94369022215`, executed `scripts/check-webapp.sh` under pinned Node.js **24.18.0** and completed every H10-101 gate before the aggregate workflow later failed in the separate `check-scripts.sh` stage on pre-existing `shfmt` diffs. Vitest passed **47/47 files and 528/528 tests**; coverage measured **87.47% statements / 83.35% branches / 91.43% functions / 87.55% lines**; the production build and all seven reported real-Chrome scenario groups passed. Full evidence: `docs/implementation-v2/H10_101_FRONTEND_VALIDATION_2026-08-13.md`. The Phase H10 exit gate remains open because H10-102/H10-103 require physical-device evidence and the complete aggregate software gate has not yet exited 0 on this candidate.

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

- [x] V2-055 settings/password/device actions,
- [x] V2-061 confirmation,
- [x] V2-062 release-all,
- [x] V2-074 selection persistence semantics,
- [x] V2-075 send recovery/helper semantics,
- [x] V2-082 startup send recovery,
- [x] V2-116 export/import,
- [x] V2-153 reset/power matrix,
- [x] V2-154 auth/network/no-secret matrix,
- [x] Phase 4/5/6/7/8/11/15 exit gates affected by these behaviors,
- [x] final sign-off checklist.

- Evidence (2026-08-13): `docs/TODO_V2.md` was re-audited against current
  implementation/evidence at product audit basis
  `cc9e05727a2767f070dc79e9e699146e10509b34`. The audit preserves valid core
  V2 checkmarks, adds scope notes where later H1-H9 hardening narrows their
  meaning, and reopens stale final-release/current-hardware claims: V2-150's
  aggregate `check-all.sh`, all current V2-151 Unity execution items, V2-153's
  post-H3 factory-reset/reprovisioning evidence, V2-154's post-H2
  password-change/PBKDF2 compound item, and the final clean-checkout
  `check-all.sh` sign-off. H1-014 is also closed from exact H10-101 executable
  browser evidence. Hardware-required reruns remain explicitly open/deferred.
  Full disposition:
  `docs/implementation-v2/H11_110_V2_CHECKBOX_REAUDIT_2026-08-13.md`.

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
