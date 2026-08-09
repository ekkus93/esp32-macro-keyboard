# V2-057 — status/limits/send/diagnostics HTTP contract matrix (2026-08-09)

## Scope

Track E, continuing V2-057 ("Contract and security tests"). Prior tracks built
full httpd-fake matrix coverage for the blob routes
(`tests/host/fakes/esp_http_server_stub`, `fakes/fake_httpd.c`,
`test_web_server_blob*.c/.inc`) and HTTP-composition-level (but not live-httpd)
coverage for session/restart/setup-conflict in `test_web_api_administration.c`.
This track's job: extend real matrix coverage — valid, missing, extra,
wrong-type, wrong-content-type, oversized, unauthorized, expired-session,
malformed-path, and method-error — to **status, limits, send, and
diagnostics**, and audit whether session/restart/setup-conflict's existing
coverage is actually the full matrix.

## Branch and commit

Worktree branch: `worktree-agent-a7f87e4ead1ee652f`.
Starting commit: `6cbd4ca2c4856ae61b468a479b1ab081dbe9610f`.
This work's commit SHA: recorded in the commit that carries this file (see
`git log -1` on this file's introducing commit).

## Investigation: which pattern fits which route

Read every route's real httpd wiring in `web_server_common.c`'s route table
before choosing an approach:

- **status, limits, send** (`web_server_status_limits.c`, `web_server_send.c`)
  each have **dedicated, fixed-URI `httpd_uri_t` registrations** (`GET
  /api/v1/status`, `GET /api/v1/limits`, `POST`/`GET`/`DELETE /api/v1/send`)
  registered directly ahead of the generic `/api/v1/*` wildcard. Their
  handlers (`status_handler`, `limits_handler`, `send_create_handler`,
  `send_get_handler`, `send_cancel_handler`) take a real `httpd_req_t*`, call
  `authorize_mutation()` themselves, and never touch
  `web_api_parse_path()`/`web_request_policy_evaluate()`/`web_api_dispatch()`
  at all — exactly like blob's handlers. **The httpd-fake approach (like
  blob) is the right fit**, and is what this track built for all three.
- **diagnostics** (`web_server_diagnostics.c`) is different: its entry point,
  `web_diagnostics_handle()`, takes a `web_api_response_t*`, not an
  `httpd_req_t*`. It is reached only through the generic wildcard pipeline
  (`api_handler` → `web_api_handle_call` → `prepare_api_call` →
  `apply_request_policy` (`web_request_policy_evaluate`) →
  `web_api_dispatch` → `web_api_handle_administration` →
  `web_diagnostics_handle`) — the same pipeline session/restart/settings/
  setup-conflict already share. **The direct-function-call approach (like
  `web_api_administration.c`) is the right fit** for diagnostics too, not a
  second httpd-fake harness; building one would mean faking
  `esp_app_desc.h`/`esp_timer.h`/`esp_heap_caps.h`/`esp_system.h` plus eight
  subsystem-health snapshot functions (`app_lifecycle_health_snapshot`,
  `storage_health_snapshot`, `auth_health_snapshot`, `usb_health_snapshot`,
  `executor_health_snapshot`, `device_controls_get_health`,
  `wifi_ap_health_derive_state`, `http_health_snapshot`) for value
  disproportionate to what's actually untested (diagnostics' own JSON
  composition already has deep, dedicated coverage — see "What remains open"
  below).
- Confirmed before writing anything that **malformed-path and method-error
  for status/limits/send are structurally impossible to test by calling the
  handler directly**: a wrong method or malformed path for these three fixed,
  single-purpose URI registrations never reaches `status_handler` etc. in
  production at all — ESP-IDF's httpd routes it to the `/api/v1/*` wildcard
  instead, which resolves the route via `web_api_parse_path()` and rejects it
  via `web_api_route_allows_method()` before any admin/status/limits/send
  logic runs. Verified `test_web_api_core.c` already asserts
  `web_api_route_allows_method(WEB_API_ROUTE_STATUS/LIMITS/SEND, …)` and
  `web_api_parse_path("/api/v1/status"|"/api/v1/limits"|"/api/v1/send", …)`
  for exactly this — that pure-function suite is the correct, and only
  correct, place for those two categories on these routes, not a gap to fill
  with a second test.

