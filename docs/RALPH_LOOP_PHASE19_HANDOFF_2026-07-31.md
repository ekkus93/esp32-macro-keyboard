# Ralph Loop handoff — FIX1 Phase 19 (+ 20.1, + on-device Unity expansion)

Written 2026-07-31, mid-session, because the user needed to switch machines.
This documents exactly where the automated Ralph Loop run left off, so a new
session can resume without re-deriving context. Read this fully before
touching code.

## Scope of this Ralph Loop run

Per the `ralph-loop` skill invocation that started this run, process ONLY:

1. `## 19. Diagnostics and observability` (all subsections) in
   `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
   — subsystem health snapshots, the redacted diagnostics HTTP route, frontend
   rendering of it, host tests, then the 3 remaining Phase 18.5 items that
   were explicitly blocked on this diagnostics route existing.
2. `### 20.1 Clean production build` — build from clean checkout, flash
   `firmware/test_app` to the attached real ESP32-S3 hardware, record real
   measured values (commit/IDF version/lock hash/binary size/partition
   headroom/webfs size/static RAM/peak heap/stack marks).
3. Expanding the on-device Unity test suite per
   `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` §9.

Explicitly OUT OF SCOPE (do not touch, do not get stuck on): Phase 20.2
(USB host matrix), 20.3 (SoftAP/browser), 20.4 (power interruption), 20.5
(physical controls), and all of Phases 21–23 — these need a human physically
operating hardware.

Ground rules from the skill (still apply): work directly on `master`, no
new branches, no force-push, one commit per task/subtask, full
`./scripts/check-all.sh`-equivalent verification before marking a TODO item
done and moving on, never mark a checkbox without real reproducible evidence.

Real ESP32-S3 hardware IS attached at `/dev/ttyACM0` (native USB-Serial/JTAG
— do NOT use `/dev/ttyUSB0`, a separate CP210x UART bridge) — task 3 (20.1)
and the on-device Unity expansion need it flashed and actually run, not just
compiled.

## Done so far (all committed and pushed to `master`)

FIX1 TODO §19.0 and all of §19.1 are complete — every subsystem now reports
through the shared `subsystem_health_state_t` vocabulary
(`firmware/components/support/include/subsystem_health.h`: HEALTHY, DEGRADED,
UNAVAILABLE, RECOVERING, FAILED).

| Commit | Subsystem | Health source |
| ---------- | ---------------------- | ----------------------------------------------------------------------- |
| `15574e8` | app lifecycle | `firmware/components/app_core/app_core_health.h/.c` (new) |
| `f11e163` | storage mount/recovery | `firmware/components/storage/storage_health.h/.c` (new) |
| `242f65f` | repository | `firmware/components/storage/repository_health.h/.c` (new) |
| `f7d876a` | authentication | `firmware/components/auth/auth_health.h/.c` (new) |
| `27841aa` | USB | `firmware/components/usb_keyboard/usb_health.h/.c` (new) |
| `9830d41` | executor | `firmware/components/macro_executor/executor_health.h/.c` (new) |
| `b4a6df4` | controls | adapted **existing** `device_controls_health_t`; new `device_controls_health_derive_state()` in `device_controls_logic.c` |
| `6323b0a` | Wi-Fi AP | adapted **existing** `wifi_ap_status_t`; new `wifi_ap_health_derive_state()` in `wifi_ap_state.c` |
| `da2cbd6` | HTTP | `firmware/components/web_server/http_health.h/.c` (new) |

Six of the nine (`app_core`, `storage`, `repository`, `auth`, `executor`,
`usb_keyboard`) use the "reset/record_primary/record_cleanup/snapshot"
pattern with a private static `g_state` and derive FAILED whenever
`cleanup_incomplete || cleanup_error != NONE || primary_error != NONE`, else
HEALTHY. Two (`controls`, `wifi_ap`) adapted a pre-existing richer struct
instead of duplicating tracking — check for an existing health struct with
zero callers before building a new one; both times it was there but unused.
`http` had no prior tracking so it got a from-scratch module.

