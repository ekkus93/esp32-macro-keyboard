# H7-072 — Executor error classification evidence

**Date:** 2026-08-11
**Task:** `H7-072 — Correct error classification`
**Regression SHA:** `db5ebc475be8a21a9f18932fdbd5827b706a9a70`

## Result

The production correction was already present in commit `48d7714d9f48621e1876c4ef3d434826542c6710`: a latched-unavailable executor returns `APP_ERROR_INTERNAL` from submit/cancel/confirm instead of the unrelated `APP_ERROR_STORAGE_UNAVAILABLE`. Current source contains no `APP_ERROR_STORAGE_UNAVAILABLE` reference anywhere under `firmware/components/macro_executor`.

This task therefore did not invent a new executor error enum or change the API contract. It added explicit boundary regressions proving the existing executor/internal classification survives through the web-send core and live HTTP adapters.

## HTTP disposition

For a generic executor-unavailable condition, the existing API behavior is intentionally fail-closed and sanitized:

- `POST /api/v1/send` -> `500 Internal Server Error`, code `internal`, message `send could not be accepted`.
- `GET /api/v1/send` when status is unavailable for a non-release-fault reason -> `500 Internal Server Error`, code `internal`, message `send status unavailable`.
- `DELETE /api/v1/send` when cancellation cannot be recorded -> `500 Internal Server Error`, code `internal`, message `cancellation could not be recorded`.

The tests assert that `storage_unavailable` does not appear in those response bodies. H7-071's separate release-fault case remains reportable through the existing sanitized `releaseError` field rather than being hidden behind the generic unavailable response.

## Validation

Targeted workflow run **31538431440** ran:

- `./scripts/run-tests.sh executor` — **2/2 passed**.
- `./scripts/run-tests.sh web` — **29/29 passed**.
- `./scripts/run-tests.sh --sanitizers executor` — **2/2 passed under ASan+UBSan**.
- `./scripts/run-tests.sh --sanitizers web` — **29/29 passed under ASan+UBSan**.
- A source guard that fails if `APP_ERROR_STORAGE_UNAVAILABLE` appears under `firmware/components/macro_executor`.
- `clang-format` and `git diff --check` on changed files.

## H7-072 disposition

Both checklist clauses are satisfied: unavailable executor state uses the executor/internal error domain, and the HTTP boundary maps it to generic fixed-vocabulary internal failures without leaking storage semantics or implementation detail.
