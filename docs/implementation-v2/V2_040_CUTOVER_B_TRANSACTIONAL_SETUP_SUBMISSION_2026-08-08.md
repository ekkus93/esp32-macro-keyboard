# V2-040 Cutover B — transactional `POST /api/v1/setup`

**Phase:** 4 — Authentication, provisioning, and device settings
**Task:** V2-040 — First-run provisioning
**Branch:** `v2-040-cutover-b` (as instructed; not merged to `master`, not pushed)
**Status:** Live transactional setup submission implemented and host-tested.
V2-040 is **not** being marked complete in `docs/TODO_V2.md` — the PBKDF2
iteration count remains provisional pending the V2-041 ESP32-S3R8 hardware
benchmark (see [PBKDF2 constant](#the-pbkdf2-iteration-count-is-provisional)
below), and no physical-hardware evidence exists for this change.

## Scope

This slice replaces the deliberate `503 Service Unavailable` stub in
`firmware/components/web_server/web_server_setup.c` (documented in
`docs/implementation-v2/V2_040_CUTOVER_A_V2_BOOT_AUTHORITY_2026-08-07.md`)
with the real transactional `POST /api/v1/setup` submission path, using the
prepared C contract from
`docs/implementation-v2/V2_040_HOST_PREPARATION_2026-08-07.md`
(`firmware/components/app_contracts_v2/setup_contract_v2.{h,c}`) as the
business-logic boundary. No logic from that contract is duplicated in the
HTTP handler.

Read first, in order, per the assigning task: `CLAUDE.md`, `docs/SPEC_V2.md`,
`docs/UI_UX_SPEC_V2.md`, `docs/TODO_V2.md` §Phase 4, the two documents named
above, and `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` §6.

## Files changed

```text
firmware/components/auth/include/auth.h                    (provisional-constant comment + alias)
firmware/components/web_server/CMakeLists.txt               (new source, device_settings dependency)
firmware/components/web_server/web_api_administration.c     (WEB_API_ROUTE_SETUP dispatch)
firmware/components/web_server/web_api_core.c                (WEB_API_ROUTE_SETUP policy)
firmware/components/web_server/web_api_core.h                (WEB_API_ROUTE_SETUP enum value)
firmware/components/web_server/web_server_common.c           (setup_session storage)
firmware/components/web_server/web_server_internal.h         (setup_session extern)
firmware/components/web_server/web_server_lifecycle.c        (setup_session lifecycle)
firmware/components/web_server/web_server_setup.c             (real POST handler; httpd adapter)
firmware/components/web_server/web_server_setup_submit.c/.h  (new: host-testable core logic)
scripts/check-v2-phase2-architecture.py                       (narrow false-positive regex fix)
tests/host/CMakeLists.txt                                     (new test target)
tests/host/test_web_server_setup_submit.c                     (new: 25 test cases)
tests/host/test_web_api_core.c                                (WEB_API_ROUTE_SETUP policy coverage)
tests/host/test_web_request_policy.c                          (no-session proof for setup route)
```

`firmware/components/web_server/web_setup_core.{c,h}` and
`web_setup_json.{c,h}` (V1-shaped, uncompiled — not in `web_server`'s
`CMakeLists.txt` `SRCS`, only reachable from two orphaned host tests) were
**not** touched or removed. They are dead V1 code, not part of the live
build; deleting them was judged out of scope for this slice and is left as a
recommendation for the integrator (see [Recommendations](#recommendations-for-the-integrator)).

## Architecture

### Host-testable core: `web_server_setup_submit.c`/`.h`

`esp_http_server.h`-dependent code cannot be compiled by the host test suite
(no fake exists for it; confirmed by grep — none of `web_server_setup.c`'s
siblings that include it are host-tested either). Following the same
separation the codebase already uses elsewhere (`device_settings_core.c` vs.
`device_settings.c`; `auth_core_*.c` vs. `auth.c`), the actual setup-submission
business logic lives in a new pure module with zero ESP-IDF dependency:

`web_setup_submit_handle(body, body_capacity, ops, session, out_response)`:

1. bounds and NUL-escape-sanitizes the raw body (item 2);
2. strictly parses it as the exact six-field V2 schema — `setupCode`,
   `deviceName`, `apSsid`, `apPassphrase`, `adminPassword`,
   `requireSerialConfirmation` — rejecting unknown fields, duplicate fields,
   missing fields, and wrong JSON types (items 1, 3);
3. reads current settings via `ops->settings_read` (item 7);
4. derives password material via `ops->password_create`, which in production
   bridges to `auth_password_create()` (item 6);
5. calls `app_v2_setup_prepare_candidate()` from the prepared contract exactly
   once — this is where the setup code is checked (item 4) and device
   name/AP SSID/AP passphrase/admin password are strictly validated (item 5);
   nothing here re-implements that validation;
6. on success, commits via `ops->settings_replace` (item 9) and **only then**
   calls `app_v2_setup_session_consume()` (item 10); on commit failure the
   session is left untouched and the setup code stays usable for a retry
   (item 15, directly tested — see below);
7. on overall success, returns the struct from
   `app_v2_setup_accepted_response_init()` unmodified (item 11).

Every secret-bearing buffer (raw JSON body, parsed cJSON tree strings,
derived password material, current/candidate settings records) is wiped via
`operations->secure_zero` on every exit path, mirroring the discipline the
prepared contract itself uses.

### httpd adapter: `web_server_setup.c`

`setup_submit_handler()` is now a thin adapter: check `WEB_SERVER_MODE_SETUP`
(unchanged — `409` when already provisioned, exactly as it was in the
Cutover A stub), heap-allocate an `APP_V2_JSON_BODY_MAX_BYTES`-sized buffer
(see [stack usage](#stack-usage-heap-allocated-request-body) below), read it
bounded, call `web_setup_submit_handle()`, and map the outcome to an HTTP
status/message or build the exact `202 Accepted` JSON body
(`accepted`, `restartRequired`, `connectionWillClose`,
`reprovisioningRequired`) from the returned struct. It never returns the
administrator password, AP passphrase, verifier, salt, setup code, or session
secret (item 12) — those never leave `web_server_setup_submit.c`.

Password derivation is bridged through `firmware/components/auth`
(`auth_password_create()`), not reimplemented, per item 6 and handoff §6.5.

### One-time setup-code persistence: `setup_session`

Cutover A copied the boot-generated plaintext setup code into
`web_server_config_t.setup_code` and then **zeroed it immediately** after
`web_server_start()` returned in setup mode — there was nothing yet that
needed to keep it. Cutover B needs the code to survive for the rest of setup
mode so a POST can be checked against it, so `web_server_start()` now also
initializes a persistent `app_v2_setup_session_t setup_session` (module-level
storage in `web_server_common.c`, declared in `web_server_internal.h`) from
that plaintext view *before* wiping the transport copy. `setup_submit_handler`
is the only reader; `setup_session` is zeroed on `web_server_stop()` and
whenever `web_server_start()` runs in normal mode.

### Reaching `/api/v1/setup` once provisioned: `WEB_API_ROUTE_SETUP`

The obvious-looking fix — also registering `/api/v1/setup` in
`normal_routes[]` in `web_server_lifecycle.c` pointing at
`setup_state_handler`/`setup_submit_handler` — is **explicitly forbidden** by
an existing CI gate, `scripts/check-setup-route-isolation.sh` (run inside
`scripts/check-v2-contracts.sh` and `scripts/check-all.sh`), which fails
closed if `/api/v1/setup` appears in `normal_routes`. That script's intent is
that once provisioned, `/api/v1/setup` is answered by the generic API
dispatcher, not by a second direct httpd registration.

So instead: `/api/v1/setup` is now a recognized route in
`web_api_core.c`'s `web_api_parse_path()` (`WEB_API_ROUTE_SETUP`), reachable
through the existing `/api/v1/*` wildcard → `api_handler` → dispatch chain
once provisioned:

- `web_api_route_allows_method`: GET and POST only;
- `web_api_route_requires_body`: true for POST (so a real setup-submission
  body is accepted rather than rejected with `422` before it can reach the
  `409`; the body is never parsed);
- `web_api_route_requires_session`: **false** — SPEC 13.4 requires the same
  404/409 behavior whether or not the caller ever logged in, so this route is
  the one exception to "every non-`UNKNOWN` route requires a session";
- `web_api_route_requires_physical_confirmation`: false (unchanged default);
- `web_api_handle_administration()`: GET → `404` (`route not found`, matching
  `setup_state_handler`'s unprovisioned-404 message exactly), POST → `409`
  (`device is already provisioned`, matching `setup_submit_handler`'s
  already-provisioned message exactly).

`setup_routes[]` (the unprovisioned-only table) is unchanged — it still has
exactly `GET /api/v1/setup`, `POST /api/v1/setup`, and the static wildcard
(item 18; `scripts/check-setup-route-isolation.sh` verifies this exactly).

### Stack usage: heap-allocated request body

The first attempt put the request body in a `char body[8193]` local in
`setup_submit_handler`. `scripts/check-stack-usage.sh` — a real gate in
`check-all.sh` that fails any first-party stack frame over 4096 bytes not
explicitly recorded in `scripts/stack-usage-allowlist.txt` (currently empty;
"a ratchet, not an amnesty") — correctly failed on the resulting 8272-byte
frame. The generic API dispatch already solves this the same way
(`web_server_api.c`'s `read_call_body()` calls `calloc()` for exactly this
size class); `setup_submit_handler` now does the same instead of adding an
allowlist entry. Confirmed: `stack usage policy passed: 518 first-party
frames analyzed, largest 1536 bytes, 0 allowlisted`.

### `check-v2-phase2-architecture.py` regex correction

`scripts/check-all.sh` also runs `python3
scripts/check-v2-phase2-architecture.py`, a fail-closed scanner for retired
V1 firmware-owned-repository patterns. Its forbidden-source regex included
`WEB_API_ROUTE_(SET(?!TING)S?|...)` — written to block a retired V1 route
like `WEB_API_ROUTE_SETS`/`WEB_API_ROUTE_SET_ACTIVE` while explicitly
excluding `WEB_API_ROUTE_SETTINGS` via the `(?!TING)` lookahead. It had no
exception for `SETUP`, so it flagged the new, SPEC_V2-mandated
`WEB_API_ROUTE_SETUP` as if it were retired V1 architecture. This is a false
positive: `/api/v1/setup` is required by `docs/SPEC_V2.md` §12.3/13.3/13.4,
not a V1 concept. The lookahead was narrowed to `(?!TING|UP)`, verified with
a standalone regex table (`WEB_API_ROUTE_SETUP` no longer matches;
`WEB_API_ROUTE_SETS`, `WEB_API_ROUTE_SET_ACTIVE`,
`WEB_API_ROUTE_EXECUTIONS`, `WEB_API_ROUTE_BACKUP`, `WEB_API_ROUTE_RESTORE`,
and `WEB_API_ROUTE_SETTINGS*` all still resolve exactly as before). This is a
one-line, narrowly-scoped correction to a CI policy script, not a weakening —
every pattern the script was written to catch still fails the build.

## The PBKDF2 iteration count is provisional

Per the assigning task: **the hardware benchmark for V2-041 has not run**, so
no frozen iteration count exists yet. `auth.h` already defined
`AUTH_PBKDF2_ITERATIONS 120000U` (used by `auth_password_create()`/
`auth_password_verify()` for the existing V2-042 login path); this slice adds
an explicitly-named alias and a comment flagging it as not-frozen:

```c
/* Provisional PBKDF2-HMAC-SHA-256 iteration count. V2-041 has not yet run the
 * ESP32-S3R8 hardware benchmark that must select the frozen value (target
 * approximately 250-500 ms derivation time); this number is NOT that frozen
 * result and must be replaced once the benchmark lands before V2-040/V2-041
 * are claimed complete. */
#define AUTH_PBKDF2_ITERATIONS 120000U
#define AUTH_V2_PBKDF2_ITERATIONS_PROVISIONAL AUTH_PBKDF2_ITERATIONS
```

`AUTH_PBKDF2_ITERATIONS` itself was **not** renamed — it is referenced by V1
legacy code (`firmware/components/provisioning/provisioning_core.c`) and its
host tests (`test_provisioning.c`, `test_web_setup.c`, `test_app_core.c`)
that are out of this slice's scope; renaming it would have widened the diff
for no functional benefit. Instead, `web_server_setup.c`'s
`setup_password_create()` bridge explicitly checks the derived record's
iteration count against `AUTH_V2_PBKDF2_ITERATIONS_PROVISIONAL` and fails
closed (`APP_ERROR_INTERNAL`) if it ever drifts, so the named constant is
load-bearing, not decorative.

**What the integrator must do once V2-041 freezes the real count:** replace
the value of `AUTH_PBKDF2_ITERATIONS` (and update the comment to remove the
"provisional" language) in `firmware/components/auth/include/auth.h`, rerun
`./scripts/run-tests.sh auth` and `./scripts/run-tests.sh web`, then
`./scripts/check-firmware.sh`, and record that evidence before checking off
any V2-040/V2-041 `docs/TODO_V2.md` item.

## Cutover B checklist (handoff §6.5) — status

| # | Requirement | Status |
| - | --- | --- |
| 1 | Accept only the exact V2 setup request schema | Done — `exact_setup_fields()`/`populate_request()`; host-tested (unknown/missing/duplicate/wrong-type field cases) |
| 2 | Bounded body size before unbounded allocation/parsing | Done — `read_bounded_body()` checks `content_len` against `APP_V2_JSON_BODY_MAX_BYTES` before any receive |
| 3 | Reject unknown JSON fields | Done — same as #1 |
| 4 | Validate setup code via the prepared setup-session contract | Done — `app_v2_setup_prepare_candidate()` is the sole validator; not reimplemented |
| 5 | Strictly validate device name/AP SSID/AP passphrase/admin password/confirmation | Done — via the same contract call; host-tested per field |
| 6 | Derive password material through the V2 password-verifier path | Done — bridges `auth_password_create()`; provisional iteration count, see above |
| 7 | Read current canonical settings | Done — `device_settings_read()` |
| 8 | Prepare candidate via `app_v2_setup_prepare_candidate()` | Done |
| 9 | Atomically replace via `device_settings_replace()` | Done |
| 10 | Consume setup code only after commit succeeds | Done — host-tested directly (retry-after-commit-failure succeeds) |
| 11 | Return exact non-secret accepted/restart/reconnect response | Done — struct from `app_v2_setup_accepted_response_init()` mapped 1:1 to JSON |
| 12 | Never return admin password/AP passphrase/verifier/salt/setup code/session secret | Done by construction — the accepted/error responses contain no request- or settings-derived text |
| 13 | Post-provisioning: setup GET→404, setup POST→409 | Done via `WEB_API_ROUTE_SETUP`; verified structurally (route policy, `check-setup-route-isolation.sh`) and by firmware build; **not** verified over a live HTTP socket or on hardware (no `esp_http_server` fake exists; see [Test evidence](#test-evidence)) |
| 14 | Test wrong/malformed/expired-reboot-stale/reused/mismatched codes | Done — `test_wrong_code_rejected_before_commit`, `test_reboot_stale_code_rejected`, `test_malformed_code_rejected`, `test_reused_code_rejected` |
| 15 | Test settings-commit failure does not consume the code | Done — `test_commit_failure_does_not_consume_code` (fails, retries the *same* session, succeeds) |
| 16 | Test preservation of unrelated settings | Done — `test_success_commits_and_consumes_code` asserts `nextBlobId`/`sendMode`/`snapshotRetentionTarget`/`showMacroSourcePreviews` survive unchanged |
| 17 | Test reconnect/AP credential transition semantics | Done at the settings-commit level — `test_reconnect_ap_credential_transition` proves the submitted AP SSID/passphrase become the committed record (empty → new, the exact boot-to-boot transition); **actual runtime AP reconfiguration/reboot execution is V2-044 scope** (device-restart is not implemented anywhere yet — `WEB_API_ROUTE_DEVICE_RESTART` also only returns the JSON flag without an `esp_restart()` call) |
| 18 | Keep all non-setup API routes unavailable while unprovisioned | Unchanged from Cutover A, reverified — `setup_routes[]` still has exactly GET/POST `/api/v1/setup` + static wildcard; `check-setup-route-isolation.sh` passes |

Items explicitly **not** claimed complete: the exact frozen PBKDF2 iteration
count (blocked on V2-041 hardware benchmark) and any physical-device proof of
items 13/17's live HTTP/restart behavior (blocked on hardware access this
worktree does not have).

## Test evidence

All commands run from the repository root on this worktree
(`/home/phil/work/esp32-macro-keyboard/.claude/worktrees/agent-a2600fe974fd292fe`),
toolchain sourced per `CLAUDE.md` (`$HOME/.local/bin` on `PATH`, ESP-IDF
`v5.5.5` `export.sh`, Node `v24.18.0` via `nvm use`).

### New host test suite

```text
$ ./scripts/run-tests.sh web
...
100% tests passed, 0 tests failed out of 11
```

`web_server_setup_submit` (new, 25 cases) covers: success plus preserved
fields plus code consumption; `requireSerialConfirmation` pass-through; AP
credential transition; wrong/stale/malformed/reused codes; already-provisioned; invalid
device name/AP SSID/AP passphrase/admin password (both contract-detected and
`auth`-bridge-detected); settings-read failure; password-derivation failure
(both `INVALID_ARGUMENT`→field error and `INTERNAL`→backend error); **commit
failure followed by a successful retry on the same session** (item 15's
strongest possible proof); unknown/missing/duplicate/wrong-type JSON fields;
trailing garbage; non-object body; malformed JSON; embedded `\u0000` escape;
empty body; null-argument handling.

`test_web_api_core.c` and `test_web_request_policy.c` gained targeted
coverage for `WEB_API_ROUTE_SETUP`'s route/method/body/session policy,
including a dedicated proof that the route evaluates successfully **without**
a session cookie (unlike every other route in the existing success matrix).

### Full host suite

```text
$ ./scripts/run-tests.sh
100% tests passed, 0 tests failed out of 39
```

(38 pre-existing targets, unaffected, plus the 1 new target.)

### Native V2 contract suite

```text
$ ./scripts/check-v2-contracts.sh --native-only
...
100% tests passed, 0 tests failed out of 6
```

(`setup_contract_v2.{h,c}` was not modified; this reconfirms it is
undisturbed.)

### Route-isolation and architecture policy scripts

```text
$ bash scripts/check-setup-route-isolation.sh
V2 setup route isolation policy passed
$ python3 scripts/check-v2-phase2-architecture.py
phase 2 architecture: no firmware-owned package or macro repository
```

### Format

```text
$ ./scripts/check-format.sh
...
All matched files use Prettier code style!
```

Exit 0; C/C-header via `clang-format` 18, CMake via `cmake-format`/`cmake-lint`, JS/TS via `prettier`.

### Firmware build + Clang-Tidy (esp-clang, `WarningsAsErrors: '*'`)

```text
$ ./scripts/check-firmware.sh
...
(exit 0)
```

Builds both `firmware/` (production app) and `firmware/test_app/` (on-device
Unity test app — build only, **not execution**), runs `run-clang-tidy` with
`misc-include-cleaner` and all other first-party rules over every changed
translation unit with zero suppressions. One real defect was found and fixed
during this work: `readability-function-cognitive-complexity` (26 > 25) in
`exact_setup_fields()`, resolved by extracting `mark_string_field_seen()` —
not suppressed.

### Stack usage

```text
$ bash scripts/check-stack-usage.sh
stack usage policy passed: 518 first-party frames analyzed, largest 1536 bytes, 0 allowlisted
```

### Fullest available gate

```text
$ ./scripts/check-all.sh
... (many stages) ...
```

Every stage passed through the full webapp suite (`231/231` Vitest tests,
`26/26` files; Playwright/"Real Chrome Phase 17.10" workflow simulation;
`check-v2-limits.py`, `check-v2-settings-schema.py`,
`check-v2-device-settings-policy.py`, `check-v2-setup-route-policy.py`,
`check-v2-api-routes.py`, `check-v2-auth-policy.py`, `check-production-config.sh`,
`check-credential-logging.sh`, `check-mount-policy.sh`,
`check-layer-boundaries.sh`, `check-removed-features.sh`,
`check-usb-identity.sh`, `check-frontend-persisted-state.sh`,
`check-v2-034-capacity.py`, `check-partitions.sh`,
`check-static-analysis-policy.sh`, `verify-toolchain.sh`,
`build-webfs-image.sh`, `generate-flash-manifest.sh`,
`check-release-budgets.sh`). It then failed inside `check-scripts.sh` at
`python3 tests/scripts/test-v2-035-hardware.py`:

```text
File "scripts/run-v2-035-hardware.py", line 683
    f"expected a physical power-on reset, found {diagnostics["resetReason"]!r}")
SyntaxError: f-string: unmatched '['
```

This is a **pre-existing, unrelated** defect: `run-v2-035-hardware.py` uses a
nested-same-quote f-string, legal only under Python 3.12+ (PEP 701); this
worktree's `python3` is 3.10.10. `git log` confirms neither file was touched
by this change (`scripts/run-v2-035-hardware.py` last modified
`2026-08-07`, before this slice began), and both belong to the V2-035
hardware-benchmark harness, unrelated to V2-040. Fixing a Python-version
compatibility gap in unrelated V2-035 tooling was judged out of scope for
this task and is flagged here for the integrator rather than silently
patched. Every individual gate `check-all.sh` runs before this point —
including every gate that touches this change — passed cleanly.

## Recommendations for the integrator

1. **Substitute the real PBKDF2 iteration count** once V2-041's ESP32-S3R8
   benchmark selects one (see above); this is the only known functional gap.
2. Consider deleting the orphaned V1 `web_setup_core.{c,h}`/
   `web_setup_json.{c,h}` and their two host tests
   (`test_web_setup.c`/`test_web_setup_json.c`) — confirmed unused by the
   live firmware build (`grep` for any reference outside those four files and
   their own tests returns nothing) but left untouched here as out of this
   slice's scope.
3. Fix (or explicitly skip) the Python-3.12-only syntax in
   `scripts/run-v2-035-hardware.py` so `check-scripts.sh`/`check-all.sh` can
   run to completion in a Python 3.10 environment such as this worktree's.
4. Physical hardware evidence for items 13/17 (live HTTP status codes across
   a real socket, actual restart/AP-reconfiguration behavior after
   `restartRequired: true`) remains open — this worktree has no device
   access. V2-044 ("Wi-Fi and reset semantics") is where the actual device
   restart/reconfiguration execution belongs; today, `restartRequired` in the
   setup response and the existing `POST /api/v1/device/restart` route are
   both response-only contracts with no `esp_restart()` call anywhere in the
   codebase yet.
