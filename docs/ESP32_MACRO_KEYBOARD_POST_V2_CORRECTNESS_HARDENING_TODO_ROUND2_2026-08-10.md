# ESP32 Macro Keyboard — Post-v2 Correctness and Hardening TODO, Round 2

**Document status:** Implementation sequence
**Date:** 2026-08-10
**Governing spec:** `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_SPEC_ROUND2_2026-08-10.md`
**Companion (unchanged, separate work):** `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md` ("Round 1 TODO", phases H0–H12)

## 0. How to use this document

### 0.1 Task discipline

Work one task at a time, in phase order unless a task's "Depends on" line says otherwise. A task is not implemented until:

1. the code change is made and matches the finding's "Required outcome" in the Round 2 spec exactly — not a superset, not a narrower partial fix, unless the finding explicitly allows an interim fix (only F-016 does, see R2-021),
2. a regression test exists proving the bug before the fix and correct behavior after (or, where the spec explicitly says host-test coverage isn't achievable, documented hardware/manual reproduction is attached instead — never silently skipped),
3. `./scripts/check-all.sh` passes clean (exit 0) on the resulting tree,
4. and the task's checkbox in this document is updated with a one-line evidence citation: commit SHA, the exact command run, and the result. Do not check a box on the basis of "should work" or a partial run.

Do not batch unrelated findings into one commit. Each finding gets its own commit (§4's minor cluster may be one commit covering multiple sub-bullets, since the spec explicitly allows batching there).

### 0.2 Completion rule

A phase is complete only when every task in it is checked with evidence, and re-running `./scripts/check-all.sh` at the phase's final commit still passes. Do not mark a phase complete from an earlier, now-stale run.

### 0.3 Frozen-specification gate

One task in this document (R2-021, finding F-016) intersects Round 1's H5 phase, which itself requires the project owner's explicit sign-off on an exact `docs/SPEC_V2.md`/`docs/UI_UX_SPEC_V2.md` diff before it may start. R2-021 must not be blocked waiting for that sign-off — it has an approved interim fix (see the task itself) that does not require any frozen-spec change. If H5's general mechanism becomes available first, prefer it over the interim fix; do not implement both.

No other task in this document touches frozen-spec content. Every other phase may proceed immediately and independently, in any order relative to Round 1's phases.

---

## Phase R0 — Baseline for this round

**Goal:** establish a clean, evidenced starting point before any Round 2 fix lands, so later evidence can cite "before" and "after" states honestly.

- [x] **R0-001** Confirm the working tree is clean at a known commit SHA and `./scripts/check-all.sh` passes at that SHA before any Round 2 work begins. Record the SHA and the full command output location (e.g. a log file path) in this document.
  - Evidence: connector-backed repository state was exact committed SHA `d5f3a41ea4d3c82e6797b336d91c27c449f61418` with no separate uncommitted local checkout; `./scripts/check-all.sh` passed in Quality run `31414501339`, job `93540178710`. The committed R0 ledger at `e0a6672776ef00844e5e2628abe413211ddeac28` then re-ran the same authoritative command and passed in Quality run `31415863213`, job `93544598485`.
- [x] **R0-002** Record, for each finding F-014 through F-025 and the §4 cluster, the exact current test-suite state relevant to it (e.g., "storage_blob_upload.c has zero host-test coverage — confirmed via `tests/host/CMakeLists.txt` grep", "macro_executor.c is excluded from the host build — confirmed via grep for its filename in CMakeLists.txt"). This is the evidence baseline that later tasks' "regression test added" claims get compared against.
  - Evidence: commit `e0a6672776ef00844e5e2628abe413211ddeac28`; finding-by-finding pre-fix coverage inventory is recorded in `docs/implementation-v2/ROUND2_R0_BASELINE_AND_TEST_COVERAGE_2026-08-10.md`; `./scripts/check-all.sh` passed on that exact SHA in Quality run `31415863213`, job `93544598485`.

---

## Phase R1 — Credential and concurrency safety

**Goal:** fix F-014 and F-015 — the two findings with direct security-sensitive material (the live password-verification record; parsed password strings in heap).

- [ ] **R1-010** (F-014) Add mutual exclusion around every read and write of `server_configuration.password_record`. Confirm the chosen mechanism does not extend any lock across the async confirmation-wait duration (`web_server_async.c`) — only the record access itself may be serialized.
  - [ ] R1-010a Implement the fix in `web_api_administration.c`'s `refresh_password_record_cache()` and `web_server_login.c`'s read site.
  - [ ] R1-010b Add a regression test. If the host fake environment cannot exercise real concurrent access, document that limitation explicitly and either add a targeted stress test (e.g. ASan/TSan-assisted) or record a hardware reproduction — do not claim this fixed on the strength of a single-threaded test alone.
  - [ ] R1-010c Run `./scripts/run-tests.sh auth` and `./scripts/run-tests.sh web`; both must pass.
- [ ] **R1-011** (F-015) Securely zero parsed password strings before `cJSON_Delete()` in both:
  - [ ] R1-011a `web_settings.c`'s `web_change_password_handle()` — all six `cJSON_Delete(root)` call sites (current line numbers 605, 612, 618, 627, 632, 639 — reconfirm exact lines before editing, since this document's earlier phases may shift them).
  - [ ] R1-011b `web_device_actions.c`'s `web_device_factory_reset_handle()` — the `cJSON_Delete(root)` call site (current line 253).
  - [ ] R1-011c Add or extend a host test asserting the parsed password buffer is zeroed before free (following the existing pattern already used for `web_auth_login_parse`/`wipe_json_tree()` — check `tests/host/` for how that convention is currently tested, and mirror it).
  - [ ] R1-011d Run `./scripts/run-tests.sh web`; must pass.

**Phase exit:** `./scripts/check-all.sh` passes; both tasks checked with evidence.

---

## Phase R2 — Storage integrity

**Goal:** fix F-016 (orphaned-blob commit accounting) and F-021 (two more cleanup-masks-primary-error sites).

- [ ] **R2-020** (F-016) Confirm current status of Round 1's H5 phase (the general commit-uncertain mechanism). Record the check (e.g. "H5 not started as of commit `<sha>` — confirmed via `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`'s H5 section").
- [ ] **R2-021** (F-016) Fix `storage_blob_upload.c`'s `storage_blob_upload_commit()` / `storage_blob_upload_core.c`'s `storage_blob_upload_commit_with_ops()` so a `sync_parent()` failure after rename cannot result in both (a) the client being told the commit failed and (b) `storage_blob_record_committed_entry()` advancing the live inventory for that blob.
  - [ ] R2-021a **If Round 1's H5 general mechanism is available:** use it. Do not implement a separate one-off fix in this case — update this subtask to point at the H5 implementation instead.
  - [ ] R2-021b **If H5 is not yet available:** implement the interim fix described in the Round 2 spec — do not advance inventory state (`committed = true` / `storage_blob_record_committed_entry()`) until `sync_parent()` has actually succeeded; treat the sync-failure case as not-yet-committed rather than committed. This interim fix must not introduce any new externally-visible response shape (stay internal to the accounting logic).
  - [ ] R2-021c Add a regression test exercising `storage_blob_upload.c`'s wrapper specifically (currently zero coverage — confirmed in R0-002) with a fake `sync_parent` that fails after a successful rename; assert the blob is not counted as committed in subsequent inventory listing/quota accounting.
  - [ ] R2-021d Trace the real caller path (`web_server_blob.c`'s `blob_create_handler`) and confirm `abort_uncommitted_upload()` now actually reclaims the blob rather than no-op'ing against an already-`committed` flag.
  - [ ] R2-021e Run `./scripts/run-tests.sh storage`; must pass.
- [ ] **R2-022** (F-021) Fix the two additional cleanup-masks-primary-error sites in `storage_atomic.c` (current lines 162-165 in `stage_temporary_file`, and 188-193 in `storage_atomic_write_with_ops_and_parent_sync`), using the same mechanism Round 1's F-006/H5 establishes (or, if H5 is not yet available, whatever minimal interim convention R2-021b used — stay consistent within this round).
  - [ ] R2-022a Implement.
  - [ ] R2-022b Add regression tests exercising both sites with a fake cleanup-unlink failure following a primary failure; assert the primary error is preserved/surfaced.
  - [ ] R2-022c Run `./scripts/run-tests.sh storage`; must pass.

**Phase exit:** `./scripts/check-all.sh` passes; all tasks checked with evidence; R2-021's chosen path (H5 vs. interim) is explicitly recorded.

---

## Phase R3 — Executor lifecycle correctness

**Goal:** fix F-017 (stuck shutdown flag) and F-025 (release-all failure never surfaced through status).

- [ ] **R3-030** (F-017) Decide and implement one of the two acceptable outcomes for `macro_executor_deinit()`'s timeout path (`macro_executor.c:236-276`):
  - [ ] R3-030a Either implement a bounded recovery path so `shutting_down` cannot remain permanently `true` after a failed/timed-out shutdown, **or**
  - [ ] R3-030b explicitly document the latched-forever behavior as an intentional fail-safe, with a way to observe/diagnose the latched state (e.g. surfaced through the existing health/diagnostics reporting), and add a code comment at the flag's definition explaining the decision.
  - [ ] R3-030c Since `macro_executor.c` is excluded from the host build (confirmed in R0-002), add either a host-linkable extraction of the relevant logic with a test, or a documented hardware/manual reproduction — do not leave this finding "fixed" with zero verification of any kind.
  - [ ] R3-030d If a code change was made, confirm `app_core.c`'s one real caller (the boot-time rollback path) still behaves correctly via `./scripts/check-firmware.sh`.
- [ ] **R3-031** (F-025) Once Round 1's F-009/H7-070 captures the release-all result from `macro_executor_engine.c`'s two submission-cleanup paths (lines 275, 281 as of this writing), ensure that captured value is actually written into `macro_execution_status_t.release_error` so `macro_executor_get_status()` surfaces it to callers.
  - Depends on: Round 1 H7-070 (or implement both together if convenient — this task does not need to wait for Round 1's phase ordering, only for F-009's capture-the-value half to exist in some form before this task's publish-the-value half is meaningful).
  - [ ] R3-031a Implement.
  - [ ] R3-031b Add a regression test asserting `release_error` reflects a failed release-all triggered during submission-cleanup, not just during normal completion (check whether `executor_terminal_tests.inc` already covers the normal-completion case and extend rather than duplicate).
  - [ ] R3-031c Run `./scripts/run-tests.sh executor`; must pass.

**Phase exit:** `./scripts/check-all.sh` passes; both tasks checked with evidence.

---

## Phase R4 — Webapp correctness

**Goal:** fix F-018 (send-tracker leak), F-019 (settings form data loss), F-020 (unbounded poll-failure retry).

- [ ] **R4-040** (F-018) Ensure every code path in `webapp/src/features/macros/v2/MacrosPage.tsx` that starts a send tracker (the `initialSend` recovery effect, `recoverActiveSend()`, and `startSend()`) has a corresponding stop triggered at minimum on unmount.
  - [ ] R4-040a Implement.
  - [ ] R4-040b Add a Vitest test in `webapp/tests/v2-macros-page.test.tsx` that starts a send via each of the three paths, unmounts the component, and asserts the underlying tracker's `.stop()` was called (or equivalently, that no further poll `fetch` calls occur after unmount) — not just that no console warning was emitted.
  - [ ] R4-040c Run `npm --prefix webapp run test`; must pass.
- [ ] **R4-041** (F-019) Fix `SettingsPage.tsx`'s `IdentityForm` so submitting a sibling form (AP or Station) cannot silently discard unsaved edits in the Identity form.
  - [ ] R4-041a Implement (scope each form's resync to only the fields it owns, or detect-and-preserve/warn on a pending local edit — either is acceptable per the spec).
  - [ ] R4-041b Add a Vitest test in `webapp/tests/v2-settings-page.test.tsx`: start editing the Identity form's fields, submit the AP or Station form, assert the Identity form's unsaved edits are still present (not silently reverted to the server's response).
  - [ ] R4-041c Run `npm --prefix webapp run test`; must pass.
- [ ] **R4-042** (F-020) Check whether Round 1's H4-042 (stale/degraded polling-state UI) has been implemented and, if so, whether it already resolves this finding as a side effect. Record that check explicitly before doing further work.
  - [ ] R4-042a If H4-042 is not implemented, or is implemented but does not give the poll loop any internal bounded-retry/give-up concept: modify `webapp/src/v2/sendClient.ts`'s poll-loop catch block so it distinguishes bounded transient retry (network blips) from a failure worth surfacing, following the pattern already established in `useDeviceReconnect.ts`'s `isTransientFailure()`.
  - [ ] R4-042b Add a Vitest test in the existing send-client test file asserting the poll loop eventually surfaces/stops on persistent failure rather than retrying forever with no signal.
  - [ ] R4-042c Run `npm --prefix webapp run test`; must pass.

**Phase exit:** `./scripts/check-webapp.sh` passes; all tasks checked with evidence.

---

## Phase R5 — Diagnostics/health synchronization

**Goal:** fix F-024 (unsynchronized `executor_health.c`/`storage_health.c` globals).

- [ ] **R5-050** (F-024) Bring `executor_health.c` and `storage_health.c` under the same synchronization discipline `macro_executor_engine_t.status` already uses (`lock_engine`/`unlock_engine`), or document explicitly why it's safe to omit (e.g. writes provably confined to a non-overlapping single-threaded boot/shutdown window).
  - [ ] R5-050a Implement or document.
  - [ ] R5-050b If implemented: add a regression test if the host fake environment can exercise the relevant concurrency; otherwise document the limitation per §0.1's rule.
  - [ ] R5-050c Run `./scripts/run-tests.sh executor` and `./scripts/run-tests.sh storage`; must pass.

**Phase exit:** `./scripts/check-all.sh` passes; task checked with evidence.

---

## Phase R6 — Dead code and routing-architecture guards

**Goal:** fix F-022 (dead code with misleading coverage) and F-023 (duplicated routing pipelines).

- [ ] **R6-060** (F-022) Decide: delete `web_setup_core.c`/`web_setup_json.c` and their host tests, or add a prominent top-of-file comment marking them intentionally retained-but-unused and why.
  - [ ] R6-060a Before deleting anything, confirm via `./scripts/check-all.sh` (full run, not a subset) that nothing else in the tree references these files — grep is not sufficient on its own for a deletion this size; the full gate must pass afterward too.
  - [ ] R6-060b Implement the chosen option.
  - [ ] R6-060c Run `./scripts/check-all.sh`; must pass.
- [ ] **R6-061** (F-023) Add a build-time or test-time check that `web_server_lifecycle.c`'s exact-match `normal_routes[]` table and `web_api_administration.c`'s wildcard dispatch switch cannot silently diverge (e.g. a test asserting every route in the exact-match table is either dispatched by name there, or has no corresponding case in `web_api_handle_administration()` — so an accidental future removal from the exact-match table doesn't silently 404 a route in production).
  - [ ] R6-061a Implement the check.
  - [ ] R6-061b Optional (recommended, per the spec): reconcile the `X-Request-ID`-vs-session-auth validation ordering inconsistency between `web_server_blob.c`'s `establish_request_id()` and the generic `web_request_policy.c` pipeline, so both pipelines validate in the same order. If this is done, update any test that currently asserts the old inconsistent ordering as correct.
  - [ ] R6-061c Run `./scripts/run-tests.sh web`; must pass.

**Phase exit:** `./scripts/check-all.sh` passes; both tasks checked with evidence.

---

## Phase R7 — Minor/quality cluster

**Goal:** fix or document the six items in the Round 2 spec's §4, batched as the spec explicitly allows.

- [ ] **R7-070** Fix or document each of:
  - [ ] R7-070a `web_server_static.c`'s dead conditional in `static_handler`'s `fclose` error path.
  - [ ] R7-070b `device_controls_logic.c`'s unreachable defensive branch (current lines 396-399) — confirm still unreachable via the current coverage report before touching it; either simplify it away with a comment explaining why, or leave it with an explicit "defensive, provably unreachable as of `<sha>`" comment.
  - [ ] R7-070c `web_api_core.c`'s permanent no-op `web_api_route_requires_worker()` — implement it for real or remove the vacuous `X || false` call site along with it.
  - [ ] R7-070d `web_request_policy.c`'s `establish_request_id()` — decide consistent treatment for an oversized-but-otherwise-valid `X-Request-ID` header (currently silently replaced, inconsistent with how a malformed one is rejected); implement the decision.
  - [ ] R7-070e `web_server_api.c`'s `restart_after_response()` — add a comment explaining the `DEVICE_RESET_SETTINGS` vs. `DEVICE_RESTART`/`DEVICE_FACTORY_RESET` immediate-restart asymmetry (not a behavior change unless investigation finds it actually is a bug, in which case treat it as a new finding and update this document).
  - [ ] R7-070f Consolidate the three duplicated file-save helper implementations across `SettingsPage.tsx`, `SnapshotsPage.tsx`, and `DiagnosticsPage.tsx` into one shared utility.
  - [ ] R7-070g Add or update tests for any of the above where behavior actually changed (not needed for pure comment/documentation-only sub-items).
  - [ ] R7-070h Run `./scripts/check-all.sh`; must pass.

**Phase exit:** all sub-items checked with evidence; single commit (or small number of related commits) covering the batch, per the spec's explicit allowance.

---

## Phase R8 — Final regression and closeout

**Goal:** confirm this round is fully done per the spec's §10 acceptance criteria, and reconcile `docs/TODO_V2.md` and this document's own checkboxes.

- [ ] **R8-080** Re-run `./scripts/check-all.sh` at the final commit of this round; must pass clean.
- [ ] **R8-081** Walk the Round 2 spec's §10 acceptance-criteria list item by item and confirm each is met, citing the specific task(s) in this document that satisfied it.
- [ ] **R8-082** Confirm no task in this round weakened any CI gate, added a suppression, or skipped a test to get to green (per Round 2 spec §6/§8) — spot-check by diffing `.clang-tidy`, `docs/STATIC_ANALYSIS_EXCEPTIONS.md`, and any `eslint-disable`/`NOLINT`/`// eslint-disable-next-line` occurrences introduced this round.
- [ ] **R8-083** Update this document's own checkboxes to their final state and record the closing SHA.
- [ ] **R8-084** If any task in this round changed behavior that Round 1's H0 baseline/failure-matrix document referenced, note the discrepancy for a future H0 reconciliation pass — do not silently let the two documents drift without a cross-reference.

**Phase exit:** this document fully checked; `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_SPEC_ROUND2_2026-08-10.md` §10 fully satisfied with cited evidence.
