# V2-120..V2-122 — Settings, Diagnostics, and destructive-operations UI

**Phase:** 12 — Settings, diagnostics, and destructive operations UI
**Tasks:** V2-120 (Settings UI), V2-121 (Restart and reset flows), V2-122
(Diagnostics UI), plus the remaining 3 of V2-103's 5 unsaved-changes warning
trigger points (Sign Out, reset settings, factory reset).
**Commit:** `5b2061b7fc105ed79081bdc0b2d6d7f975325da5` (worktree branch
`worktree-agent-a58ffefb514330101`), preceded by the starting `master`
commit `e703ff2`.
**Date:** 2026-08-09.

No unchecked TODO_V2.md task is being claimed complete by this report. Where
evidence is partial or a gap remains, it is stated below and left unchecked
in `docs/TODO_V2.md`.

## Scope actually touched

Webapp only, per this track's file-surface restriction. No firmware, host
test, or `tests/v2_contracts/` file was changed. Files:

- New: `webapp/src/features/settings/v2/SettingsPage.tsx`,
  `DiagnosticsPage.tsx`, `DeviceReconnectScreen.tsx`, `useDeviceReconnect.ts`.
- New: `webapp/src/v2/deviceActionsClient.ts`, `diagnosticsClient.ts`,
  `diagnosticsExport.ts`.
- Modified: `webapp/src/AppV2.tsx` (wires the two new routes and the
  device-reconnect state), `webapp/src/features/shell/v2/AppShellV2.tsx`
  (nav-highlight for `diagnostics`), `webapp/src/v2/apiClient.ts` (adds
  `v2PostNoContent`, `forceSessionEnded`; fixes `v2PostJson`/`v2PostNoContent`
  for a genuinely bodyless POST), `webapp/src/v2/routingV2.ts` (adds the
  `diagnostics` screen and two navigation helpers), `webapp/src/v2/settingsClient.ts`
  (adds `updateSettings`/`changePassword`).
- Deleted: `webapp/src/features/shell/v2/PlaceholderPage.tsx` (its one
  remaining caller, the `"settings"` route case, is now real).
- Test files: 8 new Vitest files, 4 modified (`v2-api-client.test.ts`,
  `v2-app-shell.test.tsx`, `v2-app-v2.test.tsx`, `v2-routing.test.ts`), plus
  `webapp/tests/browser/run-browser-tests.mjs` (new fixture routes and a
  `runSettingsWorkflows` real-Chrome workflow).
- `docs/TODO_V2.md`: only the V2-103, V2-120, V2-121, V2-122, and Phase 12
  exit-gate checkbox lines, per the assigned file surface.

## What each task actually got, and how it was verified

### V2-120 — Settings UI

Every listed field is implemented in `SettingsPage.tsx` as one of four
independent forms/sections, each submitting only through `PUT
/api/v1/settings` (device identity/behavior, access point, station,
password change via `POST /api/v1/settings/change-password`) or a plain
button (Sign Out, Diagnostics link, the danger-zone actions). None of these
forms touch `RepositoryWorkingCopyStore` — verified directly by a test that
asserts `store.getIsDirty()` stays `false` after a successful identity-form
submit.

`lastSelectedPackageId` never appears as an editable field; a dedicated test
renders the settings page with a real UUID set as the current selection and
asserts no `<input>` anywhere on the page has that UUID as its value and the
page's text content never contains it verbatim.

Evidence: `webapp/tests/v2-settings-page.test.tsx` (13 tests covering
pre-fill, identity submit, access-point requiring both fields together plus
the restart notice, station connect/remove, password-confirmation
mismatch/match and session end, sign-out clean vs. dirty, restart confirm,
reset-settings dirty-then-typed-phrase, factory-reset typed-phrase +
password, factory-reset dirty-then-export-then-discard, and a device-action
failure path), `webapp/tests/v2-settings-client.test.ts` (5 tests for
`getSettings`/`updateSettings`/`changePassword` against the real wire
contract), and the `AppV2` integration tests below.

### V2-121 — Restart and reset flows

