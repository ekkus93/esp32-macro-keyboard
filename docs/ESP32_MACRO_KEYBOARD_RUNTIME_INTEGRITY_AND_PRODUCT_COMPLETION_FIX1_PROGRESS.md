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
| 7 | Atomic-write artifact recovery | in progress (7.1 done; 7.2-7.5 open) |
| 8 | Make quarantine recoverable | not started |
| 9 | Serialize repository operations | not started |
| 10 | Separate password mismatch from crypto failure | not started |
| 11–13 | Wi-Fi / executor / controls cleanup and visibility | not started |
| 14 | Encrypted persistent provisioning | not started |
| 15 | Complete storage object repositories | not started |
| 16 | Complete the HTTP API | not started |
| 17 | Replace frontend mock behavior | not started |
| 18 | Import / export / backup / restore | not started |
| 19 | Diagnostics and observability | not started |
| 20 | Hardware and integration validation | environment-blocked (see below) |
| 21 | Release budgets and immutable CI | not started |
| 22 | Documentation synchronization | ongoing |
| 23 | Final regression and acceptance gate | not started |

## Completed tasks (commit evidence)

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
