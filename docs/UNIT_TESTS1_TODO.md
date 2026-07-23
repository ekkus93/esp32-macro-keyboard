# Unit Test Expansion 1 — Authoritative TODO

Status: **Not started**

Target repository: `ekkus93/esp32-macro-keyboard`

Target branch: `master`

This document defines the implementation plan for expanding unit-test coverage beyond the
existing macro-parser and macro-set repository tests. It is subordinate to `docs/SPEC.md`
and must not weaken any safety, lint, error-visibility, or no-fallback requirement.

## 1. Goals

The completed work must:

- add deterministic host tests for safety-critical firmware logic
- add frontend tests for the API client and implemented workflows
- add selected ESP32-S3 Unity tests for real ESP-IDF adapters
- add deterministic failure injection for clocks, randomness, FreeRTOS, USB, GPIO, Wi-Fi,
  HTTP, and filesystem operations
- test cleanup and state after every injected failure
- run all host tests in GitHub Actions
- retain binaries, logs, and coverage assets only for tagged pushes

This plan does not replace hardware-in-the-loop testing. USB enumeration, actual typing,
radio behavior, physical buttons, browser-to-device integration, and power interruption
remain covered by `docs/HIL_TEST_PLAN.md`.

## 2. Non-negotiable policy

### 2.1 First-party warnings are bugs

Do not add or use:

- `NOLINT` or `NOLINTNEXTLINE`
- ESLint disable comments
- `@ts-ignore` or `@ts-nocheck`
- `#pragma GCC diagnostic ignored`
- `-Wno-*`
- `eslint --quiet`
- `|| true`
- ignored exit codes
- flaky-test retries that hide deterministic defects
- exclusions for failing first-party files

Do not lint or modify ESP-IDF, FreeRTOS, TinyUSB, managed components, npm dependencies,
generated output, or other third-party code.

### 2.2 No test-mode runtime behavior

Do not change production behavior under `#ifdef UNIT_TEST`. Instead:

- extract pure state machines from framework adapters
- inject narrow operation tables
- keep production defaults in production adapters
- place deterministic fakes under `tests/host/fakes/`

Do not make private functions public only for testing. Extract independently owned pure
logic into internal modules instead.

### 2.3 Determinism and failure atomicity

Every test must reset fake/global state, use fake time and random data where relevant, and
leave no thread, descriptor, temporary file, or allocation behind.

For any failure, assert both:

1. the returned error
2. the resulting ownership and subsystem state

## 3. Baseline and infrastructure

### Task 3.1 — Confirm the baseline

Inspect and run:

- `tests/host/CMakeLists.txt`
- `tests/host/test_macro_parser.c`
- `tests/host/test_storage_repository.c`
- `firmware/test_app/main/*.c`
- `scripts/run-tests.sh`
- `scripts/build-device-tests.sh`
- `.github/workflows/host-tests.yml`
- `.github/workflows/device-tests-build.yml`

Completion gate:

- existing host tests pass
- device-test firmware compiles for ESP32-S3 with ESP-IDF v5.5.5
- any pre-existing failure is recorded before implementation begins

### Task 3.2 — Add shared host-test support

Create:

- `tests/host/support/test_assert.h`
- `tests/host/support/test_assert.c`
- `tests/host/support/test_memory.h`
- `tests/host/support/test_memory.c`
- `tests/host/support/test_temp_dir.h`
- `tests/host/support/test_temp_dir.c`

Modify the existing host tests to use this support.

Requirements:

- assertions remain active in release builds
- failures report file, line, expression, expected value, and actual value
- add helpers for integers, strings, buffers, pointers, UUIDs, and app errors
- add isolated temporary-directory creation and recursive cleanup
- add first-party allocation tracking and fail on leaks

### Task 3.3 — Add deterministic fakes

Create:

- `tests/host/fakes/fake_clock.[ch]`
- `tests/host/fakes/fake_random.[ch]`
- `tests/host/fakes/fake_freertos.[ch]`
- `tests/host/fakes/fake_usb_backend.[ch]`
- `tests/host/fakes/fake_gpio_backend.[ch]`
- `tests/host/fakes/fake_wifi_backend.[ch]`
- `tests/host/fakes/fake_http_backend.[ch]`
- `tests/host/fakes/fake_fs_backend.[ch]`

Each fake must:

- have an explicit reset function
- record ordered calls and arguments
- support failure of a named call or the Nth call
- fail on unexpected calls
- use bounded call logs that fail on overflow
- never sleep or use wall-clock time

