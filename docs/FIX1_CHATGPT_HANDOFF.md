# FIX1 Handoff — Resume at Phase 13 (for a fresh agent / ChatGPT)

This document hands off the **FIX1 "runtime integrity and product completion"**
program to a new agent with no prior conversation context. It captures exactly
where the work stands, the rules you must follow, the patterns already
established, and a detailed map of the next phase (13). Read it together with the
authoritative sources listed in §2 — this doc orients you; those docs are the
source of truth.

## 0. TL;DR

- Work happens on branch `master`, committed directly (no PR branch), one
  coherent commit (or a small commit sequence) per phase/sub-phase.
- **Phases 1–12 are complete and gate-green.** Pick up at **Phase 13: "Fix
  device-controls shutdown and failure visibility"** (`docs/…FIX1_TODO.md` §13).
- Every change must pass the full gate `./scripts/check-all.sh` before the phase
  is "done", and you must **activate Node 24 first** or that gate aborts (see §3).
- Do NOT relax the fail-closed / no-suppression rules (see §4). No `|| true`,
  `NOLINT`, `eslint-disable`, `-Wno-*`, `clang-format off`, or coverage-exclusion
  markers on first-party code. Only the three registered `.clang-tidy` exceptions
  (RESPONSES Q2) are allowed.
- After each phase: tick the TODO checkboxes, add an evidence entry to the
  progress doc, and record any deviation in its "Deviations from the TODO"
  section.

## 1. Project layout

Monorepo, no root build file (see `CLAUDE.md`).

- `firmware/` — ESP-IDF C11 firmware for ESP32-S3 (`firmware/test_app/` is a
  separate on-device Unity test app; both are built by the gate).
- `webapp/` — React 19 / TS / Vite frontend (served by the device).
- `tests/host/` — native C host tests (CMake/CTest) with fakes for every hardware
  backend; a **custom assert harness** (`tests/host/support/test_assert.*`), not
  Unity.
- `scripts/` — authoritative bash entry points (prefer these over ad-hoc
  commands).
- `docs/` — specs, TODO, progress, schemas.

## 2. Authoritative documents (read before coding)

- Spec: `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
- Plan/TODO (mandatory sequence, phase checkboxes):
  `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
  — **Phase 13 is section `## 13`.**
- Operator decisions (binding answers to open questions):
  `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_RESPONSES.md`
- Progress log (per-commit evidence + "Deviations from the TODO"):
  `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md`
- `CLAUDE.md` (repo root) — toolchain versions, hard rules, code style.
- The Phase 8 handoff `docs/FIX1_PHASE8_HANDOFF.md` is a good example of the
  detail level expected when reasoning about a phase.

RESPONSES §8 is important: **the TODO is an implementation plan, not an authority
above the no-suppression / fail-closed rules.** When the TODO and a hard rule
conflict, follow the rule and record the deviation in the progress doc.

## 3. Toolchain, build, and test

Exact versions are enforced; mismatches produce spurious diffs.

- **ESP-IDF must be tag `v5.5.5`, target `esp32s3`.** Before any firmware/device
  command: `. "$HOME/esp/esp-idf-v5.5.5/export.sh"` (sets `IDF_PATH`, puts the
  esp-clang toolchain on `PATH`).
- **Node must be exactly `v24.18.0`.** Activate it before `check-all.sh` /
  webapp/docs commands:
  `export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 24.18.0`.
  **Gotcha that bit me repeatedly:** `check-all.sh` verifies the toolchain first
  and **aborts with exit 1 before running anything** if Node is not 24.18.0. If
  you see `error: Node.js v24.18.0 is required`, you forgot this — it is not a
  code failure.

### Commands (run from repo root)

- Full gate (must pass to call a phase done): `./scripts/check-all.sh`
- Host tests: `./scripts/run-tests.sh [--sanitizers|--coverage] [<label>]`.
  One mode and one label max. Labels: `support parser storage executor auth web
  startup usb controls wifi model`. **Phase 13's label is `controls`.**
- Firmware build + fail-closed clang-tidy: `./scripts/check-firmware.sh` (SLOW —
  it does a full clean rebuild of the ESP32-S3 image plus the on-device test_app,
  ~8–10 min; run it in the background and wait for it).
