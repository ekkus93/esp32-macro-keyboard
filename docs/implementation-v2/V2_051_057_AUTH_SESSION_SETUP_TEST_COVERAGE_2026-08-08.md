# V2-051 / V2-057 — auth/session/setup HTTP-layer test coverage (Track A)

**Date:** 2026-08-08
**Worktree branch:** `worktree-agent-add604ada83cd603b`
**Scope:** `firmware/components/web_server/web_api_administration.c`,
`web_server_login.c`, and their headers; `tests/host/`. Host-test-only — no
hardware was used or claimed.

## Summary

Closed the specific test-coverage gaps V2-051 and (the in-scope subset of)
V2-057 identified for `GET /api/v1/auth/session`, the login cookie header,
and the provisioned-mode `WEB_API_ROUTE_SETUP` 404/409 fallback
(`setup_route_response()`), all in `web_api_administration.c` /
`web_server_login.c`. Left open, honestly: the exhaustive per-route
valid/missing/extra/wrong-type/... case matrix, the unprovisioned half of the
route-access matrix, and blob-route coverage (a different track's scope) —
see "Deferred / left open" below.

## Root cause of the original gap

`web_api_handle_administration()` (in `web_api_administration.c`) takes only
`web_api_call_t`/`web_api_response_t` — no `httpd_req_t` — so it is, in
principle, host-testable exactly like `web_send.c`/`web_settings.c`/
`web_device_actions.c`. It was never compiled into any host test target
because it transitively included `web_server_internal.h`, which pulls in
`esp_http_server.h` (no host fake exists for that header anywhere in this
codebase) purely for one declaration it needed
(`web_diagnostics_handle()`) and one extern global it used
(`server_configuration`).

## Production changes

1. **`web_diagnostics_handle()` relocated.** Declared in
   `web_diagnostics.h` (the header that already owns the diagnostics data
   types), not `web_server_internal.h`. No behavior change — same signature,
   same definition site (`web_server_diagnostics.c`), transitively still
   visible everywhere it was before.
2. **`server_configuration` relocated.** Declared in `web_server.h` (the
   header that owns `web_server_config_t`), not `web_server_internal.h`. No
   behavior change — same storage (`web_server_common.c`), same type, same
   transitive visibility. `web_server_login.c` gained a direct
   `#include "web_server.h"` (previously only transitive through
   `web_server_internal.h`; `clang-tidy`'s `misc-include-cleaner` requires
   direct inclusion of a used symbol's header).
3. **`web_api_administration.c`** now includes `web_diagnostics.h` and
   `web_server.h` directly instead of `web_server_internal.h` — the only
   symbols it actually used from that header. It no longer has any
   `esp_http_server.h` dependency, direct or transitive.
4. **New pure function `web_cookie_build_session_header()`**
   (`web_cookie.c`/`.h`) — the exact inverse of the existing
   `web_cookie_extract_session()`. Builds
   `"MKSESSION=<token>; HttpOnly; SameSite=Strict; Path=/"` with no
   `esp_http_server` dependency. `web_server_login.c::send_login_accepted()`
   now calls it instead of building the string with its own `snprintf`
   (identical output, identical failure handling).

None of this changes any wire behavior; `check-firmware.sh` (full
clang-tidy-gated build) and the full host suite both pass, so no route,
status code, JSON shape, or header value differs from before.

## New test target: `web_api_administration_tests`

Added `tests/host/test_web_api_administration.c` and wired it into
`tests/host/CMakeLists.txt` as a new CTest target (label `web`), linking the
*real* `web_api_administration.c`, `web_api_core.c`, `web_api_response.c`,
`web_api_handler_common.c`, `web_auth_routes.c`, `web_settings.c`,
`web_device_actions.c`, `web_cookie.c`, and the `app_contracts_v2` sources —
the same dependency set `web_settings_tests`/`web_device_actions_tests`/
`web_auth_routes_tests` already link individually.

`web_api_administration.c` binds to `auth.c`/`device_controls.c`/
`device_settings.c`'s *public* entry points
(`auth_session_remaining`, `auth_password_verify`, `auth_password_create`,
`auth_session_logout_all`, `device_controls_restart`/
`reset_settings`/`factory_reset`, `device_settings_read`/`replace`), none of
which are host-linkable (they pull in mbedtls, FreeRTOS, NVS, GPIO). The new
test file provides narrow, behavior-controllable stand-ins for exactly those
symbols, with the exact production signature — the same technique
`test_web_api_dispatch.c` already uses for `web_api_handle_administration()`
itself. `web_diagnostics_handle()` gets an unexercised stub for the same
link-completeness reason (its own real coverage is V2-056, a different
track). `server_configuration` gets a plain fake global (used only by
`handle_change_password()`, not exercised by any test here — out of this
track's scope).

### Tests added

- `test_handle_session_success` — `GET /api/v1/auth/session` composes
  `{"authenticated":true,"idleExpiresInSeconds":N,"absoluteExpiresInSeconds":N}`,
  status 200, and calls `auth_session_remaining()` with the exact session
  token from the request.
- `test_handle_session_backend_failure` — `auth_session_remaining()` failure
  maps to the exact error envelope (`{"error":{"code":"auth_required",
  "message":"session unavailable"}}`) and status 401.
- `test_session_and_cookie_outputs_carry_no_password_sentinel` — runs the
  real `check-secret-sentinel.py`-backed harness
  (`test_assert_no_secret_sentinel`, the same one diagnostics/status use)
  against `handle_session()`'s JSON body and
  `web_cookie_build_session_header()`'s output together, proving neither the
  session-response composition path nor the login cookie construction path
  can leak a password-shaped secret.
- `test_handle_device_restart_success` — `POST /api/v1/device/restart`
  composes the exact
  `{"accepted":true,"connectionWillClose":true,"reprovisioningRequired":false}`
  body at status 202.
- `test_handle_device_restart_backend_unavailable` — a `device_controls_restart()`
  failure maps to the correct status (503 for `APP_ERROR_STORAGE_UNAVAILABLE`)
  and error envelope.
- `test_setup_route_get_returns_not_found` / `test_setup_route_post_returns_conflict`
  — `setup_route_response()` (the live, provisioned-mode
  `WEB_API_ROUTE_SETUP` handler `web_api_handle_administration()` actually
  routes through) returns exactly 404/"route not found" on GET and
  409/"device is already provisioned" on POST. This is the real routing path,
  distinct from `test_web_server_setup_submit.c`'s
  `test_already_provisioned_rejected()`, which exercises
  `setup_submit_handler()`'s own defense-in-depth check on a route that is
  never even registered while provisioned.
- `test_null_arguments_rejected` — NULL `call`/`response` guard.

Also added to `tests/host/web_security_cookie.inc`
(`web_security_tests`, existing target):

- `test_cookie_build_session_header` — exact output string, round-trip
  through `web_cookie_extract_session()`, and every failure path (NULL
  token, NULL/zero-size output buffer, malformed token, buffer too small to
  hold the full cookie — verifies it fails closed rather than truncating).

## Commands run and results

```
$ ./scripts/run-tests.sh web
100% tests passed, 0 tests failed out of 18   (label: web)

$ ./scripts/run-tests.sh
100% tests passed, 0 tests failed out of 49   (full host suite; new total is
                                                49, up from 48 before this work
                                                — one new CTest target added)

$ . "$HOME/esp/esp-idf-v5.5.5/export.sh" && ./scripts/check-firmware.sh
(clang build + esp-clang clang-tidy, WarningsAsErrors: '*') → exit 0

$ ./scripts/check-format.sh
clang-format / cmake-format / cmake-lint / shfmt / shellcheck: all clean.
The frontend `prettier --check .` step fails with "prettier: not found" —
webapp/node_modules is not installed in this environment (pre-existing
condition; this track never touched webapp/, and `npm --prefix webapp ci`
was not run since it is outside this track's file scope).
```

## Checkboxes closed

**V2-051:**

- [x] Implement `GET /api/v1/auth/session` — `handle_session()` now compiled
  and tested end to end.
- [x] Match exact schemas, status codes, cookie behavior, and expiry fields —
  cookie string formatting now factored out and tested.

**Left open, with reason:**

- [ ] Test the complete unprovisioned/provisioned route-access matrix. The
  unprovisioned half is governed entirely by `web_server_lifecycle.c`'s
  `httpd_uri_t` route tables (`normal_routes`/`setup_routes`), which is
  outside this track's file scope and requires an `esp_http_server.h` host
  fake that does not exist anywhere in this codebase. I did not add or claim
  coverage for this; `check-setup-route-isolation.sh`'s structural check
  remains the only guarantee for the unprovisioned side.

**V2-057 (in-scope subset only — blob-route items are explicitly another
track's scope and untouched):**

- [x] Test setup POST returns `409` after provisioning, against the real
  `setup_route_response()` routing path (not the defense-in-depth function
  the existing test covers).
- [x] Test that secret-like sentinel values never appear in responses or
  logs, for session and login (send was already out of scope per this
  track's brief — its password material is secure-zeroed by construction).

**Left open, with reason (each of these bundles blob-route or
unprovisioned-httpd coverage this track does not own, so the checkbox as a
whole is not fully satisfied even though the in-scope part is now done —
the updated explanatory text under each item states exactly what changed):**

- [ ] Test every route with valid/missing/extra/.../method-error cases. The
  specific cited gap ("zero coverage for session-response composition,
  device-restart, and setup-conflict handling ... not compiled into any host
  test target") is resolved, but the full exhaustive-case matrix per route
  and a live end-to-end HTTP test remain out of reach (no `esp_http_server`
  host fake exists).
- [ ] Test setup-state GET returns only the approved two fields and 404 after
  provisioning. The provisioned-mode 404 half (`setup_route_response()`) is
  now tested; `setup_state_handler` (unprovisioned GET, `web_server_setup.c`,
  httpd-dependent) is out of this track's file scope and remains untested.
- [ ] Test exact response schemas and status codes. session/restart/
  setup-conflict are now covered; blob remains absent (another track's
  scope).

## Stretch goal (V2-057 examples.json parsing)

Not attempted — the required items above consumed the available time, and
the instructions were explicit not to let the stretch goal block them. No
claim of completion.

## Files changed

- `firmware/components/web_server/web_api_administration.c`
- `firmware/components/web_server/web_server_login.c`
- `firmware/components/web_server/web_cookie.c`
- `firmware/components/web_server/web_cookie.h`
- `firmware/components/web_server/web_server_internal.h`
- `firmware/components/web_server/include/web_diagnostics.h`
- `firmware/components/web_server/include/web_server.h`
- `tests/host/CMakeLists.txt`
- `tests/host/test_web_api_administration.c` (new)
- `tests/host/web_security_cookie.inc`
- `tests/host/web_security_main.inc`
- `docs/TODO_V2.md` (checkbox lines listed above only)

## Honesty statement

No unchecked task above is being claimed complete. All hardware-dependent
and httpd-dependent gaps this report lists as open remain open in
`docs/TODO_V2.md`. No physical device was used; this is host-test and
static-analysis evidence only.
