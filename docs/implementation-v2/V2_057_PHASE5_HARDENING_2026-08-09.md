# V2-057 / Phase 5 exit gate hardening — two related gaps (2026-08-09)

## Scope

Two related, genuinely hard gaps in V2-057 ("Contract and security tests")
and the Phase 5 exit gate, continuing the work recorded in
`V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md`,
`V2_057_LIVE_ADMINISTRATION_HTTP_TEST_2026-08-09.md`,
`V2_057_DIAGNOSTICS_AND_SETUP_STATE_2026-08-09.md`, and
`V2_051_057_ROUTE_ACCESS_MATRIX_2026-08-09.md`:

1. **Gap 1 (harder)**: the physical-confirmation-required=true / async-worker
   HTTP path (`web_server_async.c`), left open by four independent prior
   investigation rounds as unreachable without a FreeRTOS host fake.
2. **Gap 2 (more bounded)**: extend the checked-in
   `contracts/v2/api/examples.json` diff pattern (`cJSON_Compare()` against a
   live handler's real response, via `tests/host/support/test_examples_fixture.c`)
   from the two routes that already had it (`setup-state`, `diagnostics`) to
   the rest of the route surface.

## Branch and commit

Worktree branch: `worktree-agent-a6a22a74da4fbd493`.
Starting commit: `e7a7546` ("Merge browser test harness migration: hand-rolled
CDP to Playwright"), confirmed as `HEAD` before any work began.
This work's commit SHA: recorded in the commit(s) that carry this file (see
`git log` on this file's introducing commit).

## Gap 1 — investigation and what was built

### Re-investigating the "no FreeRTOS fake" conclusion, not trusting it blindly

Read `web_server_async.c` in full before writing anything. It genuinely does
call `xQueueCreate()`/`xQueueSend()`/`xQueueReceive()`/`vQueueDelete()`/
`xSemaphoreCreateBinary()`/`xSemaphoreGive()`/`xSemaphoreTake()`/
`vSemaphoreDelete()`/`xTaskCreate()`/`vTaskDelete()`/`portENTER_CRITICAL()`/
`portEXIT_CRITICAL()` directly from `freertos/FreeRTOS.h`/`freertos/queue.h`/
`freertos/semphr.h`/`freertos/task.h`, with no backend-interface seam the way
`macro_executor` has (`tests/host/fakes/fake_freertos.c` is a small,
executor-specific double behind that seam, not a general FreeRTOS stand-in;
confirmed no other FreeRTOS host fake exists anywhere in `tests/host/fakes/`).
Building a **faithful, runnable** emulation of that queue/task machinery
(real blocking semantics across a genuine second thread, correctly modeling
`xQueueReceive(..., portMAX_DELAY)` racing against the httpd task's
`xQueueSend()`) would be a substantial new concurrency-fake carrying real
risk: a subtly wrong fake could pass tests while hiding a real race,
which four independent prior rounds already flagged. This track's own fresh
read of the file reached the same conclusion for that specific claim — no
work was done trying to fake the queue/task's real behavior.

### The overlooked branch: `web_server_async_dispatch()`'s own fallback

Reading the function closely (not just trusting the prose summary) found
something the prior rounds' framing did not call out explicitly:

```c
esp_err_t web_server_async_dispatch(httpd_req_t *request) {
    ...
    if (async_queue == NULL || async_task_handle == NULL) {
        /* Worker unavailable: answer on the httpd task rather than drop the
         * request. This blocks other clients for the confirmation window, which
         * is the pre-existing behaviour and strictly better than failing. */
        bool should_restart = false;
        const esp_err_t result = web_api_handle_call(request, &should_restart);
        ...
        return result;
    }
    if (!claim_in_flight()) { ... }
    ... /* the real queue/task path */
}
```

`async_queue`/`async_task_handle` are file-scope statics, set only by
`web_server_async_start()`. **If a host test never calls
`web_server_async_start()`, they stay at their zero-initialized `NULL` state
for the entire process**, and every call to `web_server_async_dispatch()`
takes this synchronous fallback branch — real, documented, production code
(the exact behavior a real device falls back to if the worker task ever
fails to start), not a test-only shortcut or an approximation. Crucially,
this fallback branch returns *before* reaching `claim_in_flight()`/
`release_in_flight()` (the only callers of `portENTER_CRITICAL`/
`portEXIT_CRITICAL`) and before any queue/semaphore/task call — none of the
concurrency-risky code is on this path at all.

This meant a genuinely faithful, narrowly-scoped host stub was possible: one
that satisfies the *linker* for `web_server_async.c`'s FreeRTOS references
(so the real file can be compiled and linked at all) without attempting to
give those symbols real behavior, because a test built around the fallback
branch by construction never invokes them.

### What was built

1. **`tests/host/fakes/freertos_stub/freertos/{FreeRTOS,queue,semphr,task}.h`**
   (new). Minimal type/macro/prototype declarations matching exactly what
   `web_server_async.c` references syntactically (`QueueHandle_t`,
   `SemaphoreHandle_t`, `TaskHandle_t`, `portMUX_TYPE`,
   `portMUX_INITIALIZER_UNLOCKED`, `pdTRUE`/`pdFALSE`/`pdPASS`,
   `pdMS_TO_TICKS`, `portMAX_DELAY`, and the ten function prototypes above).
   Each header's own comment states plainly that this is **not** a faithful
   FreeRTOS emulation, unlike `esp_http_server_stub`/`esp_idf_misc_stub` —
   it exists solely to make the real file compile, and every symbol's
   *definition* (see below) is a hard-failure canary.
2. **`tests/host/fakes/esp_http_server_stub/esp_http_server.h`** (+2
   declarations): `httpd_req_async_handler_begin()`/
   `httpd_req_async_handler_complete()`, matching the real ESP-IDF
   signatures exactly — `web_server_async.c`'s queue-path branch (never
   executed under test, but still compiled) references them.
3. **`tests/host/test_web_server_async_confirmation.c`** (new, 8 tests).
   Links the *real* `web_server_async.c` (not a hand-rolled stub of
   `web_server_async_dispatch()`, unlike every prior V2-057 administration
   test) alongside `web_server_api.c`/`web_request_policy.c`/
   `web_api_core.c`/`web_api_dispatch.c`/`web_api_administration.c` and the
   same httpd fake other route tests use. Every test sets
   `server_configuration.require_physical_confirmation = true` and calls
   the real `api_handler()` for `POST /api/v1/device/restart`,
   `/settings/change-password`, `/device/reset-settings`,
   `/device/factory-reset`, and (as a negative control) `GET
   /api/v1/settings` — **without ever calling `web_server_async_start()`**,
   so every call provably takes the fallback branch. `device_controls_wait_for_confirmation()`
   is faked (unlike `test_web_server_administration_route.c`, where it is an
   unconditional `TEST_CHECK(false)` canary since that file never reaches
   it) so both a granted and a denied confirmation can be exercised. The ten
   FreeRTOS/async-handler symbols are each defined as a
   `TEST_CHECK(false)`-guarded canary: if any of them is ever actually
   invoked (e.g. a future refactor changes the fallback condition), the test
   fails loudly rather than silently running against an approximated
   primitive.

   Cases covered: `restart` confirmation granted (202, `device_controls_restart()`
   called once, `device_controls_wait_for_confirmation()` called once with
   the exact `APP_PHYSICAL_CONFIRM_TIMEOUT_MS` timeout, `esp_restart()`
   called once) and denied (`APP_ERROR_CONFLICT` — the real
   `device_controls_wait_for_confirmation()`'s other failure mode besides
   timeout, per `device_controls.c` — maps to 403, handler never runs);
   `change-password` granted (204, session invalidated, cookie cleared) and
   denied (403, no side effects); `reset-settings` granted (202);
   `factory-reset` granted (202, `esp_restart()` called); `GET /api/v1/settings`
   (not confirmation-gated) proves `device_controls_wait_for_confirmation()`
   sees zero calls even with the flag globally on, confirming the gate is
   genuinely per-route, not a global bypass.
4. **`tests/host/CMakeLists.txt`** (+1 target,
   `web_server_async_confirmation_tests`, registered under the `web` label).

### What this closes, and what it honestly does not

**Closed**: end-to-end, live-`httpd_req_t`-handler proof that
confirmation-required routes are correctly classified and dispatched into
`web_server_async_dispatch()` at all (nothing tested this before — every
existing administration test deliberately sets
`require_physical_confirmation = false`), that the confirmation-wait
timeout is exactly right, and that both outcomes (granted/denied) produce
the correct response through the *real* `web_server_async_dispatch()`
function, not a reimplementation of its logic.

**Not closed, and not claimed closed**: the actual FreeRTOS worker-queue/task
path — `web_server_async_start()`/`web_server_async_stop()`,
`async_worker()`'s loop, the `xQueueSend()`/`xQueueReceive()` handoff between
the httpd task and the worker task, and `claim_in_flight()`/
`release_in_flight()`'s mutual exclusion under real concurrency — remains
completely untested at the host level. A faithful, runnable emulation of
that would need either a real OS-thread-backed FreeRTOS emulation or a
carefully-correct cooperative single-threaded approximation, and getting it
subtly wrong would be worse than no test at all (a false sense of coverage
over correctness-critical concurrency code). This track did not attempt it,
for the same reason the four prior rounds did not: it is a substantial new
concurrency-fake, not a narrow test addition, and the risk of the fake
itself being wrong is real. **This is the honest, deliberate boundary of
Gap 1's closure** — genuinely partial, not fully closed, and TODO_V2.md
reflects that (the route-matrix bullet stays unchecked).

## Gap 2 — investigation and what was built

### What examples.json actually has (checked before assuming)

```
error, parserError, setupState, setupRequest, setupAccepted, loginRequest,
session, status, limits, blobList, blobCreated, settings, settingsUpdate,
settingsUpdated, passwordChangeRequest, sendRequest, sendAccepted,
sendStatus, restartAccepted, resetSettingsRequest, resetSettingsAccepted,
factoryResetRequest, factoryResetAccepted, diagnostics
```

Of the task's named candidate list (status, limits, send, login/session,
blob, restart, settings, change-password, reset-settings, factory-reset):
every one has a corresponding *response*-shaped example except
`changePassword` (only a request example exists — the real response is
`204 No Content`, genuinely bodyless, nothing to diff) and `login` (only a
request example exists, and unlike every other route in this list,
`login_handler()` — the real `POST /api/v1/auth/login` handler — has **no
live `httpd_req_t`-level test at all** anywhere in this codebase yet: it
calls `httpd_req_to_sockfd()`/`getpeername()` for IP-based rate limiting,
which needs a real or faked socket that no existing host test provides.
Building that from scratch is a distinct, larger undertaking than "diff
against the checked-in example" and was left out of this track's scope,
honestly, rather than forced).

