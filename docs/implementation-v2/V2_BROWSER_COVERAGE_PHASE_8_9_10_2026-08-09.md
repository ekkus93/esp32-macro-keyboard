# Closing browser-test coverage gaps — Phase 8, 9, and 10 exit gates (2026-08-09)

**Scope:** three specific, previously verified-real gaps in
`webapp/tests/browser/run-browser-tests.mjs`'s real-Chrome (Playwright)
coverage, named by the Phase 8, 9, and 10 exit gates in `docs/TODO_V2.md`. No
other file was touched except that TODO's three exit-gate checkbox lines.

**Base commit:** `e7a7546` ("Merge browser test harness migration: hand-rolled
CDP to Playwright") — confirmed as `HEAD` (or an ancestor of it) before any
work started, and `run-browser-tests.mjs` confirmed to already use real
Playwright (`chromium.launch()`, no hand-rolled `class Cdp`) at that commit.

**Method:** re-verified each of the three gaps against current code before
writing anything (per the task's explicit instruction not to trust the prior
audit blindly), read the actual production source for every screen/behavior a
new scenario would touch (not just the earlier implementation reports), then
added new scenario code, updated the three `docs/TODO_V2.md` exit-gate lines
with citations, and ran the full verification matrix below.

## Gaps confirmed still real, before writing anything

1. **Phase 8** (`docs/TODO_V2.md`, "Phase 8 exit gate"): the shared fixture
   server in `run-browser-tests.mjs` booted a single fixed scenario —
   `provisioned: true`, `GET /api/v1/auth/session` always `authenticated: true`,
   exactly one pre-existing blob from the first request onward. Nothing varied
   provisioning, session validity, blob count, or blob validity. "Refresh" and
   "send recovery" were the two exceptions — already covered by the existing
   `runBrowserWorkflows`'s mid-send `page.reload()` — but "first phone,"
   "expired session," "no blobs," and "invalid newest blob" had zero
   real-browser coverage.
2. **Phase 9** (`docs/TODO_V2.md`, "Phase 9 exit gate"): 10 of 12 named
   scenarios were covered; "USB unavailable" and "rapid repeated input" were
   explicitly and deliberately excluded, with a doc comment giving reasons.
   Investigation (see below) found both reasons were stale relative to the
   current implementation.
3. **Phase 10** (`docs/TODO_V2.md`, "Phase 10 exit gate"): zero browser
   coverage existed for the macro editor or package-management page — the
   harness only ever exercised the Quick Send/Macros-list flow inherited from
   Phases 8/9.

## Why the Phase 9 doc comment's stated reasons no longer held

The old comment above `runBrowserWorkflows` said:

- USB unavailable "would need a slow, real 5-second device-status poll cycle
  to flip mid-test." False: `useDeviceStatus.ts` calls `poll()` immediately on
  mount, before its `setInterval` is even registered — a fixture can simply
  start with `usb.state` already non-`ready`, no poll-flip choreography
  needed. The 5-second interval only matters for a scenario that changes USB
  state *while the page is already open* (which the new coverage also
  exercises, for the recovery direction).
- Rapid repeated input's "same-tick double-dispatch race is not reproducible
  over a real browser round trip." False in the relevant sense: the guard
  (`MacrosPage.tsx`'s `startingRef`, V2-095) is a plain ref set synchronously
  at the very top of `startSend`, before any `await` — it needs neither a
  React re-render nor a completed network round trip to work. Playwright's own
  `locator.click()` genuinely can't reproduce a same-tick double dispatch
  (each call is its own separately-awaited CDP round trip), but
  `page.evaluate()` runs its function body synchronously on the browser's own
  JS thread — two `.click()` calls inside one `evaluate()` invoke React's
  `onClick` handler twice in the same task, which is exactly the race the
  guard withstands.

## What was added

All new code lives in `webapp/tests/browser/run-browser-tests.mjs`. Nothing in
`startApplicationServer` (the existing shared fixture) or any of the existing
`run*Workflows` functions' bodies was restructured — only additive new code,
plus one new scenario block inserted into `runBrowserWorkflows` and one
updated doc comment above it.

### New fixture server: `startStartupFixtureServer(options)`

A second, independently configurable Node HTTP fixture server, separate from
`startApplicationServer`. It represents device states the shared fixture
cannot (unprovisioned, session-invalidated, blob-less, a corrupt newest blob,
a non-ready USB state from first load) without changing behavior the
Phase 9/11/12 workflows already depend on. Options: `provisioned`,
`authenticated`, `blobs`, `usbState`. Implements `GET`/`POST /api/v1/setup`,
`GET /api/v1/auth/session`, `POST /api/v1/auth/login`, `GET`/`PUT
/api/v1/settings`, `GET /api/v1/status`, `GET`/`POST /api/v1/blob`,
`GET /api/v1/blob/:id`, `GET`/`POST /api/v1/send`, and static file serving —
plus a full `requestLog` of every `/api/*` request in order, letting scenarios
assert the *sequence* of requests, not just the resulting page text.

### Phase 8 — five new scenario functions, each its own fixture + browser context

- `runStartupFirstPhoneScenario` — an unprovisioned, unauthenticated, blob-less
  device: First-Run Setup (`#setup-code`, `#device-name`, `#ap-ssid`,
  `#ap-passphrase`, `#admin-password`) → Review setup → Apply setup → Setup
  complete → Sign In → Create Your First Repository → an empty Macros page,
  asserting zero blobs were uploaded before an explicit Save snapshot.
- `runStartupRefreshAndSendRecoveryScenario` — covers "refresh" and "send
  recovery" together (the exit gate names them as a pair; both are proven by
  the same mechanism, a real `page.reload()`). Starts a send, reloads
  mid-send, and — beyond what Phase 9's own reload scenario already proves —
  asserts via the request log that the *entire* startup sequence
  (`/api/v1/setup`, `/api/v1/auth/session`, `/api/v1/settings`,
  `/api/v1/blob`, `/api/v1/blob/{id}`, `/api/v1/send`) genuinely re-runs after
  a real reload, and that the recovered send status comes from that
  sequence's own `GET /api/v1/send` step.
- `runStartupExpiredSessionScenario` — dirties the working copy (a keyboard
  reorder), flips the fixture's session to invalid server-side, waits for the
  app's own status poll to surface the resulting `401` and drop to Sign In
  *without reloading the page*, re-authenticates, and asserts zero new
  `GET /api/v1/blob/*` calls happened — proving the same in-memory working
  copy resumed (UI_UX_SPEC_V2 §3.3/§7.3), not a fresh load.
- `runStartupNoBlobsScenario` — a signed-in device with zero stored blobs:
  Create Your First Repository → an empty, unsaved Macros page, zero blobs
  uploaded.
- `runStartupInvalidNewestBlobScenario` — a corrupt newest blob (`id: "2"`,
  non-gzip bytes) alongside a valid older one (`id: "1"`): asserts Snapshot
  recovery appears with the corrupt blob identified by ID, explicit recovery
  via the older blob reaches the Macros page, and the corrupt blob is never
  deleted (SPEC_V2 §9.6).

`runStartupWorkflows(browser)` runs all five in sequence.

### Phase 9 — two additions

- A new scenario block inside the existing `runBrowserWorkflows`, right after
  the existing Quick Send assertion: three synchronous `.click()` calls inside
  one `page.evaluate()` on the "Send Open terminal" button, asserting exactly
  one `POST /api/v1/send` fired (via a before/after delta on
  `serverState.sendPostCount`, so it doesn't disturb the existing absolute-count
  assertion earlier in the same function).
- A new `runUsbUnavailableWorkflow(browser)`, against its own
  `startStartupFixtureServer({ usbState: "disconnected" })`: asserts the shell
  header shows `USB disconnected` and every Send button is `disabled` from
  first load, then mutates `fixture.state.usbState = "ready"` and asserts the
  header and Send buttons recover within one real 5-second poll cycle.

### Phase 10 — one new scenario function

`runMacroEditingWorkflows(browser)`, against its own fixture/context (so its
assertions aren't disturbed by the shared fixture's Snapshots/Settings
mutations): Add macro (name, key-press/inter-key fields, a real
focused-textarea directive-insertion click for `{ENTER}`, live validation) →
Save, landing on a dirtied Macros page; edit that macro to an invalid source,
assert the exact error location text, click "Go to error" and assert real
textarea focus, correct and save; a second Add-macro draft, discarded via
Cancel, asserted never to appear; Package management (create, rename,
duplicate, keyboard-reorder, name-bearing two-step-confirm delete, Open); and
a final `page.reload()` while the working copy is dirty, asserting the native
`beforeunload` dialog (auto-accepted via `page.on("dialog", ...)`, registered
on this scenario's own context) doesn't hang the page — the same defect
`V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md` found and fixed in
this harness previously.

## Verification

```bash
node --check tests/browser/run-browser-tests.mjs        # syntax
npm run format:write                                     # 1 file reformatted (the new code)
npm run format:check                                      # clean afterward
npm run lint                                               # clean, 0 warnings
npm run build                                               # clean
node tests/browser/run-browser-tests.mjs                    # run 1 (standalone)
node tests/browser/run-browser-tests.mjs                    # run 2 (standalone, in a loop of 4)
node tests/browser/run-browser-tests.mjs                    # run 3
node tests/browser/run-browser-tests.mjs                    # run 4
node tests/browser/run-browser-tests.mjs                    # run 5
npm run test                                                 # Vitest, 577/577 passed, 59 files
./scripts/check-webapp.sh                                    # full chain, run 1: exit 0
./scripts/check-webapp.sh                                    # full chain, run 2: exit 0
```

All 5 standalone `node tests/browser/run-browser-tests.mjs` runs exited `0`
with the identical six-line success output:

```text
Real Chrome v2 Macros page/Quick Send workflows passed.
Real Chrome v2 Snapshots/import-export workflows passed.
Real Chrome v2 Settings/Diagnostics workflows passed.
Real Chrome v2 USB-unavailable workflow passed.
Real Chrome v2 startup workflows (first phone, refresh, expired session, no blobs, invalid newest blob, send recovery) passed.
Real Chrome v2 macro-editing/package-management workflows passed.
```

No flakiness observed across the 5 runs. Standalone run time: ~44 seconds
(unchanged order of magnitude from before this task; the added scenarios each
spin up their own lightweight Node HTTP server and Playwright browser context,
with two deliberate real-time waits — the expired-session scenario's status
poll and the USB-recovery poll — bounded at a few seconds each).

`npm --prefix webapp run test` (Vitest): 577/577 passed across 59 files,
confirming the existing suite is unaffected.

`./scripts/check-webapp.sh` (the full `ci → typecheck → lint → stylelint →
test → test:coverage → build → test:browser → verify-no-remote-assets` chain):
exit `0` on both runs, including its own `test:browser` step (which rebuilds
and re-runs the exact same browser suite).

## `docs/TODO_V2.md` changes

Three exit-gate lines changed from `[ ]` to `[x]`, each with the evidence
above cited inline:

- Phase 8 exit gate: "Real-browser tests cover first phone, refresh, expired
  session, no blobs, invalid newest blob, and send recovery."
- Phase 9 exit gate: "Macros page browser tests cover idle, USB unavailable,
  quick send, confirmation, progress, cancel, complete, failure, timeout,
  release error, reload, and rapid repeated input."
- Phase 10 exit gate: "Editing and package-management unit and browser tests
  pass."

No other line in any phase was touched.

## What this does not claim

No physical-device or live-firmware validation is claimed — every scenario
above runs against a Node HTTP fixture server, not real ESP32-S3 firmware.
The fixture's `adminPassword` (`"bench-fixture-admin-pw-1"`) is a disposable,
fixture-only string checked by nothing but this in-memory Node server; it is
not a device credential.
