# V2-044 — Wi-Fi and reset semantics

**Phase:** 4 — Authentication, provisioning, and device settings
**Task:** V2-044 — Wi-Fi and reset semantics
**Branch:** `v2-044-wifi-and-reset-semantics`
**Status:** Component-level software implementation complete and host-tested. HTTP-layer
wiring (`/api/v1/device/*`) is explicitly out of scope — it belongs to Phase 5 (`web_server`,
owned by a separate concurrent stream) and remains stubbed as `"V2 configuration route is not
enabled yet"` / a hardcoded restart response, unchanged by this task. Physical hardware
Wi-Fi/AP validation is separate, later work; this task delivers host-fake-based correctness only.

## Scope

This task covers `firmware/components/wifi_ap` (AP-first operation, bounded station retry,
station status) and `firmware/components/device_controls` (restart, reset-settings,
factory-reset orchestration), plus the small additive primitives those two components needed
from `device_settings`, `auth`, and `storage` to do their job. It does **not** touch
`firmware/components/web_server`, `firmware/components/app_contracts_v2/include/device_settings_v2.h`,
`firmware/components/app_contracts_v2/device_settings_v2.c`, or any V2-043 device-UI-preference
field, per the task boundary.

## Audit: what already existed

Before writing anything, the existing implementation was read in full and checked against the
V2-044 checklist. Two significant findings changed the plan:

1. **`device_settings_reset_noncredential()` already fully implements SPEC_V2.md §11.4 "reset
   settings" at the settings-record level.** `app_v2_device_settings_reset_noncredential()`
   (`firmware/components/app_contracts_v2/device_settings_v2.c`) resets `deviceName`,
   `requireSerialConfirmation`, `sendMode`, `snapshotRetentionTarget`,
   `showMacroSourcePreviews`, and `lastSelectedPackageId` to their defaults, clears
   `stationConfigured`/`stationSsid`/`stationPassphrase`, and leaves AP SSID/passphrase, the
   admin password verifier, and the provisioned flag untouched. This was already covered by
   `tests/host/test_device_settings_core.c::test_reset_preserves_credentials_and_blob_counter`
   before this task started. **No changes were made to this function or the settings struct.**
   This existing test is direct evidence for the "reset-settings" and "credential reset" (station
   credential removal while AP/admin credentials are preserved) checklist items.