### Task 3.4 — Add CTest labels and focused commands

Modify:

- `tests/host/CMakeLists.txt`
- `scripts/run-tests.sh`
- `tests/host/README.md`

Required labels:

- `parser`
- `storage`
- `executor`
- `auth`
- `web`
- `startup`
- `usb`
- `controls`
- `wifi`

Support:

```bash
./scripts/run-tests.sh
./scripts/run-tests.sh executor
./scripts/run-tests.sh storage
```

Unknown arguments must fail visibly.

## 4. Macro executor tests

Priority: **Critical**

### Task 4.1 — Extract a deterministic executor engine

Create or modify:

- `firmware/components/macro_executor/macro_executor_engine.[ch]`
- `firmware/components/macro_executor/macro_executor_ops.h`
- `firmware/components/macro_executor/macro_executor.c`
- `firmware/components/macro_executor/CMakeLists.txt`

Keep FreeRTOS and USB calls in the adapter. Put request validation, busy transitions,
action progression, cancellation decisions, watchdog decisions, and terminal-state
construction in the engine.

### Task 4.2 — Add `macro_executor_tests`

Create `tests/host/test_macro_executor.c` and register CTest label `executor`.

Required validation tests:

- null request
- null action array
- zero actions
- action count above limit
- zero or excessive key-press duration
- excessive inter-key duration
- invalid execution, set, and macro UUIDs
- zero macro revision
- USB not ready

Required submission and ownership tests:

- successful submission transfers the plan once and clears caller ownership
- busy rejects a second request without modifying it
- queue failure clears busy and leaves caller ownership intact
- lock or unlock failure leaves recoverable state
- failed submission does not poison the next valid submission

Required execution tests:

- key action order is press, key delay, release, inter-key delay
- delay action emits no USB report
- action index is monotonic
- completion publishes completed/none
- cancellation before first action
- cancellation during key delay
- cancellation during long delay within the cancellation slice
- watchdog timeout
- press failure
- per-action release failure
- inter-key delay failure
- final release-all on completed, cancelled, and failed paths
- final release failure stored in `release_error` without replacing the primary error
- busy/cancel state reset on every terminal path
- accepted plan freed exactly once
- second execution works after every terminal outcome

### Task 4.3 — Add executor device tests

Create `firmware/test_app/main/test_macro_executor.c` with tags
`[device][executor]`.

Test initialization, idle status, USB-not-ready rejection, and cancel-while-idle. Do not
attempt actual typing in the default Unity suite.

Completion gate:

- every accepted execution has ownership tests
- every terminal path has release-all tests
- host tests pass and device-test firmware compiles

## 5. Authentication tests

Priority: **Critical**

### Task 5.1 — Inject authentication dependencies

Create or modify:

- `firmware/components/auth/auth_core.[ch]`
- `firmware/components/auth/auth_ops.h`
- `firmware/components/auth/auth.c`
- `firmware/components/auth/CMakeLists.txt`

Inject random bytes, monotonic time, locking, and PBKDF2. Production adapters must continue
to use ESP random, ESP timer, FreeRTOS, and mbedTLS.

### Task 5.2 — Add `auth_tests`

Create `tests/host/test_auth.c` and register CTest label `auth`.

Password creation tests:

- null inputs
- lengths 11, 12, 128, and 129
- embedded NUL
- deterministic salt
- exact iteration count
- PBKDF2 failure zeroes output
- known PBKDF2-HMAC-SHA-256 vectors

Password verification tests:

- correct and incorrect passwords
- short, oversized, and embedded-NUL passwords
- iteration count below minimum
- PBKDF2 failure
- temporary derived material is zeroed

Session tests:

- exact lowercase-hex token lengths
- session and CSRF tokens are distinct
- valid token pair
- wrong session and wrong CSRF tokens
- short, long, uppercase, and non-hex tokens
- sliding expiry after successful validation
- failed validation does not extend expiry
- expired entry clearing and slot reuse
- full unexpired table returns conflict
- logout, repeated logout, and invalid logout token
- lock and unlock failure behavior

Rate-limit tests:

- initial attempt allowed
- failures one through four remain allowed
- fifth failure rate-limits
- retry-after rounds upward
- retry value decreases with fake time
- exact 60-second reset boundary
- success resets the window
- counter does not wrap
- backward fake time follows an explicit tested policy

### Task 5.3 — Add authentication device tests

Create `firmware/test_app/main/test_auth.c` with tags `[device][auth]`.

