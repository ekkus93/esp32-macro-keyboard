# V2-051 / V2-057 — live route-access matrix, and the async-path gap re-examined (2026-08-09)

## Scope

Track O (Phase 5 test-coverage gaps), continuing four prior rounds of work in
this exact area (`V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md`,
`V2_057_LIVE_ADMINISTRATION_HTTP_TEST_2026-08-09.md`,
`V2_057_DIAGNOSTICS_AND_SETUP_STATE_2026-08-09.md`, and
`V2_AUDIT_PHASE_5_6_2026-08-09.md`, all read in full before starting). Two
concrete items were in scope:

1. V2-051's "Test the complete unprovisioned/provisioned route-access
   matrix" — unchecked, backed only by `scripts/check-setup-route-isolation.sh`
   (a static regex parse of `web_server_lifecycle.c`'s source text), not an
   executed test.
2. V2-057's "Test every route with valid, missing, extra, wrong-type,
   wrong-content-type, oversized, unauthorized, expired-session,
   malformed-path, and method-error cases" — the remaining, honestly
   documented gap is the `require_physical_confirmation=true` /
   `web_server_async_dispatch()` path, which every prior round investigated
   and left open as unreachable without a FreeRTOS host fake.

## Branch and commit

Worktree branch: `worktree-agent-a44f46aab1d86b2bc`.
Starting commit: `e703ff2` (`docs(webapp): fix webapp/README.md's two false
claims`).
This work's commit SHA: see `git log -1` on the commit that introduces this
file (recorded in the commit message alongside this report).

## Item 1 (V2-051): investigation

Read `web_server_lifecycle.c`'s `normal_routes[]`/`setup_routes[]` tables and
`web_server_start()`/`web_server_stop()` directly, and the existing
`tests/host/fakes/esp_http_server_stub/esp_http_server.h`/`fakes/fake_httpd.c`
pair every prior `*_route_tests` target uses. Found:

- `web_server_lifecycle.c` was linked into **no** host test target at all
  (`grep -rn web_server_lifecycle tests/host/CMakeLists.txt` returned
  nothing before this track). Every existing `*_route_tests` target calls one
  fixed-URI handler function directly against a fake request/response; none
  of them ever calls `web_server_start()`, so none of them exercises which
  URIs/methods are actually registered in which provisioning mode.
- The existing `esp_http_server_stub/esp_http_server.h` fake has no
  `httpd_start()`/`httpd_stop()`/`httpd_register_uri_handler()` — it only
  fakes the per-request body/response surface (`httpd_req_recv()`,
  `httpd_resp_send()`, ...), not registration/dispatch.
- `web_server_lifecycle.c`'s `start_server_adapter()` calls
  `web_server_async_start()` (FreeRTOS-backed: creates a real queue and
  task), which is not host-linkable — but `web_server_async_start()`/
  `web_server_async_stop()` are just two `app_error_code_t` entry points with
  no route-table content of their own, so they can be faked trivially without
  needing a FreeRTOS host mock.
- `web_server_common.c` defines the three externs
  `server_configuration`/`server_lifecycle`/`setup_session`, but also pulls in
  `auth_session_validate()` and the JSON/body-auth adapter helpers — none of
  which `web_server_lifecycle.c` itself needs — so this track defines those
  three globals directly in the new test file instead of linking that
  translation unit in, keeping the new target's dependency surface to just
  `web_server_lifecycle.c`, `web_server_adapter_lifecycle.c`,
  `web_server_adapter_common.c`, and `setup_contract_v2.c`/
  `device_settings_v2.c` (for `app_v2_setup_session_init()`).
- Concluded that registering the real `normal_routes[]`/`setup_routes[]`
  tables into a router that resolves uri/method pairs the way ESP-IDF's own
  `httpd_find_uri_handler()` does was a tractable, narrow increment: port
  that one function (and `httpd_uri_match_wildcard()`) faithfully, from the
  real ESP-IDF v5.5.5 source
  (`$IDF_PATH/components/esp_http_server/src/httpd_uri.c`), rather than
  approximate it.

## What was built

### 1. `tests/host/fakes/esp_http_server_stub/esp_http_server.h` (extended)

Added `httpd_handle_t`, `httpd_config_t` (the three fields
`web_server_lifecycle.c` actually sets: `max_uri_handlers`, `stack_size`,
`uri_match_fn`), `HTTPD_DEFAULT_CONFIG()`, `httpd_uri_t` (`uri`, `method`,
`handler`, `user_ctx`), and prototypes for `httpd_start()`/`httpd_stop()`/
`httpd_register_uri_handler()`/`httpd_uri_match_wildcard()` — declarations
only; every existing `*_route_tests` target that includes this header still
compiles unchanged since it never references these four symbols.

### 2. `tests/host/fakes/fake_httpd_router.c`/`.h` (new)

A registration/dispatch fake, deliberately separate from `fake_httpd.c`'s
per-request double (see the file's own header comment for why). Implements
`httpd_start()`/`httpd_stop()`/`httpd_register_uri_handler()` (a small,
bounded registration array) and `httpd_uri_match_wildcard()` — the last one
ported line-for-line from ESP-IDF v5.5.5's real implementation for exact
fidelity, not reapproximated. `fake_httpd_router_resolve(uri, method,
&handler)` then reimplements ESP-IDF's real `httpd_find_uri_handler()` scan:
first uri+method match wins; a uri match with a different method is
remembered and reported as `FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED` only if
nothing later matches both; otherwise `FAKE_HTTPD_ROUTE_NOT_FOUND`.

### 3. `tests/host/test_web_server_lifecycle.c` (new, 3 tests)

Drives the real, unmodified `web_server_start()`/`web_server_stop()`
(`web_server_lifecycle.c`) against the fake router. Every handler symbol the
real route tables reference by address (`status_handler`, `blob_list_handler`,
`api_handler`, `static_handler`, `setup_state_handler`, ...) is defined in
this file as a never-called stand-in (`TEST_CHECK(false)` guards each one) —
this file only compares addresses, it never invokes a handler; each one's
actual *behavior* is already covered by the existing `*_route_tests` targets.
`web_server_async_start()`/`web_server_async_stop()` are faked to succeed
trivially.

- `test_unprovisioned_route_surface()`: starts `WEB_SERVER_MODE_SETUP`,
  confirms exactly 3 routes registered, then resolves 26 uri/method
  combinations against SPEC_V2 §12.3 ("every other `/api/v1` route is
  unavailable while the device is unprovisioned").
- `test_provisioned_route_surface()`: starts `WEB_SERVER_MODE_NORMAL`,
  confirms exactly 16 routes registered, then resolves 26 uri/method
  combinations, including proving `/api/v1/setup` GET/POST fall through to
  the generic wildcard's `api_handler()` (not to `setup_state_handler()`/
  `setup_submit_handler()`) once provisioned.
- `test_route_surface_swaps_across_provisioning_transition()`: starts in
  setup mode, stops, restarts in normal mode on the same lifecycle object —
  proving the route surface actually swaps (no leaked or accumulated
  registrations) across exactly the transition a real device makes at the
  setup-completion reboot.

Total: 52 uri/method-pair assertions across the two matrix tests, plus the
transition test's own checks.

### 4. `tests/host/CMakeLists.txt` (+1 target)

New `web_server_lifecycle_tests` target, registered under the `web` CTest
label, following the existing `*_route_tests` recipe pattern.

### 5. A genuine, previously-undocumented finding

Both `normal_routes[]` and `setup_routes[]` end in a `{"/*", HTTP_GET,
static_handler}` catch-all whose *uri* pattern (via `httpd_uri_match_wildcard()`)
matches every path unconditionally, regardless of method. Running the real
dispatch algorithm (not just reading the array literal) against every
route/method pair this test tries showed that this makes a bare **404 "route
not found" impossible** at the `httpd_find_uri_handler()` level for any path
starting with `/`, in either provisioning mode:

- A GET request to an unmatched path always resolves to `static_handler()`
  (which then answers its own 404 from the static filesystem — a different
  layer, already covered by `web_server_static.c`'s existing logic, not a
  routing-layer 404).
- A non-GET request to any path the catch-all's uri pattern matches (which is
  every path) always resolves as `405 Method Not Allowed`, never `404`,
  because the catch-all's uri always matches even though its method (`GET`)
  usually does not.

This does not contradict SPEC_V2 §12.3 ("every other `/api/v1` route is
unavailable while the device is unprovisioned") — no API handler ever runs
for those paths in setup mode, satisfying the requirement — but it is a real
characteristic of *how* that unavailability is expressed (static-file 404 or
405, never a routing-layer 404) that no static analysis of the source table
could show, and that this new live-dispatch test now documents and pins with
an assertion. Not a defect: verified `web_server_static.c`'s `static_handler()`
returns `404` for any path with no corresponding file
(`open_result == APP_ERROR_NOT_FOUND` branch), so an unprovisioned client GET
to e.g. `/api/v1/status` still ultimately receives a 404, just from a
different code path than "no route matched."

## Item 2 (V2-057): the async path, re-examined

Investigated fresh (not trusted from the prior reports' prose) whether a
FreeRTOS host fake now exists anywhere that could close the remaining
`require_physical_confirmation=true` / `web_server_async_dispatch()` gap:

- `tests/host/fakes/fake_freertos.c`/`.h` exists, but is a small,
  executor-specific test double (`fake_freertos_lock()`/`_unlock()`/
  `_queue_send()`/`_notify()`/`_wait()`) behind `macro_executor`'s own backend
  interface — `macro_executor` is written against a small ops struct, not
  against FreeRTOS APIs directly, which is *why* it is host-testable at all.
- `web_server_async.c` is not written that way: it calls
  `xQueueCreate()`/`xQueueSend()`/`xQueueReceive()`/`xTaskCreate()`/
  `xSemaphoreCreateBinary()`/`xSemaphoreTake()`/`xSemaphoreGive()`/
  `portENTER_CRITICAL()`/`portEXIT_CRITICAL()` directly, from
  `freertos/FreeRTOS.h`/`freertos/queue.h`/`freertos/semphr.h`/
  `freertos/task.h`. Confirmed no stand-in for any of those four headers
  exists anywhere under `tests/host/fakes/` (`find tests/host/fakes -iname
  '*freertos*'` finds only the executor-specific double above), and confirmed
  no such header exists elsewhere on this machine either except inside the
  real ESP-IDF SDK itself and an unrelated sibling project's own host-test
  mocks (`~/work/esp32_btaudio/*/test/host_test/mocks/include/freertos/`) —
  not part of this codebase and not something to import wholesale.
- Building a faithful `freertos/queue.h`/`freertos/task.h`/`freertos/semphr.h`
  host mock capable of correctly modeling a real blocking worker task
  (`xTaskCreate()` + `xQueueReceive(..., portMAX_DELAY)` on a second
  thread, synchronized against the httpd-task caller) would mean either a
  real OS-thread-backed FreeRTOS emulation or a cooperative single-threaded
  approximation subtle enough to get queue/semaphore blocking semantics
  right — a substantial new concurrency-fake, not a narrow test addition, and
  a correctness-critical one to get wrong (a broken fake could pass tests
  while hiding a real race).

**Conclusion: this gap remains genuinely exhausted at the host-test level,**
matching all four prior rounds' independent assessments. No code was written
against it in this track. `web_server_async.c`'s own logic (`claim_in_flight()`/
`release_in_flight()`'s mutual exclusion, the body-read-before-async-begin
ordering, the always-complete-on-every-path requirement) is correctness-
critical concurrency code that would need either a real FreeRTOS host mock or
physical/QEMU hardware to test meaningfully — writing a shallow test against
a stubbed-out queue that always succeeds synchronously would not exercise the
actual concurrency behavior and would be worse than no test (a false sense of
coverage). Left open in `docs/TODO_V2.md`, not forced.

## Commands run and results

```text
./scripts/run-tests.sh web                    -> 24/24 passed (new web_server_lifecycle target included)
./scripts/run-tests.sh                        -> 55/55 passed (up from 54/54)
./scripts/run-tests.sh --sanitizers web        -> 24/24 passed (ASan+UBSan)
./scripts/run-tests.sh --sanitizers            -> 55/55 passed (ASan+UBSan)
bash scripts/check-setup-route-isolation.sh    -> "V2 setup route isolation policy passed"
./scripts/check-format.sh                      -> clean for every C/CMake file this track touched
                                                   (clang-format 18, cmake-format/cmake-lint 0.6.13);
                                                   the script's final `npm --prefix webapp run
                                                   format:check` step fails with `prettier: not
                                                   found` because webapp/node_modules is not
                                                   installed in this sandboxed worktree -- a
                                                   pre-existing environment gap already recorded by
                                                   every prior V2-057 track's report, not caused by
                                                   this track (file surface: tests/host/ and docs/
                                                   only; no webapp/ files touched)
. "$HOME/esp/esp-idf-v5.5.5/export.sh" && ./scripts/check-firmware.sh
                                                -> exit 0, zero clang-tidy findings, both projects
                                                   (idf.py --version: ESP-IDF v5.5.5); this track
                                                   made zero changes under firmware/
```

## Files changed

- `tests/host/fakes/esp_http_server_stub/esp_http_server.h` (extended:
  route registration/dispatch types and prototypes)
- `tests/host/fakes/fake_httpd_router.c` (new)
- `tests/host/fakes/fake_httpd_router.h` (new)
- `tests/host/test_web_server_lifecycle.c` (new, 3 tests)
- `tests/host/CMakeLists.txt` (+1 target, `web_server_lifecycle_tests`)
- `docs/TODO_V2.md` (V2-051's route-access-matrix bullet, checked; the Phase 5
  exit gate's "Contract and security tests pass" bullet, test-count updated
  and async-gap re-investigation recorded; no other lines touched)
- `docs/implementation-v2/V2_051_057_ROUTE_ACCESS_MATRIX_2026-08-09.md` (this
  file)

`firmware/` is unchanged: no defect was found or needed fixing in this track.

## Checkboxes changed

- **V2-051**, "Test the complete unprovisioned/provisioned route-access
  matrix": `[ ]` -> `[x]`, with the evidence above.
- **Phase 5 exit gate**, "Contract and security tests pass": remains `[ ]`
  (text updated: test count 54/54 -> 55/55, and the async-path gap explicitly
  re-confirmed as still real and still the reason this line cannot be
  checked).

## Checkboxes left unchanged

- **V2-057**'s own "Test every route with valid, missing, ... method-error
  cases" bullet: left exactly as written by the prior track. This track's new
  test proves route *reachability* (which uri/method resolves to which
  handler in which provisioning mode) — a different, narrower property than
  V2-057's per-route valid/missing/extra/wrong-type/... request-body matrix,
  which was already closed as far as it can go for status/limits/send/blob/
  administration/setup-state/diagnostics by the four prior tracks. Not
  re-claimed here.
- **V2-057**'s "consume the same checked-in examples" bullet, and every other
  already-`[x]` V2-057 sub-bullet: unchanged, not re-verified beyond what was
  needed to confirm this track's own new work did not regress them (the full
  `55/55` host suite and `--sanitizers` runs above cover that).

## Explicit statement

No task is claimed complete without the reproducible evidence above. The
async/physical-confirmation-required path remains genuinely open, re-examined
and re-confirmed unreachable at the host-test level in this track rather than
assumed from the prior reports' prose — this is a legitimate, honest,
exhausted gap, not a deferred item awaiting more effort of the same kind.
Nothing in this report claims physical-hardware or on-device validation --
everything here is host-fake-backed native test evidence, plus a real (not
firmware-code, but production-relevant) finding about the route tables'
static-catch-all-implies-no-bare-404 behavior, verified by reading
`web_server_static.c` directly, not merely inferred.
