# Phase 5 — Exact v2 HTTP API

**Task:** V2-050 through V2-054, V2-056, V2-057 (V2-055 explicitly excluded — see below)
**Branch:** `phase5-exact-v2-http-api` (as instructed; not merged to `master`, not pushed)
**Status:** Implemented and host-tested. Not claimed as physical-hardware-validated
(no device access from this worktree; see Known gaps).

## Scope actually delivered

- **V2-050 — Common HTTP policy.** Fixed a foundational, pre-existing defect:
  every already-wired v2 JSON response (`web_api_response_success`/`_error`,
  used by session, diagnostics, the device-restart stub, and blob) was wrapped
  in a v1-shaped `{"ok":true,"data":...}` / `{"ok":false,"error":...}` envelope
  that does not match SPEC_V2 §13.2's exact flat schema. Fixed both the
  cJSON-based path (`web_api_response.c`) and the second, older
  `json_writer_t`-based path (`web_adapter_build_error_json`, used by setup/
  login/logout). Extended `web_api_route_t` with `STATUS`/`LIMITS`/
  `AUTH_LOGIN`/`AUTH_LOGOUT`/`SEND` so the existing generic wildcard dispatch
  correctly answers `405` for any wrong-method request to those five routes.
- **V2-051 — Setup and authentication routes.** Setup GET/POST were already
  implemented (V2-040 Cutover A/B); verified unchanged. `GET /api/v1/auth/session`
  now reports real `idleExpiresInSeconds`/`absoluteExpiresInSeconds` instead of
  a hardcoded `{"authenticated":true}` stub (new `auth_session_remaining()`
  accessor). `login_handler` was a genuine bug — it read a v1 field name
  (`"password"`) that never matched the v2 contract (`"adminPassword"`),
  enforced no `Content-Type`, and returned the v1 envelope with no expiry
  fields; rewritten to the exact contract. `logout_handler` returned `200`
  with a body instead of the required `204 No Content`; fixed.
- **V2-052 — Status and limits routes.** `status_handler` was a fully v1-shaped
  stub (hardcoded version strings, a completely different field set, no
  session check at all); rewritten to the exact SPEC_V2 §13.6 shape, with
  session auth added to both `status_handler` and `limits_handler`.
  `limits_handler`'s existing 18-field body was already byte-identical to
  `contracts/v2/limits.json` and is unchanged in content.
- **V2-053 — Blob routes.** Already fully implemented and correctly wired
  (Phase 3 storage + a complete HTTP layer over it) before this task started;
  verified against SPEC_V2 §13.8/§10 and left unmodified except that its
  responses are now correctly unwrapped by the V2-050 envelope fix.
- **V2-054 — Send routes.** New: `POST`/`GET`/`DELETE /api/v1/send` did not
  exist anywhere before this task (only the unused `app_v2_send_*` contract
  types existed). Implemented end-to-end: exact-schema request validation,
  full-source compile-before-accept via `macro_compile_v2`, exact parser
  location on `422`, `409` on a second send while one is active, idempotent
  `DELETE`, and `404` when no send has existed since boot.
- **V2-056 — Diagnostics route.** Diagnostics was already wired through
  `web_api_handle_administration`, but its schema diverged substantially from
  `contracts/v2/api/examples.json`'s `"diagnostics"` example (extra
  `schemaVersion`/`stack` fields, flat instead of nested `memory`, no
  `usb`/`wifi` objects, an `executionState` string instead of `send{present,state}`).
  Rewritten to match the frozen contract exactly.
- **V2-057 — Contract and security tests.** Host-level route-matrix and
  pure-logic-module test coverage added throughout (see Test evidence). Full
  per-route HTTP-socket-level contract testing is **not** done — see Known
  gaps.

## Architecture decisions

- **Envelope and route-enum fixes apply everywhere**, not just to newly-touched
  routes, because `web_api_response.c`/`web_api_core.c` are shared
  infrastructure. This is the highest-leverage part of this slice: it silently
  fixed the wire shape of every already-"complete" v2 JSON response from
  earlier phases (session, diagnostics, restart-stub, blob) without touching
  those files.
- **Status/limits/login/logout/send are NOT routed through the generic
  `web_api_dispatch`/`web_api_handle_administration` pipeline.** They keep
  dedicated `httpd_uri_t` registrations in `web_server_lifecycle.c`'s
  `normal_routes[]`, exactly like blob already does — for the same reason
  blob does: JSON-only routes could go through the generic pipeline, but
  blob's binary-body streaming (chunked writes directly to a file, never
  materializing the whole body in memory) fundamentally cannot, and
  send/login/status/limits were built to match that existing, proven pattern
  rather than invent two different dispatch styles. `web_api_core.c`'s route
  enum/method/body/session tables are still authoritative metadata for these
  five routes (consumed by the wildcard-fallback path for wrong-method
  requests, and by tests), they just aren't the *primary* dispatch route.