Restart, reset-settings, and factory-reset each have their own confirmation
UI in `SettingsPage.tsx`'s danger zone:

- **Restart** — a plain confirm dialog (not the unsaved-changes prompt —
  see the V2-103 section below for why that is correct) explaining the AP
  will briefly drop and that reconnection is automatic.
- **Reset settings** — dirty-guarded via `UnsavedChangesPrompt`, then a
  typed-confirmation dialog (`ConfirmPhraseDialog`) that states the exact
  SPEC_V2 §11.4 preserve/reset matrix in its own copy (device name,
  serial-confirmation policy, sending behavior, retention target,
  source-preview preference reset; station removed; access-point
  credentials, administrator password, provisioning state, and repository
  blobs preserved) and requires typing `RESET SETTINGS` exactly.
- **Factory reset** — dirty-guarded the same way, then a typed-confirmation
  dialog requiring both the administrator password and the exact phrase
  `FACTORY RESET`, stating plainly that every stored repository blob is
  erased and the device reboots unprovisioned.

**Connection loss and recovery** is real, not fire-and-forget:
`useDeviceReconnect.ts` polls an authenticated route (`getStatus`) at a
1-second interval once a device action is accepted. A real network failure
(`TypeError`) or an `AbortError` from the request's own timeout, and a
transient `5xx` (the HTTP server can be reachable before storage/Wi-Fi
finish reporting ready), all mean "still down, keep trying." A `401` means
the device is back but its RAM-only session is gone — exactly what a reboot
does — which is reported as `needs-reauth`; the underlying `v2GetJson` call
that produced that `401` has already triggered the existing
`subscribeUnauthorized` mechanism, so `AppV2`'s top-level state drops to
Sign In without discarding the live `ready`/store (the same guarantee
SPEC_V2 §7.3 already requires for session expiry). Any other resolved
response means the session, unexpectedly, is still valid — resume
immediately and refetch settings (reset-settings can have changed several of
them). `AppV2.tsx`'s `AuthenticatedShell` renders `DeviceReconnectScreen` in
place of the entire authenticated application while this is in flight, not
just inside the Settings screen, because a device reboot is disruptive to
the whole app.

Factory reset is handled differently once reachable: since it erases every
blob and reprovisions the device, there is nothing to resume into, so
`AuthenticatedShell` calls `window.location.reload()`, which re-runs
`AppV2`'s top-level provisioning check from scratch and lands on First-Run
Setup. This is deliberately the one place this track discards the
in-memory working copy without a further prompt — the typed
`FACTORY RESET` confirmation already told the user everything would be
erased.

Evidence: `webapp/tests/v2-device-reconnect.test.tsx` (6 tests: eventual
success after repeated network failure, a transient `503` treated as
retryable, an immediate `401` resolving to `needs-reauth` with no further
polling, a genuine unexpected error stopping polling and surfacing as
`error`, and inactive/deactivated-mid-wait behavior),
`webapp/tests/v2-device-reconnect-screen.test.tsx` (5 tests for the
presentational states), `webapp/tests/v2-device-actions-client.test.ts` (6
tests for the wire contract of all four device-action routes including the
sign-out-still-notifies-on-401 case), and — the strongest evidence — a real,
unmocked, end-to-end `AppV2` integration test in `webapp/tests/v2-app-v2.test.tsx`:
click Restart, accept, see the real `DeviceReconnectScreen` render with
"restarting" copy, the first `GET /api/v1/status` reachability poll fails at
the network layer exactly as a real `fetch` would, a `1000ms` fake-timer
tick later the device answers `401`, Sign In appears, and a fresh sign-in
resumes the exact same package without a `GET /api/v1/blob` list call (i.e.
without `RepositoryStartupScreen` re-running).

### V2-122 — Diagnostics UI

