# Phase 8 (remainder) — V2-082 Authenticated startup state machine, V2-083 First repository and first package, V2-084 Startup failure surfaces

**Tasks:** V2-082, V2-083, V2-084 (V2-080/V2-081 were already implemented and checked
off by a prior task on this same worktree lineage; this task picks up immediately after
successful authentication, per its own instructions).
**Branch:** `worktree-agent-a15ac2ec67cce3f2a` (this worktree's branch; not merged to
`master`, not pushed).
**Starting commit:** `2fd5e5d8d736f4308b6d69b634a2ef557814f03d`.
**Status:** Implemented and Vitest-tested (unit/decision-table + component-level,
jsdom). No physical-device or live-firmware validation is claimed. Real-browser
(Playwright/CDP) coverage of this specific flow is **not** included — see "Deferred"
below for why and what would be needed.

## Scope decision: not wired into `App.tsx`/`main.tsx`

`webapp/src/App.tsx` is still the v1-shaped production app (`SessionBoundary` →
`LoginPage`/`SetupPage` → package/macro CRUD screens against `api/routes.ts`). It has no
v2 Macros page to hand off to — that is Phase 9's job
(`docs/TODO_V2.md` V2-090 onward), which explicitly depends on Phase 8. Wiring this
phase's output into the live app would mean either leaving the resolved-package handoff
pointing nowhere, or building a placeholder Macros page myself, both outside this task's
three-item scope and risking the passing v1 test suites (`app-routing`, `app-auth`,
`app-macros`, `app-packages`, `spec-screens`, `management-screens`, the real-Chrome
`test:browser` harness) that already exercise `App.tsx` end to end.

This follows the same precedent V2-080/V2-081 set
(`docs/implementation-v2/V2_080_081_SETUP_AND_SIGNIN_2026-08-08.md`): new, self-contained,
fully-tested v2 components under `webapp/src/features/.../v2/`, talking only to the frozen
v2 contract layer, left unwired until the phase that has a real destination to wire them
into. `RepositoryStartupScreen`'s `onReady(ready)` callback is the exact handoff point —
Phase 9 mounts this screen after `SignInPage.onAuthenticated`, and its `onReady` callback
receives everything Phase 9 needs (`store`, `packageId`, `send`) to open the resolved
package's Macros page.

## What was built

