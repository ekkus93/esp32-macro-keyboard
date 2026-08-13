# H4 Active-send recovery and degraded execution state — 2026-08-13

## Scope

This candidate addresses Phase H4 of
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.
Earlier unchecked H1/H3 items are physical-device evidence gates and are not
claimed by this software work.

## Audit result

The pre-H4 tree already distinguished startup send recovery as `none`, `known`,
or `unavailable`; a failed startup recovery no longer became a false
`null`/"no send" result. USB status polling also retained the last known state,
degraded only after a bounded consecutive-failure threshold, and cleared that
degraded state after a successful refresh.

Two active-send gaps remained:

1. after `sendClient` exhausted its bounded status-poll retries, the owning page
   retained its last active state but had no status-only recovery action; and
2. when `POST /api/v1/send` returned `409` and its follow-up status recovery
   also failed, the page could return to an idle presentation even though the
   backend had just reported an unresolved execution.

The second case could permit another send attempt before execution state had
been reconciled.

## Implementation

### Send-client recovery latch

`webapp/src/v2/sendClient.ts` now owns a single fail-closed execution-recovery
state shared by every send route. It records `unavailable` only after the
existing bounded retry policy is exhausted or when a `409` establishes that an
execution exists but its status cannot be recovered.

While recovery is unresolved:

- `sendMacro()` rejects locally before issuing another POST;
- the last trusted `SendStatusResponse`, when one exists, is retained;
- `retryExecutionRecovery()` performs only `GET /api/v1/send`;
- a true `404` clears the latch as confirmed no-send;
- a terminal recovered result clears the latch and completes through the
  original callbacks when they are available; and
- a nonterminal recovered result immediately restores the original status
  callback before polling resumes, so the owning page cannot remain idle while
  the recovery surface disappears.

For startup recovery, where no page callbacks exist yet, a recovered
nonterminal send stays fail-closed until the existing page-level recovery path
consumes it. This prevents a successful global GET from silently re-enabling
POST before the authenticated shell has adopted the recovered send.

### Cross-route recovery surface

`webapp/src/features/shell/v2/ExecutionRecoveryOverlay.tsx` is mounted beside
`AppV2` from `main.tsx`, so it survives route changes and page-local tracking
failure. It provides:

- a prominent **Execution state unavailable** warning;
- the last known state/progress when available;
- **Retry execution status**;
- **Cancel and release all keys**; and
- explicit success/failure text for cancellation delivery.

The overlay never starts a send. Retry is GET-only and cancellation is DELETE
only.

### Test isolation

The recovery latch is module state by design because send authority is global
to the browser tab. `webapp/tests/h4ExecutionRecoverySetup.ts` resets that state
before each Vitest case, and `vite.config.ts` registers that setup alongside the
existing global test setup.

## Permanent regression coverage

`webapp/tests/v2-execution-recovery.test.tsx` proves:

- three consecutive transient status failures become explicit degraded state;
- last-known execution state is retained;
- Retry reconciles with GET only and restarts polling without a POST;
- `409` plus failed recovery prevents a second POST until reconciliation;
- Retry and Cancel remain visible; and
- cancellation delivery failure is explicit.

`webapp/tests/browser/run-h4-recovery-tests.mjs` is a focused real-Chrome
scenario using the production build. It starts a real send, injects three
consecutive `503` status responses, requires the global degraded surface,
requires Cancel visibility, retries status, requires the warning to clear, and
asserts the server observed exactly one send POST. `package.json` runs this
scenario after the existing browser suite.

## Local validation actually run

The following checks passed in the sandbox on the final small-diff candidate:

- `git diff --check`;
- Node syntax check for `tests/browser/run-h4-recovery-tests.mjs`;
- TypeScript 5.8.3 `transpileModule(..., reportDiagnostics: true)` for all
  changed `.ts`/`.tsx` source and Vitest files;
- Node 22 experimental type-strip syntax check for `sendClient.ts`;
- `python3 scripts/check-h9-architecture.py`;
- `python3 scripts/check-h9-production-audit.py`;
- `bash tests/scripts/test-check-frontend-persisted-state.sh` — 6 cases;
- `bash tests/scripts/test-check-credential-logging.sh` — 18 cases;
- `bash tests/scripts/test-test-assert-redaction.sh` — 3 cases; and
- `python3 scripts/generate-spec-traceability.py --check` using the current
  generated traceability document already present on remote `master`.

The literal pinned frontend suite is not runnable in this sandbox because the
repository requires Node.js 24.18.0 while the available runtime is Node.js
22.16.0. The repository's `engine-strict` policy correctly prevents treating
that environment as authoritative. The permanent Vitest/build/browser changes
are therefore committed for the normal pinned validation layer rather than
being falsely reported as locally executed.

## Disposition

The H4 runtime and permanent regression implementation is ready for pinned
frontend/browser validation. H4 TODO checkboxes and the phase exit gate remain
unchecked until that exact candidate passes the repository's Node 24.18.0
frontend and real-Chrome gates. No hardware claim is made by this evidence.