`DiagnosticsPage.tsx` renders every field the fixed `GET /api/v1/diagnostics`
schema (SPEC_V2 §13.13) actually defines: firmware version, build ID, reset
reason, uptime, memory (free/minimum-free/largest-free-block), USB state,
Wi-Fi access-point/station state, storage (state, web/user-data
totals/used, blob count, invalid filenames, temporary files), send
presence/state, and subsystem health. It does not display package or macro
data — there is none in the schema to display, enforced twice: the existing
`isDiagnosticsResponse` guard (`apiGuards.ts`, from an earlier phase)
rejects any response carrying a key outside that exact set before it ever
reaches React state, and a dedicated Vitest test constructs a response with
an injected `packageName`/`macroSource` field and asserts it is rejected.
The real-Chrome workflow additionally asserts the rendered Diagnostics page
never contains the fixture repository's package name.

Copy and download go through `v2/diagnosticsExport.ts`'s
`buildDiagnosticsExportText`, which rebuilds the exported JSON field by
field from the typed `DiagnosticsResponse` rather than serializing whatever
object arrived — a unit test constructs a response with extra injected
fields (`packageName`, `macroSource` containing what looks like a
credential) and asserts the exported text's key set is exactly the fixed
schema's keys and that neither injected string appears anywhere in the
output.

**One line item is deliberately left unchecked, not silently dropped**:
TODO_V2.md's V2-122 second bullet reads "Show firmware/build, uptime, reset
reason, memory, **stack**, USB, Wi-Fi, storage, blob count, send state,
health, and invalid/temp filenames." SPEC_V2 §13.13's fixed diagnostics
schema — the actual wire contract, reproduced above — has no `stack` field
anywhere, at the top level or inside `memory` (`memory` has only
`freeHeapBytes`, `minimumFreeHeapBytes`, `largestFreeBlockBytes`; there is
no stack high-water-mark field). I did not fabricate one — inventing a field
outside the frozen SPEC_V2 contract is exactly what this project's earlier
incident (recorded in `CLAUDE.md`/global memory) was about. **Recommendation
to the product owner**: either SPEC_V2 §13.13 needs a genuine stack-usage
field added (a normative spec change, which I cannot make unilaterally), or
TODO_V2.md's V2-122 bullet should drop "stack" as a wording error. Every
other item in that bullet is implemented and tested.

Evidence: `webapp/tests/v2-diagnostics-page.test.tsx` (5 tests: full-field
render, load-failure-then-retry, Copy calling `copyToClipboard` with the
filtered text, Download calling `saveAsFile` with a JSON payload matching
the schema, and Back navigation), `webapp/tests/v2-diagnostics-client.test.ts`
(2 tests: the wire contract and rejection of an out-of-schema response),
`webapp/tests/v2-diagnostics-export.test.ts` (2 tests: full-field
round-trip and the filtering guarantee), and the real-Chrome workflow.

### V2-103 — the remaining 3 of 5 unsaved-changes warning triggers

Sign Out, reset settings, and factory reset now all show
`UnsavedChangesPrompt` when `store.getIsDirty()` is true at the moment the
action is attempted, offering the same Cancel/Export working
copy/Save snapshot/Discard changes choices Phase 11 already wired for
snapshot load and import replacement. After Export, Save, or Discard
resolves the dirty state, the action proceeds automatically (Sign Out calls
the API directly; reset-settings/factory-reset advance to their own
typed-confirmation dialog rather than executing immediately, since those
still need the phrase/password).

Restart is **not** wired to this prompt, and that is a deliberate reading of
SPEC_V2 §7.3, not an oversight: its own trigger list is "Sign Out, loading
another snapshot, replacing the working copy through import, reset
settings, and factory reset" — five items, restart is not among them. The
reason is structural: restart's own reconnect flow (V2-121, above) already
preserves the dirty working copy across the reboot exactly the way session
expiry does, so there is nothing for the prompt to protect against losing.
Reset-settings and factory-reset, in contrast, are prompt-guarded even
though reset-settings also preserves the working copy through the same
mechanism — SPEC_V2 explicitly lists it, so it is wired regardless of
whether the underlying mechanism would have made data loss possible anyway.