- Format check (no autofix): `./scripts/check-format.sh`. **Note:** it only scans
  `firmware/main firmware/components firmware/test_app/main` for C — **not
  `tests/host/`** (see §9).
- Docs lint: `./scripts/check-docs.sh`.
- Native coverage gate (line ≥90 / branch ≥80): `./scripts/generate-native-coverage.sh`
  — enforced only on a specific "pure-policy" file list (see §9); storage/executor
  wrapper files are NOT in it, but `macro_executor_engine.c`,
  `device_controls_logic.c`, etc. ARE.

### The per-change loop I used (fast → full)

1. `./scripts/run-tests.sh <label>` then `./scripts/run-tests.sh --sanitizers <label>`.
2. `./scripts/check-format.sh` (fast).
3. `./scripts/check-firmware.sh` (slow; background it) — only needed when firmware
   C changed. Watch for the clang-tidy landmines in §9.
4. `./scripts/check-docs.sh` after doc edits.
5. Full `./scripts/check-all.sh` (with Node 24 active) before declaring the phase
   done.

## 4. Non-negotiable rules

- **No failure-hiding / no first-party suppression.** CI runs clang-tidy with
  `WarningsAsErrors: '*'` and ESLint/stylelint `--max-warnings=0`. Every
  first-party warning is a defect. No `|| true`, error redirection, `NOLINT`,
  `eslint-disable`, `-Wno-*`, `clang-format off`, `.inc` amalgamation to dodge
  format, or coverage-exclusion markers on first-party code.
- **Fail closed.** On an error that can't be safely handled, surface it (return
  the error, leave the subsystem unavailable) rather than proceeding as if fine.
- **Git commits:** conventional style, subject < 72 chars. **Do NOT add a
  `Co-Authored-By:` trailer** — a global commit-msg hook rejects it.
- Keep the tree internally consistent per commit (callers migrate with the API
  they call; the gate is green at each commit).
- When you use a pronoun for the user or anyone else and their pronouns aren't
  stated, use they/them.

## 5. What's complete this session (Phases 8–12)

All on `master`, each full-gate-green (host tests 22/22, firmware + test_app
build, fail-closed clang-tidy 0, webapp, format, docs, coverage). Commits:

- **Phase 8 — recoverable quarantine** (`d72e997`, `23fe6e7`, `fd5495e`):
  dir-per-entry layout `/data/quarantine/<id>/{record.json,evidence}` with a
  rename-based staged 9-step creation and startup recovery; 4-field record
  (evidence_path derived); `storage_quarantine_list_t.damaged_count` resilience.
- **Phase 9 — serialize repository operations** (`d981d13`, `54b9bd4`, `a9261a9`):
  `storage_repository_lock.{c,h}` (FreeRTOS on device, re-entrancy-detecting flag
  lock on host, behind an ops seam); public repository transactions serialized via
  `_locked` cores; recovery entry points serialized; lifecycle wired into
  `app_core` mount/unmount; deterministic concurrency proofs.
- **Phase 10 — separate password mismatch from crypto failure** (`98c2c58`):
  `auth_password_verify` returns `app_error_code_t` + `bool *out_matches`; login
  handler answers 500 (no failure-count increment) on PBKDF2/record failure, 401
  only on genuine mismatch.
- **Phase 11 — fix Wi-Fi cleanup** (`cbf83e7`): `cleanup_resources` accumulates
  the first error and continues all teardown steps, clearing each ownership flag
  only on its own success; added `wifi_ap_owns_resources` wired into `app_core`.
- **Phase 12 — executor shutdown and terminal integrity** (`dd11e16`):
  cooperative worker shutdown (tagged EXECUTE/STOP queue message +
  `executor_stopped` semaphore + bounded-wait deinit); explicit
  `finish_execution` handling with an `unavailable` fault latch; new
  `EXECUTION_TIMED_OUT` terminal state. (§12.4 observability status fields
  deferred — see §8.)

Phase status table and per-commit evidence live in the progress doc; keep adding
to them.

## 6. Established patterns to reuse

These are precedents from this session that Phase 13 (and later) should follow:

