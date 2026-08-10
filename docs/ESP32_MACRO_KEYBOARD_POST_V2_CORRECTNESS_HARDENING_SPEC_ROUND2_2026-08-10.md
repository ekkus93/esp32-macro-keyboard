# ESP32 Macro Keyboard — Post-v2 Correctness and Hardening Specification, Round 2

**Document status:** Implementation specification
**Date:** 2026-08-10
**Repository:** `ekkus93/esp32-macro-keyboard`
**Baseline reviewed:** `44488753c9f4dc50d27cd4fefb4b21060c9c3948`
**Applies to:** v2 firmware, v2 React application, tests, release evidence, and `docs/TODO_V2.md`
**Companion documents:**

- `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_SPEC_2026-08-10.md` ("Round 1") — unchanged, still authoritative for F-001 through F-013
- `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md` ("Round 1 TODO") — unchanged
- `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_ROUND2_2026-08-10.md` — this document's implementation sequence

## 1. Purpose and relationship to Round 1

At the time this document was written, Round 1's implementation work (Phases H1–H12) had not started — only Round 1's Phase H0 (baseline documentation and failure-matrix recording) was complete, and even that was incomplete by its own stated exit criteria (`docs/TODO_V2.md` was never edited to match Round 1's H0-002 reconciliation decisions; the current-SHA `check-all.sh`/`--sanitizers`/coverage baseline was never recorded).

A full code review was then performed across `firmware/components/web_server/`, `firmware/components/auth/`, `firmware/components/device_controls/`, `firmware/components/macro_executor/`, `firmware/components/storage/`, and the relevant `webapp/src/` surfaces. That review confirmed twelve of Round 1's thirteen findings (F-001–F-002, F-004–F-013) exactly as described, found one (F-003) to be a mischaracterization of the actual bug (already corrected in the Round 1 TODO's H2-021/H2-022 by commit `78c356f`), and — separately — found a further set of genuine defects that Round 1 never examined, because Round 1's review scope did not extend to these files/paths.

This document specifies those additional findings. It does **not** restate, relitigate, or duplicate Round 1's F-001–F-013; implementing this document's findings has no dependency on Round 1's phases being complete first, except where explicitly noted in §9.

This document is a **post-v2 correctness specification**, exactly as Round 1 is. It may clarify implementation and failure semantics needed to satisfy existing v2 intent, but it must not silently redefine product behavior. The product requirements remain defined by `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md`, both frozen — changes to either require the project owner's explicit per-change permission, propose-then-apply, never silently. If this document's required outcomes cannot be reached without new content in those frozen files, implementation must stop at that conflict and the authoritative specification must be reconciled explicitly first. §9 identifies the one finding in this round where that applies.

## 2. Non-goals

This work must not:

- reintroduce firmware-owned package or macro repositories,
- move repository semantics back into firmware,
- add compatibility shims for retired v1 behavior,
- weaken gzip-only repository persistence,
- create a second execution engine or a second authentication/session implementation,
- redesign the UI merely for cosmetic reasons,
- relitigate or re-verify Round 1's F-001–F-013 (already verified against source; see the Round 1 documents),
- or expand scope into files/paths this document does not name without documenting why.

## 3. Findings

Each finding was verified directly against source at the baseline SHA above, with file:line citations. "Required outcome" states the correctness invariant that must hold once fixed — it does not mandate a specific implementation shape unless stated.

### F-014 — Unsynchronized data race on the RAM login password record

`firmware/components/web_server/web_api_administration.c:302-313` (`refresh_password_record_cache()`) mutates the global `server_configuration.password_record` (salt, hash, iteration count) via three separate, non-atomic field writes, holding no lock. `firmware/components/web_server/web_server_login.c:172-173` reads that same global directly during login verification, also without a lock.

`WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD` is one of the routes `web_api_physical_confirmation_required()` (`web_api_core.c:236-238`) gates. When physical confirmation is enabled — the intended secure default — a change-password request runs on the dedicated async confirmation worker task specifically so other requests continue being served on the main httpd task while it waits (`web_server_async.c`'s own header comment states this is the reason the worker exists). This is exactly the condition under which a concurrent `POST /api/v1/auth/login` on the httpd task can read `password_record` mid-update, observing a torn combination of old/new salt, hash, and iteration count.

This is not credibly an authentication bypass — a constant-time compare against a torn record simply fails — but it is a real unsynchronized-memory bug. It can produce spurious login failures in the window immediately following a password change, and it is structurally invisible to the existing host test suite, whose fakes are single-threaded.

**Required outcome:** every write to `server_configuration.password_record` and every read of it for login verification must be mutually exclusive (e.g. a mutex/critical section scoped to the record, or a copy-and-atomic-swap of the whole record). The chosen mechanism must not reintroduce blocking on the httpd task for the confirmation-wait duration (that would reopen F-008/Round-1-H6 territory) — only the record access itself needs to be serialized, not the whole request.

### F-015 — Passwords left unwiped in heap after parsing

The codebase has an established, consistently-applied convention (`web_auth_routes.c:134-136`'s `web_auth_login_parse`; `web_server_setup_submit.c`/`web_setup_json.c`'s `wipe_json_tree()`) of scrubbing cJSON's *parsed string copies* — not just the original request buffer — before deleting the parse tree, because cJSON's parser allocates new heap copies of string values that a buffer-only wipe never reaches.

Two mutation handlers do not follow this convention:

- `firmware/components/web_server/web_settings.c`'s `web_change_password_handle()`: `current_password_view`/`new_password_view` point into the parsed `root`; every `cJSON_Delete(root)` call in this function (lines 605, 612, 618, 627, 632, 639) frees without zeroing first.
- `firmware/components/web_server/web_device_actions.c`'s `web_device_factory_reset_handle()`: the parsed `password` (admin password) view points into `root`; `cJSON_Delete(root)` at line 253 frees without zeroing first.

The current/new administrator password and the factory-reset admin password are the two most sensitive strings this firmware ever handles, and they are the two paths where this discipline currently lapses.

**Required outcome:** both handlers must securely zero every parsed string value derived from the request body (mirroring `wipe_json_tree()`'s traversal or an equivalent scoped to these two smaller trees) before `cJSON_Delete()` frees the tree, on every return path including early-error returns.

### F-016 — Storage can silently commit a blob it just reported as failed

`firmware/components/storage/storage_blob_upload_core.c`'s `storage_blob_upload_commit_with_ops()` sets `upload->committed = true` (line 191) *before* checking whether the post-rename `operations->sync_parent(...)` call (line 193) succeeds. If `sync_parent` fails, the function returns an error with `*out_entry` left zeroed — but `upload->committed` is already `true`.

The wrapper one layer up, `firmware/components/storage/storage_blob_upload.c`'s `storage_blob_upload_commit()`, checks exactly that flag to decide whether to advance the live inventory:

```c
if (upload != NULL && !was_committed && upload->committed) {
    storage_blob_record_committed_entry(&committed);
}
```

This is `true` even on the `sync_parent` failure path, so `storage_blob_record_committed_entry()` (`storage_blob_core.c:102-112`) advances `next_id`/`valid_count`/`max_id` for a blob the caller was just told failed to commit. Tracing the only real caller, `web_server_blob.c`'s `blob_create_handler` (lines 317-344): on this error it calls `abort_uncommitted_upload()`, which correctly no-ops (since `committed` is already true), then returns an HTTP error to the client with no blob ID.

Net effect: the client is told the upload failed; the blob is nonetheless fully persisted on flash, permanently, and already counted in the server's live listing and quota accounting. This silently wastes repository storage capacity, and if the client reasonably retries after being told the operation failed, the waste doubles per retry. This is a data/inventory integrity defect, not merely an error-code-provenance issue — and `storage_blob_upload.c` (the wrapper where the defect lives) is entirely excluded from the host test build (`tests/host/CMakeLists.txt` links only `storage_blob_upload_core.c`), so it has zero automated coverage today.

**Required outcome:** a blob whose `sync_parent` step fails after rename must not be silently counted as committed in the live inventory while simultaneously being reported to the client as a failure. This finding is a concrete, load-bearing instance of Round-1 F-007's "post-rename parent-sync failure creates ambiguous commit state" — implementing a fix here should use whatever commit-uncertain mechanism Round 1's H5 phase establishes for exactly this class of failure, once that mechanism exists (see §9). Do not implement a narrower, one-off fix for this call site alone if H5's general mechanism is available or in progress; if H5 has not started, a minimal interim fix (e.g., only advance inventory state after `sync_parent` succeeds, treating the sync-failure case as commit-uncertain-and-not-yet-visible rather than committed) is acceptable, provided it does not contradict whatever H5 later establishes.

### F-017 — Executor shutdown can get permanently stuck

`firmware/components/macro_executor/macro_executor.c:236-276` (`macro_executor_deinit()`) sets `shutting_down = true` unconditionally at entry. It is only cleared (line 274) on a fully successful shutdown path. If the worker task does not confirm exit within `SHUTDOWN_WAIT_MS` (2000 ms), the function returns early at line 246 without clearing the flag. From that point forward, `macro_executor_submit()` (lines 279-281) unconditionally returns `APP_ERROR_CONFLICT` for every subsequent call, with no code path back to a working state short of a fully successful retry of `deinit()` — an undocumented recovery requirement.

This function is excluded from the host build entirely (`macro_executor.c`'s entry points are FreeRTOS-backed and, per the test suite's own comment, "neither host-linkable"), so this path has zero automated coverage. In the current call graph, `macro_executor_deinit()` has exactly one caller — `app_core.c`'s boot-time rollback path, invoked only before `web_server_start()` — so a live submit/deinit race is not reachable today. The module itself, however, provides no guard against this becoming reachable if that call graph ever changes, and the stuck-forever behavior itself is a real defect independent of current reachability.

**Required outcome:** either (a) `macro_executor_deinit()` must not leave `shutting_down` permanently `true` after a failed/timed-out shutdown — define and implement a bounded recovery path — or (b) if leaving it latched is intentional as a fail-safe, that must be an explicit, tested, documented design decision (with a way to observe/diagnose the latched state), not a silent side effect of an early return.

### F-018 — Send-status polling is never stopped for a self-initiated or 409-recovered send

`webapp/src/features/macros/v2/MacrosPage.tsx` has three code paths that can start send-status tracking: the `initialSend` recovery effect (lines 560-593), `recoverActiveSend()` (lines 526-551, invoked from `startSend`'s `409` catch), and `startSend` itself (lines 610-625). Only the first path's `useEffect` ever registers a cleanup function (`stopActiveTracking`), and only when it actually starts tracking — when `initialSend` is `null` (the ordinary case: no send was recovered on mount), the effect returns with no cleanup registered at all.

So for an ordinary Quick Send, or a send recovered via the 409 conflict path, nothing ever calls `.stop()` on the tracker `sendClient.ts`'s `createSendTracker` returns. If the user navigates to another route (nothing currently blocks navigation while a send is active) before the send reaches a terminal state, the tracker's internal `setTimeout` poll loop keeps issuing `GET /api/v1/send` once a second indefinitely against an unmounted component, invoking `setLifecycle`/`setReleaseError` state setters that no longer do anything useful. This is a genuine unbounded timer/network-polling leak, not merely a benign "setState on unmounted component" warning — the polling itself never stops. Not covered by `webapp/tests/v2-macros-page.test.tsx`'s existing unmount checks, which verify warning-free unmounts but not that tracking was actually stopped for a self-initiated send.

**Required outcome:** every code path in `MacrosPage.tsx` that starts a send tracker must have a corresponding stop, triggered at minimum on unmount, so no send-status poll loop can outlive the component that started it.

### F-019 — Settings identity form silently discards unsaved edits on a sibling form's submission

`webapp/src/features/settings/v2/SettingsPage.tsx:141-147`'s `IdentityForm` resyncs its local field state from the single `settings` object (lifted to `AppV2.tsx`, replaced wholesale on every successful `applyUpdate` anywhere in `SettingsPage`) via a `useEffect` keyed on `[settings]`. Because the Access Point form and the Station form live in the same `SettingsPage` and also call `applyUpdate` → `onSettingsChanged(newSettings)`, submitting either of those forms while a user has unsaved edits in the Identity form's fields (device name, retention target, preview-visibility toggle) causes those unsaved edits to be silently overwritten the instant the sibling form's response arrives — with no warning, no confirmation, no way to recover the lost input. No test in `v2-settings-page.test.tsx` exercises a concurrent-edit-across-forms scenario.

**Required outcome:** editing one settings form must not silently discard unsaved, uncommitted edits in a different settings form on the same page. Acceptable designs include (not exclusive): scoping each form's local edit state so it only resyncs from the fields it actually owns rather than the whole `settings` object, or detecting a pending local edit and preserving/warning before an external update would overwrite it.

### F-020 — Send-status polling treats every failure as identically, indefinitely transient

`webapp/src/v2/sendClient.ts:115-125`'s poll loop:

```ts
try {
  status = await v2GetJson(sendPath, isSendStatusResponse);
} catch {
  schedulePoll();
  return;
}
```

catches and reschedules on every exception uniformly — a transient network blip, a malformed response body, and a persistent, permanent 5xx are all treated identically, forever, with no backoff and no give-up bound. (A `401` is intercepted earlier inside `v2GetJson` via `notifyUnauthorized()`, so session expiry is handled separately from this path.) Contrast with `webapp/src/features/settings/v2/useDeviceReconnect.ts`'s deliberate `isTransientFailure()`, which distinguishes retriable failures from ones that should surface to the user. If `/api/v1/send` starts failing hard mid-send, the UI is stuck showing an active/sending state forever, with `onComplete` never firing and no user-visible signal that anything is wrong.

**Required outcome:** this is directly related to, but distinct from, Round 1's F-010/H4-042 (stale/degraded polling state) — that finding is about the UI failing to *show* degradation; this one is about the poll loop having no internal concept of "this has failed enough times to stop treating it as transient." Whatever H4-042 implements for stale/degraded UI state should be built on top of a poll loop that itself distinguishes bounded transient retry from a failure worth surfacing, rather than retrying an unbounded number of times with no internal signal at all. If H4-042 is implemented first, this finding may already be resolved as a side effect — check before implementing a separate fix.

### F-021 — Two more storage cleanup-overwrites-primary-error sites

Beyond the sites Round 1's F-006 already cites, two additional concrete instances exist in `firmware/components/storage/storage_atomic.c`:

- Lines 162-165 (`stage_temporary_file`, on write/sync/verify failure): `return cleanup_result == APP_ERROR_NONE ? result : cleanup_result;` — if the `unlink()` cleanup of the `.tmp` file also fails, the caller never learns the original write/verify failure, only the unlink failure.
- Lines 188-193 (`storage_atomic_write_with_ops_and_parent_sync`, on rename failure): the identical pattern — a failed cleanup-unlink after a failed `rename()` replaces the rename error with the unlink error.

**Required outcome:** same as Round 1's F-006 — these two sites should be fixed as part of whatever structured primary/cleanup error mechanism Round 1's H5 phase establishes, not as a one-off.

### F-022 — Dead code with misleading test coverage

`firmware/components/web_server/web_setup_core.c` and `web_setup_json.c` are not referenced in `firmware/components/web_server/CMakeLists.txt`'s `SRCS` list — confirmed by direct grep — and so are never compiled into the shipped firmware. They implement a complete, superseded v1-era setup flow (distinct field names: `administratorPassword`, `requirePhysicalConfirmation`, `alwaysSelectPackage`; a distinct `provisioning_config_t` type), which predates and was superseded by the real, wired-in flow in `web_server_setup_submit.c` (per git history: these files landed in `43b674c`/`ca5ed86`, superseded by `88d8099`, "V2-040 Cutover B: implement transactional POST /api/v1/setup").

`tests/host/test_web_setup.c` and `tests/host/test_web_setup_json.c` still compile and pass against this dead code, which gives a false impression that a live, tested setup-code/password code path exists here. It does not — the real flow is tested elsewhere, against `web_server_setup_submit.c`.

**Required outcome:** either delete `web_setup_core.c`/`web_setup_json.c` and their corresponding host tests, or add an explicit, prominent comment at the top of both files stating they are intentionally retained-but-unused and why, so a future reader (or auditor) does not mistake passing tests here for coverage of the real setup flow.

### F-023 — Duplicated, hand-synchronized HTTP routing pipelines

`web_server_lifecycle.c`'s `normal_routes[]` registers exact-match `httpd_uri_t` handlers for status/limits/login/logout/blob/send ahead of the generic `/api/v1/*` wildcard that reaches `api_handler()` → `web_api_dispatch()` → `web_api_handle_administration()`. That dispatch switch (`web_api_administration.c:463-489`) has no case for any of those routes — it silently 404s (`default: return APP_ERROR_NOT_FOUND`) for all of them. Correctness today depends entirely on the exact-match table staying registered ahead of the wildcard, an invariant nothing in the build (including `check-v2-api-routes.py`, which validates the contract-vs-header shape but not this registration ordering) enforces.

This duplication is also the direct cause of a concrete inconsistency: `web_server_blob.c`'s `establish_request_id()` validates `X-Request-ID` *before* session authentication, while the generic pipeline's `web_request_policy.c` validates it *after* session authentication (`enforce_session` runs before `establish_request_id` there). A client sending both an invalid session cookie and a malformed request-ID header gets a different error depending purely on which of the two pipelines happens to handle the route.

**Required outcome:** at minimum, add a build-time or test-time check that the exact-match route table and the wildcard dispatch switch cannot silently diverge (e.g., a test asserting every route in `normal_routes[]` is either dispatched there or has no matching case in `web_api_handle_administration()`, so an accidental future removal from the exact-match table doesn't silently 404 a route in production). Reconciling the `X-Request-ID`-vs-auth-ordering inconsistency itself is optional but recommended if the check above is added, since it would otherwise start failing tests written to assert consistent ordering.

### F-024 — Unsynchronized health/diagnostics globals

`firmware/components/macro_executor/executor_health.c:12` and `firmware/components/storage/storage_health.c:12` both hold plain, unsynchronized global structs, written from `app_core.c` (init/deinit adapters) and read from `web_server_diagnostics.c` on the HTTP server task — with no lock, in a codebase that otherwise locks state far less consequential than this consistently (e.g. `macro_executor_engine_t.status` via `lock_engine`/`unlock_engine` around every access in the very same component).

**Required outcome:** bring these two health-state globals under the same synchronization discipline the rest of the codebase already applies to comparable shared state, or document explicitly why it's safe to omit (e.g., if writes are provably confined to a single-threaded boot/shutdown window that can never overlap a live HTTP read — if that's the actual justification, it should be a comment, not silence).

### F-025 — Release-all failure during submission cleanup never reaches published status

Sharper than Round 1's F-009 (which already covers the `(void)`-discarded return value): `macro_executor_engine.c`'s two submission-failure cleanup paths (lines 275, 281) call `reset_terminal_flags()` after the discarded `usb_release_all()` call, and `reset_terminal_flags()` (lines 89-98) only clears `busy`/`cancellation_requested`/`confirmed_requested` — it never touches `engine->status`. So even if F-009's discarded-return-value issue is fixed to *capture* the release result, there is currently no code path that *publishes* it: `macro_execution_status_t.release_error`, which `macro_executor_get_status()` returns to callers, is simply left at whatever the previous execution set it to.

**Required outcome:** when Round 1's F-009/H7-070 is implemented, ensure the captured release-all result from these two submission-cleanup paths is actually surfaced through `macro_executor_get_status()`'s `release_error` field (or an equivalent visible to callers), not merely captured into a local variable that nothing reads afterward.

## 4. Minor/quality cluster (not independently gated, batch-fixable)

These were found during the same review but are lower-severity and do not each warrant a dedicated phase:

- `web_server_static.c`'s `static_handler`: a dead conditional where both branches of an `if (fclose(...) != 0)` return the same value.
- `device_controls_logic.c`: a defensive branch (lines 396-399) that is provably unreachable (confirmed via the committed coverage report showing 0 hits on its true branch) — misleading to a reader, though harmless.
- `web_api_core.c`'s `web_api_route_requires_worker()`: a permanent no-op (`(void)route; return false;`), making its one call site's `X || false` vacuous.
- `web_server_common.c`'s `web_server_get_header()` returns the same error for "header absent" and "header present but too long for the buffer"; `web_request_policy.c`'s `establish_request_id()` treats both identically as "absent" and silently generates a replacement ID, while a header that's present-but-malformed (bad characters) is correctly rejected two lines later — an oversized-but-otherwise-valid request ID is silently discarded rather than treated consistently with other malformed input.
- `web_server_api.c`'s `restart_after_response()` fires an immediate `esp_restart()` for `DEVICE_RESTART`/`DEVICE_FACTORY_RESET` but not `DEVICE_RESET_SETTINGS`, even though all three schedule an internal delayed restart and all three report `connectionWillClose: true`. Not a correctness bug (the device restarts either way via the internal timer) — just undocumented asymmetry worth a comment.
- Three near-identical file-save helper implementations duplicated across `webapp/src/features/settings/v2/SettingsPage.tsx`, `webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, and `webapp/src/features/settings/v2/DiagnosticsPage.tsx`.

**Required outcome:** fix or document each; batch these into one focused task rather than one per item.

## 5. Required regression-test strategy

Every finding above gets a regression test proving the bug before the fix and the correct behavior after, following the same discipline Round 1 §16 and the Round 1 TODO's completion rules already establish:

- F-014 needs a test that can detect a torn read under concurrent access — if the host fake environment cannot exercise real concurrency, this may require either a targeted ASan/TSan-style stress test or documented hardware reproduction; do not claim this fixed from a single-threaded host test alone.
- F-016 needs a test exercising `storage_blob_upload.c`'s wrapper specifically (currently untested), not just `storage_blob_upload_core.c`'s already-tested inner function — the defect lives in the wrapper's `committed` check, which the existing `storage_blob_upload_core.c` test cannot see.
- F-017 needs either a host-linkable extraction of the relevant logic, or documented hardware/manual reproduction, given `macro_executor.c` is currently excluded from the host build entirely.
- F-018/F-019/F-020 need Vitest coverage exercising the actual React component lifecycle (mount, sibling-form-submit, unmount) rather than testing the underlying client functions in isolation, since the defects are specifically in how the components wire those functions to lifecycle events.
- F-021/F-023/F-024/F-025 follow the same host-test patterns already established for their respective components.
- F-022's "fix" (deletion or documentation) has no regression test in the traditional sense; its own removal (or the added comment) is the artifact.

## 6. CI and quality gates

Unchanged from Round 1 §17: all existing quality gates remain authoritative and must not be weakened. No new analyzer exclusion, ignored exit code, `|| true`, warning suppression, test skip, or lowered coverage threshold may be added merely to make this round's work pass.

## 7. Implementation order

Recommended priority, independent of Round 1's own ordering (these findings do not block, and are not blocked by, Round 1's phases except where §9 notes otherwise):

1. F-014 (password-record race) — security-adjacent, highest severity.
2. F-016 (orphaned blob commit) — real data-integrity defect with ongoing storage-quota cost every time it triggers.
3. F-015 (unwiped passwords) — small, contained, high-value fix.
4. F-018 (unbounded send-status polling leak) — real resource leak.
5. F-019 (settings form data loss) — real user-visible data-loss bug.
6. F-017 (stuck executor shutdown) — not currently reachable in production, but a real latent defect.
7. F-020, F-021, F-024, F-025 — internal correctness/consistency work.
8. F-022, F-023, §4's minor cluster — cleanup and architectural-guard work.

## 8. Forbidden implementations

The following are specifically prohibited unless the authoritative specification is intentionally changed:

- fixing F-014 by adding a lock that can be held across the confirmation-wait duration (this would reopen Round-1 F-008/H6 territory),
- fixing F-016 by simply making `storage_blob_upload_commit()`'s inventory update unconditional regardless of `sync_parent` result (that is the bug, not a fix),
- fixing F-017 by silently retrying `deinit()` internally without the caller being able to observe that the first attempt failed,
- fixing F-018 by adding a global "stop all trackers on any navigation" that would also incorrectly stop a legitimately-continuing recovered-send poll a different screen still needs,
- deleting `web_setup_core.c`/`web_setup_json.c` (F-022) without first confirming via `check-all.sh`/`check-firmware.sh`/full host suite that nothing else references them,
- or claiming F-014's concurrency fix verified from host tests alone, given the host fake environment is single-threaded.

## 9. Frozen-specification gate

**F-016 is the one finding in this round requiring coordination with Round 1's frozen-spec gate.** F-016 is a concrete manifestation of the same underlying gap as Round 1's F-007, which Round 1's H5 phase addresses. Per the Round 1 spec-todo review (and the project owner's decision to gate those items explicitly), Round 1's H5 phase requires a new, externally-visible "commit-uncertain" storage error code — content not currently in the frozen `docs/SPEC_V2.md` — and needs the project owner's explicit sign-off on the exact spec diff before that work proceeds.

F-016 should be fixed using whatever mechanism H5 establishes once H5's frozen-spec question is resolved. If H5 has not been reached yet when F-016 is prioritized, an interim, narrower fix is acceptable (see F-016's "required outcome" above) — but it must not preempt or contradict whatever general commit-uncertain mechanism H5 later establishes, and must not itself require new frozen-spec content (i.e., the interim fix should stay internal — correcting the inventory-accounting bug — without introducing a new externally-visible response shape of its own).

No other finding in this round (F-014, F-015, F-017 through F-025, §4's cluster) requires any change to `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md` — all are internal engineering/correctness fixes with no externally-visible contract change.

## 10. Final acceptance criteria

This round is complete only when:

- the RAM password-verifier record cannot be read torn during a concurrent login (F-014),
- no administrator or factory-reset password remains unwiped in heap after its request completes (F-015),
- a blob that fails to fully commit is never simultaneously reported as failed to the client and counted as live in server inventory (F-016), or an interim fix per §9 is in place pending H5,
- a failed executor shutdown cannot silently and permanently block all future sends without an observable, documented state (F-017),
- no send-status poll loop can outlive the component that started it (F-018),
- editing one settings form cannot silently discard unsaved edits in a different settings form (F-019),
- the send-status poll loop distinguishes bounded transient failure from a failure worth surfacing, or F-020 is confirmed resolved as a side effect of Round 1's H4-042 (F-020),
- the storage cleanup-overwrites-primary-error pattern is fixed at the two newly-identified sites, using the same mechanism as Round 1's F-006 (F-021),
- `web_setup_core.c`/`web_setup_json.c` are either removed or explicitly documented as intentionally dead (F-022),
- the exact-match/wildcard routing duplication cannot silently diverge without a test catching it (F-023),
- `executor_health.c`/`storage_health.c` global state is synchronized or the omission is explicitly justified (F-024),
- a captured submission-cleanup release-all failure is actually surfaced through `macro_executor_get_status()` (F-025),
- the minor/quality cluster in §4 is fixed or documented,
- all required regression tests per §5 are committed and wired into authoritative gates,
- and the exact final SHA for this round's work passes the complete quality gate per §6.
