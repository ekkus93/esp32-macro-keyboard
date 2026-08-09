# Phase 9 — V2-090 through V2-095: Macros page and Quick Send operating console

**Tasks:** V2-090 (application shell), V2-091 (macro list), V2-092 (macro-source
privacy), V2-093 (Quick Send), V2-094 (optional preview and send), V2-095 (reload
and race handling), the Phase 9 exit gate, and Phase 8's remaining exit-gate line
(partial — see below).
**Branch:** `worktree-agent-a7cb07a46666744be` (this worktree's branch; not merged to
`master`, not pushed).
**Starting commit:** `6cbd4ca2c4856ae61b468a479b1ab081dbe9610f`.
**Status:** Implemented, Vitest-tested (unit/component, jsdom, 55 new tests across 8
files), and validated end to end against a real headless Chromium browser driven
over the Chrome DevTools Protocol — the same `tests/browser/run-browser-tests.mjs`
harness Phase 8's report identified as the thing this task would need to update.
No physical-device or live-firmware validation is claimed.

## Scope decision: `App.tsx`/`main.tsx` are cut over to the v2 app

Phase 8's report deferred "wire `RepositoryStartupScreen`'s `onReady` into the running
app shell" because there was no Macros page to hand off to, and left `main.tsx`
mounting the v1 `App` component. That is exactly this task's job. `firmware` deleted
the v1 package/macro/execution HTTP routes in Phase 2
(`docs/implementation-v2/PHASE_2_RETIRED_REPOSITORY_REMOVAL_2026-08-05.md`), so the v1
`App` component (`/api/v1/setup-state`, `/api/v1/package`, `/api/v1/executions`, …) has
been calling routes that do not exist on real firmware since that phase — keeping it as
the mounted app would be "preserving an obsolete flow" in exactly the sense
`docs/TODO_V2.md` §0.1 forbids.

`webapp/src/main.tsx` now mounts a new `webapp/src/AppV2.tsx` instead of the legacy
`App`. `AppV2` composes, in order: the unauthenticated provisioning probe →
`FirstRunSetupPage`/`SignInPage` (Phase 8, V2-080/V2-081, untouched) →
`RepositoryStartupScreen` (Phase 8, V2-082/083/084, untouched) → once `onReady` fires,
a new authenticated shell wrapping the Macros page (this task).

The legacy `App.tsx`/`src/routing.ts` and every test that renders them directly
(`app-auth.test.tsx`, `app-macros.test.tsx`, `app-packages.test.tsx`,
`app-routing.test.tsx`, `spec-screens.test.tsx`, `management-screens.test.tsx`,
`package-management.test.tsx`, `execution-confirmation.test.tsx`,
`routing-confirmation.test.ts`, …) are **left in place, untouched, and still green** —
they import `App` from `../src/App` directly, not through `main.tsx`, so none of them
observe the swap. Deleting that dead code is `docs/TODO_V2.md` V2-140's job (Phase 14),
not this task's; this task only stopped *mounting* it in production.

## What was implemented

### V2-090 — Application shell

- `webapp/src/features/shell/v2/AppShellV2.tsx`: device name, selected package name
  (or "No package selected"), a USB-state badge, `Saved`/`Unsaved changes`, a
  conditional **Save snapshot** button, and the fixed bottom navigation
  (`Macros | Packages | Snapshots | Settings`) with `aria-current="page"` and a
  non-color-only active treatment. Purely presentational — no network calls of its
  own.
- `webapp/src/features/shell/v2/useDeviceStatus.ts`: polls `GET /api/v1/status`
  every 5 seconds for the live USB state, keeping the last known value on a
  transient poll failure rather than flashing an incorrect badge.
- `webapp/src/v2/statusClient.ts`: the `GET /api/v1/status` client.
- `webapp/src/v2/routingV2.ts`: v2 hash routing (`macros`, `packages`, `snapshots`,
  `settings`, `macro-preview`, `macro-editor`) — a route may encode a macro
  selection, but the in-memory repository stays the source of truth; no route
  causes a firmware lookup.
- `webapp/src/features/shell/v2/PlaceholderPage.tsx`: the stub destination for
  Packages/Snapshots/Settings, per this task's own instructions ("only Macros needs
  to be a real destination right now").
- Safe-area support: `viewport-fit=cover` added to `webapp/index.html`'s viewport
  meta tag; `.app-header`/`.bottom-nav` padding extended with
  `env(safe-area-inset-top)`/`env(safe-area-inset-bottom)` in `webapp/src/styles.css`
  (additive to the existing v1-shared rules — verified not to visually change v1
  screens, since `env()` is `0` without `viewport-fit=cover`, which v1 never relied
  on either).
