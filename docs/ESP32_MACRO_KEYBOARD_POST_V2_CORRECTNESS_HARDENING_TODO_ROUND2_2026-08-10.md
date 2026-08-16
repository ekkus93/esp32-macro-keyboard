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

- [x] **R1-010** (F-014) Add mutual exclusion around every read and write of `server_configuration.password_record`. Confirm the chosen mechanism does not extend any lock across the async confirmation-wait duration (`web_server_async.c`) — only the record access itself may be serialized.
  - [x] R1-010a Implement the fix in `web_api_administration.c`'s `refresh_password_record_cache()` and `web_server_login.c`'s read site.
  - [x] R1-010b Add a regression test. If the host fake environment cannot exercise real concurrent access, document that limitation explicitly and either add a targeted stress test (e.g. ASan/TSan-assisted) or record a hardware reproduction — do not claim this fixed on the strength of a single-threaded test alone.
  - [x] R1-010c Run `./scripts/run-tests.sh auth` and `./scripts/run-tests.sh web`; both must pass.
  - Evidence: F-014 implementation landed in `6bc4cd703860966456487c653bff50d9cb45c303`; final corrective candidate `37644d128280cbb3d6f8d6973c2318c466b2fe46` passed literal `./scripts/run-tests.sh auth` (4/4) and `./scripts/run-tests.sh web` (27/27) in targeted run `31427313308`, job `93581969748`, including the concurrent password-record stress test and direct-access guard. `./scripts/check-all.sh` passed in Quality run `31427313311`, job `93581969466`; Host run `31427313392`, Browser run `31427313368`, and Device Test Build run `31427313324` also passed on the same SHA.
- [x] **R1-011** (F-015) Securely zero parsed password strings before `cJSON_Delete()` in both:
  - [x] R1-011a `web_settings.c`'s `web_change_password_handle()` — all six `cJSON_Delete(root)` call sites (current line numbers 605, 612, 618, 627, 632, 639 — reconfirm exact lines before editing, since this document's earlier phases may shift them).
  - [x] R1-011b `web_device_actions.c`'s `web_device_factory_reset_handle()` — the `cJSON_Delete(root)` call site (current line 253).
  - [x] R1-011c Add or extend a host test asserting the parsed password buffer is zeroed before free (following the existing pattern already used for `web_auth_login_parse`/`wipe_json_tree()` — check `tests/host/` for how that convention is currently tested, and mirror it).
  - [x] R1-011d Run `./scripts/run-tests.sh web`; must pass.
  - Evidence: factory-reset parsed-secret wiping landed in `e593eafa612a7346ae7f85625963ac59971c1cce`; change-password parsed-secret wiping in `1e166e19067ef444bccc21877eaab6a611f62fff`; the free-time cJSON regression landed in `e15374b20d1bfbf8d7f5e7f4dd5c9bbae96cb862` and was registered in `31c52f94218826e004494530290d4e7a8a667160`. Final corrective candidate `b4f91ef58c7da3e0ff5c055d542f0b88db318e2b` passed literal `./scripts/run-tests.sh web` (28/28, including `web_parsed_secret_wipe`) in targeted run `31431339921`, job `93595228753`; `./scripts/check-all.sh` passed in Quality run `31431339929`, job `93595227345`; Host run `31431339901`, Browser run `31431339930`, and Device Test Build run `31431339945` also passed on the same SHA.

**Phase exit:** `./scripts/check-all.sh` passes; both tasks checked with evidence.

---

## Phase R2 — Storage integrity

**Goal:** fix F-016 (orphaned-blob commit accounting) and F-021 (two more cleanup-masks-primary-error sites).

- [x] **R2-020** (F-016) Confirm current status of Round 1's H5 phase (the general commit-uncertain mechanism). Record the check (e.g. "H5 not started as of commit `<sha>` — confirmed via `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`'s H5 section").
  - Evidence: at Phase R1 closure SHA `fcf7a47914acbec0b42b597d9f46f15bab491ee4`, Round 1 H5 is not started: H5-050 through H5-055 and the Phase H5 exit gate are all unchecked in `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`. Therefore R2-021 must use its approved interim path unless H5 becomes available before R2-021 implementation begins.