- **Host-testable-core-plus-thin-httpd-adapter, consistently applied.** Every
  new piece of business logic (`web_status_limits.c`, `web_auth_routes.c`,
  `web_send.c`) has zero `esp_http_server` dependency and is fully
  host-tested; the corresponding httpd adapters
  (`web_server_status_limits.c`, `web_server_login.c`, `web_server_logout.c`,
  `web_server_send.c`) are thin glue, following exactly the pattern
  `web_server_setup_submit.c`/`web_server_setup.c` established in Cutover B.
- **Send's opaque `id`** is a freshly generated `app_uuid_t`, reused as
  `macro_executor`'s `set_id`/`macro_id` (with `macro_revision = 1`) purely as
  an internal bridge to that component's still-v1-shaped identity fields.
  Nothing package/macro-shaped is ever exposed over the API — the response
  only ever contains `id`, matching the opaque-blob-storage model the rest of
  v2 uses.

## Known gaps and deliberate scope boundaries

1. **V2-055 is explicitly not implemented**, per the assigning task: `GET`/
   `PUT /api/v1/settings`, `POST /api/v1/settings/change-password`, and the
   three `POST /api/v1/device/*` routes remain the pre-existing `503`
   stub (`v2_configuration_route_pending()` in `web_api_administration.c`).
   V2-043 (device-UI-preference semantics) and V2-044 (Wi-Fi/reset semantics)
   are concurrent streams that own the underlying behavior this depends on;
   implementing it now would mean stubbing or duplicating logic they own.
   **This is a real, visible gap in the exposed API surface — not silently
   left implicit.**
2. **Station Wi-Fi connection state has no live tracking anywhere in the
   firmware.** Station join is a one-shot blocking attempt at boot
   (`app_core.c` → `wifi_ap_connect_station()`), and its outcome is not
   retained anywhere. `status`, `send`'s status, and `diagnostics`'s
   `station.state`/`wifi.stationState` therefore report only
   `"disabled"`/`"unknown"` based on whether a station network is configured,
   not real connection status. Building live tracking is Wi-Fi/reset
   semantics (V2-044) scope, explicitly off-limits to this task
   (`firmware/components/wifi_ap` is on the do-not-touch list).
3. **Send has no physical-confirmation-gated `awaiting_confirmation` state.**
   `macro_executor`'s `execution_state_t` has no such state (only
   `IDLE`/`RUNNING`/terminal states), and adding one is Phase 6 executor-
   internals work per the assigning task's explicit instruction not to take
   that on. `POST /api/v1/send` does not currently gate on physical
   confirmation at all; this is a real functional gap for devices with
   `requireSerialConfirmation` enabled, flagged here rather than faked with a
   response-only field.
4. **`macro_executor_engine.c`'s `validate_request()` rejects
   `key_press_ms == 0`**, even though `macro_compile_v2`'s documented range is
   the complete 0–10000 ms. A `keyPressMs: 0` request that compiles
   successfully will fail at `ops->submit()` with `APP_ERROR_INVALID_ARGUMENT`,
   which `web_send.c` maps to a `500` rather than silently reimplementing the
   executor's (v1-shaped) bound in the HTTP layer. Reconciling this is
   Phase 6 territory.
5. **No `esp_http_server` fake exists anywhere in this codebase**, confirmed
   against every httpd-dependent module, not just the ones this task added
   (`web_api_administration.c`, `web_server_blob.c`, `web_server_setup.c`, and
   now `web_server_login.c`/`web_server_logout.c`/`web_server_status_limits.c`/
   `web_server_send.c` are all equally untested at the actual HTTP-socket
   level; only their pure-logic cores are). V2-057's request for
   "unauthorized/expired-session/malformed-path/method-error" testing per
   route is satisfied at the policy-table level
   (`test_web_api_core.c`/`test_web_request_policy.c`) and at the pure-logic
   level for every new module, but not with a live socket end-to-end. Building
   that harness is a pre-existing, codebase-wide gap this task did not create
   and did not have the scope to close.
6. **`app_core_tests` (host target) does not build**, independent of this
   work: `firmware/components/app_core/app_core_ops.h` already includes
   `device_settings_v2.h` and `provisioning_bootstrap.h` on `master` with no
   corresponding include-directory entries in `tests/host/CMakeLists.txt`'s
   `app_core_tests` target. Confirmed via `git show master:...` — this predates
   every change in this branch and was not touched here. All evidence below
   reports `38/39` host tests for this reason; the `39` figure and every other
   target are unaffected.