| File | Task | Purpose |
| --- | --- | --- |
| `webapp/src/v2/startup.ts` | V2-082/083/084 | The pure, dependency-injected decision core. `runStartup(deps)` performs UI_UX_SPEC_V2 §3.4 steps 2-9 in order (gzip feature-detect → settings → blob list → newest blob → send recovery → package resolution) and returns one of 7 exhaustive, typed outcomes (`gzip-unsupported`, `device-unreachable`, `invalid-settings`, `load-failed`, `first-repository`, `invalid-newest-snapshot`, `ready`). `resolvePackageDestination` wraps V2-074's `resolveSelectedPackage` (`packageSelection.ts`, untouched) with the "empty repository → first package" special case. Never throws — every dependency failure is caught and mapped. |
| `webapp/src/v2/settingsClient.ts` | V2-082 | New: `getSettings()` — `GET /api/v1/settings` with the existing `isSettingsResponse` guard. Did not exist before this task; every other v2 settings write path (`packageSelection.ts`) already existed. |
| `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx` | V2-082/083/084 | The React component: owns the loading state, the "Create Your First Repository"/"Create your first package" form (shared between the no-blobs and empty-loaded-repository cases per UI_UX_SPEC_V2 §8), a minimal functional Package Chooser (for the "missing or invalid selected package" case), a minimal functional snapshot-recovery view (for "invalid newest snapshot", never silently falls back, requires an explicit alternate-blob pick), and retry screens for device-unreachable/invalid-settings/load-failed/gzip-unsupported. The `existing` prop is the entire "restore the exact current route and draft when the tab still has a live working copy" mechanism at this layer: when non-null, `onReady` fires immediately with zero network calls — the caller (Phase 9's app shell) is what actually keeps a `RepositoryWorkingCopyStore` alive across remounts/reauthentication. |
| `webapp/tests/v2-startup-decision-table.test.ts` | V2-082/083/084 exit gate | 22 tests over `runStartup`/`resolvePackageDestination` with fully injected dependencies: every combination of settings ok/network-fail/server-error; blobs empty/several-valid/network-fail/server-error; newest blob ok/network-fail/unreadable/schema-invalid/gzip-unsupported; package resolution via `lastSelectedPackageId`/sole-package-default/chooser/empty; send recovery present/absent/failed-non-blocking. Explicitly asserts several valid blobs never trigger a chooser and that a failed newest blob is reported, not silently replaced by an older one. |
| `webapp/tests/v2-repository-startup-screen.test.tsx` | V2-082/083/084 | 15 component-level tests against the real default dependencies (real `v2/gzip.ts`, `v2/settingsClient.ts`, `v2/snapshotClient.ts`, `v2/sendClient.ts`) driven entirely through the existing `fakeFetch` mock — restoring an existing working copy with zero requests, the loading copy, gzip-unsupported (via `vi.stubGlobal` removing `CompressionStream`/`DecompressionStream`), device-unreachable + Retry, invalid-settings, the first-repository→first-package form producing a dirty/unsaved store, an empty-but-valid loaded repository asking for the first package with different framing, sole-package auto-resolution + persistence, no-persist-when-unchanged, send-status pass-through, newest-of-several-blobs selection, the package chooser + persisted explicit choice, snapshot recovery (pick an alternate, no alternates left, Start over). |
| `webapp/tests/v2-browser-storage-prohibition.test.tsx` (extended, not new) | V2-072 follow-through | Widened the static-scan glob to also cover `src/features/startup/v2/**/*.{ts,tsx}` (the first-package form holds a package name; the recovered `RepositoryWorkingCopyStore` holds the whole repository, both in React state) and added a runtime test that drives `RepositoryStartupScreen` through the full "no blobs → Create Your First Repository → create a package → `onReady`" flow before asserting `Storage.prototype.setItem` was never called and both storages stayed empty. |
| `docs/TODO_V2.md` | — | Checked V2-082 (7/7), V2-083 (6/7 — see "Left open" below), V2-084 (6/6), and the decision-table half of the Phase 8 exit gate. Left the real-browser exit-gate line and one V2-083 line unchecked. No other lines touched. |

Total: 22 + 15 + 1 = 38 new tests. Combined with the 314 pre-existing tests, the full
suite is **352 tests across 37 files, all passing**.

## Design notes worth recording

- **`runStartup` never throws.** Every stage (`getSettings`, `listSnapshots`,
  `loadBlobIntoStore`, `recoverSendState`) is wrapped so a caller can render every
  outcome exhaustively. Errors are classified by type, not by guessing: a thrown
  `V2ApiError` (device responded, response was wrong/erroring) maps to
  `invalid-settings`/`load-failed`; anything else (network `TypeError`, `AbortError`)
  maps to `device-unreachable`. This is what lets V2-084's "device unreachable" and
  "invalid settings response" stay genuinely distinct, both in the decision-table tests
  and in the rendered copy.
- **Reused, did not reimplement:** `resolveSelectedPackage`/`persistSelectedPackageId`
  (V2-074, `packageSelection.ts`), `loadSnapshotIntoWorkingCopy`/`listSnapshots`
  (V2-073, `snapshotClient.ts`), `recoverSendState` (V2-075, `sendClient.ts`),
  `createEmptyRepository`/`validateRepositoryForUse` (V2-071,
  `repositoryValidation.ts`), `createRepositoryWorkingCopyStore` (V2-071,
  `repositoryWorkingCopy.ts`). None of those files were modified.
- **Newest-blob loading reuses `loadSnapshotIntoWorkingCopy` via a throwaway store.**
  `runStartup` creates an empty `RepositoryWorkingCopyStore`, hands it to
  `loadSnapshotIntoWorkingCopy`, and on success that store *is* the returned working
  copy (baseline = working = the loaded repository, `dirty: false`) — no separate
  decode path was written.
- **Best-effort persistence, not silent failure-hiding.** When the sole-package default
  or an explicit chooser/recovery pick needs `persistSelectedPackageId`, a failure there
  is caught and does not block `onReady` — the package still opens correctly in this
  tab; only the device-side UI preference write may lag. This is a deliberate product
  choice (SPEC_V2 does not require blocking on it) documented inline at each of the
  three call sites, not an unexplained empty catch.
- **Package-name validation reuses `validateRepositoryForUse` directly** (builds a
  candidate one-package repository and validates the whole thing) rather than
  duplicating the 64-UTF-8-byte name rule as a second copy of the constant.
- **Minimal, not full, Package Chooser and Snapshot Recovery surfaces.** Both are
  functionally complete (list, pick, persist/load, `onReady`) but intentionally not the
  richer Phase 9 Package Chooser (V2-091-equivalent) or Snapshot Management (V2-092/093)
  pages — those own the polished versions; this phase only needed something that
  satisfies "handle missing or invalid selected package" / "handle unreadable or
  invalid newest snapshot" without inventing UI affordances the spec doesn't describe
  (e.g. no "start fresh" escape hatch was added to snapshot recovery, since
  UI_UX_SPEC_V2 §3.4 only describes picking an older snapshot explicitly).
- **A real jsdom timing lesson, recorded so it isn't rediscovered:** `gzipCompress`/
  `gzipDecompress` go through Node's zlib, which completes on the libuv threadpool — a
  real I/O callback, not a microtask. The existing `flushReact()` test helper only
  drains `Promise.resolve()` microtasks, which is sufficient for plain JSON fetches
  (as V2-080/081's tests confirm) but was measurably insufficient once several fetches
  *and* a gzip round trip chain together under full-suite parallel load (passed in
  isolation, failed intermittently under `npx vitest run` with all 37 files). The fix
  in `v2-repository-startup-screen.test.tsx` is a local `waitUntil`/`flushTick` helper
  that yields a real `setTimeout(0)` tick wrapped in `act`, polled up to 50 times,
  instead of a fixed guessed flush count. The full suite was run three consecutive
  times after this fix with zero flakiness (see Evidence).

## What was left open, and why

- **`docs/TODO_V2.md` V2-083 "Open the empty Macros page" — left unchecked.** There is
  no v2 Macros page yet (Phase 9). `RepositoryStartupScreen`'s `onReady` callback is
  called with exactly the right `store`/`packageId` for Phase 9 to open that page, and
  this is tested (the "no blobs → first package" component test asserts the resulting
  store has one package with the submitted name and that `packageId` matches it), but
  no screen titled "Macros" is ever rendered by this task, so the box is left open
  honestly rather than claimed on the strength of the handoff alone.
- **Phase 8 exit gate "Real-browser tests cover first phone, refresh, expired session,
  no blobs, invalid newest blob, and send recovery" — left unchecked.** The existing
  real-browser harness (`webapp/tests/browser/run-browser-tests.mjs`, a hand-rolled
  Chrome-DevTools-Protocol driver, not Playwright despite the TODO's phrasing) drives
  `npm run build`'s output — i.e. the current `App.tsx`/`main.tsx` v1 production
  bundle — against a hand-written v1-shaped fixture server. `RepositoryStartupScreen`
  is not reachable from that bundle at all (see "Scope decision" above), so there is
  nothing for a real-browser test to click through yet. Building one now would mean
  either fabricating a throwaway entry point never used in production (contradicting
  "no obsolete routes/flows kept around" and producing evidence for a code path users
  can never reach) or prematurely building Phase 9's Macros page inside this task. This
  is deferred to whichever task wires Phase 9's app shell together, at which point the
  real browser harness's fixture server needs v2 routes
  (`/api/v1/settings`, `/api/v1/blob*`, `/api/v1/send`, `/api/v1/auth/*`) added
  alongside its current v1 ones. The Vitest decision-table and component-level suites
  above are the load-bearing coverage for this task instead, per this task's own
  instructions allowing that trade-off when full real-browser coverage is out of reach
  in one session.
- Everything else under V2-082/V2-083/V2-084 and the decision-table half of the exit
  gate is checked with the implementation + reproducible test evidence below.

## Commands run and results

Node confirmed exactly `v24.18.0` (`.nvmrc`) via `nvm use 24.18.0` before any command
below.

```bash
cd webapp
npm ci
npm run format:check      # pass
npm run typecheck         # pass (tsc -b --pretty false, zero errors)
npm run lint               # pass (eslint . --max-warnings=0, zero errors)
npm run stylelint           # pass (no CSS changed)
npm run test               # 352 tests / 37 files, all passing
npm run test:coverage      # 352 tests, all passing; see coverage table below
npm run build               # tsc -b && vite build, succeeds
npm run test:browser        # real headless Chrome (snap chromium), v1 App.tsx flows
                             # pass unchanged: "Real Chrome Phase 17.10 workflows passed."
../scripts/verify-no-remote-assets.sh dist   # exit 0
```

Equivalently, the whole gate: `./scripts/check-all.sh`'s webapp portion via
`./scripts/check-webapp.sh` from the repo root — ran successfully end to end.

New-file coverage from `npm run test:coverage` (v8):

| File | % Stmts | % Branch | % Funcs | % Lines |
| --- | --- | --- | --- | --- |
| `src/v2/startup.ts` | 92.68 | 80.64 | 100 | 92.68 |
| `src/features/startup/v2/RepositoryStartupScreen.tsx` | 75.17 | 69.56 | 77.77 | 77.14 |

The `RepositoryStartupScreen.tsx` branch gap is concentrated in the snapshot-recovery
view's individual failure-classification branches (a rejected alternate-blob fetch vs.
an `ok:false` result) and the `PackageChooserView`'s persistence-failure catch — all
exercised by at least the success path, not every failure sub-branch, in the interest of
keeping this task's test suite size proportionate; the decision-table suite already
covers those exact classification branches at the `runStartup` level.

Full suite run three consecutive times after the jsdom-timing fix above, to confirm no
flakiness under full-suite parallel load:

```text
Test Files  37 passed (37)  /  Tests  352 passed (352)   [run 1]
Test Files  37 passed (37)  /  Tests  352 passed (352)   [run 2]
Test Files  37 passed (37)  /  Tests  352 passed (352)   [run 3]
```

## Explicit completion statement

No unchecked `docs/TODO_V2.md` task under V2-082, V2-083, V2-084, or the Phase 8 exit
gate is being claimed complete. The two items left open above (V2-083's "Open the empty
Macros page" and the exit gate's real-browser line) are left unchecked precisely because
they require a v2 Macros page and a wired production entry point that do not exist yet.