Every one of the 9 is recorded from **`firmware/components/app_core/app_core.c`**'s
existing `adapter_*_init`/`adapter_*_deinit` (or `adapter_http_start`/`stop`)
call sites — that file is the single orchestrator, by design, so
diagnostics aggregation (next task) can call each subsystem's own
`*_health_snapshot()`/`*_get_health()`/`*_get_status()` getter directly
without needing app_core.c to expose anything new.

### Two real pre-existing bugs found and fixed along the way (both in commit `b4a6df4`)

Worth knowing about since they could resurface if anyone reverts around
that commit:

1. Six of the *_health.c files added in earlier commits (`app_core_health.c`,
   `storage_health.c`, `repository_health.c`, `auth_health.c`,
   `executor_health.c`, `usb_health.c`) only transitively included
   `app_error.h`/`subsystem_health.h` via their own header, tripping
   clang-tidy's `misc-include-cleaner`. This had never been caught because
   no full `check-firmware.sh` pass had run since those commits landed
   (only `idf.py build`, which doesn't run clang-tidy). Fixed by adding
   direct includes.
2. `firmware/test_app/CMakeLists.txt`'s `EXTRA_COMPONENT_DIRS` never listed
   `../components/support`, so the on-device test app failed to configure
   once `auth`/`usb_keyboard` started requiring it (also only surfaced once
   a full `check-firmware.sh` pass — which builds test_app too — finally ran
   to completion). Fixed by adding it.

**Lesson for continuing this work**: run the FULL `check-firmware.sh` (both
`firmware/` and `firmware/test_app/`) after every batch of changes, not just
`idf.py build`. It's slow (~15-20 min) but it's the only thing that catches
these. See "Environment quirks" below for how to run it without it getting
killed.

### A third real bug found in the Wi-Fi commit (`6323b0a`)

`wifi_ap.h` is a **public, widely-included** header (by `provisioning`,
`web_server`, `app_core`). Adding `#include "subsystem_health.h"` to it broke
8 host-test CMake targets that transitively compile files including
`wifi_ap.h` but never had `../../firmware/components/support/include` in
their `target_include_directories` — because parallel `make` output
interleaves so badly that `grep -B` context around an error line is
unreliable for figuring out which target it belongs to. **If you touch a
widely-included header, rebuild host tests serially with
`cmake --build tests/host/build -j1 -- -k` (not `-j$(nproc)`) to get
attributable error output**, then fix every broken target's
`target_include_directories`. The affected targets that time were spread
across both `tests/host/CMakeLists.txt` and `tests/host/cmake/extra_tests.cmake`
— check both files.

For the still-untouched HTTP-health commit (`da2cbd6`) this didn't recur
because the new `http_health.h` was only included by `http_health.c` itself
and `app_core.c` (which already had `support` in its includes) — no ripple.

## Current task: FIX1 §19.2 "Add redacted diagnostics route" (task #36)

**Status: research only, zero code written.** I was mid-investigation of the
existing web-server routing pattern when interrupted. Task list state:
tasks #26–35 are `completed`; task #36 is `in_progress`; #37–41 are
`pending`. (These are TaskCreate/TaskUpdate tasks in this session's tracker
— a fresh session won't see them; recreate an equivalent list if useful, or
just work through the TODO doc directly.)

### What the TODO requires (FIX1 TODO §19.2, unchanged since before this session)

```text
- [ ] build ID;
- [ ] firmware and schema versions;
- [ ] reset reason and uptime;
- [ ] heap;
- [ ] task stack high-water marks;
- [ ] webfs/userdata capacity;
- [ ] quarantine count;
- [ ] current execution state;
- [ ] subsystem health.

Exclude all secret material and raw macro source.
```

### Key finding: this is simpler than the mutating `/api/v1/sets/*` routes

Earlier in this session (before compaction) I assumed a new diagnostics
route would need "6 touch points" like `/api/v1/sets/import-new` did
(`web_api_core.h` enum, `web_api_core.c` matcher + method-allowlist,
`web_api_dispatch.c` `is_set_route`, `web_server_api.c` `route_body_limit`,
a handler file, tests). **That pattern is for the `web_api_dispatch`-routed
`/api/v1/sets/...` namespace specifically.** A GET-only status-style route
uses a much simpler, already-precedented pattern — see
`firmware/components/web_server/web_server_status_limits.c`:

```c
esp_err_t status_handler(httpd_req_t *request) {
    const wifi_ap_status_t wifi = wifi_ap_get_status();
    const macro_execution_status_t execution = macro_executor_get_status();
    char response[512U];
    const app_error_code_t result = web_adapter_build_status_json(
        "0.1.0", "v5.5.5", usb_state_string(usb_keyboard_get_state()),
        wifi_state_string(wifi.state), wifi.client_count, execution_state_string(execution.state),
        response, sizeof(response));
    ...
    return send_json(request, response, "200 OK");
}
```

registered directly in the route table in
`firmware/components/web_server/web_server_lifecycle.c`:

```c
{.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
{.uri = "/api/v1/limits", .method = HTTP_GET, .handler = limits_handler},
```

with the handler declared in `firmware/components/web_server/web_server_internal.h`.
**Follow this exact pattern for the new diagnostics route** (e.g.
`/api/v1/diagnostics`, `diagnostics_handler`) — add it to
`web_server_status_limits.c` (or a new `web_server_diagnostics.c` if it gets
long, which it will, given 9 fields to aggregate) rather than reproducing
the sets-style 6-touch-point dispatch. Note `"0.1.0"` and `"v5.5.5"` are
currently **hardcoded string literals** in `status_handler` — worth checking
whether "build ID / firmware and schema versions" for the new route should
reuse/extract these rather than hardcoding a third copy; `APP_SCHEMA_VERSION`
already exists as a macro (used throughout `firmware/components/storage/`,
e.g. `storage_repository_objects_json.c`) for the schema-version field.

### Data sources for each required field — what already exists vs. what's new

- **Subsystem health** (9 fields): all done, see the table above. Each has
  a `*_health_snapshot()` / `*_get_health()` / `*_get_status()` (+ derive
  function for controls/wifi) — call them directly.
- **Current execution state**: `macro_executor_get_status()` already exists
  and is already used by `status_handler` (`macro_execution_status_t`).
- **Build ID / firmware and schema versions**: partially exists —
  `status_handler` hardcodes `"0.1.0"` and `"v5.5.5"`; `APP_SCHEMA_VERSION`
  macro exists in `macro_model`. No real "build ID" concept exists yet in
  this codebase (searched — nothing like a git-describe/CI-injected build
  identifier). Will need to decide what "build ID" means here (e.g.
  `esp_app_desc_t`'s built-in IDF app description struct, which ESP-IDF
  populates automatically with a version/date/time/idf-version at build
  time — check `esp_ota_get_app_description()` / `esp_app_get_description()`
  before inventing a new mechanism).
- **Reset reason and uptime**: nothing in this codebase yet. ESP-IDF
  provides `esp_reset_reason()` (returns `esp_reset_reason_t`) and uptime
  via `esp_timer_get_time()` (microseconds since boot) — both are
  ESP-IDF-only (not portable/host-testable), so the aggregation function
  itself will need a small seam (an ops/adapter struct or injected
  function pointers) if host tests are expected to exercise the full
  aggregation logic, matching this codebase's existing style of
  `app_core_ops_t`-style dependency injection for host-testability. Look at
  how `app_core_sequence.c`/`app_core_ops.h` do this for precedent.
- **Heap**: nothing yet. ESP-IDF `esp_get_free_heap_size()` /
  `esp_get_minimum_free_heap_size()`. Same host-testability caveat as above.
- **Task stack high-water marks**: nothing yet. ESP-IDF
  `uxTaskGetStackHighWaterMark(handle)` per FreeRTOS task. Need to decide
  which tasks to report (executor task, controls task, USB task, web server
  task?) and how to get their handles — check whether any component already
  stores its own task handle (e.g. `device_controls`, `macro_executor`) for
  this purpose, or whether new plumbing is needed.
- **webfs/userdata capacity**: nothing yet. `storage.h` defines
  `STORAGE_WEB_PARTITION "webfs"` and various `*_MAX_BYTES` constants
  (`storage_object_json.h`) but no existing "how much space is used /
  available on this LittleFS partition" query. Will likely need
  `esp_littlefs_info()` (ESP-IDF LittleFS partition info API) or similar —
  check what mount API `storage_mount_all()` in `firmware/components/storage/`
  already uses, since that will show which partition-info API is already
  linked.
- **Quarantine count**: nothing yet. `storage.h` has quarantine record
  fields (`source_path`, `evidence_path`, `reason` — see
  `STORAGE_QUARANTINE_REASON_MAX_BYTES`) and there's a
  `storage_quarantine_recover_all()` function (used in
  `app_core.c`'s `adapter_storage_recover`) but no existing "count how many
  quarantined items exist right now" query — will need a new function in
  `firmware/components/storage/storage_quarantine.c` (check exact filename
  via `find firmware/components/storage -iname '*quarantine*'`).

### Redaction requirement

"Exclude all secret material and raw macro source" — cross-check against
whatever `scripts/check-secret-sentinel.py` treats as a secret sentinel
(this is the same script that FIX1 §18.5's remaining items, task #39, will
run against this new route's real output) so the schema is right the first
time instead of needing rework. Read that script before finalizing the
diagnostics JSON shape.

## Remaining tasks after #36, in order

- **#37 — 19.2 Frontend diagnostics rendering**: wire
  `webapp/src/features/settings/DiagnosticsPage.tsx`'s stub "Full subsystem
  diagnostics" section (around lines 162–175 as of last read, disabled
  "Load full diagnostics" button) to the new route. Re-check line numbers,
  they may have drifted.
- **#38 — 19.3 Diagnostic tests**: new host test target covering: exact
  allowed fields, secret sentinels absent, bounded output, behavior when a
  subsystem health query fails, no false-healthy state when cleanup is
  incomplete.
- **#39 — Close remaining Phase 18.5 items**: FIX1 TODO §18.5 has 5
  remaining checkboxes gated on real diagnostics/log/frontend-persisted-state
  artifacts being scannable by `scripts/check-secret-sentinel.py` — only
  actionable once #36 produces real route output to scan.
- **#40 — 20.1 Build/flash/measure on real hardware**: clean build, flash
  `firmware/test_app` to the attached ESP32-S3 at `/dev/ttyACM0`, record
  real commit/IDF version/component-lock hash/binary size/partition
  headroom/webfs size/static RAM/peak heap/stack high-water marks. This is
  a genuine hardware step — do not fabricate numbers from compilation
  alone; per repo policy (`CLAUDE.md` "Active development constraints")
  never claim physical hardware validation without real reproducible
  evidence.
- **#41 — Expand on-device Unity test suite**: add missing coverage per
  `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` §9.2 (USB state transitions, real
  keyboard press/release, task lifecycle, AP start/stop, encrypted NVS
  persistence, physical confirmation/cancel via safe test adapters — not
  literal external button presses, fatal/degraded indicator behavior,
  storage mount failure without auto-formatting), flash and run on the
  attached hardware via `idf.py monitor`, document in
  `firmware/test_app/README.md`.

## Per-task checklist (apply to every remaining item — this is what was used for #26–35)

1. Read the TODO item and any referenced spec/handoff sections fully.
2. Implement.
3. Add/extend host tests for the new logic (host-testable pure-C modules
   wherever possible, matching this codebase's established
   ops/adapter-injection pattern for anything touching ESP-IDF/FreeRTOS
   APIs directly).
4. Build the specific new/changed host test target standalone and run it.
5. Run the full host suite: `./scripts/run-tests.sh` (currently 59/59
   passing).
6. `cmake-format -i` any touched `CMakeLists.txt`/`.cmake` files.
7. `./scripts/check-format.sh` (needs
   `export PATH="$HOME/go/bin:$HOME/toolchains/go/bin:$PATH"` for
   shfmt/actionlint first — see Environment section).
8. Full `./scripts/check-firmware.sh` (both `firmware/` and
   `firmware/test_app/` — see Environment section for how to run this
   without it getting killed by the environment's background-process
   limits).
9. `./scripts/check-docs.sh` after any TODO/PROGRESS doc edit.
10. Update the FIX1 TODO checkbox(es) to `[x]` and append an
    "Implemented (X): ..." paragraph in the same prose style as the
    existing ones (see §19.1 in the TODO for 9 examples of this style).
11. `git add` exactly the changed files (never `-A`), commit with a
    descriptive conventional-style message, `git push`. One task/subtask
    per commit — do not batch.
12. Move to the next item.

## Environment quirks specific to this machine/session (read before running anything)

- **Toolchain sourcing does not persist across tool calls.** Every shell
  command that needs `idf.py`/`cmake-format`/`shfmt`/`actionlint` must
  re-source in that same command:

  ```bash
  . "$HOME/esp/esp-idf-v5.5.5/export.sh"
  source "$HOME/.nvm/nvm.sh" && nvm use 24.18.0
  export PATH="$HOME/go/bin:$HOME/toolchains/go/bin:$PATH"   # for shfmt/actionlint
  ```

- **Long-running background commands get killed at roughly the 600-second
  mark**, even when started with `nohup ... & disown` or run via the tool's
  `run_in_background: true`. This isn't a script bug — `check-firmware.sh`
  legitimately takes ~15-20 minutes for both projects. Workaround that
  reliably worked this session: start the real command normally
  backgrounded (`nohup ./scripts/check-firmware.sh > /tmp/log 2>&1 & disown;
  echo pid $!`), then use the **`Monitor` tool** (not `Bash` with
  `run_in_background`) with a command that polls `kill -0 <pid>` in a loop
  and emits exactly one summary line when done, with `timeout_ms` set high
  (e.g. 3600000). `Monitor` survived past 600s repeatedly in this session
  when plain background `Bash` monitoring did not.
- **`check-firmware.sh` prints nothing on success for the clang-tidy phase**
  (only the GCC build phase and any failing clang-tidy output are visible)
  — silence between a GCC "Project build complete" line and the next
  project's set-target line is the *expected* success case, not a hang.
- **This machine is Ubuntu 22.04, not the 24.04 CI expects** for some apt
  packages (clang-format/shellcheck versions may drift slightly) — this was
  flagged and accepted by the user earlier in the session ("Run anyway,
  flag version drift").
- Real ESP32-S3 hardware is attached at `/dev/ttyACM0`, confirmed via
  `esptool chip_id`: ESP32-S3 QFN56 rev v0.2, 8MB PSRAM. Do not use
  the UART bridge for flashing.
  **Corrected 2026-08-02:** the bridge on this bench is a CH340 (`1a86:55d3`)
  at `/dev/ttyACM1`, not a CP210x at `/dev/ttyUSB0`, and it is not merely an
  alternative flashing path — it carries the **interactive serial console**,
  which native USB does not. See the port table in `CLAUDE.md`.

## Where to look first when resuming

1. This file.
2. `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
   §19 (source of truth for exact remaining checkboxes — re-read it fresh,
   don't trust this handoff's quoted excerpt if time has passed and someone
   else edited it).
3. `git log --oneline -15` to confirm which commits actually landed vs. what
   this doc claims (this doc is a snapshot, the TODO/PROGRESS docs plus git
   log are authoritative).
