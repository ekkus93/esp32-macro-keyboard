# V2-053 / V2-057 — Blob route HTTP-layer test coverage

**Date:** 2026-08-08
**Scope:** Track B of the parallel V2 TODO closeout — the blob HTTP-adapter
test gap only (`GET/POST /api/v1/blob`, `GET/DELETE /api/v1/blob/{blob_id}`).
Auth/session/setup/restart coverage under V2-057 belongs to a different
track and was not touched.

**Starting commit:** `2fd5e5d8d736f4308b6d69b634a2ef557814f03d` (worktree
branch `worktree-agent-acfa6d97e18e32b2f`, based on `master`).

## What was closed

- **V2-053**, the one open checkbox ("Verify exact binary behavior and status
  codes"): now checked.
- **V2-057**, the two sub-bullets in scope ("Test every route with valid,
  missing, extra, wrong-type, wrong-content-type, oversized, unauthorized,
  expired-session, malformed-path, and method-error cases" and "Test exact
  response schemas and status codes"): text updated to reflect blob is now
  covered; **checkboxes left unchecked** because both bullets are
  multi-route items and the non-blob parts (session-response composition,
  device-restart, setup-conflict handling in `web_api_administration.c`)
  remain uncovered — that is explicitly another track's responsibility per
  the task assignment, not something I could honestly claim closed.

## The gap, and why it existed

Before this change, `firmware/components/web_server/web_server_blob.c` (the
four blob HTTP handlers: `blob_list_handler`, `blob_create_handler`,
`blob_load_handler`, `blob_delete_handler`) was not compiled into any host
test target. Only the underlying `storage_blob_*` core (storage layer, no
HTTP) had host coverage, via `storage_blob_tests` /
`storage_blob_upload_tests` / `storage_blob_access_tests` (V2-034).

This is because `web_server_blob.c` includes ESP-IDF's real
`esp_http_server.h`, which transitively pulls in `freertos/FreeRTOS.h`,
`http_parser.h`, `sdkconfig.h`, and `esp_event.h` — none of which compile or
link on a host build. No fake for `esp_http_server` existed anywhere in this
codebase (confirmed by a repo-wide search before starting); every other
"httpd adapter" file in `web_server/` (`web_server_common.c`,
`web_server_api.c`, `web_server_setup.c`, `web_server_async.c`) has the same
property and is likewise absent from every host test target. This is a
deliberate, project-wide architectural boundary (business logic split into
host-testable `web_*.c` core files with injected `ops` structs; the thin
httpd-calling glue in `web_server_*.c` is untested at the host level), not
specific to blob — the same note already existed in TODO_V2.md for
`web_api_administration.c`.

A second, independent gap: `storage_blob_upload_commit()`'s
`persist_next_id()` (in `firmware/components/storage/storage_blob_upload.c`)
unconditionally returns `APP_ERROR_STORAGE_UNAVAILABLE` when `ESP_PLATFORM`
is not defined (no NVS on host), so even if `web_server_blob.c` could be
compiled, calling the real, production `storage_blob_upload_begin/commit`
functions on a host build can never succeed. Likewise
`storage_partition_capacity()` lives in `storage_mount.c`, which includes
ESP-IDF's `esp_littlefs.h` and does not compile on the host at all.

## What was built

New host-test-only infrastructure, entirely under `tests/host/`:

- `tests/host/fakes/esp_http_server_stub/esp_http_server.h` — a minimal
  stand-in for ESP-IDF's `esp_http_server.h`, providing only the
  `httpd_req_t` fields and function prototypes `web_server_blob.c` /
  `web_server_common.c` actually reference. Resolved via include-path
  ordering scoped to the new test target only; no other target is affected.
- `tests/host/fakes/fake_httpd.c` / `.h` — implements the `httpd_req_recv`,
  `httpd_req_get_hdr_value_len/str`, `httpd_resp_set_hdr/status/type`,
  `httpd_resp_send`, `httpd_resp_send_chunk`, `httpd_resp_send_err`
  functions declared by the stub header, backed by a scriptable
  `fake_httpd_request_t` (request body/headers in, response
  status/headers/body/chunking out) reachable through `httpd_req_t.aux`.
- `tests/host/fakes/fake_storage_blob.c` / `.h` — a link-time test double
  providing the real symbol names `storage_blob_list`,
  `storage_blob_upload_begin/write/commit/abort`,
  `storage_blob_reader_open/read/close`, `storage_blob_delete`, and
  `storage_partition_capacity`, backed by an in-memory record store with
  per-call error injection. This exists because (as above) the real
  production implementations of these functions cannot link on a host
  build; V2-034's storage tests already cover the real filename/atomic-
  commit/capacity logic, so this fake only needs to honor the
  `storage_blob.h` contract closely enough to prove the HTTP adapter wires
  status codes, content, and bytes correctly against it. Must never be
  linked alongside the production `storage_blob*.c` sources (it provides
  colliding symbol names by design).
- `tests/host/test_web_server_blob.c` + `test_web_server_blob_fixture.inc` +
  `test_web_server_blob_{list,create,load,delete}.inc` — 35 test functions
  that call `blob_list_handler` / `blob_create_handler` / `blob_load_handler`
  / `blob_delete_handler` directly (the real, unmodified production
  functions in `web_server_blob.c`) against the fakes above. The fixture
  also supplies two small test doubles, both declared by real headers and
  necessarily provided somewhere since their real implementations don't
  link on host:
  - `auth_session_validate` — real implementation lives in `auth.c`, which
    (like every other NVS/FreeRTOS-backed "outer" component file) is not
    compiled for host tests. The rest of the auth chain
    (`authorize_mutation`, `web_cookie_extract_session`,
    `web_adapter_authorize_mutation`) is the **real, production code**,
    compiled from `web_server_common.c` / `web_cookie.c` /
    `web_server_adapter_body_auth.c` — only the deepest session-validity
    check is stubbed, which is exactly the auth track's territory, not
    blob's.
  - `web_api_send_status_error` — real implementation lives in
    `web_server_api.c`, which also pulls in `device_controls`, `esp_system`
    (`esp_restart`), and the full `/api/v1/*` dispatch pipeline, all out of
    scope for a blob-focused target. The double here is a direct,
    line-by-line mirror of the production function's logic (status text,
    `Cache-Control`, `application/json` type, body send) built from the
    same real `web_api_response_error()` (compiled, unmodified).
- `tests/host/CMakeLists.txt` — one new `web_server_blob_tests` target
  (label `web`) compiling the real `web_server_blob.c`,
  `web_server_common.c`, `web_server_adapter_common.c`,
  `web_server_adapter_body_auth.c`, `web_server_adapter_static_stream.c`,
  `web_server_adapter_json.c`, `web_cookie.c`, `web_content.c`,
  `web_static_path.c`, `web_api_core.c`, `web_api_response.c`,
  `support/subsystem_health.c`, `macro_model/app_uuid.c`,
  `macro_model/app_error.c` alongside the two new fakes.

**No file under `firmware/` was modified.** `web_server_blob.c` and its
header are exercised exactly as they exist in production; nothing was
changed to make them testable.

## What the 35 tests actually verify

- **Status codes observed at the wire**: 200, 201, 204, 400, 401, 404, 413,
  415, 500, 503, 507, produced by the real handler's own
  `blob_error_status()`/`web_api_http_status_for_error()` mapping, not
  reimplemented by the tests.
- **Content types**: `application/json` on list/create responses,
  `application/gzip` on load responses (`blob_create_success_round_trips_bytes`,
  `blob_load_success_streams_exact_bytes_across_chunks`).
- **Byte-identical round trips through the handler** (not just the core):
  - `test_blob_create_success_round_trips_bytes` POSTs a 2500-byte gzip body
    fed to the fake in 333-byte `httpd_req_recv` chunks, and asserts the
    fake storage backend received the exact same bytes
    (`TEST_CHECK_EQ_BUFFER`).
  - `test_blob_load_success_streams_exact_bytes_across_chunks` seeds a
    3000-byte blob (larger than `WEB_ADAPTER_STATIC_CHUNK_BYTES` = 1024, so
    the real `web_adapter_stream_file()` chunking loop runs multiple times)
    and asserts the reassembled HTTP response body is byte-identical to
    what was seeded.
- **Request-id wiring**: explicit `X-Request-ID` echoed back exactly;
  auto-generation via the real `app_uuid_generate()` when absent; rejection
  of a malformed `X-Request-ID` (400) before any other check runs.
- **Auth**: 401 with no `Cookie` header and 401 with a syntactically valid
  cookie whose session the (stubbed) validator rejects, for all four
  handlers — through the real cookie-parsing code, not a shortcut.
- **Content-type validation on upload**: `application/json`,
  `application/gzip; charset=binary` (rejected — the real
  `web_api_content_type_is_gzip()` disallows parameters), and a missing
  header all produce 415; a valid `application/gzip` accepts.
- **Size boundaries**: empty body → 400; `content_len` one byte over
  `APP_V2_BLOB_MAX_BYTES` → 413, verified never to reach the storage
  backend at all.
- **Storage-backend error mapping**: forced `APP_ERROR_STORAGE_FULL` at
  commit → 507 with no record left behind (upload aborted/cleaned up, per
  the real `abort_uncommitted_upload()` path); forced write/backend I/O
  failure → 500 with cleanup; forced begin/reader-open/list/delete
  unavailability → 503 or 500 per the real mapping table.
- **Malformed path**: leading-zero, bare `"0"`, non-digit, and `..`
  traversal blob ids all rejected with 400 by the real
  `web_api_parse_blob_id()`.
- **Not-found**: unseeded blob id → 404 on both load and delete.

Matrix items that do not apply to blob and are noted as such rather than
silently skipped: "extra"/"wrong-type" fields (blob request/response bodies
are raw bytes or a small fixed JSON shape, not a user-supplied JSON object
with variable fields) and per-handler "method-error" (each blob handler is
registered for exactly one HTTP method in `web_server_lifecycle.c`; method
routing/405 is a `web_api_core.c` concern already covered by
`web_api_core_tests`, not something `blob_*_handler` itself decides).

## Honest limitations / what was not done

- The `esp_http_server` fake and the `storage_blob` backend fake are new,
  scoped only to this target. They are **not** reused by or wired into any
  other route's tests — `web_api_administration.c` (session/restart/
  setup-conflict) remains uncompiled by any host test target, exactly as
  before. That work belongs to whichever track owns V2-057's non-blob
  bullets.
