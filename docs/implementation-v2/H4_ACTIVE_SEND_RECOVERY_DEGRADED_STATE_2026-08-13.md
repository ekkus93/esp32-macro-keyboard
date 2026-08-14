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

## 2026-08-14 literal-checklist re-audit

A later H4 closure review compared the permanent tests against every unchecked
H4-043/H4-044 sentence rather than treating the earlier green H4 browser runner
as broader proof than it actually supplied. That review found two proof gaps:

1. the browser runner exercised degraded tracking in-place, but did not perform a
   real reload while an active send existed and force the first startup recovery
   request to fail; and
2. the runner did not dirty the working copy before execution degradation, so it
   did not literally prove that recovery failure/Retry leaves current in-memory
   edits intact.

The runtime state machine itself did not need another implementation change.
Commit `1ab7993cf460917ae89320628f16df6c08db2770` strengthens only permanent H4
coverage:

- `webapp/tests/v2-execution-recovery.test.tsx` now proves one transient poll
  failure remains quiet and clears on the next successful refresh, and proves a
  terminal reconciliation completes through the original callback path while
  clearing the fail-closed recovery latch;
- `webapp/tests/browser/run-h4-recovery-tests.mjs` now dirties the working copy
  before starting the send, proves both degradation and GET-only Retry preserve
  that dirty state, then performs a real browser reload with an active send and
  forces the first startup send-status recovery to return `503`;
- the reloaded UI must represent that state as **Execution state unavailable**,
  keep **Cancel and release all keys** visible, and recover through the
  authenticated shell's GET-only Retry without issuing a second send POST.

The reload assertion does not claim that unsaved in-memory edits survive a full
browser reload. A reload naturally reconstructs the working copy from the
canonical device snapshot; the dirty-state preservation assertion applies to the
in-place degraded/reconciled execution path, exactly where H4 requires recovery
UI not to discard current tab state.

### Local validation of the strengthened coverage

The sandbox still has Node.js 22.16.0 rather than the repository-pinned Node.js
24.18.0. The following non-authoritative checks passed on the strengthened test
files:

- `node --check webapp/tests/browser/run-h4-recovery-tests.mjs`;
- TypeScript 5.8.3 `transpileModule(..., reportDiagnostics: true)` for
  `webapp/tests/v2-execution-recovery.test.tsx`;
- `bash tests/scripts/test-check-frontend-persisted-state.sh` — 6/6; and
- byte/line-ending checks proving both changed test files end in one LF and do
  not contain CRLF line endings.

An offline `npm ci` attempt was also made with engine enforcement relaxed only
for dependency installation diagnostics. It failed because the local npm cache
is missing `yocto-queue-0.1.0`; no dependency tree was fabricated and no result
from Node 22 is being called authoritative.

### Current disposition

The earlier descendant validation supplied by the product owner proved the
pre-re-audit H4 runner reached **Real Chrome H4 degraded-send recovery workflow
passed** through the fail-fast frontend gate. That evidence remains useful, but
it predates the new literal reload/dirty-state assertions above.

Therefore H4 remains open pending one pinned Node.js 24.18.0 frontend/real-Chrome
pass on an exact descendant containing
`1ab7993cf460917ae89320628f16df6c08db2770`. No H4 TODO checkbox is closed from
source inspection or the older narrower browser result.