2. **`device_controls` had zero code for restart/reset-settings/factory-reset**, despite the
   migration map (`V2_MIGRATION_MAP.md` §2.11) assigning this to it as "Adapt." Its `README.md`
   carried a stale note ("Credential-reset and factory-reset gestures were never implemented and
   are not planned") that was true only for a *physical button gesture* — there is still no
   button — but the *network-triggered* orchestration this task needed did not exist anywhere:
   `web_api_administration.c`'s `WEB_API_ROUTE_DEVICE_RESET_SETTINGS` and
   `WEB_API_ROUTE_DEVICE_FACTORY_RESET` both return a stub `"V2 configuration route is not
   enabled yet"` error, and `WEB_API_ROUTE_DEVICE_RESTART` returns a hardcoded `202` JSON body
   with no actual side effect. This is genuine Phase 5 work not yet started; V2-044's job was to
   build the component-level functions Phase 5 will call into, not the HTTP handlers themselves.
3. **`wifi_ap` had AP-first startup and a single-attempt station connect, but no station status
   and no retry bound.** `app_core`'s `adapter_wifi_start()` (`firmware/components/app_core/app_core.c`)
   already starts the AP unconditionally before ever touching station mode, and already treats a
   station failure as non-fatal (logs a warning, returns `APP_ERROR_NONE`, leaves the AP running)
   — satisfying "AP starts first and unconditionally" and "station failure cannot prevent AP
   operation" as-is, no changes needed there. But `wifi_ap_connect_station()` was exactly one
   blocking attempt with no retry and no way to query the outcome after the call returned — a gap
   against "bound retries and expose status."
4. A second, unrelated `provisioning` component (`firmware/components/provisioning`,
   `provisioning_config_t` with `schema_version`/`revision`/`always_select_package`) has its own
   `provisioning_factory_reset()`. Reading `app_core_sequence.c` confirmed this component is not
   wired into the v2 boot sequence at all (v2 uses `device_settings`/`app_v2_device_settings_t`
   exclusively) — it is v1-era dead code from the caller's perspective, out of this task's scope,
   and was not touched.

## Checklist evidence

### Start the protected AP first and unconditionally

Unchanged, already correct: `app_core_sequence.c::start_normal_mode`/`start_setup_mode` both call
`operations->wifi_start` (→ `wifi_ap_start()`) before `http_start`, and `wifi_ap_start()` is called
before any station attempt in `adapter_wifi_start()`. Evidenced by the pre-existing
`tests/host/test_app_core.c` and `tests/host/test_wifi_ap.c::test_start_enforces_operation_sequence`
(unmodified).

### Support at most one explicitly configured station network

Unchanged, already correct: `app_v2_device_settings_t` has exactly one `station_ssid`/
`station_passphrase` pair gated by `station_configured` (`device_settings_v2.h`, not modified).

### Ensure station failure cannot prevent AP operation

Unchanged, already correct: `adapter_wifi_start()` in `app_core.c` treats a station-join failure
as a logged warning and returns `APP_ERROR_NONE` regardless.

### Bound retries and expose status (new work)

- Added `firmware/components/wifi_ap/wifi_ap_station.h`/`.c`: a host-testable bounded-retry engine
  behind an ops seam (`wifi_ap_station_ops_t`), analogous to the existing `wifi_ap_state.c`
  pattern for the AP itself. `wifi_ap_station_connect()` runs at most
  `WIFI_STATION_MAX_ATTEMPTS` (3) attempts, publishing `WIFI_STATION_CONNECTING` before each one
  and ending in `WIFI_STATION_CONNECTED` or `WIFI_STATION_FAILED` — never an unbounded loop
  (SPEC_V2.md §12.1).
- `wifi_ap.c`'s `wifi_ap_connect_station()` was refactored to delegate its retry loop to this
  engine; `attempt_station_connect_once()` is the single-attempt hardware operation the engine
  calls repeatedly (unchanged logic, just extracted into the ops-seam shape).
- Added `wifi_ap_get_station_status()`, returning `wifi_station_status_t {state, last_error,
  attempt_count}`, for the future SPEC_V2.md §13.6/§13.13 `station`/`wifi` diagnostics fields
  (Phase 5's job to wire into JSON — this task only provides the data source).
- Tests: `tests/host/test_wifi_ap_station.c` (new, registered as `wifi_ap_station` in the `wifi`
  label) — operation validation, bounded-retry exhaustion (`APP_ERROR_TIMEOUT` after exactly 3
  attempts, status lands on `WIFI_STATION_FAILED`), success on the first attempt, success on the
  last permitted attempt (prior failures absorbed), and the disabled-by-default state.

### Define and test restart, reset-settings, credential reset, and factory-reset (new work)

- Added `firmware/components/device_controls/device_controls_reset.h`/`.c`: a host-testable
  orchestration engine (ops seam: `reset_settings_noncredential`, `erase_all_settings`,
  `invalidate_all_sessions`, `delete_all_blobs`, `schedule_restart`) implementing the three
  SPEC_V2.md §13.12 device actions:
  - `device_controls_reset_engine_restart()` — schedules a reboot, touches nothing else.
  - `device_controls_reset_engine_reset_settings()` — applies §11.4 non-credential reset, then
    invalidates every session, then schedules a reboot. Aborts before touching sessions or
    scheduling a reboot if the settings write itself fails, so nothing destructive begins from a
    failed first step.
  - `device_controls_reset_engine_factory_reset()` — erases device configuration/credentials/
    provisioning state first; if that fails, aborts immediately (no sessions invalidated, no
    blobs deleted, no reboot — a failed factory reset must not destroy the repository while
    leaving old credentials in place). If it succeeds, invalidates sessions and deletes every
    blob, continuing past either failing so one stuck resource cannot strand the other, then
    always schedules the reboot (the erased settings must still take effect); the first error
    encountered is returned to the caller.
- Wired the real adapters into `device_controls.c`: `device_settings_reset_noncredential()`/
  `device_settings_factory_reset()`, `auth_session_logout_all()`, `storage_blob_delete_all()`, and
  a delayed reboot via a one-shot `esp_timer` (`DEVICE_CONTROLS_RESTART_DELAY_MS` = 500 ms, so an
  HTTP caller's `202 accepted` response has time to leave the socket before the connection drops;
  falls back to an immediate `esp_restart()` if the timer cannot even be created).
- Added public API: `device_controls_restart()`, `device_controls_reset_settings()`,
  `device_controls_factory_reset()` (`device_controls.h`) — ready for the future Phase 5 HTTP
  handlers to call.
- New small additive primitives the orchestration needed (none change an existing signature or
  struct):
  - `device_settings_core_factory_reset()` / `device_settings_factory_reset()` — replaces the
    stored record with `app_v2_device_settings_init_unprovisioned()` (an existing, unmodified
    function). Idempotent on an already-unprovisioned device.
  - `auth_core_session_logout_all()` / `auth_session_logout_all()` — clears the bounded session
    table at once, for a caller (device action) that has no single token to log out.
  - `storage_blob_delete_all_with_ops()` / `storage_blob_delete_all()` — deletes every blob found
    by a directory scan, continuing past one undeletable file rather than stopping (mirrors the
    existing wifi_ap/device_controls "continue past failure, report the first error" cleanup
    pattern already used elsewhere in this codebase).
- Tests:
  - `tests/host/test_device_controls_reset.c` (new, `device_controls_reset` in the `controls`
    label) — ops validation, restart-only-schedules-reboot, reset-settings happy path and
    abort-on-settings-failure and continues-to-restart-despite-session-failure, factory-reset
    happy path, abort-on-settings-failure (nothing else runs), continues past a session failure,
    continues past a blob-deletion failure with first-error-wins ordering.
  - `tests/host/test_device_settings_core.c::test_factory_reset_erases_everything` (new) —
    erases everything including credentials/provisioned/next_blob_id, idempotent re-run, and a
    durable-write failure clears the output and reports the error.
  - `tests/host/auth_additional_session_tests.inc::test_session_logout_all` (new) — clears every
    active session, empty-table no-op, and a locking failure leaves every session untouched.
  - `tests/host/test_storage_blob.c::test_delete_all_removes_every_blob` and
    `::test_delete_all_continues_past_one_failure` (new) — every valid blob gone, invalid names
    untouched, empty-directory no-op, and one undeletable file does not strand the rest.

### Ensure factory reset erases repository blobs only when the specification requires it, and reports the connection loss clearly

- "Only when required": enforced by construction — `device_controls_reset_engine_reset_settings()`
  never calls `delete_all_blobs`; only `device_controls_reset_engine_factory_reset()` does.
  `test_reset_settings_happy_path` explicitly asserts `delete_blobs_calls == 0`.
- "Reports the connection loss clearly": the `connectionWillClose`/`reprovisioningRequired`/
  `repositoryBlobsPreserved` response fields (SPEC_V2.md §13.12) are HTTP response construction —
  Phase 5/`web_server` territory, explicitly out of this task's scope. This task provides the
  correctly-ordered, correctly-erasing component functions Phase 5 needs; it does not implement
  the HTTP response.

## Commands and results

All commands run from the repository root on branch `v2-044-wifi-and-reset-semantics`.

```console
$ export PATH="$HOME/.local/bin:$PATH"
$ ./scripts/run-tests.sh
...
100% tests passed, 0 tests failed out of 41
```

New/changed test binaries in that run: `device_controls_reset` (9 cases), `wifi_ap_station`
(5 cases), `auth` (session-logout-all added), `storage_blob` (delete-all added),
`device_settings_core` (factory-reset added) — plus every pre-existing suite, unchanged and
still green.

```console
$ ./scripts/check-format.sh
```

clang-format and cmake-lint report zero findings across every file this task touched (21
CMakeLists.txt files scanned, "found lint:" empty; targeted `clang-format --dry-run --Werror`
against all 25 touched C/H files also confirms zero diffs). `check-format.sh`'s webapp
`prettier` step fails in this sandbox with `prettier: not found` — this is a pre-existing
environment gap (`npm --prefix webapp ci` was never run in this session) unrelated to this task;
no webapp file was touched.

```console
$ source "$HOME/esp/esp-idf-v5.5.5/export.sh"
$ export NVM_DIR="$HOME/.nvm"; source "$NVM_DIR/nvm.sh"; nvm use
$ ./scripts/check-firmware.sh
```

First run caught two genuine `clang-tidy` `misc-include-cleaner` errors in the new files
(`device_controls_reset.c` and `wifi_ap_station.c` relied on transitively-included
`app_error.h`/`stdint.h` instead of including them directly). Both were fixed by adding the
direct includes; the second run built clean (ESP-IDF v5.5.5, target `esp32s3`) with clang-tidy
`WarningsAsErrors: '*'` passing on every first-party file, no suppressions added.

```console
$ ./scripts/generate-native-coverage.sh
```

Added `device_controls_reset.c` and `wifi_ap_station.c` to `pure_policy_args` in
`scripts/generate-native-coverage.sh` (alongside the existing `device_controls_logic.c` and
`wifi_ap_state.c` entries they are the direct counterpart of). Gate result:
`device_controls_reset.c` 100% line / 100% branch, `wifi_ap_station.c` 100% line / 100% branch;
overall pure-policy total 95% line / (gate requires ≥90 line / ≥80 branch) — passes with margin,
exit 0.

## What this task did not do (explicitly out of scope)

- No `/api/v1/device/restart`, `/api/v1/device/reset-settings`, or `/api/v1/device/factory-reset`
  HTTP handler was implemented or modified. `web_api_administration.c` is unchanged. That is
  Phase 5 (V2-051/V2-052/V2-053), owned by a separate concurrent stream; this task's device_controls
  functions are what those future handlers will call after validating the confirmation phrase and
  (for factory reset) the admin password, per SPEC_V2.md §13.12's requirement that "the
  destructive operation MUST NOT begin until the complete request, password, and confirmation
  phrase have been validated" — validation is the caller's job, not device_controls's.
- No physical hardware Wi-Fi/AP/reset validation was performed or claimed. This is host-fake-based
  correctness only, per the task's own instruction that hardware validation is separate, later
  work.
- `device_settings_v2.h`, `device_settings_v2.c`, `provisioning`, V2-043 UI-preference fields, and
  every file under `firmware/components/web_server` were not modified.
- The stale `firmware/components/auth/auth_core.c` filter entry in
  `scripts/generate-native-coverage.sh` (that file does not exist — the component was split into
  `auth_core_common.c`/`auth_core_password.c`/`auth_core_session.c`/`auth_core_rate_limit.c` at
  some point without updating this filter) was left as found; fixing it is unrelated to V2-044.

## Files touched

- `firmware/components/wifi_ap/include/wifi_ap.h` — station status types, `WIFI_STATION_MAX_ATTEMPTS`, `wifi_ap_get_station_status()`.
- `firmware/components/wifi_ap/wifi_ap.c` — bounded-retry wiring for `wifi_ap_connect_station()`.
- `firmware/components/wifi_ap/wifi_ap_station.h`/`.c` — new bounded-retry engine.
- `firmware/components/wifi_ap/CMakeLists.txt` — added `wifi_ap_station.c`.
- `firmware/components/wifi_ap/README.md` — documents AP-first/bounded-retry/status.
- `firmware/components/device_controls/include/device_controls.h` — `device_controls_restart()`/`reset_settings()`/`factory_reset()`.
- `firmware/components/device_controls/device_controls.c` — reset-ops adapters, delayed-reboot timer.
- `firmware/components/device_controls/device_controls_reset.h`/`.c` — new orchestration engine.
- `firmware/components/device_controls/CMakeLists.txt` — added source and component `REQUIRES`.
- `firmware/components/device_controls/README.md` — documents the three device actions.
- `firmware/components/device_settings/device_settings_core.h`/`.c` — `device_settings_core_factory_reset()`.
- `firmware/components/device_settings/include/device_settings.h`, `device_settings.c` — `device_settings_factory_reset()`.
- `firmware/components/auth/auth_core.h`, `auth_core_session.c` — `auth_core_session_logout_all()`.
- `firmware/components/auth/include/auth.h`, `auth.c` — `auth_session_logout_all()`.
- `firmware/components/storage/storage_blob_internal.h`, `storage_blob.c`, `include/storage_blob.h` — `storage_blob_delete_all[_with_ops]()`.
- `tests/host/CMakeLists.txt` — new `wifi_ap_station_tests`, `device_controls_reset_tests` targets; `storage_blob_access_core.c` added to `storage_blob_tests`/`storage_mount_tests` (now a link dependency of `storage_blob.c`).
- `tests/host/test_wifi_ap_station.c`, `tests/host/test_device_controls_reset.c` — new.
- `tests/host/test_device_settings_core.c`, `tests/host/test_storage_blob.c`, `tests/host/test_auth.c`, `tests/host/auth_additional_session_tests.inc` — new test cases appended.
- `scripts/generate-native-coverage.sh` — added the two new pure-policy files to the coverage gate.
