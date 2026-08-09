# V2-055 — Settings and device-action routes

**Phase:** 5 — Exact v2 HTTP API
**Task:** V2-055 — Settings and device-action routes
**Branch:** `v2-055-settings-device-action-routes` (not merged to `master`, not pushed)
**Status:** Implemented and verified with `./scripts/check-all.sh` (exit 0, full gate,
48/48 host tests including the three new suites this task adds). Not claimed as
physical-hardware-validated (no device access from this worktree; see Known gaps).

## Scope

Replaces `v2_configuration_route_pending()` (the deliberate `503` stub) for all
six routes `docs/implementation-v2/PHASE_5_EXACT_V2_HTTP_API_2026-08-08.md`
explicitly deferred:

- `GET /api/v1/settings`
- `PUT /api/v1/settings`
- `POST /api/v1/settings/change-password`
- `POST /api/v1/device/restart`
- `POST /api/v1/device/reset-settings`
- `POST /api/v1/device/factory-reset`

## Architecture

Follows the host-testable-core-plus-thin-httpd-adapter pattern Phase 5 already
established (`web_send.c`/`web_status_limits.c`/`web_auth_routes.c`), and the
setup-contract-plus-JSON-marshalling split Cutover B established
(`setup_contract_v2.c` + `web_server_setup_submit.c`):

```text
settings_contract_v2.c   (app_contracts_v2)  pure field validation / candidate building
web_settings.c           (web_server)        JSON parsing/serialization + ops orchestration
web_device_actions.c     (web_server)        JSON parsing/serialization + ops orchestration
web_api_administration.c (web_server)        thin dispatch: real ops -> the above modules
```

