# H6-062 — Async completion-result and cleanup evidence

**Date:** 2026-08-11
**Task:** `H6-062 — Stop ignoring completion results`
**Implementation/test SHA:** `f20b470bb914d3b06bae259a45fb5311dea70cc6`

## Result handling

The runtime result-observation code was introduced in H6-061 at `8f4ccfae3abf3803f70fc487cb6471039d9d13ab`: `web_api_handle_call_with_body()`, `httpd_req_async_handler_complete()`, and the stop-signal `xQueueSend()` are all checked and mapped into sanitized async HTTP health. H6-062 adds the worker-capable proof that those observations preserve request/socket cleanup semantics instead of only recording counters on dead paths.

## Worker-capable host seam

`test_web_server_async_results.c` links the real `web_server_async.c` and supplies a deliberately narrow pthread-backed model for exactly the FreeRTOS primitives it uses: a one-slot queue, binary semaphore, task, and critical section. This is not a general FreeRTOS emulator. It exists so the real worker loop, async clone/completion path, in-flight ownership, and stop sentinel can execute under deterministic fault injection.

The older `web_server_async_confirmation` target remains a deliberate H6-060 dead-path test: its FreeRTOS/httpd async functions are still hard-failure canaries and it never starts the worker.

## Cleanup-safety regressions

The new worker-capable target proves four properties:

1. A handler/send failure is health-recorded as `WORKER_RUN` and still calls `httpd_req_async_handler_complete()` exactly once before the worker is stopped.
2. An async-completion failure is observed as `COMPLETION`; the worker still releases in-flight ownership and can complete bounded shutdown.
3. A dispatch queue-send failure sends the visible 503 path, completes the already-cloned async request, releases in-flight ownership, and permits a subsequent request to enter the worker rather than leaving a phantom pending confirmation.
4. A stop-signal send failure is returned as `APP_ERROR_TIMEOUT` and health-recorded as `WORKER_STOP` without destroying queue/semaphore/task ownership; a later stop retry succeeds and performs cleanup.

## Validation

Targeted workflow run **31534770326**, job **validate-and-record**, ran `./scripts/run-tests.sh web` and `./scripts/run-tests.sh --sanitizers web`. Both suites passed with the new `web_server_async_results` test included. The workflow also ran clang-format on the changed C/header files and `git diff --check` before committing.

## Scope

This closes H6-062 only. The H6-063 matrix remains open for its explicit pending-confirmation bounded-stop and unrelated-status responsiveness/concurrency cases; the new worker-capable seam is intended to support those next.
