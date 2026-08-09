# V2-057 — live end-to-end HTTP test for the administration route group (2026-08-09)

## Scope

Track I, closing V2-057's last remaining item as framed by
`docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md`'s "What
remains open" section: session, restart, settings, change-password,
reset-settings, factory-reset, and setup-conflict were tested only via direct
function calls (`web_api_handle_administration()`, `tests/host/test_web_api_administration.c`)
and `web_request_policy_evaluate()` unit coverage — no live end-to-end HTTP
test wired `web_server_api.c`/`web_request_policy.c` to the httpd fake for
that group, the way `test_web_server_blob*.c`, `test_web_server_send_route.c`,
and `test_web_server_status_limits_route.c` already do for their routes.

## Branch and commit

Worktree branch: `worktree-agent-a42a037016f25fb86`.
Starting commit: `9bae4455c20b23c34367e65477fe74d043a0885c`.
This work's commit SHA: recorded in the commit that carries this file (see
`git log -1` on this file's introducing commit).

## Investigation: was the prior track's framing still accurate?

Confirmed, before writing anything, that the prior track's assessment of what
closing this gap needs was still correct:

- `tests/host/fakes/esp_http_server_stub/esp_http_server.h`'s `httpd_req_t`
  had no `method` field and no `httpd_method_t`-equivalent enum.
  `web_server_api.c`'s `method_from_request()` (`api_handler()`'s first step)
  switches on `request->method` against `HTTP_GET`/`HTTP_POST`/`HTTP_PUT`/
  `HTTP_DELETE` — none of which existed in the stub. No route the prior
  track's new tests touched (status/limits/send have fixed single-purpose
  `httpd_uri_t` registrations and never call `method_from_request()`) needed
  it; the generic `/api/v1/*` wildcard's `api_handler()` does.
- No `esp_idf_misc_stub/esp_system.h` existed. `web_server_api.c` calls
  `esp_restart()` (declared in the real ESP-IDF `esp_system.h`, which cannot
  be compiled on a host build) after a successful `202` restart/factory-reset
  response.
- `web_api_route_requires_worker()` (`web_api_core.c`) is unconditionally
  `false` for every route (confirmed by reading it directly, not assumed),
  so with `server_configuration.require_physical_confirmation = false` set
  in every test below, `web_api_request_requires_worker()` is `false` for
  every administration route too — `api_handler()` always takes the
  synchronous `web_api_handle_call()` path under test, never
  `web_server_async_dispatch()`'s FreeRTOS worker queue, which is not
  host-linkable and stays out of scope (see "What remains open").

Both gaps were real and both were closed. No third gap was found.

## What was built

### 1. `tests/host/fakes/esp_http_server_stub/esp_http_server.h` (+`method`/`httpd_method_t`)

Added a minimal `httpd_method_t` enum (`HTTP_GET`/`HTTP_POST`/`HTTP_PUT`/
`HTTP_DELETE` — the only four `method_from_request()` maps; values are
internal to this stub, nothing compares them against the real ESP-IDF enum)
and a `method` field on `httpd_req_t`. Defaults to `0` (`HTTP_GET`) after
`fake_httpd_bind()`'s `memset`, so every pre-existing caller (blob/status/
limits/send, none of which read this field) is unaffected.

### 2. `tests/host/fakes/fake_httpd.c`/`.h` (+`fake_httpd_set_method()`)

A small setter, called after `fake_httpd_bind()` (which `memset`s the whole
`httpd_req_t` including `method`), for the one new test file that needs a
method other than the default GET.

### 3. `tests/host/fakes/esp_idf_misc_stub/esp_system.h` (new)

Following the exact rationale already documented in
`esp_http_server_stub/esp_http_server.h` and `esp_idf_misc_stub/esp_app_desc.h`/
`esp_timer.h`: declares only `void esp_restart(void)` — the one symbol
`web_server_api.c` needs from the real header, deliberately without
`noreturn` so the host-test fake definition (in the new test file) returns
control to the caller instead of terminating the test process.

