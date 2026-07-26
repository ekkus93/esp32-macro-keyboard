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
| 5 | Correct HTTP partial-start lifecycle | not started |
| 6 | Correct filesystem mount ownership and topology | not started |
| 7 | Atomic-write artifact recovery | not started |
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

- None yet. Any deviation will be recorded here with rationale and the FIX1 decision
  reference, per RESPONSES §8.