Test password create/verify and session create/validate/logout using real ESP timer, mutex,
and random source. Never print passwords, salts, hashes, session tokens, or CSRF tokens.

## 6. HTTP security and static-serving tests

Priority: **Critical**

### Task 6.1 — Extract pure helpers

Create:

- `firmware/components/web_server/web_cookie.[ch]`
- `firmware/components/web_server/web_origin.[ch]`
- `firmware/components/web_server/web_static_path.[ch]`
- `firmware/components/web_server/web_content.[ch]`

Move cookie extraction, Host/Origin matching, static URI normalization, content-type
selection, and `Accept-Encoding` token parsing out of `web_server.c`.

### Task 6.2 — Add `web_security_tests`

Create `tests/host/test_web_security.c` and register CTest label `web`.

Cookie tests:

- session cookie first, middle, and last
- spaces and semicolon boundaries
- similar cookie names do not match
- duplicate session cookies are rejected
- missing, empty, short, long, uppercase, and non-hex values
- output cleared on failure

Origin/Host tests:

- exact host and host-with-port matches
- mismatched host or port
- HTTPS while product is HTTP-only
- trailing slash, userinfo, path, query, fragment, and whitespace
- null and empty values
- explicit case-handling policy
- IPv4 host and port

Static-path tests:

- `/` and normal asset paths
- query removal
- all `/api/` paths rejected
- traversal, repeated dots, backslash, percent encoding, control bytes, spaces, and NUL
- exact output-buffer boundary and one-byte-small buffer
- failure clears output
- result remains beneath `STORAGE_WEB_MOUNT`

Encoding and MIME tests:

- `gzip` and `br, gzip`
- reject `xgzip` and `gzip;q=0`
- HTML, JS, CSS, SVG, JSON, PNG, and unknown types
- reject suffix tricks such as `.js.txt`

### Task 6.3 — Add `web_server_adapter_tests`

Create `tests/host/test_web_server_adapter.c` with the `web` label.

Test:

- bounded request bodies from zero to maximum
- oversized body rejected before reading
- partial reads and timeout retries
- early EOF and negative receive
- buffer clearing on failure
- missing headers
- invalid origin before session validation
- valid mutation authorization
- status, limits, and error envelopes are valid JSON
- untrusted error text is escaped through cJSON or equivalent
- gzip selection and fallback
- missing files return 404
- bounded streaming
- read, chunk-send, final-chunk, and close failures
- API paths never fall through to static serving
- partial route-registration rollback
- stop failure state

Completion gate:

- security helpers have explicit boundary tests
- mutable user storage cannot be mapped by static serving
- no untrusted text is interpolated into JSON without escaping

## 7. Storage fault-injection tests

Priority: **Critical**

### Task 7.1 — Add injectable filesystem operations

Create or modify:

- `firmware/components/storage/storage_fs_ops.[ch]`
- `storage_atomic.c`
- `storage_repository_io.c`
- `storage_transaction.c`
- `storage_quarantine.c`

Wrap open, read, write, close, sync, rename, unlink, stat, mkdir, directory iteration, and
removal. Preserve `errno` before subsequent calls can overwrite it. Missing callbacks must
never mean success.

### Task 7.2 — Add `storage_fault_tests`

Create `tests/host/test_storage_faults.c` and register label `storage`.

Atomic-write tests:

- successful replacement
- short write
- zero-byte write without infinite loop
- file sync failure
- close failure
- readback open/read/size/content/close failures
- readback mismatch
- rename failure preserves old destination
- parent-directory sync failure
- temporary-name collisions and bounded exhaustion
- cleanup failure visibility
- zero-length policy
- maximum file size and one byte above

Path tests:

- valid set, staging, trash, transaction, quarantine, global macro, procedure, and progress
  paths
- null and invalid UUID inputs
- exact buffer boundary
- traversal and raw-path rejection
- every path remains under its intended mount

Index/JSON tests:

- empty and maximum valid indexes
- one item over limit
- duplicate IDs
- invalid UUID and schema
- missing or wrong-type fields
- trailing garbage and embedded NUL policy
- quarantine of corrupt indexes
- missing initialized index is not recreated
- first-use metadata only when an empty store is proven

Transaction tests for every recognized type and phase:

- expected paths present
- each required path missing individually
- conflicting paths present
- malformed manifest
- invalid schema, UUID, type, phase, revision, and path
- successful recovery produces one authoritative state
- second recovery is idempotent
- unknown type is preserved and reports storage corruption
- multiple manifests processed deterministically
- one corrupt manifest does not discard a valid one