- [x] **R2-021** (F-016) Fix `storage_blob_upload.c`'s `storage_blob_upload_commit()` / `storage_blob_upload_core.c`'s `storage_blob_upload_commit_with_ops()` so a `sync_parent()` failure after rename cannot result in both (a) the client being told the commit failed and (b) `storage_blob_record_committed_entry()` advancing the live inventory for that blob.
  - [x] R2-021a **If Round 1's H5 general mechanism is available:** use it. Do not implement a separate one-off fix in this case — update this subtask to point at the H5 implementation instead. H5 was not available at implementation start, so the approved interim path was selected per R2-020.
  - [x] R2-021b **If H5 is not yet available:** implement the interim fix described in the Round 2 spec — do not advance inventory state (`committed = true` / `storage_blob_record_committed_entry()`) until `sync_parent()` has actually succeeded; treat the sync-failure case as not-yet-committed rather than committed. This interim fix must not introduce any new externally-visible response shape (stay internal to the accounting logic).
  - [x] R2-021c Add a regression test exercising `storage_blob_upload.c`'s wrapper specifically (currently zero coverage — confirmed in R0-002) with a fake `sync_parent` that fails after a successful rename; assert the blob is not counted as committed in subsequent inventory listing/quota accounting.
  - [x] R2-021d Trace the real caller path (`web_server_blob.c`'s `blob_create_handler`) and confirm `abort_uncommitted_upload()` now actually reclaims the blob rather than no-op'ing against an already-`committed` flag.
  - [x] R2-021e Run `./scripts/run-tests.sh storage`; must pass.
  - Evidence: interim implementation commit `829a47cb563f320aa97ea6b806d981a53be3c32e` defers `committed`/inventory visibility until parent sync succeeds and adds explicit owned-final reclamation; policy-guard corrective/final code candidate `269e8cef49b39466da900e070fb00d6d6861f130` enforces rename → ownership → parent sync → committed ordering and wrapper/reclamation regressions. Exact `./scripts/run-tests.sh storage` passed 13/13 in run `31443276281`, job `93632202437`. The real `blob_create_handler()` calls `abort_uncommitted_upload()` after commit failure; the regression proves the failed-sync upload remains uncommitted, inventory/quota counters remain zero, and public abort reclaims the owned final path. Exact-SHA `./scripts/check-all.sh` passed in Quality run `31443219818`, job `93632036167`; Host run `31443219878`, Browser run `31443219837`, and Device Test Build run `31443219861` also passed on `269e8cef49b39466da900e070fb00d6d6861f130`.
- [x] **R2-022** (F-021) Fix the two additional cleanup-masks-primary-error sites in `storage_atomic.c` (current lines 162-165 in `stage_temporary_file`, and 188-193 in `storage_atomic_write_with_ops_and_parent_sync`), using the same mechanism Round 1's F-006/H5 establishes (or, if H5 is not yet available, whatever minimal interim convention R2-021b used — stay consistent within this round).
  - [x] R2-022a Implement.
  - [x] R2-022b Add regression tests exercising both sites with a fake cleanup-unlink failure following a primary failure; assert the primary error is preserved/surfaced.
  - [x] R2-022c Run `./scripts/run-tests.sh storage`; must pass.
  - Evidence: implementation/provenance commits `74cdfc2ae2f40ead7ee41570a7f5dedf318032f5` and `4cb8db24ed64823152391b4813194667c7991ba8` changed both atomic-write cleanup sites so the initiating error remains primary while cleanup failure is retained separately in `app_operation_result_t`; formatter/include-integration follow-ups culminated in final code candidate `5405043b33f9bd3b449f8a78f05678f88d6908d7`. The regressions inject a primary `ENOSPC` and cleanup-unlink `EIO` at both the staging and rename sites, asserting `primary_error == APP_ERROR_STORAGE_FULL`, `cleanup_error == APP_ERROR_IO`, `cleanup_incomplete == true`, and that the stable `app_error_code_t` wrapper still returns the primary error. Exact `./scripts/run-tests.sh storage` passed 13/13 in run `31448233062`, job `93646945486`; exact-SHA `./scripts/check-all.sh` passed in Quality run `31448204802`, job `93646907485`; Host run `31448204773`, Browser run `31448204765`, and Device Test Build run `31448204780` also passed on `5405043b33f9bd3b449f8a78f05678f88d6908d7`.

**Phase exit:** `./scripts/check-all.sh` passes; all tasks checked with evidence; R2-021's chosen path (H5 vs. interim) is explicitly recorded.

---

## Phase R3 — Executor lifecycle correctness

**Goal:** fix F-017 (stuck shutdown flag) and F-025 (release-all failure never surfaced through status).

- [x] **R3-030** (F-017) Decide and implement one of the two acceptable outcomes for `macro_executor_deinit()`'s timeout path (`macro_executor.c:236-276`):
  - [ ] R3-030a Either implement a bounded recovery path so `shutting_down` cannot remain permanently `true` after a failed/timed-out shutdown, **or**
  - [x] R3-030b explicitly document the latched-forever behavior as an intentional fail-safe, with a way to observe/diagnose the latched state (e.g. surfaced through the existing health/diagnostics reporting), and add a code comment at the flag's definition explaining the decision.
  - [x] R3-030c Since `macro_executor.c` is excluded from the host build (confirmed in R0-002), add either a host-linkable extraction of the relevant logic with a test, or a documented hardware/manual reproduction — do not leave this finding "fixed" with zero verification of any kind.
  - [x] R3-030d If a code change was made, confirm `app_core.c`'s one real caller (the boot-time rollback path) still behaves correctly via `./scripts/check-firmware.sh`.
  - Evidence: selected the R3-030b fail-safe path (R3-030a intentionally not selected). Implementation commit `7548a68af89fa04f5933d4779bcd65006af65e8c` replaces the implicit raw shutdown flag with a portable `executor_shutdown_state_t`: deinit begins by closing submissions; an unconfirmed worker stop latches `shutting_down=true`/`stop_unconfirmed=true`, records the stop error into executor health with `cleanup_incomplete=true`, and only a later confirmed worker stop clears the latch. `tests/host/test_executor_health.c` proves the latch stays fail-closed across a retry and reopens only after confirmed stop, and proves the timeout is visible as failed executor health. Exact-SHA Host run `31450120844` passed (including ASan/UBSan and coverage), Browser run `31450120843` passed, Device Test Build run `31450120833` passed, and `./scripts/check-all.sh` passed in Quality run `31450120828`, job `93652582137`. Required literal `./scripts/check-firmware.sh` passed against exact SHA `7548a68af89fa04f5933d4779bcd65006af65e8c` in verification run `31450241069`, job `93652942140`.
- [x] **R3-031** (F-025) Once Round 1's F-009/H7-070 captures the release-all result from `macro_executor_engine.c`'s two submission-cleanup paths (lines 275, 281 as of this writing), ensure that captured value is actually written into `macro_execution_status_t.release_error` so `macro_executor_get_status()` surfaces it to callers.
  - Depends on: Round 1 H7-070 (or implement both together if convenient — this task does not need to wait for Round 1's phase ordering, only for F-009's capture-the-value half to exist in some form before this task's publish-the-value half is meaningful).
  - [x] R3-031a Implement.
  - [x] R3-031b Add a regression test asserting `release_error` reflects a failed release-all triggered during submission-cleanup, not just during normal completion (check whether `executor_terminal_tests.inc` already covers the normal-completion case and extend rather than duplicate).
  - [x] R3-031c Run `./scripts/run-tests.sh executor`; must pass.
  - Evidence: implementation commit `e60e9d73c8ea9494957228c3a734f48aeec8566a` routes failures from both submission-cleanup release-all paths through `record_release_failure()`, writes the failure into `macro_execution_status_t.release_error` for `macro_executor_get_status()`, preserves the primary execution error separately, and adds deterministic regressions for both cleanup paths; formatter follow-up `44fc3f3be622ff49185419cd1de3b7b091a33740` is code-equivalent. Literal `./scripts/run-tests.sh executor` passed 2/2 (`macro_executor`, `executor_health`) against the uploaded matching master snapshot using only a sandbox-local cJSON 1.7.18 development-header/pkg-config shim; no repository file was changed by that shim. Exact-SHA Host run `31464144676`, Browser run `31464144657`, Device Test Build run `31464144658`, and `./scripts/check-all.sh` Quality run `31464144673`, job `93693418731`, passed on validation SHA `576fad519616844fb1d8ef6aa162e5ea6ac80d56`.

**Phase exit:** `./scripts/check-all.sh` passes; both tasks checked with evidence.

---

## Phase R4 — Webapp correctness

**Goal:** fix F-018 (send-tracker leak), F-019 (settings form data loss), F-020 (unbounded poll-failure retry).

- [x] **R4-040** (F-018) Ensure every code path in `webapp/src/features/macros/v2/MacrosPage.tsx` that starts a send tracker (the `initialSend` recovery effect, `recoverActiveSend()`, and `startSend()`) has a corresponding stop triggered at minimum on unmount.
  - [x] R4-040a Implement.
  - [x] R4-040b Add a Vitest test in `webapp/tests/v2-macros-page.test.tsx` that starts a send via each of the three paths, unmounts the component, and asserts the underlying tracker's `.stop()` was called (or equivalently, that no further poll `fetch` calls occur after unmount) — not just that no console warning was emitted.
  - [x] R4-040c Run `npm --prefix webapp run test`; must pass. — **56 files, 538 tests, all passed** (2026-08-15, Node v24.18.0). The sandbox limitation recorded below no longer applies: this run used the pinned Node with `webapp/node_modules` installed.
  - Evidence: commit `425e4a135580b43fcecad2799f36d94db8555fb3` centralizes tracker ownership in `activeHandleRef`, stops the active tracker on unmount, and stops a `sendMacro()` handle that resolves after unmount. `webapp/tests/v2-macros-page.test.tsx` covers the `initialSend`, 409 recovery, ordinary `startSend()`, and late-resolution race paths. Local npm execution remains open because the sandbox has Node 22, while this repository pins Node 24.18.0 with `engine-strict=true`, and the uploaded snapshot contains no `webapp/node_modules`; no test pass is claimed for R4-040c.
- [x] **R4-041** (F-019) Fix `SettingsPage.tsx`'s `IdentityForm` so submitting a sibling form (AP or Station) cannot silently discard unsaved edits in the Identity form.
  - [x] R4-041a Implement (scope each form's resync to only the fields it owns, or detect-and-preserve/warn on a pending local edit — either is acceptable per the spec).
  - [x] R4-041b Add a Vitest test in `webapp/tests/v2-settings-page.test.tsx`: start editing the Identity form's fields, submit the AP or Station form, assert the Identity form's unsaved edits are still present (not silently reverted to the server's response).
  - [x] R4-041c Run `npm --prefix webapp run test`; must pass. — **56 files, 538 tests, all passed** (2026-08-15, Node v24.18.0).
  - Evidence: commit `7054e42fffb168a869a4bd1aa556437339b6adae` scopes `IdentityForm` resynchronization to identity-owned settings fields instead of the whole settings object. The added regression edits the device name, submits an access-point update, reproduces the parent settings rerender, and asserts the unsaved Identity edit survives. Local npm execution is blocked by the same sandbox Node/dependency limitation recorded under R4-040; no test pass is claimed for R4-041c.
- [x] **R4-042** (F-020) Check whether Round 1's H4-042 (stale/degraded polling-state UI) has been implemented and, if so, whether it already resolves this finding as a side effect. Record that check explicitly before doing further work.
  - [x] R4-042a If H4-042 is not implemented, or is implemented but does not give the poll loop any internal bounded-retry/give-up concept: modify `webapp/src/v2/sendClient.ts`'s poll-loop catch block so it distinguishes bounded transient retry (network blips) from a failure worth surfacing, following the pattern already established in `useDeviceReconnect.ts`'s `isTransientFailure()`.
  - [x] R4-042b Add a Vitest test in the existing send-client test file asserting the poll loop eventually surfaces/stops on persistent failure rather than retrying forever with no signal.
  - [x] R4-042c Run `npm --prefix webapp run test`; must pass. — **56 files, 538 tests, all passed** (2026-08-15, Node v24.18.0).
  - Evidence: Round 1 H4-042 remains entirely unchecked on master at `e5ed9349e9277b42c1a91e8e7773cc080a108c48`, so F-020 required its own implementation. Commit `c1ed265e2af3e3ba33b5bc2db822cf5eb79a73f5` bounds consecutive transient polling failures at three, stops immediately on non-transient failures, surfaces `onError`, and keeps the last active-send state/Cancel affordance visible in `MacrosPage`. Regressions cover three consecutive 503 failures, immediate 400 failure, and visible tracker failure without hiding Cancel. Local npm execution is blocked by the same sandbox Node/dependency limitation; no test pass is claimed for R4-042c.

**Phase exit:** `./scripts/check-webapp.sh` passes; all tasks checked with evidence.
— **Met 2026-08-15.** `./scripts/check-webapp.sh` → `EXIT=0` (ci → typecheck →
eslint `--max-warnings=0` → stylelint → vitest 538/538 → build → local-assets).

### R4 verification note (2026-08-15)

The implementations for F-018/F-019/F-020 were already on `master`
(`425e4a1`, `7054e42`, `c1ed265`); only the `c` sub-tasks were open, because the
environment that wrote them had Node 22 and no `webapp/node_modules` and
correctly declined to claim a test pass it had not run. Running them was
therefore the whole of the remaining work.

**A passing test is not evidence that the test would catch the bug**, so each
fix was reverted in the working tree and the suite re-run to prove the
regressions are not vacuous. All three failed as intended, and were restored:

| Finding | Revert applied | Result |
| --- | --- | --- |
| F-018 | `stopActiveTracking()` removed from the unmount cleanup in `MacrosPage.tsx` | **2 failed** — `stops the 409-recovery tracker on unmount`, `stops a newly-started send tracker on unmount` |
| F-019 | `IdentityForm.tsx` resync effect keyed back on `[settings]` | **1 failed** — `access-point update preserves unsaved identity edits when settings refresh` |
| F-020 | `sendClient.ts` poll-failure branch returned to unconditional `schedulePoll()` | **2 failed** — `persistent transient poll failures surface and stop tracking`, `non-transient poll failure surfaces without retrying` |

The F-018 result matches the finding precisely: two paths fail, not three,
because the `initialSend` effect already registered a cleanup before the fix —
exactly what F-018 says ("Only the first path's `useEffect` ever registers a
cleanup function"). A third failure would have meant the test was asserting
something other than the defect.

The code these fixes live in was relocated by the large-file refactor
(`docs/REFACTOR_LARGE_FILES_TODO_2026-08-15.md`): F-019's fix is now in
`webapp/src/features/settings/v2/IdentityForm.tsx`, and F-018's regressions are
now in `webapp/tests/v2-macros-page-send.test.tsx`. Both moves were
behaviour-preserving and the revert-checks above were run against the current
layout, so the fixes demonstrably survived the split.

---

## Phase R5 — Diagnostics/health synchronization

**Goal:** fix F-024 (unsynchronized `executor_health.c`/`storage_health.c` globals).

- [ ] **R5-050** (F-024) Bring `executor_health.c` and `storage_health.c` under the same synchronization discipline `macro_executor_engine_t.status` already uses (`lock_engine`/`unlock_engine`), or document explicitly why it's safe to omit (e.g. writes provably confined to a non-overlapping single-threaded boot/shutdown window).
  - [x] R5-050a Implement or document.
  - [x] R5-050b If implemented: add a regression test if the host fake environment can exercise the relevant concurrency; otherwise document the limitation per §0.1's rule.
  - [x] R5-050c Run `./scripts/run-tests.sh executor` and `./scripts/run-tests.sh storage`; must pass.
  - Evidence: commit `5773f9fb1e1f6768b80bc820831749c744ccc7be` protects executor/storage health updates and snapshots with FreeRTOS `portMUX_TYPE` critical sections, maps that primitive to a real pthread mutex in the focused host stub, and adds 50,000-iteration writer/four-reader stress regressions for each subsystem. The exact R5 files and CMake deltas were reconstructed in the uploaded sandbox without changing repository source; literal `./scripts/run-tests.sh executor` passed 2/2 and literal `./scripts/run-tests.sh storage` passed 13/13. The same reconstructed host tree passed the full 59/59 host suite. The parent task remains open until its required resulting-tree `./scripts/check-all.sh` evidence exists.

**Phase exit:** `./scripts/check-all.sh` passes; task checked with evidence.

---

## Phase R6 — Dead code and routing-architecture guards

**Goal:** fix F-022 (dead code with misleading coverage) and F-023 (duplicated routing pipelines).

- [ ] **R6-060** (F-022) Decide: delete `web_setup_core.c`/`web_setup_json.c` and their host tests, or add a prominent top-of-file comment marking them intentionally retained-but-unused and why.
  - [x] R6-060a Before deleting anything, confirm via `./scripts/check-all.sh` (full run, not a subset) that nothing else in the tree references these files — grep is not sufficient on its own for a deletion this size; the full gate must pass afterward too.
  - [x] R6-060b Implement the chosen option.
  - [ ] R6-060c Run `./scripts/check-all.sh`; must pass.
  - Evidence: commit `7292ba3af20b74c034ea09eb578febb4f7806570` selected the allowed retention path and adds prominent `LEGACY / NOT SHIPPED` documentation to the setup core/JSON source and headers, explicitly naming `web_server_setup_submit.c` as the shipped replacement and warning that the legacy tests are not production-route coverage. R6-060a is not applicable to deletion because no deletion was attempted; it is checked as a satisfied conditional precondition. R6-060c remains open pending resulting-tree `check-all.sh`.
- [ ] **R6-061** (F-023) Add a build-time or test-time check that `web_server_lifecycle.c`'s exact-match `normal_routes[]` table and `web_api_administration.c`'s wildcard dispatch switch cannot silently diverge (e.g. a test asserting every route in the exact-match table is either dispatched by name there, or has no corresponding case in `web_api_handle_administration()` — so an accidental future removal from the exact-match table doesn't silently 404 a route in production).
  - [x] R6-061a Implement the check.
  - [x] R6-061b Optional (recommended, per the spec): reconcile the `X-Request-ID`-vs-session-auth validation ordering inconsistency between `web_server_blob.c`'s `establish_request_id()` and the generic `web_request_policy.c` pipeline, so both pipelines validate in the same order. If this is done, update any test that currently asserts the old inconsistent ordering as correct.
  - [ ] R6-061c Run `./scripts/run-tests.sh web`; must pass.
  - Evidence: commit `46beba4b4db0ffb216fc06250a71109c8ddbe1ff` adds `scripts/check-web-route-dispatch-sync.py`, wires it into repository checks, adds a fail-closed shell regression, and reconciles blob request-ID validation ordering with the generic policy path. The exact guard/test was reconstructed locally and passed all four cases: current partition accepted; dedicated-route removal rejected; duplicate wildcard dispatch rejected; unclassified future route rejected. R6-061c remains open because the sandbox cannot reproduce the complete current source tree from the uploaded pre-R6 snapshot with enough fidelity to claim the literal current-tree web target.

**Phase exit:** `./scripts/check-all.sh` passes; both tasks checked with evidence.

---

## Phase R7 — Minor/quality cluster

**Goal:** fix or document the six items in the Round 2 spec's §4, batched as the spec explicitly allows.

- [ ] **R7-070** Fix or document each of:
  - [x] R7-070a `web_server_static.c`'s dead conditional in `static_handler`'s `fclose` error path.
  - [x] R7-070b `device_controls_logic.c`'s unreachable defensive branch (current lines 396-399) — confirm still unreachable via the current coverage report before touching it; either simplify it away with a comment explaining why, or leave it with an explicit "defensive, provably unreachable as of `<sha>`" comment.
  - [x] R7-070c `web_api_core.c`'s permanent no-op `web_api_route_requires_worker()` — implement it for real or remove the vacuous `X || false` call site along with it.
  - [x] R7-070d `web_request_policy.c`'s `establish_request_id()` — decide consistent treatment for an oversized-but-otherwise-valid `X-Request-ID` header (currently silently replaced, inconsistent with how a malformed one is rejected); implement the decision.
  - [x] R7-070e `web_server_api.c`'s `restart_after_response()` — add a comment explaining the `DEVICE_RESET_SETTINGS` vs. `DEVICE_RESTART`/`DEVICE_FACTORY_RESET` immediate-restart asymmetry (not a behavior change unless investigation finds it actually is a bug, in which case treat it as a new finding and update this document).
  - [x] R7-070f Consolidate the three duplicated file-save helper implementations across `SettingsPage.tsx`, `SnapshotsPage.tsx`, and `DiagnosticsPage.tsx` into one shared utility.
  - [x] R7-070g Add or update tests for any of the above where behavior actually changed (not needed for pure comment/documentation-only sub-items).
  - [ ] R7-070h Run `./scripts/check-all.sh`; must pass.
  - Evidence: commit `e5ed9349e9277b42c1a91e8e7773cc080a108c48` removes the dead static close conditional and vacuous worker predicate, replaces the unreachable device-controls branch with its proved deinit invariant, rejects oversized request-ID headers consistently instead of silently replacing them, documents the reset-settings restart asymmetry, and consolidates browser file downloads in `webapp/src/v2/saveFile.ts`. Host/request-policy tests changed with the behavioral items; pure comments require no regression. R7-070h remains open pending a complete resulting-tree `check-all.sh`.

**Phase exit:** all sub-items checked with evidence; single commit (or small number of related commits) covering the batch, per the spec's explicit allowance.

---

## Phase R8 — Final regression and closeout

**Goal:** confirm this round is fully done per the spec's §10 acceptance criteria, and reconcile `docs/TODO_V2.md` and this document's own checkboxes.

- [ ] **R8-080** Re-run `./scripts/check-all.sh` at the final commit of this round; must pass clean.
- [ ] **R8-081** Walk the Round 2 spec's §10 acceptance-criteria list item by item and confirm each is met, citing the specific task(s) in this document that satisfied it.
  - Acceptance mapping prepared: F-014 -> R1-010; F-015 -> R1-011; F-016 -> R2-021 interim path; F-017 -> R3-030b fail-safe latch; F-018 -> R4-040; F-019 -> R4-041; F-020 -> R4-042; F-021 -> R2-022; F-022 -> R6-060 retention/documentation path; F-023 -> R6-061; F-024 -> R5-050; F-025 -> R3-031/H7-070; §4 minor cluster -> R7-070. The final two §10 criteria (all required regressions wired/passing and exact final SHA passing the complete quality gate) remain open, so R8-081 is intentionally not checked yet.
- [x] **R8-082** Confirm no task in this round weakened any CI gate, added a suppression, or skipped a test to get to green (per Round 2 spec §6/§8) — spot-check by diffing `.clang-tidy`, `docs/STATIC_ANALYSIS_EXCEPTIONS.md`, and any `eslint-disable`/`NOLINT`/`// eslint-disable-next-line` occurrences introduced this round.
  - Evidence: comparison from Round 2 baseline-ledger SHA `e0a6672776ef00844e5e2628abe413211ddeac28` through implementation head `e5ed9349e9277b42c1a91e8e7773cc080a108c48` shows neither `.clang-tidy` nor `docs/STATIC_ANALYSIS_EXCEPTIONS.md` changed. Repository search found no production `eslint-disable-next-line` addition; existing `NOLINT` policy references are outside this round's changed files. `scripts/check-all.sh`/`scripts/check-scripts.sh` only gained the R6 route-synchronization guard/test; no analyzer exclusion, ignored exit code, test skip, or lowered threshold was introduced.
- [ ] **R8-083** Update this document's own checkboxes to their final state and record the closing SHA.
- [x] **R8-084** If any task in this round changed behavior that Round 1's H0 baseline/failure-matrix document referenced, note the discrepancy for a future H0 reconciliation pass — do not silently let the two documents drift without a cross-reference.
  - Reconciliation note: Round 1 H0-003 is still unchecked and its formal failure matrix has not yet been closed. When that future H0 pass is completed, its `rename success + parent sync failure` row must cite R2-021, its `storage primary failure + cleanup failure` row must cite R2-022, and its `executor submit failure + release-all failure` row must cite R3-031/H7-070 rather than describing the pre-Round-2 behavior.

**Phase exit:** this document fully checked; `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_SPEC_ROUND2_2026-08-10.md` §10 fully satisfied with cited evidence.
