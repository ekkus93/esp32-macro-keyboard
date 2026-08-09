# V2-057 — unprovisioned setup-state GET and live diagnostics HTTP coverage (2026-08-09)

## Scope

Track L, closing V2-057's three remaining items as framed by the task brief and
the two prior evidence reports:
`docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md` and
`docs/implementation-v2/V2_057_LIVE_ADMINISTRATION_HTTP_TEST_2026-08-09.md`.

1. Unprovisioned-mode `setup_state_handler()` (GET `/api/v1/setup` while
   `server_configuration.mode == WEB_SERVER_MODE_SETUP`, `web_server_setup.c`)
   had no test at all — the provisioned-mode `404` path
   (`setup_route_response()` in `web_api_administration.c`) was already
   tested, but not this one.
2. `GET /api/v1/diagnostics` had no live httpd-handler-level test — its JSON
   composition already had deep coverage
   (`test_web_server_adapter_diagnostics_json.inc`), but nothing proved the
   route actually wires to that composition through
   `web_request_policy_evaluate()` -> dispatch, the way status/limits/send/
   administration now do.
3. Stretch goal: make the C-side test suite consume the same checked-in
   `contracts/v2/api/examples.json` the TypeScript side already validates
   against, for the routes touched above.

All three were closed. Item 2 turned out to need substantially less new
infrastructure than either prior report anticipated — see "Investigation"
below.

## Branch and commit

Worktree branch: `worktree-agent-ad8a47d01f3dae6b7`.
Starting commit: `e0a610a293e6d5f75e830b51988261ed98d50804`.
This work's commit SHA: recorded in the commit that carries this file (see
`git log -1` on this file's introducing commit).

## Investigation: item 1 (setup-state GET)