- **`settings_contract_v2.h`/`.c`** (new): `app_v2_settings_response_from_settings()`
  builds the sanitized GET/PUT-response view; `app_v2_settings_prepare_update()`
  applies a strict partial update (one `apply_*()` helper per field, composed by
  the entry point to stay under clang-tidy's cognitive-complexity threshold);
  `app_v2_password_change_validate()`/`app_v2_password_change_prepare_candidate()`
  do the equivalent for change-password. All of `app_v2_device_settings_validate()`'s
  existing rules (`device_settings_v2.c`) are the final backstop before any commit.
- **`web_settings.h`/`.c`** (new): JSON marshalling for all three settings routes
  plus an ops seam (`settings_read`/`settings_replace`/`password_verify`/
  `password_create`/`invalidate_all_sessions`) bridging to `device_settings`/`auth`.
- **`web_device_actions.h`/`.c`** (new): JSON marshalling for the three device
  actions plus an ops seam (`settings_read`/`password_verify`/`restart`/
  `reset_settings`/`factory_reset`) bridging to `device_settings`/`auth`/
  `device_controls`. `device_controls_restart()`/`reset_settings()`/
  `factory_reset()` already implement the correct
  settings-then-sessions-then-blobs-then-reboot sequencing
  (`device_controls_reset.h`, V2-044); this module only validates the request
  (confirmation phrase, and for factory-reset, the admin password) and decides
  whether to call it — SPEC_V2 13.12: "the destructive operation MUST NOT begin
  until the complete request, password, and confirmation phrase have been
  validated."
- **`web_api_administration.c`** (rewritten): wires the two modules above to real
  `device_settings`/`auth`/`device_controls` ops, maps every result variant to
  the correct HTTP status/field, and does two things no pure-logic module can:
  refreshes `server_configuration.password_record` (the in-RAM login cache) after
  a successful password change, and guards every body-required route against a
  present-but-empty body (see "204 + cookie-clear infrastructure" below for why
  this matters).

### 204 + cookie-clear infrastructure

`POST /api/v1/settings/change-password` must answer `204 No Content` and clear
the session cookie (SPEC_V2 13.9), but — unlike login/logout, which are
dedicated `httpd_uri_t` registrations that bypass the generic `/api/v1/*`
dispatch entirely — it cannot get a dedicated registration: `web_api_core.c`'s
`web_api_physical_confirmation_required()` already lists
`WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD` as one of the four routes requiring
physical confirmation when enabled, and that gate only runs inside the generic
pipeline (`web_request_policy_evaluate()`, invoked from
`apply_request_policy()` in `web_server_api.c`). Moving change-password off
that pipeline would silently drop physical-confirmation enforcement for it.

So the generic pipeline itself gained the missing capability instead:

- `web_api_response_no_content()` / `web_api_handler_no_content()`
  (`web_api_response.{h,c}`, `web_api_handler_common.{h,c}`): a body-less
  success response. `response->status` (not a non-NULL body) is now the
  readiness signal `web_server_api.c`'s `dispatch_api_call()` checks.
- `send_api_response()` (`web_server_api.c`) sends an empty body without
  `Content-Type` when the response carries none, instead of unconditionally
  requiring a JSON body.
- `should_clear_session_cookie()` (`web_server_api.c`) sets the same
  `MKSESSION=; ...; Max-Age=0` header `logout_handler` already uses, exactly
  when a request answers `WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD` with `204`.
- `WEB_HTTP_STATUS_NO_CONTENT` (`web_http_status.h`, `204U`).

### Empty-body defense (`call_has_body()`)

Every body-required route in the generic pipeline (`PUT /settings`,
`POST /settings/change-password`, `POST /device/reset-settings`,
`POST /device/factory-reset`) can, in principle, receive a request with a
valid `Content-Type: application/json` header but a zero-length body — the
generic policy layer (`enforce_body_content_type()` in `web_request_policy.c`)
validates the content-type whenever the route requires a body, but does not
itself reject a missing one. When that happens, `authorize_and_read_api_call()`
in `web_server_api.c` sets `call->body` to the read-only string literal `""`
(not a heap buffer). `web_settings_put_handle()`/`web_change_password_handle()`/
etc. wipe their `body` argument on every exit path (they may carry a Wi-Fi
passphrase or an admin password) — wiping a string literal is undefined
behavior and would crash on real hardware, where `.rodata` is flash-mapped and
not writable. `web_api_administration.c`'s `call_has_body()` guards every such
route and answers the field-appropriate `422` directly before ever touching
`call->body`, matching how `send_create_handler`/`login_handler` already guard
their own (heap-backed) bodies before parsing.

## Checklist evidence

### `GET /api/v1/settings` without returning passphrases

`app_v2_settings_response_from_settings()` only ever copies
`deviceName`/`requireSerialConfirmation`/`sendMode`/`snapshotRetentionTarget`/
`showMacroSourcePreviews`/`lastSelectedPackageId`/`apSsid`/`stationConfigured`/
`stationSsid` — matching `contracts/v2/api/examples.json`'s `"settings"` example
field-for-field. `password_iterations`/`password_salt`/`password_verifier`/
`ap_passphrase`/`station_passphrase` are never read by this function.
`test_get_success` (`tests/host/test_web_settings.c`) asserts the produced JSON
does not contain the string `"password"` or the test fixture's passphrase
literal.

### `PUT /api/v1/settings` with unambiguous preserve/remove semantics; reject ambiguous empty strings

SPEC_V2 13.9's exact rule, not an invented one:

- an omitted field (`has_*` false) preserves `current`'s value;
- `station: null` (the only field this applies to) removes the configured
  station network;
- **empty credential strings are rejected**, not treated as "preserve" or
  "remove" — enforced structurally: `accessPoint.ssid`/`accessPoint.passphrase`/
  `station.ssid`/`station.passphrase` all go through `valid_text()` with a
  minimum length of 1 (SSID) or 8 (`APP_V2_WIFI_PASSPHRASE_MIN_BYTES`,
  passphrase), so an empty string can never pass validation and there is no
  code path where it is silently interpreted as anything else;
- an update with every field omitted is rejected
  (`APP_V2_SETTINGS_UPDATE_EMPTY` / `WEB_SETTINGS_PUT_EMPTY`, 422).

`lastSelectedPackageId` is the one field where an explicit JSON `null` *value*
(distinct from the key being *absent*) is spec-legal and means "clear it" —
`test_put_last_selected_package_id_null_clears` exercises this distinctly from
omission.

Test coverage (`tests/host/test_settings_contract_v2.c`,
`tests/host/test_web_settings.c`): device-name/snapshot-retention/last-selected-
package-id/access-point/station success and failure paths, empty-object
rejection, unknown/duplicate/wrong-type field rejection, and the exact
preserve-vs-omit behavior (`test_prepare_update_device_name` asserts untouched
fields like `apSsid` are unchanged after a device-name-only update).

### `POST /api/v1/settings/change-password`

- Requires and verifies `currentPassword` against the stored credential
  (`auth_password_verify()`, via `settings_ops_password_verify()` reconstructing
  an `auth_password_record_t` from the settings record's
  `password_iterations`/`password_salt`/`password_verifier`) before deriving
  new material — an incorrect current password returns `403`
  (`test_change_password_incorrect_current_password` asserts
  `password_create`/`settings_replace` are never called).
- `newPassword` is validated 12-128 UTF-8 bytes
  (`app_v2_password_change_validate()`, SPEC_V2 11.3) *before* the (slow, ~450ms
  PBKDF2) current-password verification runs — `test_change_password_new_password_too_short_rejected`
  asserts `password_verify` is never called for a too-short new password.
- On success: replaces the settings record with the new credential material,
  invalidates every session (`auth_session_logout_all()`, SPEC_V2 13.9: "all
  sessions including the current one"), refreshes the in-RAM
  `server_configuration.password_record` login cache so a subsequent login —
  without a reboot — verifies against the new password (login_handler
  otherwise only reads this once, at boot), and returns `204 No Content` with
  the session cookie cleared.

### `POST /api/v1/device/restart`

`device_controls_restart()` (schedules the delayed reboot;
`device_controls_reset.h`'s "no state changes at all" case). Response:
`{"accepted":true,"connectionWillClose":true,"reprovisioningRequired":false}`,
byte-identical field set to `contracts/v2/api/examples.json`'s
`"restartAccepted"`. `web_server_api.c`'s pre-existing `restart_after_response()`
(built ahead of this task, anticipating it) still fires `esp_restart()`
immediately after the response is flushed for this route.

### `POST /api/v1/device/reset-settings`

Requires the exact confirmation phrase `"RESET SETTINGS"`
(`test_reset_settings_wrong_confirmation_rejected` — case-sensitive, exact
match, wrong phrase never reaches `ops->reset_settings`). On match, calls
`device_controls_reset_settings()` (applies SPEC_V2 11.4, invalidates every
session, schedules the reboot) and returns
`{"accepted":true,"connectionWillClose":true,"reprovisioningRequired":false,"repositoryBlobsPreserved":true}`.

### `POST /api/v1/device/factory-reset`

Requires the exact confirmation phrase `"FACTORY RESET"` *and* the current
admin password; neither check runs before both the confirmation phrase and
request body shape have been validated first (confirmation is checked before
the admin password, which is deliberately the more expensive check —
`test_factory_reset_wrong_confirmation_rejected_before_password_check` asserts
`settings_read`/`password_verify`/`factory_reset` are never called for a wrong
confirmation). A wrong admin password returns `403`
(`test_factory_reset_wrong_password_rejected` asserts `factory_reset` is never
called). On success, calls `device_controls_factory_reset()` (erases
configuration/credentials/provisioning state, invalidates sessions, deletes
every blob, schedules the reboot into the unprovisioned state — all already
correctly sequenced by V2-044) and returns
`{"accepted":true,"connectionWillClose":true,"reprovisioningRequired":true,"repositoryBlobsPreserved":false}`.

### Exact accepted/reconnect/reprovision/preservation fields

Every response shape above was checked field-by-field against
`contracts/v2/api/examples.json` (`settings`, `settingsUpdate`,
`settingsUpdated`, `passwordChangeRequest`, `restartAccepted`,
`resetSettingsRequest`, `resetSettingsAccepted`, `factoryResetRequest`,
`factoryResetAccepted`) and matches exactly.

**One field-shape decision beyond the literal spec text, made explicit and
justified:** SPEC_V2 13.9 states only that "changing access-point credentials
sets both flags to true. Other fields do not require a restart unless their
implementation cannot safely apply them live; any such case MUST be specified
and contract-tested rather than inferred by React." `wifi_ap` has no live
station-reconfigure path — `wifi_ap_connect_station()` is called exactly once,
at boot (confirmed by re-reading `app_core.c`'s `adapter_wifi_start()` and the
V2-044 report's own audit of this) — so a station-credential change (setting a
new network or removing one) cannot take effect without a reboot. Reporting
`restartRequired: false` for a station change would be a lie the client would
act on. `app_v2_settings_prepare_update()` therefore sets `restartRequired` for
any station change and leaves `reconnectRequired` false (a station change does
not disturb the browser's own access-point session). This is documented in
`settings_contract_v2.h`'s doc comment and exercised by
`test_prepare_update_station_set_requires_restart_only` /
`test_put_station_removal`.

## Spec conflict check

No genuine SPEC_V2 §13 conflict was found. One resolved ambiguity, not a
conflict: SPEC_V2 13.14's status table lists `403 credential or policy failure`
but `web_api_http_status_for_error()` (`web_api_core.c`) — the sole status
deriver for every route already on the generic
`web_api_handler_administration()` pipeline — has no `app_error_code_t` case
mapped to `403` (only `401` exists, for `AUTH_REQUIRED`/`AUTH_FAILED`). Rather
than either (a) reinterpreting the two 403-requiring cases as 401 (wrong — 401
is reserved for missing/invalid session, and the caller already has a valid
session at this point) or (b) adding a new case to that shared function purely
for two call sites, both `change-password`'s incorrect-current-password and
`factory-reset`'s incorrect-admin-password branches build their
`web_api_error_spec_t` directly with an explicit `.status = 403` field,
exactly the pattern `web_server_send.c`'s `set_error_response()` already uses
outside this pipeline. No spec text was reinterpreted or overridden.

## Commands and results

All commands run from the repository root
(`/home/phil/work/esp32-macro-keyboard/.claude/worktrees/agent-a5fa446d6b2e95edd`)
on branch `v2-055-settings-device-action-routes`.

### Host test suite (narrow loop, iterating)

```console
cmake -S tests/host -B tests/host/build
cmake --build tests/host/build --parallel -- -k
ctest --test-dir tests/host/build --output-on-failure
```

New targets added by this task, all passing: `settings_contract_v2_tests` (21
cases), `web_settings_tests` (22 cases), `web_device_actions_tests` (18 cases).
`test_web_api_response.c` gained two cases for the new
`web_api_response_no_content()` constructor.

### Firmware build + clang-tidy (esp-clang, `WarningsAsErrors: '*'`)

```console
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
export PATH="$HOME/.local/bin:$PATH"
./scripts/check-firmware.sh
```

Result: `CHECK_FIRMWARE_EXIT=0`.

Run as a real, synchronous, blocking background process, waited on to
completion and read directly from the captured log (not summarized from
memory), twice: the first run surfaced six real defect classes across the new
files, all fixed without suppression (see "Real defects found and fixed"
below); the second run, after those fixes, completed clean.

### Full quality gate

```console
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh" && nvm use
export PATH="$HOME/.local/bin:$PATH"
./scripts/check-all.sh
```

Result: `CHECK_ALL_EXIT=0`.

Run as a real, synchronous, blocking background process, captured to a log
file with its own exit code appended, waited on to completion, and the
captured log read directly afterward — not summarized from memory, not
assumed. It exceeded the tool's synchronous cap and was polled to completion
rather than restarted. The complete log ends with the success marker above,
and was additionally swept for failure signatures (error markers, FAIL, npm
ERR, Traceback). Every match found is either a legitimate `check-scripts.sh`
self-test description reporting its own `ok` result, or a `storage_mount`
test's own descriptive assertion name beginning `PASS:` — not a real failure.

Confirmed from the same log, verbatim:

- `100% tests passed, 0 tests failed out of 48` (host CTest — includes
  `app_core`, which a separately-run `ctest` against this worktree's own
  manually-configured `tests/host/build` directory could not build due to a
  stale CMake cache from earlier in this session; the authoritative
  `check-all.sh` run, with a fresh configure, built and passed it along with
  every other target).
- `Test Files  33 passed (33)` / `Tests  298 passed (298)` (webapp vitest).
- `Real Chrome Phase 17.10 workflows passed.` (webapp Playwright/browser
  suite).
- `stack usage policy passed: 585 first-party frames analyzed, largest 1536
  bytes, 0 allowlisted`.
- `V2 device settings persistence policy checks passed`,
  `V2 credential logging policy passed`,
  `V2 setup route isolation policy passed`,
  `V2 authentication policy checks passed`,
  `phase 2 architecture: no firmware-owned package or macro repository`,
  `frontend persisted-state policy passed`.
- `webfs image written` / `release budgets within threshold`.
- Every `check-scripts.sh` regression suite (partitions, production-config,
  credential-logging, setup-route-isolation, frontend-persisted-state,
  release-budgets, build-webfs-image, generate-flash-manifest, stack-usage,
  npm-audit, secret-sentinel) reported `N tests, 0 failed`.

### Real defects found and fixed (not suppressed)

`check-firmware.sh`'s first run against the new pure-logic files found:

1. **Missing direct includes** (`misc-include-cleaner`) in
   `settings_contract_v2.c`, `web_settings.c`, `web_device_actions.c` — types
   like `app_v2_device_settings_t`/`app_v2_string_view_t`/`uint8_t` were only
   reachable transitively. Fixed by including `api_contracts_v2.h`/
   `device_settings_v2.h`/`setup_contract_v2.h`/`<stdint.h>` directly wherever
   used.
2. **Cognitive complexity over threshold**
   (`readability-function-cognitive-complexity`, limit 25):
   `app_v2_settings_prepare_update()` (47) split into one `apply_*()` helper
   per settings field; `populate_settings_update_request()` (37) split into one
   `populate_*()` helper per JSON field; `web_settings_put_handle()` (26) had
   its response-JSON construction extracted into
   `build_settings_put_response_json()`.
3. **Unsafe out-of-range enum cast**
   (`clang-analyzer-optin.core.EnumCastOutOfRange`): the original `sendMode`
   parser encoded "structurally a string but not `quick`/`preview`" by casting
   `-1` into `app_v2_send_mode_t`. Replaced with an explicit
   `*out_invalid_send_mode` flag threaded through `populate_send_mode()`, so no
   out-of-range enum value is ever constructed.
4. **Adjacent easily-swapped `bool *` parameters**
   (`bugprone-easily-swappable-parameters`):
   `apply_access_point()`'s `out_restart_required`/`out_reconnect_required`
   bundled into a new `settings_update_flags_t` struct passed by one pointer.
5. **Magic number** (`readability-magic-numbers`): `255.0` in the
   `snapshotRetentionTarget` wire-shape bound named `SETTINGS_RETENTION_RAW_MAX`.
6. **Suspicious call argument** (`readability-suspicious-call-argument`): a
   local cJSON object named `settings_object` was flagged as possibly swapped
   with `cJSON_AddItemToObject`'s first parameter (named `object`) purely
   because the old name contained the substring "object" — renamed to
   `settings_fields`.

`check-format.sh`'s first run found real `clang-format`/`cmake-format`
violations in every new file (none had been run through the formatter before
that pass) — fixed by running `clang-format -i`/`cmake-format -i` on exactly
the touched files, then re-verified clean.

## Files changed

```text
firmware/components/app_contracts_v2/CMakeLists.txt              (new source)
firmware/components/app_contracts_v2/include/settings_contract_v2.h  (new)
firmware/components/app_contracts_v2/settings_contract_v2.c          (new)
firmware/components/web_server/CMakeLists.txt                    (new sources)
firmware/components/web_server/web_settings.h                    (new)
firmware/components/web_server/web_settings.c                    (new)
firmware/components/web_server/web_device_actions.h              (new)
firmware/components/web_server/web_device_actions.c               (new)
firmware/components/web_server/web_api_administration.c          (real dispatch)
firmware/components/web_server/web_api_response.{h,c}            (no-content constructor)
firmware/components/web_server/web_api_handler_common.{h,c}      (no-content wrapper)
firmware/components/web_server/web_http_status.h                 (WEB_HTTP_STATUS_NO_CONTENT)
firmware/components/web_server/web_server_api.c                  (no-content + cookie-clear plumbing)
scripts/generate-native-coverage.sh                               (new pure-policy filters)
docs/TODO_V2.md                                                   (V2-055 checkboxes)
tests/host/CMakeLists.txt                                         (3 new test targets)
tests/host/test_settings_contract_v2.c                            (new)
tests/host/test_web_settings.c                                    (new)
tests/host/test_web_device_actions.c                              (new)
tests/host/test_web_api_response.c                                (no-content coverage)
```

## Known gaps and deliberate scope boundaries

1. **`web_api_administration.c` is not host-tested.** No `esp_http_server`
   fake exists anywhere in this codebase (a pre-existing, codebase-wide gap
   documented in the Phase 5 report, not created or widened by this task);
   this file was already untested before this task
   (`handle_session()`/`setup_route_response()`). Every genuinely new piece of
   logic this task added lives in the host-tested pure modules
   (`settings_contract_v2.c`, `web_settings.c`, `web_device_actions.c`)
   instead; `web_api_administration.c`'s job is thin real-ops wiring, verified
   by `./scripts/check-firmware.sh`'s clean compile + clang-tidy pass rather
   than a unit test.
2. **No physical-hardware evidence.** Everything above is host-test and
   `check-firmware.sh`/`check-all.sh` (compilation + clang-tidy + full gate)
   evidence only, consistent with `CLAUDE.md`'s rule against claiming hardware
   validation from compilation or host fakes alone. In particular, the exact
   real-device timing of the `esp_restart()`/delayed-reboot behavior for
   restart/reset-settings/factory-reset, and the actual browser experience of
   the connection dropping mid-response, are unverified here.
3. **Wi-Fi live-reconnect is out of scope**, as it was for V2-044: this task
   only decides the *reported* `restartRequired`/`reconnectRequired` flags
   correctly for the wifi_ap capabilities that already exist; building a live
   AP-reconfigure or repeated station-reconnect-attempt path remains future
   work, not something this task's route handlers can or should paper over.
4. **`server_configuration.password_record` cache refresh is best-effort.** If
   `device_settings_read()` fails immediately after a successful
   `device_settings_replace()` (extremely unlikely — same underlying NVS
   record, no intervening write), the response still reports the already-
   successful change (write done, sessions invalidated) but the in-RAM login
   cache would lag until reboot. Not treated as a change-password failure,
   since the change genuinely succeeded.