- `Save snapshot` is wired to the real `saveWorkingCopyAsSnapshot` (V2-073, Phase 7)
  — clicking it uploads the working copy and clears dirty state on success, or
  leaves it dirty with an inline error on failure. The full Manual Save snapshot
  *workflow* (retry affordances, upload progress detail, retention-target UI) is
  Phase 11's job (V2-110); this task only needed the shell-level control to exist
  and work.

### V2-091 — Macro list

- `webapp/src/features/macros/v2/MacrosPage.tsx`: ordered macro rows for the
  resolved package, macro count, **Add macro**, **Edit**, **Send**, **Preview and
  send**, and accessible **Move up**/**Move down** reordering (button-based, the
  standard accessible-reordering pattern — verified keyboard-operable, not just
  mouse-clickable, in the real-browser suite below). Reordering reuses the existing
  `RepositoryWorkingCopyStore.applyContentChange` primitive from V2-071 (its own
  doc comment already names "move, reorder" as in scope) — no new mutation API was
  added.
- Send is disabled unless `usbState === "ready"` and no send is active (including,
  after a fix made during real-browser testing — see "Defects found" below, during
  the brief completion-acknowledgement window).
- Quick Send never navigates: `startSend` calls no navigation function at any point
  in its lifecycle.

**Left unchecked:** "Show Add macro, Edit, Send, and overflow controls." Add macro,
Edit, Send, and Preview-and-send all exist as direct row controls, but there is no
literal overflow-menu affordance, and Duplicate/Delete (UI_UX_SPEC_V2 §5.1's other
overflow examples) are not present at all — full macro CRUD is V2-101 (Phase 10),
which this task deliberately did not reach into. Add macro/Edit navigate to the
Phase-10 placeholder screen rather than a working editor, for the same reason.

### V2-092 — Macro-source privacy

- Source is hidden by default behind a `Source hidden` placeholder; a per-row
  **Reveal**/**Hide source** toggle shows/hides only that row; the device-wide
  `showMacroSourcePreviews` setting (already implemented server-side; read via the
  existing `getSettings()` client) reveals all rows.
- Verified that hidden source never appears in the DOM (`document.querySelectorAll('code').length === 0`
  before reveal, in the real-browser suite) and that no acknowledgement, banner,
  accessible name, or `aria-label` in `MacrosPage.tsx`/`MacroPreviewPage.tsx` ever
  interpolates `macro.source` — only `macro.name`. Nothing in this task's code calls
  `console.*` with macro content.

### V2-093 — Quick Send

- The primary Send issues exactly one `POST /api/v1/send` via the existing
  `sendMacro()` helper (V2-075, Phase 7) — not reimplemented.