- **Ops-seam + fake for host testing (the key idea).** Firmware code that touches
  FreeRTOS/GPIO/USB is split into: (a) a pure engine/logic module that takes an
  operations struct of function pointers, host-tested with a fake backend; and
  (b) a thin firmware adapter that fills the struct with real ESP-IDF calls and is
  validated only by compile + clang-tidy + eventual device testing. See
  `wifi_ap_state.c` (engine) vs `wifi_ap.c` (adapter), and
  `macro_executor_engine.c` vs `macro_executor.c`. **`device_controls` does NOT
  yet have this split** — see §7 for the decision this forces in Phase 13.
- **`_locked` transaction split** (`storage_repository_sets.c`): a public function
  acquires a lock then calls a `static … _locked` core; internal callers use the
  `_locked` form and never re-acquire (a non-recursive lock would deadlock). The
  host default lock detects re-entrancy so a missing seam fails a test.
- **Accumulate-and-continue cleanup** (`wifi_ap_state.c` `cleanup_resources` +
  `record_first_error`): never early-return on the first teardown failure; run
  every step, clear each ownership flag only on its own success, return the first
  error. **Phase 13's deinit wants exactly this shape.**
- **Cooperative task shutdown** (`macro_executor.c`): a tagged stop message OR a
  `volatile bool stop_requested` the task observes, a `*_stopped` binary semaphore
  the task gives on exit, and a deinit that requests stop → waits bounded → only
  then deletes the queue/semaphores → clears handles. Never `vTaskDelete(other
  task)` mid-work. **Phase 13 replaces device_controls' forced `vTaskDelete` with
  this.** On wait timeout, do NOT delete the resources the task may still touch
  (use-after-free); fail closed and return the error.
- **Fault latch / "leave unavailable"** (`macro_executor_engine.c` `unavailable`):
  when a terminal state can't be published, latch a monotonic flag that rejects
  new work until re-init, rather than appearing falsely idle.
- **Health struct + getter** returned by value (see Phase 13 §13.2 shape). Update
  atomically, log failures via ESP logging, and expose only redacted fields to
  diagnostics.
- **Doc bookkeeping every phase**: tick TODO checkboxes; add a progress "Completed
  tasks" entry with the commit hash + validation evidence; record deviations.

## 7. Where to pick up — Phase 13 (detailed map)

**Goal (TODO §13): "Fix device-controls shutdown and failure visibility."** It is
the device-controls analog of Phase 12. Sub-phases §13.1–§13.4.

### Current code (to change)

- `firmware/components/device_controls/device_controls.c` — the firmware layer:
  a `controls_task` polling loop (`confirmation_semaphore`, cancel-button debounce
  → `macro_executor_cancel()`, LED/indicator via GPIO), `device_controls_init`,
  and a **`device_controls_deinit` that force-deletes the task with
  `vTaskDelete(controls_task_handle)`** (the bug: same class as Phase 12's old
  deinit — it can kill the task mid-poll and does not leave outputs safe).
- `firmware/components/device_controls/device_controls_logic.{c,h}` — the pure
  logic (debounce, indicator on/off phase). **Host-tested** via
  `tests/host/test_device_controls.c` (label `controls`), and
  `device_controls_logic.c` is on the strict coverage list (≥90/≥80).
- `firmware/components/device_controls/include/device_controls.h` — public API
  (`device_controls_init/deinit`, `set_indicator`, `wait_for_confirmation`). No
  health getter yet.
- `app_core.c` has `adapter_controls_init/deinit` and `adapter_set_indicator`; the
  diagnostics wiring for controls health will land when §13.2 exposes it.

### What §13 asks for

- **§13.1** — track the task handle + a stop request + a `controls_stopped`
  semaphore:

  ```c
  static TaskHandle_t controls_task_handle;      // already exists
  static SemaphoreHandle_t controls_stopped;     // new
  static volatile bool controls_stop_requested;  // new
  ```

  The task must break **only after observing the stop request**, set a health
  result, signal completion (give `controls_stopped`), and delete itself.