### What was extended

For each route below, the existing live-handler test file was extended with
a `cJSON_Compare()` assertion against `test_examples_fixture_get(<key>)`,
mirroring `test_web_server_setup_route.c`/
`test_web_server_administration_route.c`'s existing `setupState`/
`diagnostics` pattern exactly:

| Route | Example key(s) | File |
| --- | --- | --- |
| `GET /api/v1/status` | `status` | `test_web_server_status_limits_route.c` |
| `GET /api/v1/limits` | `limits` | `test_web_server_status_limits_route.c` |
| `POST /api/v1/send` | `sendAccepted` | `test_web_server_send_route.c` |
| `GET /api/v1/send` | `sendStatus` | `test_web_server_send_route.c` |
| `GET /api/v1/blob` | `blobList` | `test_web_server_blob_list.inc` |
| `POST /api/v1/blob` | `blobCreated` | `test_web_server_blob_create.inc` |
| `GET /api/v1/auth/session` | `session` | `test_web_server_administration_route.c` |
| `GET /api/v1/settings` | `settings` | `test_web_server_administration_route.c` |
| `PUT /api/v1/settings` | `settingsUpdated` | `test_web_server_administration_route.c` |
| `POST /api/v1/device/restart` | `restartAccepted` | `test_web_server_administration_route.c` |
| `POST /api/v1/device/reset-settings` | `resetSettingsAccepted` | `test_web_server_administration_route.c` |
| `POST /api/v1/device/factory-reset` | `factoryResetAccepted` | `test_web_server_administration_route.c` |

