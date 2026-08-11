# H6-061 — Async HTTP subsystem health evidence

**Date:** 2026-08-11
**Task:** `H6-061 — Track async subsystem health`
**Implementation SHA:** `8f4ccfae3abf3803f70fc487cb6471039d9d13ab`

## Implemented health model

The existing `http_health` subsystem now records sanitized async failure metadata without changing the frozen v2 diagnostics schema. Internal snapshots retain a first-failure stage plus an `app_error_code_t` for worker start, worker run, worker stop, queue, and async request completion failures.

Raw ESP-IDF/FreeRTOS return values are not exposed. The first async failure wins, so later completion or shutdown fallout cannot erase the earlier actionable cause. Production uses a FreeRTOS critical section around health state; host builds use a real pthread mutex so concurrent host tests exercise exclusion rather than a no-op fake.

## Production instrumentation

`web_server_async.c` records health for worker start/create failures, worker-unavailable dispatch, queue receive/send failures, request allocation and async-handler-begin failures, handler/send failure, async-request completion failure, inconsistent or timed-out worker stop, and stop-signal failure.

The H6-060 worker-unavailable path remains fail-closed with HTTP 503; health recording does not restore any synchronous fallback.

## Diagnostics contract decision

The authoritative diagnostics contract fixes eight subsystem entries and forbids response fields outside the checked-in contract. H6-061 therefore does not add a ninth subsystem or raw failure-detail fields. An async failure degrades the existing `http` subsystem entry to `failed`. A route regression verifies the response still contains exactly eight entries and the existing `http` entry changes to `failed`.

Detailed first-failure stage/error metadata remains internal in `http_health_snapshot()` for diagnostics logic, tests, and future contract-approved use.

## Regression coverage

`test_http_health.c` verifies healthy defaults, sanitized stage/error capture, first-failure-wins behavior, invalid-pair no-op behavior, and a pthread stress test proving concurrent snapshots never observe a torn stage/error pair.

`test_web_server_async_confirmation.c` verifies worker-unavailable confirmation requests record a worker-start health failure while preserving H6-060's 503/no-handler/no-confirmation-wait behavior; a non-confirmation settings request records no async fault.

`test_web_server_administration_route.c` verifies an internal async-completion failure is surfaced through only the existing diagnostics `http` subsystem state while retaining the eight-entry contract.

## Validation

Targeted workflow run **31533545046**, job **93919101444**, ran `./scripts/run-tests.sh web` with **28/28 passed** and `./scripts/run-tests.sh --sanitizers web` with **28/28 passed under ASan + UBSan**. The validator also ran clang-format on changed C/header files and `git diff --check` before committing and pushing the implementation.

## Scope intentionally left open

H6-062 and the remaining H6-063 failure-injection cases are not closed here. H6-061 necessarily begins observing handler/completion/stop-signal results so they can be health-tracked, but H6-062 still requires cleanup-safety proof and H6-063 still requires worker-capable queue/handler/completion/pending-stop/concurrency injections.