Quarantine tests:

- evidence and metadata success
- evidence move failure
- metadata failure preserves evidence
- missing/unavailable quarantine directory
- malformed entry list
- maximum entry count
- bounded source and reason strings
- partial failure never deletes the only evidence copy

### Task 7.3 — Split object repository tests when implementations exist

Create:

- `tests/host/test_storage_macros.c`
- `tests/host/test_storage_procedures.c`
- `tests/host/test_storage_progress.c`

Cover CRUD, ordering, stale revisions, limits, duplicate IDs, references, progress
invalidation, deletion cleanup, and corrupt-object quarantine.

## 8. Application startup tests

Priority: **High**

### Task 8.1 — Extract injectable startup sequencing

Create or modify:

- `firmware/components/app_core/app_core_sequence.[ch]`
- `firmware/components/app_core/app_core_ops.h`
- `firmware/components/app_core/app_core.c`

Inject NVS, storage, recovery, repository, auth, USB, executor, controls, Wi-Fi, HTTP,
indicator, random, and logging operations.

### Task 8.2 — Add `app_core_tests`

Create `tests/host/test_app_core.c` and register label `startup`.

Success tests:

- exact stage order
- ready indicator after full success
- degraded indicator only for permitted storage-corrupt recovery
- distinct development AP and web credentials
- production refuses networking without persistent provisioning

Inject failure at each stage:

- NVS
- storage mount
- transaction recovery
- repository
- authentication
- USB
- executor
- controls
- credential generation
- password-record creation
- Wi-Fi
- HTTP

For each failure assert:

- no later stage starts
- only owned resources are cleaned up
- cleanup order is HTTP, Wi-Fi, storage
- fatal indicator is selected
- original error is returned
- cleanup errors do not replace the original error
- credentials are not logged after generation/hash failure

NVS policy tests:

- no-free-pages and new-version-found map to storage corruption
- other failures map to storage unavailable
- no path automatically erases NVS

## 9. USB keyboard tests

Priority: **High**

### Task 9.1 — Inject TinyUSB and time operations

Create or modify:

- `firmware/components/usb_hid/usb_keyboard_ops.h`
- `firmware/components/usb_hid/usb_keyboard_state.[ch]`
- `firmware/components/usb_hid/usb_keyboard.c`

### Task 9.2 — Add `usb_keyboard_tests`

Create `tests/host/test_usb_keyboard.c` and register label `usb`.

Test:

- initial, enumerating, disconnected, ready, suspended, and error states
- driver install success and failure
- mount, unmount, suspend, and resume callbacks
- resume while mounted and unmounted
- press in every non-ready state
- zero usage
- HID-ready immediate success and delayed success
- HID-ready timeout
- deadline tick wraparound
- exact modifier and six-key report contents
- report failure
- release while unmounted or suspended
- release readiness and report contents

### Task 9.3 — Add USB device-state tests

Create `firmware/test_app/main/test_usb_state.c` with `[device][usb]`. Test initialization,
safe rejection before enumeration, and zero-usage rejection. Actual typing remains HIL.

## 10. Device-control tests

Priority: **High**

### Task 10.1 — Extract pure debounce and LED logic

Create:

- `firmware/components/device_controls/device_controls_logic.[ch]`

Pure functions must accept one sample or elapsed milliseconds and return explicit results.

### Task 10.2 — Add `device_controls_tests`

Create `tests/host/test_device_controls.c` and register label `controls`.

Debounce tests:

- one and two matching samples do not trigger
- third pressed sample triggers once
- held press does not retrigger
- release and new press retrigger correctly
- alternating bounce never triggers
- candidate changes reset count
- count cannot wrap
- active-high and active-low adapter behavior

Indicator tests:

- ready always on
- booting, executing, degraded, and fatal timing patterns
- exact boundaries before and after every transition
- unknown state off
- large tick and wraparound behavior

Adapter tests:

- duplicate GPIO assignments
- GPIO configuration failures
- semaphore and task creation failures with cleanup
- cancel while idle is not fatal
- other cancel errors are fatal
- confirmation give failure
- GPIO write failure
- zero timeout
- confirmation success and timeout

## 11. Wi-Fi AP tests

Priority: **High**

### Task 11.1 — Extract Wi-Fi lifecycle state and inject operations

Create or modify:

- `firmware/components/wifi_ap/wifi_ap_state.[ch]`
- `firmware/components/wifi_ap/wifi_ap_ops.h`
- `firmware/components/wifi_ap/wifi_ap.c`

