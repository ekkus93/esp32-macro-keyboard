# H6-063 — Async HTTP regression matrix evidence

**Date:** 2026-08-11
**Task:** `H6-063 — Regression tests`
**Implementation/test SHA:** `4e0355f33cb3ea025dbb6525479dc541f7e4af84`

## Matrix closure

H6-060 already proved worker-unavailable requests fail quickly with a visible 503 and never execute confirmation synchronously. H6-062 commit `f20b470bb914d3b06bae259a45fb5311dea70cc6` added the worker-capable seam and proved queue-send failure completes the cloned request and degrades health, handler failure still completes the async request, and async-completion failure is captured in HTTP health.

This H6-063 commit extends that same real-worker seam with two concurrency/shutdown regressions:

1. **Stop while confirmation is pending:** the worker is held deterministically inside the handler boundary representing the physical-confirmation wait. `web_server_async_stop()` runs concurrently, is proven not to finish or destroy ownership while the request remains incomplete, then completes successfully after the blocked handler is released. The async request is completed exactly once and the bounded stop returns without abandoning the socket/request.
2. **Unrelated status responsiveness:** while that same worker is blocked, the real production `status_handler()` runs against its normal HTTP adapter/JSON path and returns `200 OK` in under one second. The confirmation request remains incomplete until explicitly released, proving the confirmation wait is isolated from the main status-serving path rather than blocking it.

The production stop budget remains explicitly bounded at `APP_PHYSICAL_CONFIRM_TIMEOUT_MS + 5000U`, so it outlasts the maximum confirmation wait while preventing an unbounded shutdown wait.

## Validation

Targeted workflow run **31536438755** ran `./scripts/run-tests.sh web` and `./scripts/run-tests.sh --sanitizers web`. Both passed with **29/29** CTest cases, including the expanded `web_server_async_results` executable under normal and ASan+UBSan builds. The workflow also ran clang-format and `git diff --check` before committing.

## Phase H6 disposition

The complete H6 matrix now proves: no worker-unavailable fallback to the single httpd task; async subsystem failures are health-tracked; handler/completion/stop results are observed; cleanup ownership survives failures; shutdown during a pending confirmation is bounded and safe; and unrelated status handling remains responsive while confirmation waits. The H6 exit gates are therefore satisfied on this SHA.