Read `web_server_setup.c`'s `setup_state_handler()` and confirmed it is a
thin, self-contained fixed-URI handler: it reads only
`server_configuration.mode`/`server_configuration.setup_device_name` (both
plain globals, no NVS/FreeRTOS access) and calls `send_json()`/`send_error()`
(`web_server_common.c`). No authentication, no request body. The same
`fakes/esp_http_server_stub`/`fakes/fake_httpd.c` technique
`test_web_server_status_limits_route.c` already uses for other fixed-URI GET
handlers applies directly, with no auth/subsystem fakes needed at all — the
only wrinkle is that `web_server_setup.c` also defines `setup_submit_handler()`
(the POST handler, out of this item's scope per the task brief), which
references `device_settings_read()`/`device_settings_replace()`/
`auth_password_create()`; those need narrow linker-satisfying stand-ins (never
called by any test in this file) the same way other route-test files already
stub out entry points their target's other, untested handler needs to link.
`web_server_setup_submit.c` (the POST handler's business logic) is already
host-testable with no ESP-IDF dependency (confirmed by the pre-existing
`web_server_setup_submit_tests` target) and links in real.

## Investigation: item 2 (diagnostics)

Both prior reports state `collect_diagnostics()`
(`web_server_diagnostics.c`) needs "eight subsystem-health snapshot functions
... none of which exist" as host stand-ins. Reading each function's own
module, rather than assuming the prior framing was still accurate, found this
was no longer (or never was) fully true:

- `app_lifecycle_health_snapshot()` (`support/app_lifecycle_health.c`),
  `storage_health_snapshot()` (`storage/storage_health.c`),
  `auth_health_snapshot()` (`auth/auth_health.c`),
  `usb_health_snapshot()` (`usb_keyboard/usb_health.c`),
  `executor_health_snapshot()` (`macro_executor/executor_health.c`), and
  `http_health_snapshot()` (`web_server/http_health.c`) are each pure C
  operating on a private static struct, with no ESP-IDF/FreeRTOS/NVS
  dependency — `app_lifecycle_health.h`'s own comment says so explicitly
  ("Portable C with no ESP-IDF dependency, so it is host-testable
  directly"), and the other five follow the identical shape. **Six of the
  eight link in real, completely unfaked.**
- `device_controls_get_health()` (`device_controls/device_controls.c`) and
  `wifi_ap_get_status()` (`wifi_ap/wifi_ap.c`) do live in
  ESP-IDF/FreeRTOS-bound files and genuinely need fakes — but the *pure logic*
  that turns their results into a `subsystem_health_state_t`
  (`device_controls_health_derive_state()` in `device_controls_logic.c`,
  `wifi_ap_health_derive_state()` in `wifi_ap_state.c`) is itself
  ESP-IDF-free and already proven host-linkable by the pre-existing
  `device_controls_tests`/`wifi_ap_tests` targets. So only **two** of the
  eight needed a fake at all, and even then only for the raw hardware-facing
  entry point, not the derivation logic.

The remaining real gap was two small ESP-IDF headers:
`esp_heap_caps.h` (`heap_caps_get_largest_free_block()`, `MALLOC_CAP_8BIT`)
had no host stand-in, and the existing `esp_idf_misc_stub/esp_system.h`
(added by the administration-route track for `esp_restart()`) was missing
`esp_reset_reason_t`/`esp_reset_reason()`/`esp_get_free_heap_size()`/
`esp_get_minimum_free_heap_size()`. Both were copied from the real ESP-IDF
v5.5.5 headers (`$IDF_PATH/components/heap/include/esp_heap_caps.h`,
`$IDF_PATH/components/esp_system/include/esp_system.h`) for exact enum/type
fidelity, following the rationale `esp_http_server_stub/esp_http_server.h`
and the existing `esp_app_desc.h`/`esp_timer.h` stand-ins already document.

Given this, extending the existing `test_web_server_administration_route.c`
(rather than building a second, largely-duplicate httpd-fake harness) was the
right fit: it already links `web_server_api.c`/`web_request_policy.c`/
`web_api_core.c`/`web_api_dispatch.c`/`web_api_administration.c` (diagnostics'
real dispatch path) and already has auth/device_settings/device_controls
fakes for its other routes, so only the diagnostics-specific additions were
new.

## What was built

### 1. `tests/host/test_web_server_setup_route.c` (new, 3 tests)

Drives the real `setup_state_handler()` against `fakes/esp_http_server_stub`
+ `fakes/fake_httpd.c`. Cases: valid response while unprovisioned (200,
exactly the two SPEC_V2 13.4 fields, deep-compared via `cJSON_Compare()`
against `contracts/v2/api/examples.json`'s `setupState` fixture — see item 3
below), the response reflecting a different configured device name, and `404`
once `server_configuration.mode` is `WEB_SERVER_MODE_NORMAL` (a
defense-in-depth check of the handler's own guard; the real production
404-after-provisioning path is the wildcard's `setup_route_response()`,
already tested by `test_web_server_administration_route.c`). Authentication,
wrong-content-type, oversized, wrong-type, extra/missing-body,
malformed-path, and method-error categories are documented in the file's
header comment as inapplicable, for the same structural reasons already
recorded for status/limits (bodyless, unauthenticated, fixed single-method
URI ahead of the generic wildcard).

### 2. `tests/host/test_web_server_administration_route.c` (+4 tests, diagnostics)

Added `wifi_ap_get_status()`, `macro_executor_get_status()`,
`usb_keyboard_get_state()`, `storage_partition_capacity()`,
`storage_mount_state()`, `storage_blob_collect_diagnostics()`,
`device_controls_get_health()`, `esp_app_get_description()`,
`esp_app_get_elf_sha256()`, `esp_timer_get_time()`, `esp_reset_reason()`,
`esp_get_free_heap_size()`, `esp_get_minimum_free_heap_size()`, and
`heap_caps_get_largest_free_block()` fakes (the diagnostics-specific
dependencies `status_handler()`'s pre-existing fakes in
`test_web_server_status_limits_route.c` do not all cover), then removed the
stub `web_diagnostics_handle()` definition and linked the real
`web_server_diagnostics.c` plus the six real health-snapshot `.c` files and
`device_controls_logic.c`/`wifi_ap_state.c` (see "Investigation" above).

New cases: `GET /api/v1/diagnostics` valid (200; deep-compared via
`cJSON_Compare()` against `contracts/v2/api/examples.json`'s `diagnostics`
fixture with `subsystems` normalized to an empty array first — see item 3;
`subsystems` array length and one entry's derived `healthy` state checked
separately since a real device always reports all 8 entries, unlike the
fixture's placeholder empty array), unauthorized (no cookie), unauthorized
(expired session), and one representative backend-failure mapping
(`storage_blob_collect_diagnostics()` failure -> 503). Every other
already-covered category (malformed-path/method-error at the pure-function
level, the physical-confirmation-required=true/async-worker path) is
unaffected and unchanged from the prior track.

### 3. `tests/host/fakes/esp_idf_misc_stub/esp_system.h` (extended)

Added `esp_reset_reason_t` (copied from the real ESP-IDF v5.5.5 enum) and
`esp_reset_reason()`/`esp_get_free_heap_size()`/
`esp_get_minimum_free_heap_size()` declarations, alongside the existing
`esp_restart()`.

### 4. `tests/host/fakes/esp_idf_misc_stub/esp_heap_caps.h` (new)

`MALLOC_CAP_8BIT` and `heap_caps_get_largest_free_block()`, the one flag and
one entry point `web_server_diagnostics.c` uses, following the same
rationale as the other `esp_idf_misc_stub` headers.

### 5. `tests/host/support/test_examples_fixture.c`/`.h` (new)

A small shared helper: loads and parses `contracts/v2/api/examples.json`
once per test binary (path supplied by a new `EXAMPLES_JSON_PATH` compile
definition, the same convention `TEST_SECRET_SENTINEL_SCANNER_PATH` already
uses for `scripts/check-secret-sentinel.py`) and returns a borrowed,
cached `cJSON*` for a given top-level key. Added only to the two targets
that use it (not the shared `test_support` library, to avoid forcing a new
compile-time dependency onto every one of the ~90 other host test targets
that do not need it).

### 6. `tests/host/CMakeLists.txt`

New `web_server_setup_route_tests` target (links `web_server_setup.c`,
`web_server_setup_submit.c`, and their shared adapter/cookie/json
dependencies, following the `web_server_status_limits_route_tests` recipe).
Extended `web_server_administration_route_tests` with the diagnostics
sources listed above and `storage/include`/`device_controls`/`wifi_ap`
(bare, for `device_controls_logic.h`/`wifi_ap_ops.h`) include directories.
Both targets, plus a new top-level `API_EXAMPLES_JSON` path variable (next to
the existing `SECRET_SENTINEL_SCANNER` one) and an `EXAMPLES_JSON_PATH`
compile definition on each. Both registered under the `web` CTest label.

### 7. `docs/TODO_V2.md`

Updated only the V2-057 bullets (route matrix, setup-state, exact schemas,
consume-checked-in-examples) and the Phase 5 exit gate's two bullets
(test-count 53/53 -> 54/54, API-examples-match-observed-responses). No other
lines touched, per the file-surface instruction that another track is
editing this file in a separate worktree.

## Item 3 (stretch goal): what "consuming the same examples" means here

`webapp/tests/v2-api-contracts.test.ts` runs each `examples.json` fixture
through its own runtime type guard (`isSetupStateResponse(examples.setupState)`,
`isDiagnosticsResponse(examples.diagnostics)`, ...) and separately asserts an
injected unknown field is rejected — a schema-conformance check, not a
literal byte-for-byte comparison against a specific numeric fixture. The C
side does something stronger for the two routes in scope: it configures the
real handler's backend fakes to produce output that, field-for-field, equals
the checked-in example, then asserts `cJSON_Compare(actual, example, true)`
directly — proving the live response is byte-for-byte identical to the
committed contract example, not merely shaped like it.

For `setupState`, this is a full, unconditional deep compare (the response
has exactly two fields, both scalar, with no environment-dependent content).
For `diagnostics`, every field lines up with the example's fixture values
after `reset_fakes()` was updated to produce them exactly (heap sizes:
200000/180000/120000 bytes; `webfsUsedBytes` 500000; the rest already
matched) — except `subsystems`, which `examples.json` deliberately leaves as
an empty array (a documentation placeholder) while a real handler always
reports all `WEB_DIAGNOSTICS_SUBSYSTEM_COUNT` (8) entries. The test
normalizes a duplicate of the real response's `subsystems` to `[]` before
comparing against the example, and separately asserts the real,
un-normalized response has exactly 8 subsystem entries with the expected
`controls` entry's derived `healthy` state — so both the "matches the
contract for everything the contract actually pins" and the "the dynamic
field behaves correctly" properties are proven, neither silently skipped.

Not attempted: extending this to any other route (status, limits, send,
login, session, restart, settings, blob, ...). Their C tests still hand-build
fixture JSON inline, matching the pre-existing convention every prior V2-057
track used. The task scoped this stretch goal to "at least the routes you're
touching in this task," and this track's touched routes are exactly
setup-state and diagnostics.

## Commands run and results

Fast loop (`web` label only):

```bash
./scripts/run-tests.sh web
```

23/23 passed, including the new `web_server_setup_route` target (3 tests) and
the extended `web_server_administration_route` target (20 tests, 16 prior +
4 new diagnostics tests).

Full host suite:

```bash
./scripts/run-tests.sh
```

54/54 passed (0 failed), all labels (`auth`, `controls`, `executor`, `model`,
`parser`, `startup`, `storage`, `support`, `usb`, `web`, `wifi`).

Sanitizers (`web` label, ASan+UBSan):

```bash
./scripts/run-tests.sh --sanitizers web
```

23/23 passed — in particular, this exercises the `cJSON_Duplicate()`/
`cJSON_ReplaceItemInObjectCaseSensitive()` calls the new diagnostics
fixture-comparison test added, with no leak/UB findings.

Format check:

```bash
./scripts/check-format.sh
```

Clean for every C/CMake file this track touched (`clang-format 18`,
`cmake-format`/`cmake-lint 0.6.13` — `cmake-format -i`/`clang-format -i`
applied during development, then reverified clean). The script's final `npm
--prefix webapp run format:check` step fails in this environment with
`prettier: not found` because `webapp/node_modules` is not installed in this
sandboxed worktree — a pre-existing environment gap already recorded by both
prior V2-057 tracks' reports, not caused by this track (file surface was
`tests/host/` and `docs/` only; no `webapp/` files touched).
`markdownlint-cli2` (used by `check-docs.sh` for this file and the
`docs/TODO_V2.md` edit) is similarly unavailable in this sandboxed worktree
for the same reason (it also ships from `webapp/node_modules`); not run here.

Firmware build + clang-tidy:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh" && ./scripts/check-firmware.sh
```

Run against the exact pinned toolchain (ESP-IDF `v5.5.5`, confirmed via
`idf.py --version` -> `ESP-IDF v5.5.5`). Exit code `0`. This track made
**zero changes under `firmware/`** (no defect was found this round; file
surface was `tests/host/` and `docs/` only). Both projects (`firmware/`,
`firmware/test_app/`) built cleanly with the GCC toolchain, both
clang-toolchain compile databases generated successfully, and
`run-clang-tidy` reported zero first-party findings in either
(`grep -cE ':[0-9]+:[0-9]+: (warning|error):'` over the captured log
returned `0`).

## Files changed

- `tests/host/test_web_server_setup_route.c` (new)
- `tests/host/test_web_server_administration_route.c` (+4 tests, diagnostics
  fakes, real `web_server_diagnostics.c`/health-file linkage, examples-fixture
  comparison)
- `tests/host/fakes/esp_idf_misc_stub/esp_system.h` (extended)
- `tests/host/fakes/esp_idf_misc_stub/esp_heap_caps.h` (new)
- `tests/host/support/test_examples_fixture.c` (new)
- `tests/host/support/test_examples_fixture.h` (new)
- `tests/host/CMakeLists.txt` (new `web_server_setup_route_tests` target;
  extended `web_server_administration_route_tests`; new
  `API_EXAMPLES_JSON`/`EXAMPLES_JSON_PATH` plumbing)
- `docs/TODO_V2.md` (V2-057 bullets and Phase 5 exit gate bullets only)
- `docs/implementation-v2/V2_057_DIAGNOSTICS_AND_SETUP_STATE_2026-08-09.md`
  (this file)

## What remains open (honestly, not claimed complete)

- **The physical-confirmation-required=true/async-worker path** for the
  administration route group (restart/reset-settings/factory-reset/
  change-password routed through `web_server_async_dispatch()`'s FreeRTOS
  worker queue and `device_controls_wait_for_confirmation()`) remains
  live-untested — unchanged from both prior tracks' assessment, and out of
  this track's scope (`web_server_async.c`, not `web_server_api.c`/
  `web_request_policy.c`, and not host-linkable).
- **Consume the same checked-in examples**: done only for setup-state and
  diagnostics, deliberately not extended to any other route (see item 3
  above).
- **The prior tracks' `WEB_DIAGNOSTICS_SUBSYSTEM_COUNT` (8) real-device
  content is not compared against `examples.json`'s `subsystems: []`** —
  that field is a documentation placeholder in the fixture, not a value a
  real handler is expected to reproduce; this is a deliberate exclusion, not
  a gap, and is stated explicitly in the new test's own comment.
- Nothing in this report claims physical-hardware or on-device validation —
  everything here is host-fake-backed native test evidence, matching the
  hardware-evidence rules in `docs/TODO_V2.md` §0.1.

## Explicit statement

All three items in this task's scope are closed with real implementation and
reproducible evidence: item 1 (setup-state GET) and item 2 (diagnostics live
coverage) each have new, passing, ASan/UBSan-clean tests exercising the real
production handler; item 3 (consume checked-in examples) is implemented for
both routes this track touched, deliberately not generalized further. The two
V2-057 checkboxes this track's evidence supports (`setup-state`, and the
`exact response schemas` bullet's diagnostics/setup-state clause) are marked
`[x]`; every other V2-057 bullet and both Phase 5 exit-gate bullets remain
`[ ]`, matching the genuine, disclosed gap above (the
physical-confirmation-required=true/async-worker path) and the deliberately
narrow scope of item 3.