### 4. `tests/host/test_web_server_administration_route.c` (new, 16 tests)

Drives the real `api_handler()` (`web_server_api.c`'s generic `/api/v1/*`
wildcard httpd handler — not `web_api_handle_administration()` called
directly) against `fakes/esp_http_server_stub` + `fakes/fake_httpd.c`, with
narrow behavior-controllable test doubles for every dependency that is
NVS/mbedtls/GPIO/FreeRTOS-backed in production and not host-linkable: `auth`
(`auth_session_validate`, `auth_session_remaining`, `auth_password_verify`,
`auth_password_create`, `auth_session_logout_all`), `device_settings`
(`device_settings_read`/`replace`), `device_controls`
(`restart`/`reset_settings`/`factory_reset`/`wait_for_confirmation`),
`esp_restart()`, and `web_server_async_dispatch()` (present only for the
linker; never reached, see below). `web_diagnostics_handle()` gets the same
never-exercised stub treatment `test_web_api_administration.c` already uses.

`server_configuration.require_physical_confirmation` is left `false` for
every test, so `api_handler()` always takes the synchronous path under test
(see the investigation section above).

Cases covered:

- `GET /api/v1/auth/session`: valid (200, exact `authenticated`/
  `idleExpiresInSeconds` schema, `X-Request-ID` header present), unauthorized
  (no cookie), unauthorized (expired/invalid session).
- `POST /api/v1/device/restart`: valid (202, `device_controls_restart()`
  called once, `esp_restart()` called once — the one behavior only the live
  `api_handler()` path, not `web_api_handle_administration()` called
  directly, can prove), unauthorized (no cookie; neither `restart` nor
  `esp_restart` called).
- `GET /api/v1/settings`: valid (200, flat schema).
- `PUT /api/v1/settings`: valid (200, response wraps the updated settings
  under `"settings"` — the shape differs from GET's flat shape, which the
  first draft of this test got wrong and a live run caught, see below),
  unauthorized (expired session; backend never touched).
- `POST /api/v1/settings/change-password`: valid (204, `Set-Cookie` cleared,
  `auth_session_logout_all()` called once, `esp_restart()` NOT called),
  incorrect current password (403, no `Set-Cookie`).