### Task 11.2 — Add `wifi_ap_tests`

Create `tests/host/test_wifi_ap.c` and register label `wifi`.

Validation tests:

- null/empty SSID
- SSID lengths 32 and 33
- passphrase lengths 11, 12, 63, and 64
- start while not stopped

Configuration tests:

- AP mode
- configured channel
- maximum four clients
- WPA2/WPA3 PSK
- PMF required
- exact SSID/passphrase copying with zeroed remainder

Event tests:

- AP start and stop
- connect increments and saturates at four
- disconnect decrements and never underflows
- unknown event no-op

Failure matrix:

- netif initialization
- event-loop creation
- AP netif creation
- Wi-Fi initialization
- event registration
- mode configuration
- AP configuration
- Wi-Fi start

For each failure verify reverse cleanup, error state, cleanup failure visibility, and later
clean-start behavior according to policy.

Stop tests:

- already stopped
- normal cleanup
- stop failure
- unregister and deinit failures
- deterministic final state after partial cleanup

No test or implementation may permit an open AP or disabled PMF.

## 12. Frontend tests

Priority: **High**

### Task 12.1 — Add frontend test support

Create:

- `webapp/tests/setup.ts`
- `webapp/tests/fakeFetch.ts`
- `webapp/tests/fakeLocation.ts`

Modify Vitest configuration and package scripts.

Requirements:

- fake timers for timeout and polling
- fetch fake records URL, method, headers, credentials, body, and abort signal
- unexpected fetch fails
- reset CSRF, timers, fetch plans, and hash after each test
- unexpected console errors and unhandled rejections fail tests

### Task 12.2 — Add API-client tests

Create `webapp/tests/api.test.ts`.

Test:

- reject non-`/api/` path before fetch
- same-origin credentials
- JSON Accept header
- Content-Type only with body
- preserve caller headers
- CSRF on POST/PUT/PATCH/DELETE, not GET
- success envelope
- HTTP failure plus API failure
- HTTP failure plus success envelope
- HTTP success plus failure envelope
- non-JSON response
- JSON with charset
- malformed JSON
- invalid envelope shape
- network failure
- ten-second abort
- timer cleared after every outcome
- caller abort behavior explicitly supported or rejected

### Task 12.3 — Add application tests

Create:

- `webapp/tests/app-routing.test.tsx`
- `webapp/tests/app-auth.test.tsx`
- `webapp/tests/app-execution.test.tsx`
- `webapp/tests/error-banner.test.tsx`

Routing:

- empty and unknown hash to login
- every implemented route renders
- hash listener cleanup
- no stale polling timer after route change

Authentication:

- password submission
- success stores CSRF and navigates
- structured failure banner
- network failure
- repeated submission concurrency policy

Execution:

- immediate poll
- running progress
- completed, cancelled, and failed terminal navigation
- polling failure stays visible and does not synthesize completion
- timer cleanup on terminal state and unmount
- cancel POST
- cancel failure visible
- cancel success does not claim completion before polling confirms it

Error banner:

- empty message hidden
- text rendered safely, not as HTML
- clearing removes banner
- long/untrusted text cannot execute markup

## 13. Parser and model boundary tests

Priority: **Medium**

### Task 13.1 — Expand parser boundaries

Modify `tests/host/test_macro_parser.c`.

Add explicit tests for:

- null source with zero and nonzero length
- empty source
- exact source/action/duration limits and one above each
- duration arithmetic overflow
- default and custom timing options
- zero/excessive timing values
- every named key and modifier
- modifier order and duplicates
- case and whitespace behavior
- CRLF, LF, lone CR, and tab
- exact multiline byte/line/column locations
- directive-length boundaries
- escaped braces adjacent to directives
- invalid UTF-8 and valid non-ASCII UTF-8
- output-plan reuse contract

### Task 13.2 — Add `macro_model_tests`

Create `tests/host/test_macro_model.c`.

Test text boundaries, embedded NUL, revision boundaries, null/double free safety, clearing
ownership fields, and partially initialized procedure cleanup. Use leak tracking.

## 14. Sanitizers and coverage

### Task 14.1 — Add sanitizer mode

Modify:

- `tests/host/CMakeLists.txt`
- `scripts/run-tests.sh`
- `.github/workflows/host-tests.yml`

Add AddressSanitizer and UndefinedBehaviorSanitizer host builds. Do not add sanitizer
suppressions for first-party findings. Missing sanitizer support must fail clearly rather
than silently skip.