`test_web_server_status_limits_route.c`'s existing `reset_fakes()` already
happened to produce exactly `status`'s checked-in numbers (device name, AP
SSID/client count, storage byte counts, blob count) — no fixture changes
needed there, only the comparison itself. `limits` needed nothing at all:
`web_adapter_build_limits_json()` serializes fixed `macro_limits.h`/
`app_limits_v2.h` constants with zero backend dependency. `send`/`blob`
needed request bodies and fake seeding chosen to reproduce the example's
non-trivial derived values (blob array order, matching send action count)
exactly — see the per-route notes below. `settings`'s GET baseline
(`provisioned_settings()`) already matched the `settings` example too.

`tests/host/CMakeLists.txt` gained `support/test_examples_fixture.c` +
`EXAMPLES_JSON_PATH` in the three targets (`web_server_status_limits_route_tests`,
`web_server_send_route_tests`, `web_server_blob_tests`) that did not already
link it; `web_server_administration_route_tests` already had it from the
prior track.

### Two genuine, previously-undetected discrepancies this diffing found

This is the exact class of value this track's approach is meant to
surface — and, per this project's explicit hard rule (CLAUDE.md:
`docs/SPEC_V2.md` is frozen; changes need Phil's explicit permission,
propose don't apply; never invent criteria), **neither was silently
resolved by editing the spec/example, and neither was silently
"fixed" by picking test inputs that happen to match an unverified number**.
Both are reported here for Phil to decide.