- **§13.2** — add a health struct + getter:

  ```c
  typedef struct {
      app_error_code_t last_error;
      app_error_code_t cleanup_error;
      bool task_running;
      bool indicator_output_failed;
      bool confirmation_signal_failed;
      bool cancel_request_failed;
  } device_controls_health_t;
  device_controls_health_t device_controls_get_health(void);
  ```

  Update health atomically (a critical section / the existing `indicator_lock`
  pattern), log failures via ESP logging, and expose **redacted** health through
  diagnostics (diagnostics report schema is `docs/schemas/diagnostic-report.schema.json`;
  it forbids secret-ish key names but health booleans/error codes are fine).
- **§13.3** — deinit: request task stop; wait bounded; configure outputs to a
  **documented safe state** (e.g. LED off / indicator BOOTING — document exactly);
  delete semaphores after task exit; clear handles; return a cleanup failure if
  any step fails (accumulate-and-continue, first error wins). Follow the Phase 12
  deinit shape precisely, including the "on stop-wait timeout, do not delete
  resources; fail closed" rule.
- **§13.4** — tests: semaphore give failure; cancel failure; GPIO output failure;
  task stop timeout; second deinit call; no use-after-free after deinit.

### The design decision Phase 13 forces (flagged for you)

The §13.4 tests are **task/GPIO/semaphore-behavioral**, but `device_controls.c`
talks to FreeRTOS/GPIO **directly** — there is no engine/ops-seam like `wifi_ap`
or `macro_executor` have, so these behaviors are not host-testable as-is. You have
two defensible options; pick one and record it as a deviation with rationale:

1. **Introduce a `device_controls` engine + ops seam** (mirrors
   `wifi_ap_state.c`/`wifi_ap.c`): move the shutdown/health state machine into a
   host-testable engine driven by an ops struct (task-stop signal, semaphore
   give, GPIO write, cancel request, now/wait), fake it in
   `tests/host/test_device_controls.c`, and make `device_controls.c` a thin
   adapter. This is the most faithful to §13.4 ("Add tests …") and matches the
   established pattern, but is the larger change. **Recommended** if you want the
   §13.4 matrix genuinely host-tested (the FIX1 program clearly values that — see
   Phases 11/12 test matrices).