7. **`npm run format:check` (webapp) could not run** — `prettier` is not on
   `PATH` in this worktree/session (Node toolchain not set up here; no webapp
   files were touched by this task). All C/CMake/shell formatting
   (`clang-format`, `cmake-format`, `shfmt`) is verified clean.
8. **No physical-hardware evidence.** Everything below is host-test and
   `check-firmware.sh` (compilation + clang-tidy) evidence only, consistent
   with `CLAUDE.md`'s rule against claiming hardware validation from
   compilation or host fakes alone.
9. **Blob and send lack HTTP-handler-level host tests** (see gap 5) — their
   correctness is evidenced by code review against the spec, the shared
   envelope/route-policy tests, and (for send) exhaustive pure-logic-core
   tests; blob's handlers were pre-existing and out of this task's primary
   scope to restructure for testability.
10. **Diagnostics does not emit a `stack` field.** `docs/TODO_V2.md`'s V2-056
    checklist item literally says "...memory/stack/storage/USB/Wi-Fi/send/..."
    but neither `docs/SPEC_V2.md` §13.13 nor the checked-in
    `contracts/v2/api/examples.json` `"diagnostics"` example has a `stack`
    field. Per `CLAUDE.md`'s rule that the committed contract corpus is
    authoritative and may only gain fields through an explicit specification
    update, this was treated as a TODO-wording/frozen-contract mismatch and
    resolved in the contract's favor — not applied to the contract itself.
    Flagged here rather than silently decided.

## Files changed

```text
firmware/components/auth/auth.c                              (auth_session_remaining wrapper)
firmware/components/auth/auth_core.h                          (auth_core_session_remaining decl)
firmware/components/auth/auth_core_session.c                  (auth_core_session_remaining impl)
firmware/components/auth/include/auth.h                       (auth_session_remaining decl)
firmware/components/web_server/CMakeLists.txt                 (new sources)
firmware/components/web_server/include/web_diagnostics.h      (exact v2 diagnostics schema)
firmware/components/web_server/web_api_administration.c       (real session-TTL response)
firmware/components/web_server/web_api_core.{h,c}             (5 new routes)
firmware/components/web_server/web_api_handler_common.{h,c}   (field/parser-error helpers)
firmware/components/web_server/web_api_response.{h,c}         (flat v2 envelope)
firmware/components/web_server/web_auth_routes.{c,h}          (new: login/session pure logic)
firmware/components/web_server/web_send.{c,h}                 (new: send pure logic)
firmware/components/web_server/web_status_limits.{c,h}        (new: status pure logic)
firmware/components/web_server/web_server_adapter.h           (removed dead v1-shaped status builder decl)
firmware/components/web_server/web_server_adapter_json.c      (fixed error-JSON envelope; new diagnostics builder)
firmware/components/web_server/web_server_diagnostics.c       (exact v2 diagnostics collection)
firmware/components/web_server/web_server_internal.h          (new handler decls)
firmware/components/web_server/web_server_lifecycle.c         (registers /api/v1/send)
firmware/components/web_server/web_server_login.c             (real login wiring)
firmware/components/web_server/web_server_logout.c            (204 No Content)
firmware/components/web_server/web_server_send.c              (new: send httpd adapter)
firmware/components/web_server/web_server_status_limits.c     (exact v2 status; session auth)
tests/host/CMakeLists.txt                                     (3 new test targets, 1 pre-existing include-dir fix)
tests/host/auth_additional_session_tests.inc                  (auth_session_remaining coverage)
tests/host/test_auth.c
tests/host/test_web_api_core.c                                (route-matrix coverage for 5 new routes)
tests/host/test_web_api_response.c                            (flat envelope + parser-location coverage)
tests/host/test_web_auth_routes.c                              (new)
tests/host/test_web_send.c                                     (new)
tests/host/test_web_status_limits.c                            (new)
tests/host/test_web_server_adapter_diagnostics_json.inc       (exact v2 schema coverage)
tests/host/test_web_server_adapter_json_static.inc            (flat error envelope)
tests/host/test_web_server_adapter_main.inc
```

## Test evidence

All commands run from the repository root on this worktree
(`/home/phil/work/esp32-macro-keyboard/.claude/worktrees/agent-a96cad61d78c31f84`),
toolchain sourced per `CLAUDE.md` (`$HOME/.local/bin` on `PATH`, ESP-IDF
`v5.5.5` environment, Node not required — no webapp files touched).

### Host test suite

```text
$ cmake -S tests/host -B tests/host/build
$ cmake --build tests/host/build --parallel -- -k
$ ctest --test-dir tests/host/build --output-on-failure
...
97% tests passed, 1 tests failed out of 39
The following tests FAILED:
	 14 - app_core (Not Run)                                startup
```