- `POST /api/v1/device/reset-settings`: valid (202, `esp_restart()` NOT
  called — `device_controls` itself reboots; `web_server_api.c` only calls
  `esp_restart()` for `DEVICE_RESTART`/`DEVICE_FACTORY_RESET`), wrong
  confirmation phrase (422 — `APP_ERROR_INVALID_ARGUMENT` maps to 422, not
  400; the test's first draft assumed 400 and a live run caught that too).
- `POST /api/v1/device/factory-reset`: valid (202, `esp_restart()` called
  once), incorrect administrator password (403, neither `factory_reset` nor
  `esp_restart` called).
- `GET`/`POST /api/v1/setup` (provisioned-mode conflict, SPEC_V2 13.4): 404
  and 409 respectively, reached with no session (SETUP is exempt from
  `web_api_route_requires_session()`).

Unauthorized/expired-session are not duplicated per route: proven once on a
bodyless GET (session), once on a bodyless POST (restart), and once on a
body-bearing PUT (settings) to cover every request shape the shared
`web_request_policy_evaluate()` gate sees, per the task's own guidance that
this is redundant with `test_web_request_policy.c`'s route-agnostic
`test_fail_closed_ordering()` if repeated per route.

### 5. A real defect found and fixed: `firmware/components/web_server/web_server_api.c`

`status_text()` had no `case` for `WEB_HTTP_STATUS_NO_CONTENT` (204) — the
exact status the successful change-password response uses
(`web_api_handler_no_content(response, WEB_HTTP_STATUS_NO_CONTENT)` in
`web_api_administration.c`). It fell through to the `default` branch and
returned `"500 Internal Server Error"`. `response->status` itself (204) was
correct throughout — the bug was purely in the HTTP status *line*
`send_api_response()` sends via `httpd_resp_set_status()` — so a real client
completing a password change successfully (new password stored, all
sessions invalidated, cookie cleared) would have seen the request reported
as a server error.

No existing test could have caught this: every prior change-password test
(`tests/host/test_web_settings.c`, `test_web_api_administration.c`) either
calls `web_change_password_handle()`/`web_api_handle_administration()`
directly and inspects `response.status` (the integer `204`, never touched by
the bug) or never reaches `send_api_response()`/`status_text()` at all — only
a live `api_handler()` test exercises that function. Fixed by adding the
missing case (`"204 No Content"`); `test_change_password_post_valid()` above
asserts the fixed wire status line and documents why in its own comment.

### 6. `tests/host/CMakeLists.txt` (+1 target)

New `web_server_administration_route_tests` target, following the existing
`web_server_send_route_tests`/`web_api_administration_tests` recipes: links
`web_server_api.c`, `web_request_policy.c`, `web_api_core.c`,
`web_api_dispatch.c`, `web_api_administration.c`, `web_api_response.c`,
`web_api_handler_common.c`, `web_api_json.c`, `web_auth_routes.c`,
`web_settings.c`, `web_device_actions.c`, `web_cookie.c`,
`web_server_common.c`, the three `web_server_adapter_*.c` files,
`support/subsystem_health.c`, `macro_model/app_uuid.c`/`app_error.c`,
`app_contracts_v2/settings_contract_v2.c`/`device_settings_v2.c`/
`setup_contract_v2.c`, and `fakes/fake_httpd.c`. Registered under the `web`
CTest label.

### 7. `docs/TODO_V2.md`

Updated only the V2-057 route-matrix bullet (documenting the new live
coverage for session/restart/settings/change-password/reset-settings/
factory-reset/setup-conflict, the `status_text()` defect fix, and what is
still not live-tested) and the Phase 5 exit gate's test-count note
(52/52 → 53/53). No other lines touched.

## What remains open (honestly, not claimed complete)

- **diagnostics** (`GET /api/v1/diagnostics`): still not live-httpd-tested.
  `web_diagnostics_handle()` needs host stand-ins for `esp_heap_caps.h`/
  `esp_system.h`'s `esp_reset_reason()`/`esp_get_free_heap_size()`/
  `esp_get_minimum_free_heap_size()` plus eight subsystem-health
  snapshot functions, none of which exist — unchanged from the prior track's
  assessment, and out of this track's file surface to build (would mean
  faking real subsystem behavior, not just a thin ESP-IDF header). Its own
  JSON composition remains covered by `test_web_server_adapter_diagnostics_json.inc`
  (7 tests) and its dispatch wiring/policy gate by
  `test_web_api_administration.c`/`test_web_request_policy.c`, per the prior
  track.
- **The physical-confirmation-required=true path** for restart/
  reset-settings/factory-reset/change-password: not exercised at the live
  `api_handler()` level. With `require_physical_confirmation = true`,
  `web_api_request_requires_worker()` routes these through
  `web_server_async_dispatch()`'s FreeRTOS worker queue and
  `device_controls_wait_for_confirmation()` (`web_server_async.c`) instead of
  the synchronous path this track's tests exercise — that queue is not
  host-linkable. `policy_confirm()`'s decision of *whether* a route requires
  confirmation (the part `web_request_policy.c` itself owns) is already
  covered by `test_web_request_policy.c`/`test_web_api_core.c`; only the
  async-dispatch plumbing around it is untested, and that plumbing lives in
  `web_server_async.c`, a different file from the two this task named
  (`web_server_api.c`/`web_request_policy.c`).
- **malformed-path / method-error** for the administration group: not
  applicable the same way status/limits/send's "fixed single-purpose URI"
  carve-out isn't either — administration routes ARE the generic
  `/api/v1/*` wildcard's payload, so a malformed path or disallowed method
  for them is answered by `web_api_parse_path()`/`web_api_route_allows_method()`
  before dispatch, already covered at the pure-function level in
  `test_web_api_core.c`. Not a new gap; not re-tested here to avoid
  duplicating that coverage.