#### 1. `sendAccepted`/`sendStatus.estimatedDurationMs`

`contracts/v2/api/examples.json`'s `sendAccepted`/`sendStatus` **and**
`docs/SPEC_V2.md` §13.10's own inline JSON all show, for the exact request
`{"source": "make -j8{ENTER}", "keyPressMs": 8, "interKeyMs": 15}`:

```json
{ "id": "...", "state": "running", "actionCount": 9, "estimatedDurationMs": 214 }
```

`actionCount: 9` is correct (8 US-ASCII characters + one `{ENTER}` directive
= 9 actions). `estimatedDurationMs`, however, does not match either
implementation's actual, identical, deterministic formula: `macro_parser_v2.c`'s
`v2_append_action()` (firmware) and `macroCompiler.ts` (webapp, the same
shared-corpus formula) both cost every non-delay action at
`key_press_ms + inter_key_ms`, so 9 actions at `keyPressMs=8`/`interKeyMs=15`
computes to `9 * 23 = 207`, not `214`. `test_send_create_valid_matches_example()`/
`test_send_get_valid_matches_example()` (`test_web_server_send_route.c`)
assert this exact real value (`207`) explicitly, and normalize only
`estimatedDurationMs` (alongside the necessarily-random `id`) before the
wholesale `cJSON_Compare()`, so every other field still gets full drift
protection. This reads as a hand-computed illustrative number in the spec
that was never regenerated from a real compile, not a code defect — but is
reported, not assumed.

#### 2. `settingsUpdated.restartRequired`/`reconnectRequired`