### Task 14.2 — Add native and frontend coverage

Add dedicated coverage builds. Include only first-party production sources and exclude test
support, fakes, generated code, ESP-IDF, and third-party code.

Requirements:

- line and branch coverage
- summary in CI logs/job summary
- no retained coverage artifact on normal push or pull request
- tagged pushes may retain HTML or machine-readable reports
- missing coverage tools fail the coverage job

### Task 14.3 — Establish gates after suites exist

After Sections 4 through 13 are implemented:

- require at least 90% line and 80% branch coverage for new pure policy modules
- require every documented state transition and failure branch to be exercised for executor,
  auth core, HTTP security helpers, startup sequence, debounce logic, and Wi-Fi lifecycle
- do not add coverage-ignore directives or exclude difficult first-party code

## 15. Device-test and CI integration

### Task 15.1 — Expand the Unity application

Modify:

- `firmware/test_app/main/CMakeLists.txt`
- `firmware/test_app/README.md`
- root `README.md`

Document and register:

- `[device][executor]`
- `[device][auth]`
- `[device][usb]`

Device tests must not erase NVS, format LittleFS, start an open AP, print secrets, or hang
indefinitely waiting for input.

### Task 15.2 — Expand host-test GitHub Actions

Modify `.github/workflows/host-tests.yml`.

Requirements:

- run on `master`, pull requests, tags, and manual dispatch
- use the same scripts as local execution
- run normal, sanitizer, coverage, and frontend tests when the frontend lockfile exists
- no `continue-on-error`
- preserve complete failure output
- upload binaries, logs, and coverage only when `github.ref_type == 'tag'`
- normal pushes and pull requests retain no artifacts
- tagged package excludes secrets, caches, and dependencies

### Task 15.3 — Update documentation only after green CI

Modify:

- `tests/README.md`
- `tests/host/README.md`
- `firmware/test_app/README.md`
- `.github/workflows/README.md`
- root `README.md`
- `docs/IMPLEMENTATION_STATUS.md`

Clearly distinguish host-tested, sanitizer-tested, device-build-tested, device-executed,
and HIL-verified states. Do not mark physical execution complete until real serial output is
reviewed.

## 16. Final acceptance checklist

### Infrastructure

- [ ] Shared assertions, temporary directories, and leak tracking exist.
- [ ] Deterministic clock, random, FreeRTOS, USB, GPIO, Wi-Fi, HTTP, and filesystem fakes exist.
- [ ] Unexpected fake calls fail tests.
- [ ] Test labels and focused commands work.

### Firmware host tests

- [ ] Macro executor tests pass.
- [ ] Authentication/session tests pass.
- [ ] HTTP security and adapter tests pass.
- [ ] Storage fault-injection tests pass.
- [ ] Startup/rollback tests pass.
- [ ] USB keyboard tests pass.
- [ ] Device-control tests pass.
- [ ] Wi-Fi AP tests pass.
- [ ] Expanded parser/model tests pass.

### Frontend

- [ ] API-client tests pass.
- [ ] Routing, login, execution, and error-banner tests pass.
- [ ] TypeScript, ESLint, Stylelint, Prettier, and Vitest report zero warnings and errors.

### Runtime quality

- [ ] AddressSanitizer and UndefinedBehaviorSanitizer are clean.
- [ ] Leak tracking is clean.
- [ ] Coverage contains only first-party production code.
- [ ] Coverage gates pass without ignore directives.

### Device and CI

- [ ] Device-test firmware compiles with ESP-IDF v5.5.5 for ESP32-S3.
- [ ] New Unity tags are documented.
- [ ] Device tests do not print secrets or destructively recover storage.
- [ ] Normal CI runs retain no artifacts.
- [ ] Tagged runs retain expected test and coverage assets.
- [ ] Documentation matches implemented commands and validation state.

## 17. Required implementation order

Implement in this dependency order:

1. baseline confirmation
2. shared host support and fakes
3. narrow testability seams
4. macro executor
5. authentication
6. HTTP security and adapter
7. storage fault injection
8. startup and rollback
9. USB keyboard
10. device controls
11. Wi-Fi AP
12. frontend
13. parser/model boundaries
14. sanitizers and coverage
15. device-test and CI integration
16. documentation and final acceptance

Do not enforce coverage thresholds before the relevant suites exist. Do not defer executor,
authentication, HTTP security, or storage failure tests in favor of a repository-wide
coverage percentage.
