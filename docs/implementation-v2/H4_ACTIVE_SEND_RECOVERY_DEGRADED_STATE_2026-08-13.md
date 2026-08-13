# H4 Active-send recovery and degraded execution state — 2026-08-13

## Scope

This implementation candidate addresses Phase H4 of
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.
The earlier H1-H3 unchecked items are physical-device evidence gates, so H4 is
the next locally actionable software phase. This document does not claim those
hardware gates are satisfied.

## Audit result

The current `master` snapshot already distinguished startup send recovery as
`none`, `known`, or `unavailable`; a failed startup recovery no longer became a
false `null`/"no send" result. USB polling also already retained the last known
value, degraded after a bounded failure count, and cleared the degraded state
on recovery.

Two gaps remained in the in-tab send lifecycle. After an active send tracker
exhausted its status retries, `MacrosPage` surfaced an error string but stopped
tracking permanently. The last send remained visible, but there was no
status-only recovery action. In addition, if `POST /api/v1/send` returned `409`
(already sending) and the follow-up status recovery failed, the page collapsed
back to `idle`. That re-enabled Send even though the backend had just confirmed
that an execution existed. Both cases violated H4's unknown-execution and
no-duplicate-send requirements.

## Changes

### Explicit degraded active-send state

`webapp/src/features/macros/v2/MacrosPage.tsx` now has an explicit `degraded`
send lifecycle state. When tracking fails it:

- retains the last known send status and macro identity,
- states that execution status is unavailable and the device may still be
  running the send,
- disables new send starts while execution is unknown,
- keeps **Cancel and release all keys** available,
- exposes cancellation delivery failure explicitly, and
- provides **Retry execution status**.

Retry performs only `GET /api/v1/send` recovery. It never reposts the macro. A
confirmed 404/no-send result returns to idle; a known nonterminal send restarts
tracking; and a known terminal send uses the existing terminal-state path.
Repeated recovery failure leaves the degraded state visible. A `409` send
conflict whose follow-up status GET fails now enters the same degraded state
without inventing a last-known status; Send remains disabled, Retry performs
GET-only reconciliation, and Cancel remains available. If the reconciliation
returns a terminal send, the existing terminal rendering is used rather than
misrepresenting that result as "no send."

### Startup-level cancellation affordance

`webapp/src/AppV2.tsx` now keeps **Cancel and release all keys** next to the
startup-level execution-state-unavailable warning. Successful cancellation
states that status must still be reconciled before treating the send as
terminal. Failed cancellation states explicitly that the request could not be
delivered.

### Regression coverage added

`webapp/tests/v2-macros-page.test.tsx` now covers:

- tracker failure becoming a visible degraded state,
- status-only retry recovering without another send POST,
- repeated recovery failure remaining degraded,
- cancellation remaining reachable, and
- explicit cancellation-delivery failure,
- `409` conflict plus failed status recovery remaining degraded and blocking a
  second POST, and
- terminal state recovered from a `409` conflict rendering as terminal rather
  than as no execution.

`webapp/tests/v2-app-v2.test.tsx` now covers cancellation from startup-level
unknown execution state without discarding the loaded package/working copy.

`webapp/tests/browser/run-browser-tests.mjs` now makes the first status recovery
request fail during an active-send reload, asserts the unknown-execution
warning, retries recovery, and verifies the send POST count does not increase.
The existing package/working-copy assertion remains in that reload workflow.

## Local validation actually run

Passed in the sandbox after the resumed H4 audit:

- `git diff --check`
- `node --check webapp/tests/browser/run-browser-tests.mjs`
- TypeScript parse/transpile of `AppV2.tsx`, `MacrosPage.tsx`, and the two
  changed Vitest files using TypeScript 5.8.3 with diagnostics enabled
- `python3 scripts/check-h9-architecture.py`
- `python3 scripts/check-h9-production-audit.py`
- `bash tests/scripts/test-check-frontend-persisted-state.sh` — 6 cases
- `bash tests/scripts/test-check-credential-logging.sh` — 18 cases
- `bash tests/scripts/test-test-assert-redaction.sh` — 3 cases
- `python3 scripts/generate-spec-traceability.py --check`

The literal frontend gate is not runnable in this sandbox. The repository
enforces Node.js 24.18.0 through `.npmrc`/`package.json`, while this environment
only provides Node.js 22.16.0. `npm ci` therefore fails closed with
`EBADENGINE`; attempting a non-authoritative engine override cannot resolve the
packages in this container. Chromium is installed, but the Playwright/frontend
dependencies are not, so the real-browser runner cannot execute locally.

Accordingly, H4 TODO checkboxes and the H4 exit gate remain unchecked until the
normal pinned frontend/browser validation layer executes successfully. No
hardware claim is made by this evidence.