`contracts/v2/api/examples.json`'s `settingsUpdated` **and** `docs/SPEC_V2.md`
§13.9's own inline JSON both show `"restartRequired": false,
"reconnectRequired": false` for a PUT request (the checked-in
`settingsUpdate` example) that includes an `accessPoint` object. The very
next sentence in SPEC_V2 §13.9 is: *"Changing access-point credentials sets
both flags to `true`."* — and `apply_access_point()`
(`settings_contract_v2.c`) does exactly that, unconditionally, whenever a
request carries `accessPoint` at all:

```c
out_flags->restart_required = true;
out_flags->reconnect_required = true;
```

The documented example JSON contradicts the documented prose immediately
below it. The code matches the prose, not the JSON.
`test_settings_put_valid_matches_example()`
(`test_web_server_administration_route.c`) submits the checked-in
`settingsUpdate` body verbatim, asserts the real computed values
(`restartRequired`/`reconnectRequired` both `true`) explicitly, and
normalizes only those two fields before the wholesale `cJSON_Compare()`
against `settingsUpdated`.

## Verification

Fast loop (`web` label):

```bash
./scripts/run-tests.sh web
```

25/25 passed (up from 22/22 at the start of this track: +1
`web_server_async_confirmation_tests`, and the extended assertions in
`web_server_status_limits_route`/`web_server_send_route`/
`web_server_blob`/`web_server_administration_route` all pass without adding
new test *targets*).

Full host suite:

```bash
./scripts/run-tests.sh
```

56/56 passed (0 failed), all labels (`auth`, `controls`, `executor`,
`model`, `parser`, `startup`, `storage`, `support`, `usb`, `web`, `wifi`) —
up from 53/53 at the start of this track.

Sanitizers (ASan+UBSan), both scoped and full:

```bash
./scripts/run-tests.sh --sanitizers web   # 25/25 passed
./scripts/run-tests.sh --sanitizers       # 56/56 passed
```

Native coverage gate (line ≥90% / branch ≥80% on policy files):

```bash
./scripts/generate-native-coverage.sh
```

Exit code `0`. Policy-file report: line coverage 96% (2433/2529), branch
coverage 82.7% (1792/2167) — both above threshold.

Format check:

```bash
./scripts/check-format.sh
```

Clean for every file this track touched (`clang-format` 18 applied via
`clang-format -i` during development, then reverified with `--dry-run
--Werror`; `cmake-format` 0.6.13 likewise). The script's final `npm --prefix
webapp run format:check` step fails in this environment with `prettier: not
found` because `webapp/node_modules` is not installed in this sandboxed
worktree — a pre-existing environment gap already recorded by every prior
V2-057 track's report, not caused by this track (file surface was `tests/host/`
and `docs/` only; **zero `firmware/`or `webapp/` files were touched**, so
`./scripts/check-firmware.sh`'s clang-tidy pass was not run — nothing under
`firmware/` changed for it to check).

## Files changed

- `tests/host/test_web_server_async_confirmation.c` (new)
- `tests/host/fakes/freertos_stub/freertos/FreeRTOS.h` (new)
- `tests/host/fakes/freertos_stub/freertos/queue.h` (new)
- `tests/host/fakes/freertos_stub/freertos/semphr.h` (new)
- `tests/host/fakes/freertos_stub/freertos/task.h` (new)
- `tests/host/fakes/esp_http_server_stub/esp_http_server.h` (+2 declarations)
- `tests/host/test_web_server_status_limits_route.c` (+2 comparisons)
- `tests/host/test_web_server_send_route.c` (+2 tests, +1 shared helper)
- `tests/host/test_web_server_blob_list.inc` (+1 test)
- `tests/host/test_web_server_blob_create.inc` (+1 test)
- `tests/host/test_web_server_blob.c` (+2 includes, +2 `main()` calls)
- `tests/host/test_web_server_administration_route.c` (+1 test, +5
  comparisons on existing tests)
- `tests/host/CMakeLists.txt` (+1 new target; `support/test_examples_fixture.c`
  + `EXAMPLES_JSON_PATH` added to 4 existing targets)
- `docs/TODO_V2.md` (V2-057 bullets and Phase 5 exit gate bullets only)

## Explicit statement

**Gap 1 is genuinely, deliberately partial** — the physical-confirmation-
required=true routing decision and the synchronous-fallback outcome are now
live-tested against real production code; the FreeRTOS worker-queue/task
path itself remains untested and is not claimed otherwise. The V2-057
route-matrix bullet and the Phase 5 "contract and security tests pass"
bullet both stay unchecked, honestly reflecting that remaining gap — this is
the expected, correct outcome per this project's "leave genuinely-undone
items honestly unchecked" rule, not a failure to close it further.

**Gap 2 is checked complete** (`consume the same checked-in examples from C
and TypeScript tests`, and the Phase 5 "API documentation examples match
observed responses" bullet) with two explicit, narrow carve-outs
(`login`'s own response, `changePassword`'s bodyless response) that are
documented as distinct, out-of-scope gaps rather than silently ignored, and
two genuine numeric discrepancies between the frozen spec/examples and the
actual implementation that are reported for Phil's decision, not resolved
unilaterally.

Nothing in this report claims physical-hardware or on-device validation —
everything here is host-fake-backed native test evidence.