## What was built

### 1. `tests/host/test_web_server_status_limits_route.c` (new, 11 tests)

Drives the real `status_handler()`/`limits_handler()` against
`fakes/esp_http_server_stub` + `fakes/fake_httpd.c`, with narrow
behavior-controllable test doubles for every dependency that is
NVS/FreeRTOS/hardware-backed in production and not host-linkable: `auth`
(`auth_session_validate`), `device_settings` (`device_settings_read`),
`wifi_ap` (`wifi_ap_get_status`), `macro_executor`
(`macro_executor_get_status`), `usb_keyboard` (`usb_keyboard_get_state`),
`storage` (`storage_partition_capacity`, `storage_mount_state`),
`storage_blob` (`storage_blob_list`) — the same substitution technique
`test_web_server_blob_fixture.inc` already uses for `auth_session_validate`/
`storage_blob`.

Two of those dependencies are declared in **ESP-IDF-only headers** that
cannot be compiled on a host build (`esp_app_desc.h`, `esp_timer.h`); new
minimal host stand-ins for both were added at
`tests/host/fakes/esp_idf_misc_stub/{esp_app_desc.h,esp_timer.h}`, following
the exact rationale `esp_http_server_stub/esp_http_server.h` already
documents for why a real ESP-IDF header can't link on host.