- `storage_partition_capacity` and `auth_session_validate` are test doubles,
  not the real production implementations (impossible to link on host, as
  explained above). The real storage_blob filename/next-id/atomic-commit/
  capacity logic already has host coverage from V2-034
  (`storage_blob_tests`, `storage_blob_upload_tests`,
  `storage_blob_access_tests`); this change does not duplicate or replace
  that coverage, it tests the HTTP layer's use of the contract.
- No hardware was used. This is host-test-only evidence; it says nothing
  about behavior on the reference ESP32-S3R8, real `esp_http_server`, real
  NVS, or real LittleFS.
- `./scripts/check-firmware.sh` (clang-tidy via the ESP-IDF esp-clang
  toolchain) could **not** be run in this session: the harness this agent
  ran in refuses any `source`/`.` invocation as a worktree-isolation safety
  measure, and `check-firmware.sh` requires the ESP-IDF environment
  (`. "$HOME/esp/esp-idf-v5.5.5/export.sh"`) to be sourced first. This is an
  environment limitation, not a result. Mitigating fact: **zero files under
  `firmware/` were changed** by this track (`git diff --stat` against the
  starting commit touches only `tests/host/` and `docs/`), so clang-tidy's
  first-party findings over `firmware/` cannot have changed. This is
  reported rather than silently skipped; whoever next has a working
  ESP-IDF-sourced shell should still run
  `./scripts/check-firmware.sh` before merging to confirm.