The one failure is `app_core_tests`, a pre-existing, unrelated build break on
`master` (see Known gaps #6). Every other target, including all new ones
(`web_status_limits`, `web_auth_routes`, `web_send`) and every touched
existing one (`auth`, `web_api_core`, `web_api_response`, `web_api_dispatch`,
`web_request_policy`, `web_server_adapter`, `web_server_setup_submit`),
passes.

### Firmware build + clang-tidy (esp-clang, `WarningsAsErrors: '*'`)

```text
$ ./scripts/check-firmware.sh
...
(exit 0)
```

Builds both `firmware/` and `firmware/test_app/` and runs `run-clang-tidy`
with `misc-include-cleaner` and all other first-party rules over every
changed translation unit with zero suppressions. Real defects found and
fixed during this work, not suppressed:

- `web_server_status_limits.c`: missing direct `<stdint.h>` include
  (`misc-include-cleaner`).
- `web_server_diagnostics.c`: `macro_limits.h` included but no longer used
  directly after removing the `schemaVersion` field (`misc-include-cleaner`).
- `web_send.c`/`web_server_send.c`: several symbols from `macro_parser.h`/
  `macro_executor.h` reachable only transitively via `web_send.h`, not
  included directly (`misc-include-cleaner`).
- `web_send.c`: a 3-branch `switch` in `web_send_cancel_handle()` where all
  three cases returned the identical value (`bugprone-branch-clone`) —
  merged into one case list with the reasoning in a comment.
- `auth_core_session.c`: `find_session_remaining()`'s two adjacent
  `uint32_t *` output parameters (`bugprone-easily-swappable-parameters`) —
  bundled into a small local struct.
- `web_server_login.c`: `send_login_error_and_forget_session()`'s two
  adjacent `const char *` parameters (same rule) — changed the session
  parameter's type to `const auth_session_view_t *` so the two can no
  longer be confused for each other.

### Format

```text
$ ./scripts/check-format.sh
...
Summary
=======
files scanned: 21
found lint:
```

(empty — no findings) for `clang-format`/`cmake-format`/`shfmt` across every
first-party C/CMake/shell file. The script's final step,
`npm --prefix webapp run format:check` (prettier), fails with
`prettier: not found` — this worktree's Node/webapp toolchain was never set
up in this session and no webapp file was touched by this task (see Known
gaps #7).

### Stack usage

```text
$ bash scripts/check-stack-usage.sh
stack usage policy passed: 531 first-party frames analyzed, largest 1536 bytes, 0 allowlisted
```

### Architecture/contract gates

```text
$ bash scripts/check-setup-route-isolation.sh
V2 setup route isolation policy passed
$ python3 scripts/check-v2-api-routes.py
v2 API route manifest and C mirror match (21 routes)
$ python3 scripts/check-v2-phase2-architecture.py
phase 2 architecture: no firmware-owned package or macro repository
$ bash scripts/check-credential-logging.sh
V2 credential logging policy passed
```

### Not run

`./scripts/check-all.sh` and `./scripts/check-webapp.sh` were not run in
full: they require the webapp Node toolchain (`prettier`, ESLint, Vitest,
Playwright), which this session's environment does not have set up, and no
webapp file was touched by this task. The relevant subset of `check-all.sh`
for this task's scope — host tests, firmware build/clang-tidy, format,
stack usage, the setup-route-isolation/API-route-manifest/phase-2-
architecture/credential-logging gates — was run individually above.

## Explicit V2-055 follow-up note

**V2-055 (settings and device-action routes) was intentionally not
implemented**, per the assigning task. `GET`/`PUT /api/v1/settings`,
`POST /api/v1/settings/change-password`, `POST /api/v1/device/restart`,
`POST /api/v1/device/reset-settings`, and `POST /api/v1/device/factory-reset`
all still return the pre-existing `503 Service Unavailable` stub
(`v2_configuration_route_pending()` in `web_api_administration.c`). V2-043
(device-UI-preference semantics) and V2-044 (Wi-Fi/reset semantics) are
concurrent streams that own the underlying behavior these routes depend on;
the route policy metadata for all five (methods, session requirement, body
requirement, physical-confirmation requirement) is already correctly
registered in `web_api_core.c` from prior work, so implementing V2-055 once
those streams land should be a contained addition to
`web_api_handle_administration()`'s existing four stub cases plus new HTTP-
adapter wiring for `device/restart`'s actual `esp_restart()` call (still
missing everywhere in the codebase, noted independently in the V2-040
Cutover B report) — not a redesign.
