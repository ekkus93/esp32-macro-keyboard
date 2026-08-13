# H6-060 — Async confirmation fail-closed evidence

**Date:** 2026-08-11  
**Task:** `H6-060 — Remove synchronous fallback`  
**Pre-fix product baseline:** `8575dbe05f5ef7a20d154eca74f5fe601a944834`  
**Implementation SHA:** `30301a89cef655c9bf6420c1192c19bdb2f3a09c`

## Defect

Before H6-060, `web_server_async_dispatch()` treated an unavailable confirmation worker as permission to execute confirmation-gated work synchronously on the main ESP-IDF httpd task. The code explicitly described that behavior as preferable to failing even though it could block the single server task for the physical-confirmation window.

That fallback was unsafe for the hardened contract: failure of the async confirmation subsystem changed the execution model silently, could make unrelated HTTP work unresponsive for the confirmation timeout, and made the service failure look like ordinary confirmation processing.

## Implemented behavior

Commit `30301a89cef655c9bf6420c1192c19bdb2f3a09c` removes the synchronous fallback from `firmware/components/web_server/web_server_async.c`.

When either the async queue or async worker task is unavailable, `web_server_async_dispatch()` now:

1. returns an explicit `503 Service Unavailable`,
2. emits the normal v2 error envelope with sanitized code `internal` and message `confirmation service unavailable`,
3. does not call `web_api_handle_call()` on the httpd task,
4. does not call `device_controls_wait_for_confirmation()`,
5. does not execute the protected administration operation,
6. does not restart the device, and
7. leaves unrelated, non-confirmation-gated routes on their normal synchronous route path.

The fail-closed response is produced before any queue/task/async-httpd operation when the worker is absent. There is therefore no confirmation timeout wait on the main httpd task in this failure mode.

## Regression coverage

`tests/host/test_web_server_async_confirmation.c` was deliberately changed from a test that exercised and therefore blessed the old synchronous fallback into a fail-closed regression target.

The test links the real `web_server_async.c` but intentionally does not call `web_server_async_start()`. Its FreeRTOS queue/task and async-httpd stubs remain hard-failure canaries: if the worker-unavailable path ever reaches those primitives, the test fails instead of pretending to emulate FreeRTOS concurrency.

The regression verifies all of the following with the physical-confirmation setting enabled:

- `POST /api/v1/device/restart` -> `503`; zero confirmation waits; zero restart handler calls; zero `esp_restart()` calls.
- `POST /api/v1/settings/change-password` -> `503`; zero confirmation waits; zero session invalidation calls; no `Set-Cookie` mutation.
- `POST /api/v1/device/reset-settings` -> `503`; zero confirmation waits; zero reset-settings handler calls; zero restart calls.
- `POST /api/v1/device/factory-reset` -> `503`; zero confirmation waits; zero factory-reset handler calls; zero restart calls.
- `GET /api/v1/settings`, which is not confirmation-gated, remains usable and returns `200 OK` while the confirmation worker is unavailable.

This test intentionally proves only the **worker-unavailable fail-closed boundary**. It does not claim to model the real FreeRTOS worker, queue, async request lifetime, queue-failure, completion-failure, or stop behavior. Those remain under H6-061/H6-062 and the remaining H6-063 cases.

## Validation

The implementation was applied and validated in targeted workflow run **31530721738**, job **93909820931**, before the validated product commit was pushed to `master`.

Commands and results:

```text
./scripts/run-tests.sh web
```

Result: **28/28 passed**, including `web_server_async_confirmation`.

```text
./scripts/run-tests.sh --sanitizers web
```

Result: **28/28 passed under ASan + UBSan**, including `web_server_async_confirmation`.

The validator also ran `clang-format` on the changed C/header files and `git diff --check` successfully before committing. The targeted runner used Ubuntu 24.04, GCC 13.3.0, clang-format 18, and libcjson 1.7.17.

The temporary validator workflows were deleted by the validated product commit and are not part of the resulting product tree.

## H6-060 acceptance mapping

- **Delete synchronous fallback:** satisfied by `30301a89cef655c9bf6420c1192c19bdb2f3a09c`.
- **Return explicit 503 or contract-consistent equivalent:** satisfied; exact host regression asserts `503 Service Unavailable`.
- **Do not bypass physical confirmation:** satisfied for worker-unavailable dispatch; protected operations are not executed and the confirmation wait is not invoked.
- **Do not block the whole server for the confirmation timeout:** satisfied for worker-unavailable dispatch; it fails immediately instead of entering the synchronous confirmation path.
- **H6-063 worker-unavailable regression:** satisfied by the dedicated fail-closed host test and sanitizer run.

## Scope still open

H6 as a phase is **not complete**. This evidence does not close:

- H6-061 async subsystem health,
- H6-062 observation/preservation of handler, async-completion, and stop-signal failures,
- H6-063 queue failure, handler failure, completion failure, pending-confirmation stop, or concurrent unrelated-request responsiveness,
- the H6 phase exit gate.

Historical v2 implementation reports that describe the old worker-unavailable synchronous fallback are preserved as historical evidence rather than rewritten. This report records the current post-H6-060 behavior.