- Inline states per UI_UX_SPEC_V2 §5.5: selected-row `Sending…`, a page-level
  progress banner naming the macro and `action X of Y`, other Send controls
  disabled, `awaiting_confirmation` shown as "Waiting for physical confirmation…",
  **Cancel and release all keys** (calls `DELETE /api/v1/send`), a `Sent <name>.`
  acknowledgement for exactly 4000 ms (within the spec's 3-5 s window) that then
  restores the ordinary Send control, and persistent cancelled/failed/timed-out
  banners with their own **Dismiss** control (failed shows the exact `error` text).
  A release error (`releaseError` non-empty) is reported as an independent,
  separately dismissible banner, exactly matching "report it separately and
  prominently."
- Source is never included in any banner or accessible name.

### V2-094 — Optional Preview and Send

- `webapp/src/features/macros/v2/MacroPreviewPage.tsx`, reached from each row's
  **Preview and send** control: package name, macro name, full readable source
  (source privacy does not apply here, per UI_UX_SPEC_V2 §5.2), key-press/inter-key
  timing, action count and estimated duration (computed client-side via the
  existing `compileMacro()`, V2-060's shared corpus implementation — not
  reimplemented), current USB state, and explicit **Send now**/**Cancel**.
- **Send now** calls the same `sendMacro()` helper, then hands the accepted status
  to the app shell (`onSendInitiated`) and navigates back to the Macros page, which
  adopts that status as its next `initialSend` and resumes tracking there — this
  screen never itself renders send progress ("return to the Macros page for
  progress").
- A `409` here is handled the same way as on the Macros page: recover the actual
  current send and hand that off instead of failing.

**Left unchecked:** "Make preview available from overflow actions" (same overflow
caveat as V2-091 — it's a direct button, not an overflow menu). **"Honor Always
Preview when configured" is a real gap, not a naming quibble:** `MacrosPage` reads
`settings.sendMode` and shows an informational note when it is `"preview"`, but the
primary Send control still always Quick-Sends directly — it does not open
`MacroPreviewPage` first as SPEC_V2 §14.5 requires ("With `sendMode: preview`, the
primary Send control opens the Preview and Send screen first"). This was not
implemented and must not be claimed done.

### V2-095 — Reload and race handling

- **Recover inline send state after reload:** `MacrosPage` accepts an `initialSend`
  prop (the send `RepositoryStartupScreen` already recovers via `GET /api/v1/send`
  during startup, UI_UX_SPEC_V2 §3.4 step 8). A non-terminal `initialSend` resumes
  tracking immediately (no macro name available, since the wire protocol does not
  carry one); a still-undismissed terminal issue is restored as a persistent
  banner; a stale `completed` status is not resurrected (its acknowledgement window
  has necessarily already passed).
- **Prevent double-send on rapid taps:** a ref (`startingRef`) is set synchronously
  before the first `await`, so a second click dispatched before React re-renders is
  a no-op. Proven directly: three rapid clicks produce exactly one `POST`.
- **Prevent duplicate completion callbacks:** a `completedSendIdsRef` set, keyed by
  send ID, guards every path that can observe a terminal status (a send this tab
  started, a reload recovery, and a `409` recovery) so `onComplete`'s handling runs
  at most once per send ID regardless of which path observed it first.
- **Handle `409` by showing the actual current send:** on `409`, `recoverSendState()`
  is called and its result adopted as the active send (no macro identity, same as
  reload recovery); if that too comes back terminal/absent, an explanatory error is
  shown instead of a silent failure.
- **Handle session expiry without discarding the working copy:** `AppV2` subscribes
  to the existing `subscribeUnauthorized` global listener (`v2/apiClient.ts`, Phase
  7). On a `401` from anywhere, it drops back to `SignInPage` but keeps its `ready`
  (the live `RepositoryWorkingCopyStore` + resolved package) in React state; once
  re-authenticated, `AppV2` renders `AuthenticatedShell` directly from the preserved
  `ready` without ever re-running `RepositoryStartupScreen`'s blob-loading sequence.
  Proven in `tests/v2-app-v2.test.tsx`: after a simulated session expiry and
  re-login, `GET /api/v1/blob` is not called again and the just-created package is
  still showing.

### `sendClient.ts` extension (Phase 7 module, additive)

`webapp/src/v2/sendClient.ts` gained one new export, `trackSend(seed, callbacks)`,
factored out of `sendMacro`'s existing poll loop (`createSendTracker`, shared by
both). It resumes polling an already-known send **without issuing a new `POST`** —
exactly the primitive V2-095's reload/`409` recovery needs, and explicitly required
reusing rather than reimplementing the send helper. `sendMacro`'s existing exported
behavior and signature are unchanged; its own pre-existing tests still pass
unmodified.

## Defects found and fixed during this task

1. **`GET /api/v1/send` guard silently rejected without `estimatedDurationMs`** — not
   a product bug (the field is genuinely required by `isSendStatusResponse`), but
   found via the real-browser fixture initially omitting it: `sendClient`'s poll
   loop swallows a guard failure into an infinite silent retry with no visible
   error, which is itself worth noting as a real (if intentional-by-design)
   trade-off: a malformed status response looks identical to "still polling" from
   the UI's perspective. No product code change; documented for whoever next
   touches this path.
2. **Send controls were not disabled during the completion-acknowledgement window.**
   UI_UX_SPEC_V2 §5.5 says completed "show `Sent` … for approximately three to five
   seconds, **then** restore the ordinary Send control" — the initial implementation
   only disabled Send while `lifecycle.kind` was `"starting"`/`"active"`, so a new
   send could be started (and a stale reused send ID in the first real-browser fixture
   attempt made this visible as a hang) during the four-second acknowledgement.
   Fixed: `sendActive` (and therefore `sendDisabled`) now also covers `"completed"`.
   A persistent cancelled/failed/timed-out banner deliberately still allows starting
   a new send, since UI_UX_SPEC_V2 §5.5 says that banner lasts "until dismissed **or
   another send begins**." `startSend`'s own guard was updated to match (it now
   accepts `"idle"` or `"terminal-issue"`, not only `"idle"`). Covered by
   `tests/v2-macros-page.test.tsx` and the real-browser suite.

## Real-browser (Chrome DevTools Protocol) coverage

`webapp/tests/browser/run-browser-tests.mjs` was rewritten. Its old fixture server
spoke v1 routes (`/api/v1/setup-state`, `/api/v1/package`, `/api/v1/executions`, …)
against a v1-shaped UI that no longer exists in the mounted app; it would have failed
outright the moment `main.tsx` changed. The new fixture serves the exact v2 route set
(`/api/v1/setup`, `/api/v1/auth/session`, `/api/v1/settings`, `/api/v1/blob`,
`/api/v1/blob/1`, `/api/v1/send`, `/api/v1/status`) and a **real gzip-compressed**
repository blob (`node:zlib gzipSync`, decoded by the browser's own
`DecompressionStream("gzip")` — jsdom has no such API, so this is coverage Vitest
structurally cannot provide). Against the real `npm run build` output in headless
Chromium (via the pre-existing `chromium` snap), it now drives:

- idle load of a real gzip-decoded repository onto the Macros page;
- responsive layout and 44×44 touch targets at mobile (360×640) and desktop
  (1280×800) viewports (kept from the prior suite);
- accessible reordering via a real keyboard `Enter` on a focused Move button (not
  just a mouse click);
- macro-source privacy (hidden by default, revealed/hidden on demand, verified via
  DOM inspection, not just text search);
- Quick Send: progress, completion acknowledgement, and its self-clearing;
- the serial-confirmation waiting state, end to end to completion;
- Cancel and release all keys, with its persistent dismissible acknowledgement;
- failure (exact error text) and timeout, each persistent until dismissed;
- a release error reported as its own separate, dismissible banner;
- reload recovery: a send started, the page reloaded mid-flight via `Page.reload`,
  and send tracking resumed from `GET /api/v1/send` afterward.

Run three times in a row locally with no flakiness observed.

**Deliberately not covered in the real browser** (documented in the test file
itself):

- **USB unavailable** — would need the real 5-second device-status poll interval to
  flip mid-test, which is slow and was judged not worth the added runtime for one
  scenario when the underlying `usbState !== "ready"` gating is already covered by
  `tests/v2-macros-page.test.tsx`.
- **Rapid repeated input** — a same-tick double-dispatch race is not reliably
  reproducible over a real CDP round trip (whose network latency alone exceeds
  React's synchronous re-render time; a disabled button masks what would actually be
  tested). `tests/v2-macros-page.test.tsx` proves the guard deterministically
  instead (three synchronous clicks, exactly one `POST`).

Because of these two gaps, **the Phase 9 exit-gate line naming all twelve scenarios is
left unchecked** — ten of twelve are proven in a real browser; the remaining two are
proven in Vitest only. The independent second exit-gate line ("Ordinary Quick Send
never navigates to a standalone progress/result route") is checked: no navigation
call exists anywhere in the send code path, and `tests/v2-app-v2.test.tsx` asserts
the bottom navigation stays the real four-item nav throughout.

## Phase 8's remaining exit-gate line

"Real-browser tests cover first phone, refresh, expired session, no blobs, invalid
newest blob, and send recovery" is **left unchecked**, but with real, if partial,
progress to report honestly:

- **refresh** and **send recovery** are now covered by the real-browser suite above
  (the reload-recovery workflow reloads the page and re-verifies send tracking).
- **first phone**, **expired session**, **no blobs**, and **invalid newest blob** are
  not exercised against a real browser. `tests/v2-app-v2.test.tsx` covers "no blobs"
  (the fixture used there has no stored blobs at all, driving the real
  "Create Your First Repository" flow) and "expired session" (a simulated `401`
  drops back to Sign In and a re-login resumes without re-fetching the blob list) —
  but only against the Vitest fake-fetch harness, not a real browser. "First phone"
  and "invalid newest blob" are not covered by either suite in this task; the latter
  is already covered by `RepositoryStartupScreen`'s own component-test suite from
  Phase 8 (unchanged, still passing), just not in a real browser.

Extending the real-browser fixture to cover the remaining four scenarios was judged
out of reach for this session without risking the stability of what is already
proven; deferred honestly rather than forced.

## Files changed

- `webapp/index.html` — `viewport-fit=cover`.
- `webapp/src/main.tsx` — mounts `AppV2` instead of the legacy `App`.
- `webapp/src/AppV2.tsx` — new; the v2 application root.
- `webapp/src/styles.css` — safe-area padding additions to shared `.app-header`/`.bottom-nav`
  rules, plus new `.send-status`/`.send-status[role="alert"]` rules.
- `webapp/src/v2/sendClient.ts` — adds `trackSend`, factors `createSendTracker`.
- `webapp/src/v2/statusClient.ts`, `webapp/src/v2/routingV2.ts` — new.
- `webapp/src/features/shell/v2/AppShellV2.tsx`, `useDeviceStatus.ts`,
  `PlaceholderPage.tsx` — new.
- `webapp/src/features/macros/v2/MacrosPage.tsx`, `MacroPreviewPage.tsx` — new.
- `webapp/tests/browser/run-browser-tests.mjs` — rewritten for v2 routes/workflows.
- `webapp/tests/v2-send-client.test.ts` — extended with `trackSend` coverage.
- `webapp/tests/v2-routing.test.ts`, `v2-status-client.test.ts`,
  `v2-use-device-status.test.tsx`, `v2-app-shell.test.tsx`,
  `v2-macro-preview-page.test.tsx`, `v2-macros-page.test.tsx`, `v2-app-v2.test.tsx`
  — new.
- `docs/TODO_V2.md` — checkbox lines for V2-090 through V2-095 and one Phase 9
  exit-gate line only (see diff; no other lines touched).

No firmware, `tests/host/`, or `tests/v2_contracts/` files were touched — this task's
scope was entirely `webapp/`.

## Commands run and results

Node confirmed exactly `v24.18.0` via the `nvm`-managed toolchain before any command
below (`.nvmrc`).

```bash
cd webapp
npm ci
./scripts/check-webapp.sh   # from repo root; runs the full chain below
```

Full chain (`./scripts/check-webapp.sh`), run from a clean `npm ci`:

```text
npm run format:check      # pass
npm run typecheck         # pass (tsc -b --pretty false, zero errors)
npm run lint               # pass (eslint . --max-warnings=0, zero errors)
npm run stylelint           # pass (stylelint 'src/**/*.css' --max-warnings=0, zero errors)
npm run test                # 407 tests / 44 files, all passing
npm run test:coverage       # 407 tests, all passing
npm run build                # tsc -b && vite build, succeeds
npm run test:browser         # npm run build && node tests/browser/run-browser-tests.mjs
                              # "Real Chrome v2 Macros page/Quick Send workflows passed."
verify-no-remote-assets.sh   # pass (part of check-webapp.sh)
```

`./scripts/check-webapp.sh` exit code: `0`.

New/updated test counts (baseline before this task, per the Phase 8 report: 352
tests / 37 files):

| File | Tests |
| --- | --- |
| `tests/v2-send-client.test.ts` (extended) | 11 (was 8; +3 for `trackSend`) |
| `tests/v2-routing.test.ts` (new) | 12 |
| `tests/v2-status-client.test.ts` (new) | 1 |
| `tests/v2-use-device-status.test.tsx` (new) | 3 |
| `tests/v2-app-shell.test.tsx` (new) | 5 |
| `tests/v2-macro-preview-page.test.tsx` (new) | 6 |
| `tests/v2-macros-page.test.tsx` (new) | 21 |
| `tests/v2-app-v2.test.tsx` (new) | 4 |

New total: 407 tests / 44 files (352 + 55 new = 407; 37 + 7 new files = 44).

The real-browser suite (`npm run test:browser`) was additionally run three
consecutive times manually (outside the `check-webapp.sh` chain, which runs it once)
to confirm no flakiness; all three passed with an identical outcome.

## Explicit completion statement

No unchecked `docs/TODO_V2.md` task under V2-090 through V2-095, the Phase 9 exit
gate, or Phase 8's remaining exit-gate line is being claimed complete. Left open,
with reasons recorded above:

- V2-091's "overflow controls" (no literal overflow menu; Duplicate/Delete absent,
  deferred to Phase 10's V2-101).
- V2-094's "overflow actions" (same reason) and "Honor Always Preview when
  configured" (a genuine, not-yet-implemented gap: the primary Send control does
  not redirect to Preview when `sendMode: preview`).
- The Phase 9 exit gate's twelve-scenario browser-test line (ten of twelve covered
  in a real browser; USB-unavailable and rapid-repeated-input covered in Vitest
  only, for the reasons given above).
- Phase 8's remaining exit-gate line (two of six scenarios now covered in a real
  browser; two more covered in Vitest only; two not covered by either suite in this
  task).