Cases covered: valid schema (status and limits), send-present reflecting
executor state, unauthorized (no cookie), unauthorized (expired/invalid
session), and three backend-failure-to-503 mappings (device-settings read
failure, storage-capacity query failure/inconsistency, blob-listing
failure). Wrong-type/wrong-content-type/oversized/extra/missing-body do not
apply — both routes are bodyless GETs (documented in the file's header
comment, the same carve-out already recorded for blob's raw-byte body).
Malformed-path/method-error: see the investigation section above — covered
at the pure-function level, not here.

### 2. `tests/host/test_web_server_send_route.c` (new, 20 tests)

Drives the real `send_create_handler()`/`send_get_handler()`/
`send_cancel_handler()` against the same httpd fake, with test doubles for
`auth_session_validate` and `macro_executor`'s public entry points
(`macro_executor_submit`/`macro_executor_get_status`/`macro_executor_cancel`)
— neither NVS/mbedtls- nor FreeRTOS-backed, so not host-linkable — and the
real `macro_parser_v2` compile path linked in (as `web_send_tests` already
does), so a genuine macro-syntax parse error is exercised end to end.

Cases covered for `POST /api/v1/send`: valid (202, exact accepted schema),
unauthorized (no cookie), unauthorized (expired session), missing
Content-Type (415), wrong Content-Type (415), missing body (400), oversized
body (413), wrong-type field (400), extra field (400), missing field (400),
macro-syntax parse error (422, with byte offset/line/column), executor busy
(409), USB not ready (503). For `GET /api/v1/send`: valid (200, exact
status schema), unauthorized (no cookie), unauthorized (expired session),
never-sent (404). For `DELETE /api/v1/send`: valid (202, accepted), 
unauthorized (no cookie), never-sent (404). Malformed-path (no path
parameter on this route) and method-error (e.g. `PUT /api/v1/send`) are
covered at the pure-function level in `test_web_api_core.c`
(`WEB_API_ROUTE_SEND` cases), for the same structural reason as status/limits.

### 3. `tests/host/test_web_request_policy.c` (+1 test)

Added `test_diagnostics_route_unauthorized_and_expired_session()`: exercises
`web_request_policy_evaluate()` — the exact gate `web_server_api.c` calls
before `web_api_dispatch()` ever runs — for `WEB_API_ROUTE_DIAGNOSTICS_FULL`
specifically, both for a missing cookie (unauthorized) and an
invalid/expired session (`validate_session` returning
`APP_ERROR_AUTH_REQUIRED`). The pre-existing `test_fail_closed_ordering()`
already proved these failure modes are route-agnostic using
`WEB_API_ROUTE_SETTINGS`/`BLOB_COLLECTION`/`DEVICE_FACTORY_RESET` as
representative routes; this removes any doubt for diagnostics specifically
rather than leaving it inferred by analogy.

### 4. `tests/host/test_web_api_administration.c` (+1 test)

Added `test_diagnostics_route_dispatches_to_handler()`: proves
`web_api_handle_administration()`'s switch statement actually routes
`WEB_API_ROUTE_DIAGNOSTICS_FULL` to `web_diagnostics_handle()` (against the
existing stub) rather than silently falling into the `APP_ERROR_NOT_FOUND`
default case alongside every dedicated-handler route. Nothing in this suite
tested that wiring before.

### 5. `tests/host/CMakeLists.txt`

Two new host test targets, `web_server_status_limits_route_tests` and
`web_server_send_route_tests`, following the existing `web_server_blob_tests`
recipe (same adapter/common/cookie/core sources, `fakes/fake_httpd.c`, the
same include-directory set). Both registered under the `web` CTest label.

### 6. `docs/TODO_V2.md`

Updated only the V2-057 matrix bullet, the "exact response schemas" bullet,
and the Phase 5 exit gate's test-count note (45/45 → 52/52) — no other lines
touched, to minimize conflict with the two other tracks editing this file in
parallel worktrees.

## What remains open (honestly, not claimed complete)

- **status, limits, send**: full matrix now closed at the live-handler level
  for every category that structurally applies to them. Malformed-path and
  method-error are closed at the pure-function level (`test_web_api_core.c`)
  rather than the handler level, because that is the code that actually
  answers those cases in production for these three routes — documented in
  each new test file's header comment and in the TODO_V2.md bullet.
- **diagnostics**: unauthorized/expired-session now specifically verified
  (via `web_request_policy_evaluate()`), and the dispatch wiring to
  `web_diagnostics_handle()` now verified (via
  `web_api_handle_administration()` against the stub). Diagnostics' own JSON
  composition (schema, exact status codes, secret-sentinel absence,
  bounded-output/corrupt-input handling) already has deep, dedicated coverage
  in `test_web_server_adapter_diagnostics_json.inc` (7 tests exercising
  `web_adapter_build_diagnostics_json()` directly). What is **not** built: a
  live httpd-level test of `collect_diagnostics()` itself (the function that
  gathers the snapshot from all eight subsystem-health functions plus
  storage/wifi/usb/executor/settings state) — that would need host stand-ins
  for `esp_heap_caps.h`/`esp_system.h`'s `esp_reset_reason`/
  `esp_get_free_heap_size`/`esp_get_minimum_free_heap_size` plus all eight
  `*_health_snapshot()`/`*_get_health()` functions, none of which exist yet.
  Investigated and deliberately deferred as disproportionate scope for this
  track given the composition logic is a mechanical field-by-field pass-
  through already covered end-to-end by the JSON-builder tests plus the
  now-added policy/dispatch tests. Left open rather than falsely checked.
- **session, restart, settings, change-password, reset-settings,
  factory-reset, setup-conflict**: unchanged from the prior track — still
  direct-function-call coverage (`web_api_administration_tests`) plus the
  shared `web_request_policy_evaluate()` unit coverage, not a live end-to-end
  HTTP test through `web_server_api.c`'s `api_handler()`. Building that would
  require: (a) extending `esp_http_server_stub`'s `httpd_req_t` with a
  `method` field and an `httpd_method_t`-equivalent enum (the current stub
  has none — `web_server_api.c`'s `method_from_request()` needs it, but no
  route this track touched does), (b) a new `esp_system.h` stand-in for
  `esp_restart()`, and (c) wiring `web_server_api.c` + `web_request_policy.c`
  to the fake alongside `web_api_administration.c`'s existing test doubles.
  Investigated as the single highest-leverage remaining gap (it's the literal
  "no single live end-to-end HTTP test exercises every route the same way"
  complaint TODO_V2.md already records) but not built in this track — left
  open in TODO_V2.md, not claimed complete.
- **Consume the same checked-in examples from C and TypeScript tests**: not
  attempted. This track's httpd-fake tests hand-build fixture JSON inline
  (matching the existing blob/administration convention) rather than parsing
  `contracts/v2/api/examples.json`; per the task's own framing this is a
  stretch goal only after the matrix work is solid, and the C side still
  never parses that file.

## Commands run and results

Host tests (fast loop, `web` label only):

```
./scripts/run-tests.sh web
```

21/21 passed, including the two new targets
(`web_server_status_limits_route`, `web_server_send_route`) and the updated
`web_request_policy`/`web_api_administration` targets.

Full host suite:

```
./scripts/run-tests.sh
```

52/52 passed (0 failed), all labels (`auth`, `controls`, `executor`, `model`,
`parser`, `startup`, `storage`, `support`, `usb`, `web`, `wifi`).

Format check:

```
./scripts/check-format.sh
```

Clean for every C/CMake/shell/YAML file this track touched (clang-format 18,
cmake-format/cmake-lint 0.6.13, shfmt/shellcheck, yamllint, actionlint — all
ran with no violations reported). The script's final `npm --prefix webapp run
format:check` step fails in this environment with `prettier: not found`
because `webapp/node_modules` is not installed in this sandboxed worktree —
a pre-existing environment gap, not something this track's changes (`tests/
host/` and `docs/` only) caused or could fix; this track touched no `webapp/`
files.

Firmware build + clang-tidy:

```
. "$HOME/esp/esp-idf-v5.5.5/export.sh" && ./scripts/check-firmware.sh
```

Run against the exact pinned toolchain (ESP-IDF `v5.5.5`, confirmed via
`idf.py --version` → `ESP-IDF v5.5.5`). This track made **zero changes under
`firmware/`** (file surface was `tests/host/` and `docs/` only). Both
projects (`firmware/`, `firmware/test_app/`) built cleanly with the GCC
toolchain, both clang-toolchain compile databases generated successfully,
and `run-clang-tidy` reported **zero first-party findings** in either —
`grep -cE ':[0-9]+:[0-9]+: (warning|error):'` restricted to
`/firmware/(main/|components/|test_app/main/)` returned 0 matches. Exit
code confirmed `0`. The only warning anywhere in the full log is a
pre-existing, unrelated `unknown kconfig symbol 'TINYUSB_HID_ENABLED'`
notice from `firmware/test_app/sdkconfig.defaults`, not a clang-tidy finding
and not touched by this track.

## Files changed

- `tests/host/test_web_server_status_limits_route.c` (new)
- `tests/host/test_web_server_send_route.c` (new)
- `tests/host/fakes/esp_idf_misc_stub/esp_app_desc.h` (new)
- `tests/host/fakes/esp_idf_misc_stub/esp_timer.h` (new)
- `tests/host/CMakeLists.txt` (two new targets)
- `tests/host/test_web_request_policy.c` (+1 test)
- `tests/host/test_web_api_administration.c` (+1 test)
- `docs/TODO_V2.md` (V2-057 bullets only)

## Explicit statement

No V2-057 checkbox is claimed complete by this track. The matrix bullet, the
Phase 5 "contract and security tests pass" bullet, and the "consume checked-in
examples" bullet remain unchecked, matching the genuine, disclosed gaps
above. Nothing in this report claims physical-hardware or on-device
validation — everything here is host-fake-backed native test evidence.