- **Consume the same checked-in examples from C and TypeScript tests**: not
  attempted, same as every prior V2-057 track — this track's tests hand-build
  fixture JSON inline, matching the existing convention.

## Commands run and results

Fast loop (`web` label only):

```bash
./scripts/run-tests.sh web
```

22/22 passed, including the new `web_server_administration_route` target.

Full host suite:

```bash
./scripts/run-tests.sh
```

53/53 passed (0 failed), all labels (`auth`, `controls`, `executor`, `model`,
`parser`, `startup`, `storage`, `support`, `usb`, `web`, `wifi`).

Sanitizers (`web` label, ASan+UBSan):

```bash
./scripts/run-tests.sh --sanitizers web
```

22/22 passed. The new target was additionally built and run standalone under
a fresh `--sanitizers`-mode CMake configuration during development (before
being folded into the standard `web` label run above) to catch the two bugs
in the test itself described below.

Format check:

```bash
./scripts/check-format.sh
```

Clean for every C/CMake file this track touched (clang-format 18,
cmake-format/cmake-lint 0.6.13 — both applied via `clang-format -i`/
`cmake-format -i` during development, then reverified clean). The script's
final `npm --prefix webapp run format:check` step fails in this environment
with `prettier: not found` because `webapp/node_modules` is not installed in
this sandboxed worktree — a pre-existing environment gap already recorded by
the prior V2-057 track's report, not caused by this track (file surface was
`tests/host/`, `firmware/components/web_server/web_server_api.c`, and
`docs/` only; no `webapp/` files touched).

Firmware build + clang-tidy:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh" && ./scripts/check-firmware.sh
```

Run against the exact pinned toolchain (ESP-IDF `v5.5.5`). Both projects
(`firmware/`, `firmware/test_app/`) built cleanly with the GCC toolchain,
both clang-toolchain compile databases generated successfully, and
`run-clang-tidy` reported **zero first-party findings** in either (the
script prints a report only on failure; the run exited `0` with no findings
text emitted for either project, and a `grep -E ':[0-9]+:[0-9]+: (warning|error):'`
over the captured log found only the pre-existing, unrelated
`unknown kconfig symbol 'TINYUSB_HID_ENABLED'` notice from
`firmware/test_app/sdkconfig.defaults`, not a clang-tidy finding).

## Two real bugs a live run caught in the test itself (not firmware defects)

Documented here because they are exactly the kind of thing this track's
"live pipeline, not narrow direct calls" approach is meant to surface, even
when the bug is in the test rather than the code under test:

1. `PUT /api/v1/settings`'s success response wraps the updated settings under
   a `"settings"` key (`{"settings":{...},"restartRequired",...}`) — unlike
   `GET /api/v1/settings`'s flat shape. The test's first draft assumed the
   flat shape and dereferenced a `NULL` `cJSON` item; ASan/UBSan caught the
   resulting `SIGSEGV` immediately. Fixed by reading the response through the
   `"settings"` wrapper.
2. `auth_password_create()`'s fake initially left the returned salt/hash
   all-zero. `app_v2_password_change_prepare_candidate()`
   (`settings_contract_v2.c`) rejects an all-zero salt/verifier via
   `app_v2_device_settings_validate()`, so change-password silently fell
   through to `WEB_CHANGE_PASSWORD_INTERNAL` (500) instead of succeeding.
   Fixed by giving the fake non-zero salt/hash bytes, matching what the real
   `auth_password_create()` always produces.

Both were caught and fixed during this track's own development, before the
final commit — they are not open issues.

## Explicit statement

No new V2-057 checkbox is claimed complete by this track. The route-matrix
bullet remains unchecked (diagnostics and the physical-confirmation-required
async path are still genuinely untested at the live level, matching the
honest-gap style established by the prior track), as does the "consume
checked-in examples" bullet and the Phase 5 "contract and security tests
pass" bullet. Nothing in this report claims physical-hardware or on-device
validation — everything here is host-fake-backed native test evidence, plus
one first-party defect (`status_text()`'s missing 204 case) found and fixed
with a host-fake-backed test proving the fix.
