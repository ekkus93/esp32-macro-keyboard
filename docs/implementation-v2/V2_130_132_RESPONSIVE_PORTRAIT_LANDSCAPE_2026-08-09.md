# V2-130..V2-132 — Responsive layout, portrait-required phones, landscape active-send safety

**Phase:** 13 — Portrait phones, responsive layout, and accessibility
(V2-130/V2-131/V2-132 only; V2-133 accessibility is a separate, parallel
track — this report and its edits deliberately avoid that surface).
**Base:** `e7a7546` ("Merge browser test harness migration: hand-rolled CDP
to Playwright").
**Worktree branch:** `worktree-agent-abd8de3f4bc27a524`.
**Date:** 2026-08-09.

No unchecked `docs/TODO_V2.md` item is claimed complete without the specific
file/test evidence cited next to it in that file. Where evidence is partial
or a real gap remains (device hardware, manifest infrastructure), it is
stated below and left unchecked there — see "What was deliberately left
open" below.

## Scope actually touched

Webapp only. No firmware, host test, or `tests/v2_contracts/` file changed.

- New: `webapp/src/features/shell/v2/activeSendSummary.ts` (the
  `ActiveSendSummary` type), `useLandscapePhoneBlock.ts` (the device
  classification hook), `LandscapeBlockSurface.tsx` (the orientation
  surface component).
- New tests: `webapp/tests/fakeMatchMedia.ts` (a controllable
  `window.matchMedia` fake — jsdom implements no `matchMedia` at all),
  `webapp/tests/v2-landscape-phone-block.test.tsx` (7 tests).
- Modified: `webapp/src/AppV2.tsx` (wires the hook/surface into
  `AuthenticatedShell`), `webapp/src/features/macros/v2/MacrosPage.tsx` (new
  optional `onActiveSendChange` prop reporting the active-send summary
  upward), `webapp/src/styles.css` (touch-target fix, safe-area coverage,
  wider-viewport rule, `.landscape-block` styles), `webapp/tests/v2-macros-page.test.tsx`
  (4 new tests), `webapp/tests/v2-app-v2.test.tsx` (2 new end-to-end tests).
- `docs/TODO_V2.md`: only the V2-130, V2-131, V2-132, and Phase 13 exit-gate
  "Active-send cancellation remains available in landscape" lines.

## What each task actually got, and how it was verified

### V2-131 — Portrait-required phone surface (built from nothing)

Per the prior audit (`V2_AUDIT_PHASE_13_14_15_2026-08-09.md` §V2-131), no
landscape-blocking CSS or logic existed at all before this work. Built:

- **Device classification** — `useLandscapePhoneBlock.ts` exports
  `landscapePhoneMediaQuery`, the exact string from UI_UX_SPEC_V2 §12.4:
  `"(orientation: landscape) and (pointer: coarse) and (max-height: 600px)"`.
  The hook reads `window.matchMedia(landscapePhoneMediaQuery).matches` on
  mount and subscribes to the `MediaQueryList`'s `change` event —
  event-driven, not polled.
- **The orientation surface** — `LandscapeBlockSurface.tsx` renders the
  exact UI_UX_SPEC_V2 §12.2 copy ("Rotate your phone" /
  "ESP32 Macro Keyboard is designed for portrait mode.") and, when an active
  send is reported to it, the macro name/progress/Cancel described in §12.3
  (see V2-132 below).
- **No reload, no state loss** — `AppV2.tsx`'s `AuthenticatedShell` keeps its
  entire normal render tree (`AppShellV2` and everything routed inside it)
  mounted unconditionally. The only thing `useLandscapePhoneBlock()`'s
  result controls is a `style={{ display: "none" }}` on one wrapper `<div>`
  around that tree, with `LandscapeBlockSurface` rendered as a sibling when
  blocked. Nothing unmounts, so React state, refs, in-flight `sendMacro`
  polling, and the `hashchange`/`useDeviceStatus` listeners are all
  untouched by an orientation change — this is the mechanism, not a claim
  added after the fact.
- **Manifest hint** — **not done**. See "What was deliberately left open".

Evidence:

- `webapp/tests/v2-landscape-phone-block.test.tsx` (jsdom, via
  `fakeMatchMedia.ts`): the exact media-query string; initial state on
  mount; reacting to a later `change` event without remounting (three
  transitions observed by one component instance: `false`, `true`, `false`);
  listener cleanup on unmount (`fakeMedia.set(...)` after `unmount()`
  produces no further callback).
- `webapp/tests/v2-app-v2.test.tsx`, "hides the app behind Rotate your phone
  in landscape, and restores the exact route and dirty state on return — no
  reload, no re-fetch, no lost draft": signs in through the real running
  app (working copy already dirty from creating the first package),
  navigates to Snapshots, flips the fake `matchMedia` to landscape (asserts
  "Rotate your phone" shows and `.app-shell` is still present in the DOM —
  hidden, not gone), flips back to portrait, and asserts the route is still
  "Snapshots", `"Unsaved changes"` is still shown, and the `/api/v1/blob`
  `GET` count is unchanged from before the excursion (proving
  `RepositoryStartupScreen`, the only place that request is issued, never
  remounted).

### V2-132 — Landscape active-send safety (built from nothing)

`MacrosPage.tsx` is the only screen that tracks send lifecycle —
`MacroPreviewPage.tsx`'s own doc comment says it "never itself shows send
progress" and hands tracking off to the Macros page — so it is the one
place that needed to report upward. A new optional
`onActiveSendChange?: (summary: ActiveSendSummary | null) => void` prop is
called from a `useEffect` keyed on `lifecycle`:

- `lifecycle.kind === "starting"` -> `{ macroName, statusText: "Sending
  {name}…", onCancel: null }` (nothing to cancel yet — the `POST` hasn't
  resolved).
- `lifecycle.kind === "active"` -> `{ macroName, statusText:
  activeStatusText(...) (the same string the inline UI shows), onCancel:
  () => cancelActiveSend() }` (the same function the inline "Cancel and
  release all keys" button calls — no second cancel implementation).
- Anything else (`idle`/`completed`/`terminal-issue`) -> `null`. UI_UX_SPEC_V2
  §12.3 names "awaiting confirmation or running" specifically; a completion
  acknowledgement or a persistent terminal-issue banner has nothing left to
  cancel, so the orientation surface shows nothing extra for those.

`AppV2.tsx` passes `setActiveSend` (an `AuthenticatedShell`-local
`useState`) as `onActiveSendChange` and threads the resulting
`ActiveSendSummary | null` into `LandscapeBlockSurface`.

Evidence:

- `webapp/tests/v2-macros-page.test.tsx`, "MacrosPage — V2-132 landscape
  active-send summary" (4 tests): the starting summary observed in
  isolation via a dependency-injected `sendMacro` that deliberately never
  resolves (the real fake-fetch harness resolves fast enough that a single
  `act()` flush already reaches "active" before any assertion can run —
  documented in the test itself, not silently worked around); active
  progress text and a working `onCancel`; `null` once the completion
  acknowledgement clears; and that nothing is reported when the caller
  passes no callback (opt-in, not a forced dependency for every other
  `MacrosPage` test in the suite).
- `webapp/tests/v2-landscape-phone-block.test.tsx`: `LandscapeBlockSurface`
  shows macro name/progress and a working Cancel when `activeSend` is
  non-null, and omits the Cancel button entirely when `onCancel` is `null`
  (the "starting" case).
- `webapp/tests/v2-app-v2.test.tsx`, "an active send's macro name, progress,
  and Cancel remain accessible while landscape-blocked": starts a real send
  through the running app, switches to landscape, asserts the macro
  name/progress text and a "Cancel and release all keys" button are visible,
  locates that button specifically inside `.landscape-block` (not merely
  matching the text anywhere in the document — the ordinary, now-hidden
  Macros page has its own copy of the same button, and DOM order would find
  that one first if the test weren't scoped), clicks it, and asserts the
  resulting `DELETE /api/v1/send` and the "Send Open terminal was
  cancelled." acknowledgement.

### V2-130 — Responsive layout (partial; most of the range was already true or is now fixed)

- **Touch targets (fixed).** The prior audit's two concrete counter-examples
  — `.header-button` (`min-height: 36px`) and `.directive-toolbar button`
  (`38px`) — are both now `44px` in `webapp/src/styles.css`. `grep -n
  "min-height" webapp/src/styles.css` shows no value below `44px` anywhere
  in the file. Caveat stated plainly in `TODO_V2.md`: the pre-existing
  real-browser `assertTouchTargets()` check
  (`webapp/tests/browser/run-browser-tests.mjs`) passes, but it only runs
  early on the Macros page, before either fixed control is ever on-screen
  (`.header-button`/"Save snapshot" needs a dirty working copy state beyond
  what's present at that point in the flow; `.directive-toolbar` only
  exists on the macro editor page, never visited before that check runs) —
  so that specific browser assertion does not independently exercise the
  fix. The claim rests on the source diff and the grep.
- **Wider tablet/desktop layout.** `.standalone`/`.app-shell`'s max width
  moves from a flat 48rem cap to 64rem at `@media (width >= 60rem)` — a
  relaxed cap, not a workflow change (identical single-column markup, no
  split panes, no new routes). The pre-existing real-browser
  `assertResponsiveLayout()` already asserts desktop content (1280x800) is
  wider than mobile content (360x640) with no horizontal scroll at either
  size, and `./scripts/check-webapp.sh`'s `test:browser` step passed with
  this change in place. UI_UX_SPEC_V2 §13's optional "MAY use wider cards,
  split panes, or denser management layouts" was not built — only the
  "without changing workflow" half is claimed.
- **Safe areas (widened, still left unchecked).** `.standalone` — the
  full-screen container for every screen outside the authenticated shell
  (First-Run Setup, Sign In, the device-unreachable/loading/reconnect
  screens) — previously had zero safe-area handling; it now pads all four
  sides with `env(safe-area-inset-*)`, matching what `.app-header`/
  `.bottom-nav` already did for the authenticated shell. The new
  `.landscape-block` surface does the same. Left unchecked in `TODO_V2.md`
  regardless, per the hard rule against claiming device validation from
  source review alone: no physical device with a notch/cutout has verified
  any of this, before or after this change.
- **Bottom navigation not covering final actions (newly audited, checked).**
  `AppShellV2.tsx` renders `<header>`/`<main>`/`<nav>` as plain block-flow
  siblings, and `.bottom-nav` uses `position: sticky; bottom: 0`, not
  `fixed`/`absolute` — a sticky element reserves its own space in normal
  flow, so it structurally cannot overlap `main`'s content box. Confirmed
  with an isolated Playwright reproduction of the exact same markup/CSS at
  375x700 with short main content: the last actionable button's bottom edge
  landed at y=356, the nav's top edge at y=372 — no overlap. (Throwaway
  verification script, not committed — the claim in `TODO_V2.md` is the CSS
  mechanics, which this reproduction corroborates, not a permanent test.)
- **Left unchanged, still unchecked:** the 320px-viewport item (the
  real-browser check tests 360px, not 320px — no change made here) and the
  "single-column phone layout... deliberately audited against every
  screen" item (out of this track's time budget; nothing regressed it, but
  nothing newly proves it either).

## What was deliberately left open, and why

- **`orientation: "portrait-primary"` in a web app manifest (V2-131).**
  Checked first, per this task's own instructions: `webapp/index.html` has
  no `<link rel="manifest">`, and there is no `webapp/public/manifest.json`,
  no `.webmanifest` file, and no `webapp/public/` directory at all anywhere
  in the repository. Introducing a PWA manifest from scratch is a larger,
  separate decision (icon assets, `start_url`, `display` mode, whether the
  device's HTTP server should serve it with the right content type) than
  this one checkbox, and was not invented here. UI_UX_SPEC_V2 §12.4 itself
  says this is a "SHOULD... as progressive enhancement" whose correctness
  "does not depend on" it, so nothing else in V2-131/V2-132 was blocked by
  leaving this open.
- **Real device verification (V2-131/V2-132's device-classification claims,
  V2-130's cutout/safe-area claims).** Per the hard project rule ("Don't
  claim physical hardware validation from compilation, host fakes, or CI
  device builds alone"), every claim here about real Chromium's compound
  media-query evaluation, real notches/cutouts, or real tablets/foldables
  is left unchecked or explicitly caveated, even where the CSS/logic is
  genuinely in place. The Phase 13 exit gate's "Real Android phone
  portrait/landscape tests pass" and "Tablet and desktop landscape tests
  pass" remain unchecked for the same reason.
- **No new Playwright browser-level scenario for the landscape block
  itself.** `webapp/tests/browser/run-browser-tests.mjs` is a large (1300+
  line), actively shared file — a parallel agent may be adding V2-133
  accessibility scenarios to it concurrently. Per this task's own
  coordination instructions, jsdom-level coverage (the `matchMedia` fake) is
  used instead, and this is the honest gap it leaves: Chromium's actual
  evaluation of `(pointer: coarse)` combined with `(orientation: landscape)`
  and `(max-height: 600px)` under real touch/viewport emulation is not
  exercised anywhere in this repository yet. The existing
  `assertTouchTargets()`/`assertResponsiveLayout()` real-browser checks
  were reused as-is (not modified) to validate the V2-130 touch-target and
  wider-layout claims, since they already existed and already ran green
  against this change.
- **320px viewport, full single-column audit (V2-130).** Not attacked in
  this track; left exactly as the prior audit found them.

## Commands run and results (this worktree, node v24.18.0 via nvm)

- `npm --prefix webapp run test` (Vitest): before this work, `577 passed
  (577)` across 59 files (`git stash -u` baseline); after, `590 passed
  (590)` across 60 files. Net +13 tests (11 new dedicated tests across the
  new `v2-landscape-phone-block.test.tsx` file (7) and the `v2-macros-page.test.tsx`
  V2-132 block (4), plus 2 new end-to-end tests in `v2-app-v2.test.tsx`).
- `npm --prefix webapp run typecheck` — clean.
- `npm --prefix webapp run lint` (`--max-warnings=0`) — clean. No new
  suppression comments were added anywhere (the one place a suppression
  looked necessary — `react-hooks/exhaustive-deps` on the new active-send
  effect — was avoided by wrapping `cancelActiveSend` in `useCallback`
  instead, not by disabling the rule).
- `npm --prefix webapp run stylelint` (`--max-warnings=0`) — clean.
- `./scripts/check-webapp.sh` (full chain: `npm ci` -> `format:check` ->
  `typecheck` -> `lint` -> `stylelint` -> `test` -> `test:coverage` ->
  `build` -> `test:browser` -> `verify-no-remote-assets.sh`) — exit 0, run
  twice (once before, once after the final `v2-app-v2.test.tsx` additions).
  `test:browser`'s three real-Chrome workflows (Macros/Quick Send,
  Snapshots/import-export, Settings/Diagnostics) all passed unmodified.

## Files touched (for reference)

```text
webapp/src/AppV2.tsx                                       (modified)
webapp/src/features/macros/v2/MacrosPage.tsx                (modified)
webapp/src/features/shell/v2/activeSendSummary.ts            (new)
webapp/src/features/shell/v2/useLandscapePhoneBlock.ts       (new)
webapp/src/features/shell/v2/LandscapeBlockSurface.tsx        (new)
webapp/src/styles.css                                        (modified)
webapp/tests/fakeMatchMedia.ts                                (new)
webapp/tests/v2-landscape-phone-block.test.tsx                 (new)
webapp/tests/v2-macros-page.test.tsx                         (modified)
webapp/tests/v2-app-v2.test.tsx                              (modified)
docs/TODO_V2.md                                              (modified)
```
