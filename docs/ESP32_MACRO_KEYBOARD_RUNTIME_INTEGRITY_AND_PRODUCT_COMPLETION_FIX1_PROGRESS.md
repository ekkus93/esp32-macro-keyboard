# FIX1 Runtime Integrity and Product Completion — Progress

**Specification:** `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
**Plan:** `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
**Operator decisions:** `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_RESPONSES.md`

## Baseline

- Reviewed ancestor (from spec): `992f2a018aff97e5b167c98d6a0d469d6a7c84ff`.
- **Actual starting `master` SHA:** `e757c5f30108caa97542d8fdc2abcbacfec151f3`
  (`docs: answer FIX1 implementation questions`).
- Work proceeds directly on `master` per RESPONSES Q4.

### Baseline validation (before any FIX1 change)

| Check | Command | Result |
| --- | --- | --- |
| Full gate | `./scripts/check-all.sh` | pass (Quality CI green on `e757c5f`'s ancestors) |
| Sanitizers | `./scripts/run-tests.sh --sanitizers` | 17/17 pass, ASan/UBSan/leak clean |
| Native coverage | `./scripts/generate-native-coverage.sh` | pass, 95% total (line ≥90 / branch ≥80 on policy files) |
| Frontend coverage | `./scripts/generate-frontend-coverage.sh` | pass, 92.48% statements / 96.77% branch |
| Device build | `./scripts/build-device-tests.sh` | pass (ESP32-S3 Unity firmware builds) |

### Baseline metrics

- Production firmware binary (`firmware/build/esp32_macro_keyboard.bin`): 907,680 bytes.
- webapp production build (`webapp/dist`): ~216 KiB.
- Host test suites: 17.

### Baseline notes / known pre-FIX1 state

- `scripts/check-firmware.sh` currently runs `run-clang-tidy … || true` and gates on
  first-party *finding location* — the pattern FIX1 Phase 2 replaces (RESPONSES Q1).
- `.clang-tidy` disables three checks with documented rationale
  (`insecureAPI.DeprecatedOrUnsafeBufferHandling`, `readability-non-const-parameter`,
  `concurrency-mt-unsafe`) — approved as reviewed exceptions (RESPONSES Q2), to be
  formalized in `docs/STATIC_ANALYSIS_EXCEPTIONS.md` with a policy check.
- `auth_password_verify` / `auth_core_password_verify` return `bool` (crypto failure vs
  mismatch conflated) — corrected in Phase/Section 10.
- Execution terminal states have no `TIMED_OUT` — added end-to-end per RESPONSES Q3.

## Phase status

| Phase | Title | Status |
| --- | --- | --- |
| 1 | Establish the FIX1 baseline | done |
| 2 | Make the quality gate fail closed | done |
| 3 | Structured failure and ownership reporting | done |
| 4 | Correct application lifecycle ownership | done (persistent-provisioning load + setup mode deferred to Phase 14) |
| 5 | Correct HTTP partial-start lifecycle | done |
| 6 | Correct filesystem mount ownership and topology | done (LittleFS permission verification is device-observable) |
| 7 | Atomic-write artifact recovery | done (4 object validators deferred to Phase 15; quarantine-dir scan residual resolved by Phase 8's staged-dir layout) |
| 8 | Make quarantine recoverable | done |
| 9 | Serialize repository operations | done (import/restore serialization deferred with Phase 18) |
| 10 | Separate password mismatch from crypto failure | done |
| 11 | Fix Wi-Fi cleanup | done |
| 12 | Fix executor shutdown and terminal integrity | done (§12.4 observability fields deferred to Phase 16/19) |
| 13 | Fix device-controls shutdown and failure visibility | done (diagnostics aggregation deferred to Phase 19) |
| 14 | Encrypted persistent provisioning | done (physical confidentiality remains Phase 20 hardware evidence) |
| 15 | Complete storage object repositories | done |
| 16 | Complete the HTTP API | done (package transactions Phase 18; diagnostics aggregation Phase 19) |
| 17 | Replace frontend mock behavior | in progress (17.1-17.4, 17.8, and settings complete) |
| 18 | Import / export / backup / restore | not started |
| 19 | Diagnostics and observability | not started |
| 20 | Hardware and integration validation | environment-blocked (see below) |
| 21 | Release budgets and immutable CI | not started |
| 22 | Documentation synchronization | ongoing |
| 23 | Final regression and acceptance gate | not started |

## Completed tasks (commit evidence)

- Phase 17 foundation (split shell, runtime response validation, setup/login/logout and reload-safe session lifecycle, live set selection, settings, and execution-result semantics) — complete in the commit containing this progress update. The authenticated session route restores only the CSRF token after reload; the HttpOnly session token remains undisclosed. Invalid response data fails closed, 401 clears CSRF and stops polling, login throttling is visible, the hardcoded model/set is removed, and deferred Phase 18/19 functions are explicit rather than inert.

- Phase 16 (complete HTTP API) — complete. The final Phase 16 commit adds
  transactional deep set duplication without progress, complete set ordering,
  centralized current and exact-ID cancellation, repository-backed route acceptance,
  strict shared request policy, execution ownership/cancellation tests, and synchronized
  API documentation. Import/export/backup/restore remain explicit 503 boundaries owned
  by Phase 18; diagnostics aggregation remains Phase 19. The final commit SHA is the
  commit containing this progress update.

- Phase 1.1–1.3 (baseline + this document): `7cf917a`, `4a17aeb`.
- Phase 2.1 / 2.2 (fail-closed clang-tidy gate + regression tests): `9e0498c`.
  `check-firmware.sh` preserves run-clang-tidy's exit status; third-party
  diagnostics are excluded before emission (`-exclude-header-filter` +
  `misc-header-include-cycle.IgnoredFilesList`). 8/8 regression tests pass; the
  real gate passes end to end.
- RESPONSES Q2 (static-analysis exception register + policy gate): `0dc0e03`.
  `docs/STATIC_ANALYSIS_EXCEPTIONS.md` + `scripts/check-static-analysis-policy.sh`
  (wired into `check-all.sh`); 6/6 policy tests pass. TODO §2.1 wording synced to
  the implemented mechanism.
- Phase 2.3 (remove first-party formatting suppression / source amalgamation):
  `eccf7ce` (auth_core → 4 TUs), `638a75e` (web_server → 7 TUs +
  `web_server_internal.h`), `9e539fb` (web_server_adapter → 5 TUs +
  `web_server_adapter_internal.h`). No first-party `clang-format off` / `.inc`
  amalgamation remains.
- Phase 2.4 (Phase 2 gate verification): fail-closed behavior demonstrated
  (broken analyzer → fail, first-party warning → fail, restored tree → all four
  gate commands pass); see the 2.4 progress section above for evidence.
- Phase 12 (fix executor shutdown and terminal integrity) — complete. One commit.
  §12.1/§12.2: `macro_executor.c` replaced the forced `vTaskDelete` shutdown with a
  cooperative one — a tagged worker message (EXECUTE/STOP), an `executor_stopped`
  binary semaphore, a depth-2 queue, and a `shutting_down` latch. `deinit` now
  rejects new submissions, requests cancellation, enqueues STOP, waits a bounded
  time for the worker to acknowledge exit (leaving the queue/semaphores intact and
  failing closed if it does not), releases USB keys, drains and frees any queued
  plan, deletes the queue and semaphores only after the task exits, clears handles
  and engine state, and retains the first release/shutdown error (including a
  recorded stop-signal failure). §12.3: the ignored `(void)finish_execution(...)`
  now handles its result, and a terminal-publish/reset failure latches the engine
  `unavailable` (rejecting new submits with the primary error retained in status)
  rather than leaving it falsely idle. §12.4: added `EXECUTION_TIMED_OUT` (engine
  maps `APP_ERROR_TIMEOUT` to it; web `execution_state_string` gains "timed_out")
  and a key-release-failure-after-success test. The engine/web changes are host
  tested (executor + web suites green + ASan/UBSan); the FreeRTOS shutdown code is
  firmware-only (compile + fail-closed clang-tidy 0, device-observable).
  **Deferred (recorded, per RESPONSES §8):** the §12.4 observability status fields
  — set_id/macro_id identity, accepted/started/completed timestamps, and a
  current-action summary — to Phase 16 (HTTP API) / Phase 19 (diagnostics), where
  they are consumed and their JSON shape is designed (operator decision confirmed
  in-session); execution_id is already in the status.
- Phase 11 (fix Wi-Fi cleanup) — complete. One commit. `cleanup_resources`
  (`wifi_ap_state.c`) no longer returns early on the first failing teardown step;
  it records the first error and continues through all four steps (wifi_stop,
  handler_unregister, wifi_deinit, netif_destroy), clearing each ownership flag
  only when its own step succeeds, and returns the first error. A stuck resource
  therefore no longer strands the others, and a retry reattempts exactly the
  resources still held. Added `wifi_ap_engine_owns_resources` (true when any of
  netif_created/wifi_initialized/handler_registered/wifi_started is set) and the
  public `wifi_ap_owns_resources`, and wired `app_core`'s
  `adapter_wifi_owns_resources` (previously a conservative `return false`
  placeholder) to it. Host tests updated to the accumulate semantics and extended
  to the §11.3 matrix: for each cleanup step, fail it and assert the later steps
  still run, that only the failing step keeps its ownership flag, that a retry
  clears the residual ownership, and that the original start error stays visible
  when a cleanup step also fails. wifi host suite green + ASan/UBSan + startup,
  fail-closed clang-tidy 0, check-format clean.
- Phase 10 (separate password mismatch from crypto failure) — complete. One
  commit. `auth_password_verify` / `auth_core_password_verify` now return
  `app_error_code_t` with a `bool *out_matches` instead of a bare `bool`, so the
  three outcomes are distinct: APP_ERROR_INVALID_ARGUMENT for a malformed
  argument or a corrupt record (iteration count below the policy floor,
  out-of-range length, embedded NUL), APP_ERROR_INTERNAL for a PBKDF2 derivation
  failure, and APP_ERROR_NONE with `out_matches` for an actual comparison. The
  login handler (`web_server_login.c`) now answers 500 "authentication subsystem
  unavailable" on any non-NONE verify result -- so it does **not** increment the
  login-failure count on a PBKDF2 failure and does **not** answer 401 on a corrupt
  record -- and only records a failure + returns 401 on a genuine mismatch; a
  missing/non-string password is a 400. Host tests cover all four result
  combinations (match, mismatch, derive failure, corrupt record) plus the
  argument/boundary cases; the on-device `test_app` and the existing auth suites
  were migrated to the new signature. storage/auth/web host suites green +
  ASan/UBSan, fail-closed clang-tidy 0, check-format clean.
- Phase 9 (serialize repository operations) — complete. Three commits.
  - **§9.1 (lock abstraction): `d981d13`.** `storage_repository_lock.{c,h}`: one
    non-recursive lock behind an operations seam. Platform default is FreeRTOS
    (`xSemaphoreCreateMutex` / take `portMAX_DELAY` / give) on device; on host it
    is a re-entrancy-detecting flag lock so a take-while-held (a would-be
    production deadlock through a missing `_locked` seam) becomes a visible test
    failure. Unit test 3/3.
  - **§9.2 (serialize mutations): `54b9bd4`.** Every public repository
    transaction split into a lock-acquiring wrapper over a `_locked` core:
    set_read/list/create/update/delete, storage_repository_init, and (new
    non-acquiring `storage_quarantine_file_locked` for the read/load/recovery
    paths) storage_quarantine_file/list. The three recovery entry points
    (atomic/transaction/quarantine `recover_all`) acquire the lock around their
    lock-free `_with_ops` cores (RESPONSES §2.3). Every wrapper reports
    APP_ERROR_INTERNAL on unlock failure so a broken lock is never mutation
    success. app_core initializes the lock in the storage-mount step (before
    recovery) and tears it down in unmount. storage 10/10 + ASan/UBSan,
    fail-closed clang-tidy 0.
  - **§9.3 (concurrency tests): this commit.** Deterministic proofs via the lock
    ops seam (no host threads): same-expected-revision updates cannot both
    succeed; a one-shot interloper fired while the lock is held shows an API
    mutation is blocked during recovery and a delete is blocked during create;
    and a give-failing / take-failing stub shows unlock failure surfaces as
    INTERNAL (never success) and take failure refuses the mutation with the disk
    revision unchanged. The set-repository suite's green run under the
    re-entrancy-detecting default lock also proves no `_locked` seam re-enters.
    storage 10/10 + ASan/UBSan; test-only (no firmware change since `54b9bd4`).
- Phase 8 (make quarantine recoverable) — complete. Three commits.
  - **§8.1–8.2 (layout + staged creation): `d72e997`.** Replaced the flat
    two-files-per-entry quarantine layout with a directory-per-entry layout —
    committed `/data/quarantine/<id>/{record.json,evidence}`, staging
    `/data/staging/quarantine-<id>/{record.json,evidence}` — and a rename-based
    staged 9-step creation (mkdir staging → rename source into evidence → write
    record → fsync evidence+record → validate read-back → fsync staging dir →
    rename directory into quarantine → fsync parent). The record dropped
    `evidence_path` (now 4 fields, derived from the id); `storage_quarantine_list`
    gained `damaged_count` resilience. The obsolete Phase 7 atomic-write
    QUARANTINE_RECORD classifier/validator (and `storage_quarantine_read_record_with_ops`)
    were removed. storage 9/9 + ASan/UBSan, fail-closed clang-tidy 0, check-format
    clean.
  - **§8.3 (startup recovery + wiring): `23fe6e7`.** Added
    `storage_quarantine_recover_all[_with_ops]` implementing the five recovery
    rules (finish provably-complete staging; discard staging whose source was
    never durably moved; preserve ambiguous staging as evidence; never delete an
    unmatched evidence file; a filesystem fault fails closed while a preserved
    ambiguous entry is a non-fatal-to-other-entries health error). Wired into
    `app_core` after transaction recovery (FIX1 §7.4 order); the transaction
    staging-empty assertion now skips `quarantine-<id>` directories via the shared
    `STORAGE_QUARANTINE_STAGING_PREFIX`. storage 9/9 + ASan/UBSan + startup 1/1,
    fail-closed clang-tidy 0.
  - **§8.4 (tests): this commit.** Added the power-loss-after-each-phase matrix
    (the nine phase boundaries collapse to four crash-recoverable states: empty
    staging → discard, evidence-only → preserve, complete staging → finish,
    committed → no-op-and-listable) and the seven corruption cases (record only,
    evidence only, directory name, record id mismatch, unsafe source path, empty
    reason, duplicate committed id). Constructed post-step on-disk states (per the
    §7.5 precedent), because a mid-write fault triggers the create's own rollback
    rather than the crash state recovery handles. storage 9/9 + ASan/UBSan;
    test-only (no firmware change since `23fe6e7`).
- Phase 7.5 (fault injection) — Phase 7 complete. Added a deterministic
  crash-consistency matrix: for each of the eleven atomic-write steps (temporary
  open, partial write, file sync, close, readback, destination-to-backup rename,
  first parent sync, temporary-to-destination rename, second parent sync, backup
  removal, final parent sync) the exact on-disk state a crash would leave is
  constructed and reconciled, asserting the destination ends OLD-complete or
  NEW-complete with no leftover artifacts — never an ambiguous partial state.
  Steps 1-7 recover to OLD (keep-canonical or restore-backup), 8-11 to NEW.
  storage 9/9, full host suite 21/21, ASan/UBSan clean, check-format clean; no
  firmware change (test-only). Recorded deviation: implemented in
  test_storage_atomic_recovery.c using constructed post-step states, because a
  mid-write fault triggers the write's own rollback rather than the crash state
  recovery handles.
- Phase 7.4 (startup ordering): `app_core`'s storage-recovery step now runs
  `storage_atomic_recover_all()` (public entry added to `storage.h`) **before**
  `storage_transaction_recover_all()`, so a manifest's own interrupted write and
  any staging artifact are reconciled before transaction recovery enumerates the
  strict `<uuid>.bin` manifest filenames. Firmware builds, fail-closed clang-tidy
  0, startup host suite passes. §7.5 (fault injection) remains.
- Phase 7.3 (atomic-recovery reconciliation): two commits. Part 1 added the pure
  `storage_atomic_reconcile_decide` decision (exhaustive combinatorial tests).
  Part 2 adds the executor `storage_atomic_recover_all[_with_ops]`: it enumerates
  every leftover `.tmp`/`.bak` artifact across the mount root, `global/`,
  `transactions/`, and each `sets/`/`staging/` subdirectory, groups them by
  destination, gathers on-disk state (canonical presence via stat; backup validity
  via the §7.2 validators; temporaries are never validated because they are always
  discarded unless roll-forward is proven, which never happens at the atomic
  layer), applies the decision, and executes with every rename/unlink/parent-sync
  checked. Malformed/conflicting artifacts are quarantined (evidence retained: on
  any failure the artifact is left in place and the error returned for retry). The
  large artifact list is heap-allocated (recovery is single-threaded at startup).
  A decision refinement (recorded): a temporary's content-validity is consulted
  only in the roll-forward-proven activation path, so unclassifiable transient
  staging temporaries are cleanly discarded rather than wrongly quarantined.
  `quarantine/` is not scanned (its artifacts can't be re-quarantined) — a tracked
  residual. End-to-end executor tests cover keep-canonical, restore-backup,
  discard-temporary, quarantine-conflict, and quarantine-corrupt-backup on a real
  filesystem. storage 9/9, full host suite 21/21, ASan/UBSan clean, firmware
  builds, fail-closed clang-tidy 0, check-format clean. §7.4 (startup ordering)
  and §7.5 (fault injection) remain.
- Phase 7.2 (atomic-recovery validators): new `storage_atomic_validators.{c,h}`
  with a destination classifier (10 object types + unknown), an object-specific
  validator dispatch, and validators for the six types that have serialization
  today: schema marker, set index, global macro index, set metadata (id must
  match the destination's set), transaction manifest (binary shape + id), and
  quarantine record (id must match). The dispatch **fail-closed refuses**
  (`APP_ERROR_NOT_FOUND`) any type without a validator — the macro/procedure/
  progress/settings object types (deferred to Phase 15) and unknown paths — so
  recovery can never activate a candidate it cannot validate. To reuse the
  canonical validation logic, three previously-static readers were exposed:
  `storage_repository_parse_index`, `storage_quarantine_read_record_with_ops`,
  and a new id-parameterized `storage_transaction_read_manifest_with_ops` (the
  existing path-derived reader now delegates to it). New
  `storage_atomic_validators_tests` cover the classifier, all six validators
  (valid + id-mismatch + malformed), and the refuse-without-validator path.
  storage 9/9, full host suite 21/21, ASan/UBSan clean, firmware builds,
  fail-closed clang-tidy 0, check-format clean.
  **Deviation (recorded per RESPONSES §8):** the §7.2 validator typedef's two
  adjacent `const char *` parameters are folded into one `storage_atomic_candidate_t`
  struct to satisfy `bugprone-easily-swappable-parameters` (no first-party
  exemptions); semantics unchanged. §7.3-7.5 (reconciliation, startup ordering,
  fault injection) remain.
- Phase 7.1 (atomic-write artifact parsing): new `storage_atomic_recovery.{c,h}`
  parsing leftover `<destination>.tmp.<uuid>` / `.bak.<uuid>` artifacts back into
  their destination, operation id, and kind. The trailing 36 chars must be a valid
  RFC-4122 v4 UUID (which also guarantees the suffix has no separator, so the
  destination cannot escape the artifact's directory); an empty destination, a
  destination ending in `/`, or any `..` component is rejected; a
  deduplicating list rejects duplicate artifact paths and a full list. Distinguishes
  "not an artifact" (skip) from "malformed artifact" (reject). New
  `storage_atomic_recovery_tests` (storage label). storage 8/8, full host suite
  20/20, ASan/UBSan clean, firmware builds, fail-closed clang-tidy 0, format clean.
  §7.2 (object validators), §7.3 (reconciliation rules), §7.4 (startup ordering),
  and §7.5 (fault injection) remain.
- Phase 6 (filesystem mount ownership and topology): two commits.
  (1) Storage side: replaced the two loose `web_mounted`/`data_mounted` booleans
  with an explicit `storage_mount_state_t` + public `storage_mount_state()`
  query; moved the mount/rollback/ownership orchestration into a host-testable
  `storage_mount_core` behind an ops seam, leaving `storage_mount.c` a thin
  littlefs wrapper that owns the state; replaced `mkdir_checked()` with
  `ensure_directory()` (`storage_mount_topology.c`) which accepts a pre-existing
  path only when it is genuinely a directory and rejects a regular file / symlink
  to a non-directory as `APP_ERROR_STORAGE_CORRUPT`. New `storage_mount_tests`
  cover all six §6.3 rollback scenarios plus the symlink/regular-file rejection.
  (2) app_core side: added a `storage_owns_mount` op wired to `storage_mount_state`
  so cleanup unmounts a partition that is still mounted after a partial/failed
  `storage_mount_all()` even though `storage_mounted` was never set — same
  residual-ownership pattern as http. Host test proves a failed mount that leaves
  a partition mounted is still unmounted during cleanup. storage 7/7, startup 1/1,
  full host suite 19/19, ASan/UBSan clean, firmware builds, fail-closed clang-tidy
  0, check-format clean. (LittleFS directory-permission verification is a
  device-observable item — modes are set 0750 but not asserted on host.)
- Phase 5 (HTTP partial-start lifecycle): added
  `web_adapter_lifecycle_owns_resources()` (a retained handle == still-owned) and
  the public `web_server_owns_resources()`, and wired the latter into app_core's
  `adapter_http_owns_resources` — **closing the Phase 4 http placeholder**. The
  adapter lifecycle already preserved partial-start state (on a route-registration
  failure whose stop also fails it keeps the handle, retains the registered-route
  count and cleanup error, returns the stable `APP_ERROR_IO`, and rejects a new
  start while the residual handle exists) and made stop idempotent (no handle →
  success; success clears config + lifecycle; failure retains state for retry);
  Phase 5 verifies these with the six §5.4 scenarios plus `owns_resources`
  coverage in the `web` suite. With the query wired in, app_core cleanup now
  stops a partially-started server again via the `owns_resources` branch even when
  `http_started` was never set. web suite 2/2, ASan/UBSan clean, firmware builds,
  fail-closed clang-tidy 0, check-format clean. (The wifi ownership adapter
  remains a documented placeholder pending the Wi-Fi cleanup phase.)
- Phase 4.2–4.6 (application lifecycle ownership): extended `app_core_ops_t`
  with the six `*_deinit` teardown callbacks plus `http_owns_resources` /
  `wifi_owns_resources` residual-ownership queries (all checked by
  `operations_valid`); added an `app_core_owned_t` stage tracker; replaced the
  three-flag cleanup with an exhaustive reverse teardown driven by a
  `teardown_stage` helper and `app_operation_result_t` (first primary error and
  first cleanup error preserved separately, `cleanup_incomplete` tracked, every
  remaining stage attempted, an owned flag cleared only when its teardown
  succeeds). Reordered startup so the provisioning decision runs after auth and
  **before** USB/executor/controls — an unprovisioned production device now stops
  cleanly instead of initializing normal-operation tasks and only then returning
  `APP_ERROR_AUTH_REQUIRED` (§4.5). The `*_owns_resources` firmware adapters are
  conservative placeholders (return false) until Phase 5 wires
  `web_server_owns_resources`; the ownership/partial-start logic is fully
  host-tested via fakes. Tests (§4.6): full primary-failure matrix asserting the
  exact reverse teardown of every owned stage; a per-stage cleanup-failure loop
  proving all remaining stages are still attempted and the primary+cleanup errors
  are both retained with `cleanup_incomplete` set; a partial-HTTP-ownership test
  exercising the `owns_resources` branch; and the reordered production-refusal
  test proving USB/executor/controls are never initialized. `cleanup_after_failure`
  was refactored through `teardown_stage` to stay under the clang-tidy
  cognitive-complexity limit. §4.7 gate: `run-tests startup` pass,
  `run-tests --sanitizers startup` pass, `check-firmware` clean; full host suite
  18/18, check-format clean.
- Phase 4.1 (subsystem deinitialization APIs): added `auth_deinit`,
  `usb_keyboard_deinit`, `macro_executor_deinit`, `device_controls_deinit`, and
  `storage_repository_deinit`, each reversing its component's init. `auth_deinit`
  deletes the mutex and zeroes the core; `macro_executor_deinit` deletes the
  worker task before the queue it blocks on, then the queue and mutex;
  `device_controls_init` now captures its task handle so `device_controls_deinit`
  can stop the polling task before freeing the semaphore and resetting the GPIOs;
  `usb_keyboard_deinit` routes through a new host-tested state-machine
  `usb_keyboard_state_deinit` + `driver_uninstall` op (a failed uninstall leaves a
  non-uninitialized state so residual ownership stays visible);
  `storage_repository_deinit` is a documented no-op (the repository holds no
  in-memory state). Host coverage: usb state-machine deinit test (success,
  uninitialized no-op, failed-uninstall residual-ownership) and a
  storage-repository deinit no-op test; the four ESP-IDF wrappers are
  firmware-build + clang-tidy verified (they are not host-compiled, matching how
  their inits are structured). Full host suite 18/18, ASan/UBSan clean, firmware
  builds, fail-closed clang-tidy 0, check-format clean.
- Phase 3.2 (structured startup log events): extended `app_core_log_event_t`
  with `cleanup_error` (renamed from `secondary_error`), `cleanup_incomplete`,
  and `operation_id`; the existing `stage` field is the affected-subsystem
  indicator required by SPEC §3.2 (documented in the header). The console adapter
  reports cleanup completeness; the cleanup path marks `cleanup_incomplete`.
  Structured events never carry credentials/tokens/cookies/macro source (only the
  development-only credentials event carries the dev AP/web strings). Host tests
  add an exact ordered-stage assertion for a clean startup, a no-secret-leak
  assertion across every structured event, and `cleanup_incomplete`/`operation_id`
  checks on the cleanup event. `startup` suite passes, ASan/UBSan clean, full host
  suite 18/18, firmware builds, fail-closed clang-tidy 0, check-format clean.
  Phase 3 complete.
- Phase 3.1 (structured operation result): new `support` component with
  `app_operation_result_t` (first primary error preserved; first cleanup error
  preserved separately; `cleanup_incomplete` flag), `app_operation_success` /
  `app_operation_result_ok` / `app_operation_record_primary` /
  `app_operation_record_cleanup`. Additive only — no existing `app_error_code_t`
  API collapsed. 8 host tests under the `support` label (first-primary-wins,
  first-cleanup-wins, primary+cleanup coexist, NONE no-ops, NULL-safe); host
  tests pass, ASan/UBSan clean, firmware builds, fail-closed clang-tidy 0,
  check-format clean.

## Residual items to close (tracked, not hidden)

- **Phase 2 / RESPONSES Q1 — explicit first-party include-cycle regression
  test. CLOSED.** Added `tests/scripts/test-clang-tidy-include-cycle.sh`, a
  real-analyzer regression test that builds crafted fixtures and runs the pinned
  clang-tidy (esp-clang 19 when the ESP-IDF toolchain is on PATH; otherwise the
  CI-pinned apt clang-tidy 18 — both honor `IgnoredFilesList` identically) with
  the repo `.clang-tidy`. It proves a genuine first-party header cycle is
  reported and fails, the same cycle under a `managed_components` root is excluded
  cleanly (no finding, zero exit), and a no-cycle unit passes (negative control) —
  5/5. A matching fake-suite case (`zero exit with a first-party include-cycle
  finding fails`) was added to `test-check-firmware.sh` (now 9/9). Wired into
  `check-scripts.sh`. With this, all six of RESPONSES Q1's required regression
  cases are covered: first-party warning, first-party include cycle, third-party
  cycle excluded, analyzer crash/nonzero exit, zero first-party TUs, and clean
  run.

### 2.3 progress — complete

All three source amalgamations are now normal `.c` translation units; no
first-party `clang-format off` / `.inc` amalgamation remains (the only surviving
`clang-format off` are in third-party `managed_components` and one legitimate
key-table exemption in `test_app/main/test_auth.c`, both out of scope).

- `auth_core` → four `.c` units (`eccf7ce`). No static crossed a fragment
  boundary, so no new externs were needed.
- `web_server` → seven `.c` units + `web_server_internal.h` (`638a75e`). Shared
  state (`server_configuration`, `server_lifecycle`) became single-definition
  `extern`; ~7 shared helpers + 7 handlers became non-static.
- `web_server_adapter` → five `.c` units + `web_server_adapter_internal.h`.
  `json_writer_t` and the cross-fragment helpers (`writer_append_text` /
  `writer_append_escaped` / `writer_finish` used by the json unit,
  `valid_lifecycle_ops` used by the lifecycle unit) moved to the internal header
  and gained external linkage; `writer_append_byte` stays static (common-only).
  Both the component `CMakeLists.txt` and the `web_server_adapter_tests` host
  target list the five units.

Verified: firmware builds and links (no duplicate symbols), fail-closed
clang-tidy 0 (include-cleaner clean), `check-format.sh` clean, `web` host tests
2/2, native coverage gate 95%.

### 2.4 progress — complete (Phase 2 gate verification)

Fail-closed behavior demonstrated three ways:

- **Analyzer command broken → gate fails.** The 2.2 regression suite exercises
  the real `check-firmware.sh` logic against a fake analyzer:
  `analyzer executable missing fails`, `analyzer nonzero exit with no warnings
  fails`, `missing compile database fails`, `invalid compile database fails`,
  `zero first-party translation units fails` — 8/8 pass.
- **First-party warning → gate fails (on the real tree).** Injected an unused
  `#include "macro_limits.h"` into `web_server/web_origin.c`; the real
  `check-firmware.sh` emitted `misc-include-cleaner … not used directly` and
  exited 1 (`run-clang-tidy failed … with status 1`). Reverted.
- **Restored tree → all checks pass.** The four §2.4 gate commands all exit 0:
  `check-format.sh` (0), `check-firmware.sh` (0), `check-scripts.sh` (0),
  `run-tests.sh` (17/17 suites, 0 failed). Script regression suites: 8/0
  (`test-check-firmware.sh`) and 6/0 (`test-static-analysis-policy.sh`).

Phase 2 (make the quality gate fail closed) is complete.

## Environment-blocked (hardware / HIL) items

These remain **open** until observed on real hardware (RESPONSES Q5). Each will carry
prerequisite hardware, exact procedure, expected evidence, pass/fail criteria, and safety
notes when its phase is reached:

- eFuse / flash-encryption (or HMAC-key) provisioning confirmation (§14.2);
- Linux + ChromeOS USB enumeration/typing matrix (§20.2);
- real SoftAP / browser integration on the ESP32-S3 (§20.3);
- real power-interruption testing (§20.4);
- measured physical cancellation latency and reset-gesture validation (§20.5);
- observed release-all behavior on hardware.

The *software* portion of secure provisioning (encrypted-NVS config, provisioning state
machine, readback validation, setup/reset APIs, production-config rejection, host-testable
policy) is **not** hardware-blocked and is implemented in its phase.

## Unresolved blockers

- None yet.

## Deviations from the TODO

- **§7.2 validator typedef signature.** The FIX1 §7.2 sketch declared
  `storage_atomic_validate_fn` with two separate adjacent `const char *`
  parameters (`destination`, `candidate_path`). Those are folded into one
  `storage_atomic_candidate_t` struct because the first-party
  `bugprone-easily-swappable-parameters` policy (RESPONSES Q2 / FIX1 §3.4)
  forbids exempting swappable parameters we control — the `.clang-tidy` comment
  requires restructuring, not `IgnoredParameterNames`. Semantics are unchanged;
  the validator still receives the destination and candidate path. Recorded per
  RESPONSES §8 (the TODO is an implementation plan, not an authority above the
  no-suppression rule).
- **§8.1 and §8.2 landed as one commit.** The TODO lists the layout change (§8.1)
  and the staged creation (§8.2) as separate steps, but the staged 9-step
  creation *is* the new create — a layout-only intermediate would ship a
  throwaway non-staged create plus throwaway tests. They were implemented
  together in `d72e997` so every committed state is internally consistent and
  gate-green.
- **Quarantine source ownership uses rename, not copy (§8.2 offered "copy or
  rename").** The source is atomically renamed into staged evidence, matching the
  nine-step list (which has no explicit source-removal step) and LittleFS/POSIX
  atomic-rename semantics. **Documented rollback behavior:** a step-7 directory
  rename is atomic, so an interrupted create leaves *either* the staging
  directory *or* the committed directory, never a half-committed both; a failure
  during steps 3–6 (record write, syncs, validation) renames the evidence back to
  the source and removes the staging directory, so the source is restored; a
  failure at step 1/2 leaves the source untouched. Recovery's rule-2 "restore the
  source when activation never occurred" is therefore automatic: an interrupt
  before durable evidence never moved the source, so reconciliation only removes
  the empty staging directory.
- **Record dropped `evidence_path`; list gained `damaged_count`.** Under the
  dir-per-entry layout the evidence path is fully determined by the entry id, so
  storing it is redundant; the record is four fields (`schema_version`, `id`,
  `source_path`, `reason`) and the struct's `evidence_path` is derived. To satisfy
  §8.3's "return valid entries plus a health error", `storage_quarantine_list_t`
  gained a `damaged_count`: a corrupt/missing record, missing evidence, or bad
  directory name is counted rather than failing the whole list. Both are safe for
  the unreleased `0.1` format (no migration; no production consumer of the struct
  yet). Operator decisions confirmed in-session.
- **Transaction staging-empty assertion now skips quarantine staging.** Not in
  the TODO, but required for correctness: quarantine staging shares
  `/data/staging` with transaction staging, and quarantine recovery runs *after*
  transaction recovery (FIX1 §7.4 order). The transaction staging-empty check in
  `storage_transaction.c` therefore skips `quarantine-<id>` directories (shared
  `STORAGE_QUARANTINE_STAGING_PREFIX`) so a leftover in-progress quarantine
  staging is not misreported as a stray transaction staging entry before
  quarantine recovery reconciles it.
- **Phase 7 quarantine atomic validator removed.** The dir-per-entry staged
  creation writes the record with a plain durable write inside staging and
  commits by directory rename, so there are no per-file `.tmp/.bak` quarantine
  artifacts. The Phase 7 `STORAGE_ATOMIC_OBJECT_QUARANTINE_RECORD` classifier and
  `validate_quarantine_record` (and their only helper,
  `storage_quarantine_read_record_with_ops`) are obsolete and were removed; the
  atomic-validators classifier test now asserts a flat
  `/data/quarantine/<uuid>.json` classifies as UNKNOWN, and the atomic-recovery
  quarantine-count tests expect one committed subdirectory per artifact.
- **§9 serialization scope is today's repository surface.** FIX1 §7.5 lists set,
  macro, procedure, progress, and settings mutations plus import/restore; only
  set CRUD, quarantine mutation, and startup recovery exist now. Those are
  serialized. The macro/procedure/progress/settings object repositories are
  Phase 15 and import/restore is Phase 18; each will acquire the same lock in its
  phase (the §9.3 exclusion proof generalizes). The §9.3 "import/restore excludes
  all other mutations" checkbox is therefore left open, not falsely ticked.
- **`storage_transaction_write_manifest` and `storage_repository_deinit` stay
  lock-free.** §9.2 says public repository functions acquire the lock, but
  `storage_transaction_write_manifest` is a manifest-write primitive invoked by
  set create/delete *while they hold the lock*; making it self-acquire would
  deadlock the non-recursive mutex, so it stays lock-free (always called under the
  lock). `storage_repository_deinit` holds no state and does no I/O (a documented
  no-op), so it takes no lock; the lock's own teardown is
  `storage_repository_lock_deinit` in the unmount step.
- **Lock lifecycle lives in the storage mount/unmount steps.** The TODO §9.1
  shows `storage_repository_lock_init` but not its call site. It is initialized in
  `app_core`'s storage-mount adapter (so the lock exists before startup recovery,
  which serializes behind it) and torn down in the unmount adapter, which
  unmounts even if lock teardown fails. init/deinit are exposed on the public
  `storage_repository.h`; the take/give mechanism and the test ops seam stay in
  the private `storage_repository_lock.h`.
- **§10 login: a missing/non-string password is now 400, not 401.** Previously
  the handler folded a missing password field into the same "invalid credentials"
  401 path (incrementing the failure count). With the verify result now
  distinguished, an absent or non-string `password` is a malformed request
  (400 Bad Request) and is not counted as a login attempt; only a genuine
  password mismatch increments the failure count and returns 401. The host auth
  suites in `tests/host/` are not covered by `check-format.sh` (it scans only
  `firmware/`), so the fixture `.inc` include is kept in its own include block to
  stay stable under editor include-sorting while remaining first.

## Phase 13 — Device-controls shutdown and failure visibility

Status: **Implementation complete; diagnostics aggregation remains open for Phase 19.**

Evidence:

- implementation commit: `dbe4bcb409af751a290739a5751231419fcd787b`;
- cooperative task stop, bounded wait, safe GPIO state, cleanup accumulation, idempotent deinit,
  and atomic health updates are implemented behind the host-testable operations seam;
- host tests cover confirmation-signal failure, cancellation failure, GPIO output failure,
  stop timeout, repeated deinit, and no callback/resource use after deletion;
- the controls policy exceeded the required 90% line and 80% branch coverage thresholds;
- redacted controls-health aggregation is intentionally still unchecked and will be completed
  with the common subsystem diagnostics report in Phase 19.

## Phase 14 — Encrypted persistent provisioning

Status: **Software implementation and CI validation complete. Physical confidentiality remains
an explicit Phase 20 hardware claim.**

Evidence commit: `840544dbbe3bdd93366c4ac160cb7d658bfe1542`.

Validation evidence:

- Quality workflow run `30295745891`: authoritative `./scripts/check-all.sh` passed, including
  ESP-IDF v5.5.5 production GCC build, production and device-test clang-tidy, formatting,
  script policy, documentation, frontend checks, and host tests;
- Host Tests workflow run `30295743521`: normal host tests, ASan/UBSan, native coverage,
  frontend tests, and frontend coverage all passed;
- Device Test Build workflow run `30295743845`: device-test lint and ESP32-S3 firmware build
  passed using ESP-IDF v5.5.5.

Implemented behavior:

- the 8 MiB partition layout contains exactly one 4 KiB encrypted `nvs_keys` partition while
  retaining aligned equal-size OTA slots;
- production NVS uses HMAC-backed encryption and the production gate rejects disabled encryption,
  alternate/unprotected schemes, and manufacturing/development credential logging;
- the provisioning repository uses a canonical bounded record, revision conflicts,
  `nvs_commit()`, readback verification, factory reset verification, and secure zeroing;
- setup AP credentials and one-time setup code are domain-separated HMAC derivations bound to
  the device SoftAP MAC; the matching offline manufacturing label generator has fixed vectors;
- unprovisioned devices expose only setup routes and do not start USB, executor, repositories,
  or normal authenticated routes;
- setup JSON rejects trailing data, duplicate/unknown fields, wrong types, oversized values,
  and embedded NUL escapes; credential-bearing buffers are wiped on every exit path;
- ordinary logs contain no plaintext credentials; the only plaintext path is a permanently
  warned manufacturing-only compile option rejected by production configuration.

Not claimed by this phase:

- a physical ESP32-S3 has not yet been eFuse-provisioned and inspected for raw-flash secrecy;
- real first-run, reboot persistence, button confirmation, and interruption behavior remain in
  the Phase 20 hardware matrix.

## Phase 15 — Complete storage object repositories

Status: **Complete.**

Implementation commit: `b1ad2b8bb56d999dd88df8ace328c9990b41100c`.

Implemented and validated:

- macro list/create/read/update/delete/duplicate/reorder, bounded procedure-reference details,
  scope validation, revision conflicts, and corrupt-object/order quarantine;
- procedure list/create/read/update/delete/reorder, unique step IDs, strict macro scope/reference
  validation, exact fields and bounds, revision conflicts, corrupt-object quarantine, and progress
  removal before procedure deletion;
- progress read/update/reset with canonical JSON, procedure and step validation, atomic writes,
  completed/skipped exclusivity, explicit stale-revision snapshots, and corruption quarantine;
- redacted non-secret settings read/update over the encrypted provisioning record, including
  always-select-set, active-set selection, and physical-confirmation policy;
- set deletion clears a matching encrypted-NVS active-set selection before moving the set to
  transaction-owned trash, so interruption cannot leave a dangling selection; existing delete
  transaction recovery remains idempotent and preserves global macros.

Validation:

- `./scripts/run-tests.sh storage`;
- `./scripts/run-tests.sh --sanitizers storage`;
- `./scripts/generate-native-coverage.sh`;
- authoritative `./scripts/check-all.sh`;
- ESP-IDF v5.5.5 production and device-test builds with fail-closed clang-tidy;
- macro, procedure, progress, active-set deletion, provisioning-settings, atomic-validator, and
  transaction-recovery host suites all execute as registered CTest targets.