Evidence: the SettingsPage tests above (dirty-then-warn scenarios for all
three), plus a real, unmocked, end-to-end `AppV2` test: sign in (which
itself leaves the working copy dirty from creating the first package),
navigate to Settings, click Sign Out, see the exact warning text
("Continuing to sign out..."), click Discard changes, see the real
`POST /api/v1/auth/logout` fire, land on Sign In, sign back in, and land
back in the authenticated shell (not stuck, not crashed). This test
deliberately uses Discard rather than Save snapshot — see "Known
limitations" below for why.

## Commands run

```bash
export PATH="/home/phil/.nvm/versions/node/v24.18.0/bin:$PATH"   # Node v24.18.0, per .nvmrc
cd webapp
npm ci
npx vitest run tests/v2-settings-page.test.tsx tests/v2-diagnostics-page.test.tsx \
  tests/v2-device-reconnect.test.tsx tests/v2-device-reconnect-screen.test.tsx \
  tests/v2-device-actions-client.test.ts tests/v2-diagnostics-client.test.ts \
  tests/v2-diagnostics-export.test.ts tests/v2-settings-client.test.ts \
  tests/v2-api-client.test.ts tests/v2-app-shell.test.tsx tests/v2-routing.test.ts \
  tests/v2-app-v2.test.tsx
npm run test                 # full Vitest suite
npm run typecheck
npm run lint
npm run format:check
cd ..
./scripts/check-webapp.sh    # ci -> format:check -> typecheck -> lint -> stylelint ->
                              # test -> test:coverage -> build -> test:browser ->
                              # verify-no-remote-assets.sh
node webapp/tests/browser/run-browser-tests.mjs   # run standalone, 3 additional times
```

## Results

- `npm run test` (Vitest, full suite): **577/577 passed**, 60 test files, run
  5 times consecutively with no failures once the fake-timer/gzip and
  cross-test-mount-leak issues described below were fixed. Before those
  fixes, the new `v2-app-v2.test.tsx` Phase-12 tests were genuinely flaky
  under the full 59→60-file suite (passed reliably in isolation, failed
  roughly every other run in the full suite) — root-caused, not just
  retried away; see "Known limitations and things fixed along the way."
- `npm run typecheck`, `npm run lint` (`--max-warnings=0`), `npm run
  stylelint`, `npm run format:check`: all clean.
- `npm run build`: succeeds, 298.37 kB JS / 13.28 kB CSS (uncompressed).
- `node tests/browser/run-browser-tests.mjs` (real headless Chromium via
  CDP): **run 5 times total** across this session (2 standalone before the
  full gate, 2 inside two full `check-webapp.sh` runs, 1 more standalone
  after) — **passed every time**, including the new "Real Chrome v2
  Settings/Diagnostics workflows passed" line (device-name edit
  round-tripping through a real `PUT`, Diagnostics rendering the fixed
  schema with no package/macro leakage, Back navigation).
- `./scripts/verify-no-remote-assets.sh webapp/dist`: passes.
- `./scripts/check-webapp.sh` (the full authoritative chain): **passed
  clean, twice**, exit code `0` confirmed via `$?` immediately after the
  script (not after a pipe to `tail`, which would have hidden a real
  failure the first time this was checked incorrectly during this session).

## Known limitations and things fixed along the way

- **Real browser coverage does not include the
  restart/reset-settings/factory-reset reconnect sequence.** The hand-rolled
  CDP fixture server (`run-browser-tests.mjs`) has no mechanism to simulate
  a genuine connection loss and recovery — the entire point of that
  feature — without either faking it (defeating the purpose of a
  real-browser test) or building substantial new fixture-server
  infrastructure (a real "go offline for N seconds then come back"
  simulation) that this track's time budget did not allow doing safely.
  That sequence is instead covered end-to-end against real, unmocked v2 API
  clients in `webapp/tests/v2-app-v2.test.tsx`, which is a meaningfully
  strong substitute (it exercises the actual fetch/promise/React-state
  machinery, not a hand-simplified mock) but is not evidence of real
  browser/network behavior. Left as a known gap rather than a checked box
  claiming otherwise.