## Commands run and results

All from the repo root, in this worktree.

```console
$ ./scripts/run-tests.sh web
...
100% tests passed, 0 tests failed out of 18
Label Time Summary:
web    =   0.05 sec*proc (18 tests)
```

New test `web_server_blob` (35 assertions across 35 `TEST_CHECK*`-driven test
functions, see list in `tests/host/test_web_server_blob.c`'s `main()`) is
part of this 18/18 `web`-labeled group and passed.

```console
$ ./scripts/run-tests.sh --sanitizers web
...
100% tests passed, 0 tests failed out of 18
```

ASan+UBSan build of the same 18 `web` tests, including the new fakes'
`malloc`/`free` usage in `fake_storage_blob.c` — clean (no leaks, no UB
detected).

```console
$ ./scripts/run-tests.sh
...
100% tests passed, 0 tests failed out of 49
```

Full host suite (all 49 registered tests across every label) — unaffected by
this change outside the new `web_server_blob` target.

```console
./scripts/check-format.sh
```

clang-format / shfmt / shellcheck / cmake-format / cmake-lint / yamllint /
actionlint all clean for the changed files (`tests/host/CMakeLists.txt`
reformatted in place by `cmake-format -i`; the five new `.c`/`.h`/`.inc`
files reformatted in place by `clang-format -i` before the final clean run).
The webapp `format:check` step failed with `prettier: not found` — this
environment has no `webapp/node_modules` installed (`npm --prefix webapp ci`
was never run in this worktree) and this track made no changes under
`webapp/`; this is a pre-existing environment gap, not a result of this
change.

```console
./scripts/check-firmware.sh
```

**Not run** — see "Honest limitations" above (sandbox blocks the required
`source` of ESP-IDF's `export.sh`). No files under `firmware/` were changed.

## Files changed

- `docs/TODO_V2.md` — closed V2-053's one open checkbox; updated (without
  checking) the two blob-relevant V2-057 sub-bullets to note blob coverage
  while leaving the non-blob gaps in those same bullets accurately described
  as still open.
- `tests/host/CMakeLists.txt` — added the `web_server_blob_tests` target.
- `tests/host/fakes/esp_http_server_stub/esp_http_server.h` (new)
- `tests/host/fakes/fake_httpd.c` (new)
- `tests/host/fakes/fake_httpd.h` (new)
- `tests/host/fakes/fake_storage_blob.c` (new)
- `tests/host/fakes/fake_storage_blob.h` (new)
- `tests/host/test_web_server_blob.c` (new)
- `tests/host/test_web_server_blob_fixture.inc` (new)
- `tests/host/test_web_server_blob_list.inc` (new)
- `tests/host/test_web_server_blob_create.inc` (new)
- `tests/host/test_web_server_blob_load.inc` (new)
- `tests/host/test_web_server_blob_delete.inc` (new)
- `docs/implementation-v2/V2_053_057_BLOB_ROUTE_TEST_COVERAGE_2026-08-08.md`
  (this file)

No unchecked TODO_V2.md task is being claimed complete by this report beyond
the one V2-053 checkbox explicitly closed above.