2. **Keep the shutdown/health in firmware** (validated by compile + fail-closed
   clang-tidy + device testing, like Phase 12's `macro_executor.c` §12.1/§12.2),
   and factor only the **atomic health-update logic** into
   `device_controls_logic.c` for host tests. Document the §13.4 behaviors as
   device-observable (Phase 20 does hardware validation). Smaller, but leaves the
   §13.4 checkboxes partly device-verified rather than host-tested.

If unsure, prefer option 1 — it keeps the phase's test intent intact and reuses a
pattern that is already proven twice in this codebase. Ask the user (Phillip) if
the scope tradeoff needs a decision; otherwise proceed and record the choice.

### Suggested Phase 13 commit sequence

1. §13.1+§13.2: engine/ops seam (if option 1) or firmware state + health struct +
   `device_controls_get_health`; task observes stop request, sets health, signals,
   self-deletes. Host tests for the health/stop logic.
2. §13.3: deinit (stop → bounded wait → safe outputs → delete after exit → clear
   handles → accumulate cleanup error). Wire diagnostics exposure of redacted
   health.
3. §13.4: the six-case test matrix. Full `check-all.sh` gate; TODO checkboxes;
   progress entry + deviation.

## 8. Deferred-items registry (do not lose these)

Recorded in the progress doc "Deviations" section; re-surface them in their target
phases:

- **Phase 12.4 observability status fields** — `set_id`/`macro_id` object identity
  in the execution status, accepted/started/completed timestamps, and a
  current-action summary — deferred to **Phase 16 (HTTP API)** / **Phase 19
  (diagnostics)**, where they are consumed and their JSON shape is designed.
  (`execution_id` is already in the status; the timeout is already a distinct
  `EXECUTION_TIMED_OUT` state.)
- **Phase 9 import/restore serialization** — the one open §9.3 checkbox — deferred
  to **Phase 18** (import/restore feature). It will acquire the same repository
  lock; the §9.3 exclusion proof generalizes.
- **Object repositories** (macros/procedures/progress/settings) and their atomic
  validators — **Phase 15**. The Phase 7 atomic classifier already recognizes
  their destinations but has no validator (recovery refuses to activate them).
- **Environment-blocked / hardware (HIL) items** — see the progress doc's
  "Environment-blocked" section (USB enumeration matrix, real SoftAP, power-loss
  on device, eFuse/flash-encryption provisioning confirmation, cancellation
  latency). These wait for **Phase 20** on real hardware — which is when Phillip
  said the original agent (me) will resume.

## 9. Gotchas & landmines

- **clang-tidy (esp-clang 19, `WarningsAsErrors: '*'`), enforced by
  `check-firmware.sh`:**
  - `readability-function-cognitive-complexity` limit **25** — split helpers when
    you add branches. (Hit this twice this session: the login handler and would
    have on executor deinit; extracting helpers fixed it.)
  - `readability-identifier-length` — identifiers must be **≥3 chars**; `id` is
    rejected (use `entry_id`, etc.). `ops` (3) is fine.
  - `bugprone-easily-swappable-parameters` — two adjacent same-type params are
    flagged **unless they're used together in the same call**
    (`SuppressParametersUsedTogether`) or the run is broken by a different-typed
    param. Restructure (fold into a struct, reorder, or pass a struct pointer) —
    do NOT exempt. `.clang-tidy` `IgnoredParameterNames` only exempts
    third-party-dictated adjacencies.
  - `misc-include-cleaner` — include headers for the symbols you actually use;
    prefer manual includes (it rewrites `<cstddef>`→`<stddef.h>` under `--fix`).
    Platform-divergent includes go behind `#ifdef ESP_PLATFORM` so the analyzed
    (ESP) build doesn't see an "unused" host include.
- **Formatter / `tests/host` is NOT format-gated.** `check-format.sh` scans only
  `firmware/…`. An editor/IDE clang-format-on-save can reorder `#include` lines
  and reflow host test files; that's fine functionally, but **`.inc` include order
  matters** in the test aggregators (e.g. `tests/host/test_auth.c` includes a
  fixture `.inc` that must come first). Keep such a must-be-first include in its
  **own include block** (blank line separated) so include-sorting can't move it.
- **Coverage gate** (`generate-native-coverage.sh`, ≥90 line/≥80 branch) applies
  only to a hard-coded "pure-policy" file list (engine/logic files like
  `macro_executor_engine.c`, `device_controls_logic.c`, `auth_core.c`,
  `web_*` policy files, `wifi_ap_state.c`, `app_core_sequence.c`). If you add code
  to one of those, keep it covered. Wrapper/firmware files aren't in the list.
- **Firmware-only code has no host tests** — `macro_executor.c`,
  `device_controls.c`, `wifi_ap.c`, the `app_core` adapters, etc. are validated by
  compile + clang-tidy + device. Reason carefully; there's no test net until
  Phase 20.
- **The gate is slow** because `check-firmware.sh` (also run inside `check-all.sh`)
  does `idf.py set-target` → full clean rebuild + the test_app build. Background it
  and wait. `check-all.sh` end-to-end is ~10+ min.
- **`sdkconfig` is gitignored** (only `sdkconfig.defaults` is tracked). A stray
  `TINYUSB_HID_ENABLED` kconfig warning from `test_app` is pre-existing and
  non-fatal.

## 10. Handy references

- App error codes: `firmware/components/macro_model/include/app_error.h` (there is
  no dedicated "unavailable/shutting-down" code; I used `APP_ERROR_INTERNAL` for a
  fault latch and `APP_ERROR_CONFLICT` for "rejected during shutdown").
- Host test fakes: `tests/host/fakes/` (`fake_fs_backend`, `fake_wifi_backend`,
  `fake_call_log` with `fake_call_log_fail_on(name, occurrence)` single-occurrence
  fault injection and strict call-log expectation mode, etc.).
- Host CMake: `tests/host/CMakeLists.txt` — when a firmware source starts
  referencing new symbols, every host target that compiles it must also compile
  the new dependency source (no `--gc-sections`); run the build and fix undefined
  references. `cmake-format -i tests/host/CMakeLists.txt` after editing.