- **The "stack" field named in TODO_V2.md's V2-122 bullet does not exist in
  SPEC_V2's fixed diagnostics schema.** See the V2-122 section above; left
  unchecked with a recommendation, not fabricated.
- **A genuine test-suite defect was found and fixed, not worked around**:
  `webapp/tests/v2-app-v2.test.tsx`'s existing tests (predating this track)
  render `<AppV2 />` and never call the returned `unmount()`. Individually
  this was harmless, but appending new tests after several such un-unmounted
  trees exposed real cross-test pollution — an orphaned `AuthenticatedShell`'s
  `hashchange` listener and `useDeviceStatus` poll interval kept running
  and intercepting `fetch()` calls meant for a later test (a race, not
  deterministic — it reproduced with as few as one preceding un-unmounted
  test). Root-caused by bisecting with `-t` test-name filters and direct
  `getFetchCalls()` inspection rather than adding a retry or a longer
  timeout. Fixed by having `signIn()` (the shared test helper) return its
  render result and adding `await view.unmount()` to every test in that
  file that renders `AppV2`, including the four pre-existing tests this
  track did not otherwise need to touch — a small, contained, well-justified
  fix rather than editing the shared `tests/render.tsx` helper used by all
  50+ other Vitest files.
- **A second, independent flake was found and fixed**: the original Sign
  Out integration test exercised the real Save-snapshot path, which
  compresses with the browser's native `CompressionStream` API
  (`v2/gzip.ts`). Under this test file's fake timers (`vi.useFakeTimers()`,
  needed for the restart-reconnect test's 1-second polling interval),
  `CompressionStream`'s own internal scheduling did not reliably resolve
  within a bounded number of `vi.advanceTimersByTimeAsync(0)` flushes,
  causing intermittent failures under the CPU contention of a full
  60-file parallel test run (never reproduced in isolation). No existing
  test in the codebase exercises real gzip compression under fake timers —
  the two suites that do exercise real gzip (`v2-snapshots-page.test.tsx`,
  `v2-snapshot-client.test.ts`) both use real timers. Fixed by changing that
  one test to exercise Discard changes instead of Save snapshot (still a
  complete, real assertion of the V2-103 Sign Out warning trigger, and
  additionally proves the stronger SPEC_V2 §8.7 claim that "the UI MUST NOT
  claim that a closed dirty working copy can be recovered" — Discard really
  discards); the Save-snapshot-then-sign-out path remains covered by
  `SettingsPage`'s own unit test with an injected fake `signOut`.
- **A real API-client bug was found and fixed before it shipped**:
  `v2PostJson`/`v2PostNoContent` originally always called
  `JSON.stringify(body)` unconditionally, which for the bodyless routes
  this track introduced real callers for (`restart`, `sign-out`) produced
  the literal string `"undefined"` as the request body and set an incorrect
  `Content-Type: application/json` header the route contracts say those
  requests never have. Caught by `exactOptionalPropertyTypes` during
  `npm run typecheck`, not by a test that happened to notice — fixed to
  omit the `body` key (and `Content-Type` header) entirely when the caller
  passes `undefined`, with dedicated tests for both helpers' bodyless case.
- Every unit and integration test added in this track uses dependency
  injection (a `dependencies` prop with a `vi.fn()`-based default factory,
  matching `SnapshotsPage.tsx`'s established pattern) except the `AppV2`
  integration tests, which deliberately exercise the real, unmocked v2
  clients end to end against a fake-fetch harness — the same choice
  `v2-app-v2.test.tsx` already made for every earlier phase.
- No `tests/v2_contracts/` (firmware/C-side) coverage was added or checked
  for device-action or diagnostics routes — outside this track's
  webapp-only file surface. `webapp/src/v2/apiRouteManifest.ts`'s existing
  route-shape check (unmodified) still covers those routes' method/path/
  status/content-type contract at the TypeScript layer.

## Explicit statement

No unchecked task in `docs/TODO_V2.md` is being claimed complete by this
report. The V2-122 "stack" line item and the Phase 12 exit gate's
firmware-contract-test note remain honestly unchecked/caveated for the
reasons stated above.
