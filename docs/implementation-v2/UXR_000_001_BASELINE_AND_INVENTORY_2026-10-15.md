# UXR-000 / UXR-001 — Baseline and route/state inventory

- **Date:** 2026-10-15
- **Tasks:** UXR-000 (repository baseline), UXR-001 (route and behavior map) in
  `docs/UI_UX_REDESIGN_TODO_2026-10-15.md`
- **Baseline SHA:** `5bed6a6746c5d1518d0509afeaa86340ee129bbf` (`master`); the
  working tree held only untracked `.serena/` before this work started.
- **Node:** `v24.18.0`. Host fonts: `fc-match system-ui` resolves to Noto Sans.

No product behavior changed. Phase 0 adds a reproducible screenshot script and
this evidence.

## UXR-000 — Baseline

### Gate results on the unmodified tree

Command: `./scripts/check-webapp.sh > log 2>&1`. Exit status: **1**, at the
visual-regression step only.

| Step | Result |
| --- | --- |
| `npm ci`, format check, typecheck, lint, stylelint | pass |
| `vitest run` | 60 files, 570 tests, pass |
| `vitest run --coverage` | pass |
| `npm run build` | pass |
| `test:browser` (workflows, axe-core, USB-unavailable, startup, editing, H4, H5) | pass |
| `test:visual` | **fail**: all 86 scenario/viewport pairs differ from the committed baselines |

The visual failure is pre-existing and environmental. Every diff is a text-width
change (for example a 252.28 px label measuring 219.98 px), consistent with the
committed JSON baselines having been generated against different font metrics
than this host's Noto Sans. The baselines record computed geometry, not pixels
(see `webapp/tests/browser/visual/baselines/README.md`). Consequence for the
redesign: the visual baselines must be regenerated deliberately, once, after the
redesign lands, and will then be valid for this host only. This is a limitation,
not evidence that the old baselines were wrong.

### Bundle and firmware web-asset size

| Asset | Raw bytes | Gzip bytes |
| --- | --- | --- |
| `assets/index-*.js` | 313,486 | 93,241 |
| `assets/index-*.css` | 27,946 | 6,193 |
| `index.html` | 791 | 484 |
| Fonts / images | none | none |

webfs image (`scripts/build-webfs-image.sh --skip-frontend-build`, non-erased
4 KiB blocks, the metric `check-release-budgets.sh` uses): **466,944 bytes**
against a budget of **891,289 bytes** (85% of the 1 MiB `webfs` partition).
Headroom: 424,345 bytes (about 48% of budget unused).

### Screenshots

120 PNGs in `docs/ui-redesign/baseline-screenshots/`, named
`<scenario>--<width>x<height>.png`. Viewports: 320x568, 360x800, 390x844,
768x1024 (tablet portrait), 1024x768 (tablet landscape), 1440x900 (desktop).
Twenty scenarios: first-run-setup, first-run-review, signin-error,
startup-no-blobs, startup-empty-package, startup-snapshot-recovery, macros,
macros-overflow-menu, macros-dirty, macro-editor, packages, snapshots,
snapshots-advanced, settings, diagnostics, send-in-flight, send-failure,
execution-recovery-overlay, dialog-restart, unsaved-changes-prompt.

Regenerate with:

```bash
npm --prefix webapp run build
node webapp/tests/browser/visual/screenshot-set.mjs --out docs/ui-redesign/baseline-screenshots
```

Not captured: phone-landscape block surface (only structural JSON exists),
reconnect screen, plain loading/checking screens, and the invalid-editor-link
and settings-load-error screens. They are small text screens and are covered in
the inventory below.

## UXR-001 — Route and state inventory

Gathered by a read-only code sweep, then spot-checked against the screenshots.
The sweep did not view the diagnostics, first-run-review, startup-no-blobs or
macro-editor-over-limit screenshots and did not read `SnapshotsPage.tsx` lines
1-330; those entries are from code only.

### Phases and routes

- Pre-auth phases (`src/AppV2.tsx:57-62`): `checking`, `setup`, `signin`,
  `authenticated`, `device-unreachable`.
- Hash routes (`src/v2/routingV2.ts:19-26`): `macros` (default), `packages`,
  `snapshots`, `settings`, `diagnostics`, `macro-editor` (optional
  `?macroId=<uuid>`; absent means create, malformed means invalid).
- Bottom navigation: Macros, Packages, Snapshots, Settings. Diagnostics
  highlights Settings; the editor highlights nothing.

### Screens, dialogs, and states

| Screen | States and overlays | Destructive or consequential actions |
| --- | --- | --- |
| Checking, device unreachable | loading text; error plus Retry (reload) | none |
| First-run setup (`FirstRunSetupPage`) | loading, unavailable, form, review, complete | Apply setup (restarts Wi-Fi) |
| Sign in (`SignInPage`) | session check, check-error with Retry, form, inline error | none |
| Repository startup (`RepositoryStartupScreen`) | loading, gzip-unsupported, device-unreachable, settings-load-failed, load-failed (3 stages), first-package, package-chooser, snapshot-recovery | Start over |
| Reconnect (`DeviceReconnectScreen`) | waiting (+ slow note after 15 s), needs-reauth, error | none |
| Landscape block | idle; active-send with Cancel | Cancel and release all keys |
| Execution recovery overlay | fixed bottom panel, z-1000, not dismissible | Cancel and release all keys |
| Macros | no-package, empty, list, send states (sending, awaiting confirmation, sent), dismissible banners (cancelled, timed out, failed, key-release failed) | Delete macro (inline confirm inside overflow menu) |
| Macro editor | create, edit, missing target, invalid link, valid/invalid validation text, Advanced dialog | none (Cancel discards edits without a prompt) |
| Packages | create form, search, no-match, row with rename and delete confirm | Delete package (inline confirm) |
| Snapshots | loading, error, empty, retention advisory, row load error, delete (type ID), replace (Advanced), import confirm, export | Delete, Replace, Import replace; Unsaved-changes prompt on load/import |
| Settings | Device, Access point, Station, Password, Diagnostics link, Session, Device actions | Restart dialog; Reset settings and Factory reset (type phrase); Remove station network has no confirmation |
| Diagnostics | loading, error, loaded cards, copy/download | none |
| Unsaved-changes prompt | dialog: Cancel, Export, Save snapshot, Discard | Discard |

Global banners rendered above every authenticated route (`AppV2.tsx:426-468`):
stale USB status, execution-state unavailable with retry, package-selection
persistence failure with retry.

### Layout facts

- Authenticated shell: `h-dvh overflow-hidden` column; header and bottom nav are
  fixed chrome; `#main-content` is the single scroll region. Width
  `min(100%,48rem)`, `64rem` from `60rem` up. A phone-first column, not a
  desktop layout.
- Breakpoints in markup: `min-[60rem]`, `min-[34rem]` (Card two-column),
  `width<=32rem` (HeaderActions, MacroRow, PackageRow, SnapshotRow, recovery
  overlay), `width<=40rem` (Dialog heading), `26rem` (storage summary). The only
  CSS `@media` is reduced motion. Landscape block is JS `matchMedia`
  `(orientation: landscape) and (pointer: coarse) and (max-height: 600px)`.
- Sticky/fixed: macro editor block `sticky top-0 z-10`; Dialog `fixed inset-0
  z-20`; landscape surface `fixed inset-0 z-[100]`; recovery overlay `fixed
  z-[1000]`.
- Safe areas: `viewport-fit=cover`; insets used by shell header/nav,
  StandaloneScreen, landscape surface, recovery overlay. Dialog does not use
  them.
- Virtual keyboard: no handling beyond `dvh` and browser defaults.
- Keycap and uppercase styling: `src/styles.css:184-246` (bottom-edge button
  with press travel), `Eyebrow.tsx`, nav labels, `dt`. Palette is the
  "PBT and petrol" `@theme`.
- Stale references: comments mention a `short:` variant that no longer exists,
  and a 42rem editor boundary scenario has no matching source rule.

### Macro editor insertion tools

All in `MacroEditorPage.tsx`: `insertAtCursor` (`:188`), named keys including
Tab, F1-F12, navigation keys and CTRL/SHIFT/ALT/GUI (`:52-90`), literal `{{` and
`}}` buttons, delay insertion (`:197`, silently ignores an out-of-range value),
and Go to error. There is **no key-combo builder**: a simultaneous group such as
`[{CTRL}{SHIFT}t]` is typed by hand (the compiler supports it, and a test types
it by hand). There is **no escaped-bracket button** (`[[` and `]]` are valid
source).

### Settings gap

Code comments in `SettingsPage.tsx` and `ConfirmPhraseDialog.tsx` mention
sending-behavior and source-preview settings, but no control for them exists.
`SPEC_V2.md` and `UI_UX_SPEC_V2.md` were not consulted for whether they should;
the redesign must not add one without that check.

### Tests that assert visible text or structure

| Area | Files in `webapp/tests/` |
| --- | --- |
| Phases, startup | `v2-app-v2-startup`, `v2-app-v2-wiring`, `v2-repository-startup-screen` |
| Setup, sign in | `v2-first-run-setup-page`, `v2-sign-in-page` |
| Shell | `v2-app-shell`, `v2-before-unload-guard`, `v2-app-v2-orientation` (hooks `.app-shell`, `.landscape-block`), `v2-landscape-phone-block` |
| Recovery, reconnect | `v2-execution-recovery`, `v2-device-reconnect`, `v2-device-reconnect-screen` |
| Macros, editor | `v2-macros-page-list/send/actions`, `v2-macro-editor-page` (button texts `ENTER`, `CTRL`, `Literal {`, `Insert delay`, `Advanced`) |
| Packages, snapshots | `v2-package-management-page`, `v2-snapshots-page-*`, `v2-unsaved-changes-prompt` |
| Settings, diagnostics | `v2-settings-page`, `v2-diagnostics-page` |
| Class guards | `component-variant-maps`, `no-descendant-variant-nesting`, `no-fixed-standalone-child`, `no-rendered-property-collisions`, `tailwind-class-collisions` |
| Real Chrome | `tests/browser/workflows/*.mjs`, `run-*-tests.mjs`, axe via `lib/accessibility.mjs`, selector `.storage-summary` |
| Visual | `tests/browser/visual/` and 93 JSON files under `baselines/` |

Static scans that apply to new code: browser-storage and console prohibition
tests, and `verify-no-remote-assets.sh`.

## Problem list with target and acceptance condition

Phase 0 exit gate: each known problem names the task that fixes it and a
checkable condition.

| # | Problem (evidence) | Target | Acceptance condition |
| --- | --- | --- | --- |
| 1 | Header takes 2-3 rows at 320 px when dirty; Save snapshot wraps (`macros-dirty--320x568`) | UXR-020 | App bar plus nav leave at least 60% of 568 px for content; Save visible without scroll |
| 2 | Bottom nav clips "SNAPSHOTS" at 320 px | UXR-020 | All four labels fully visible at 320 px |
| 3 | Macro row shows 7 buttons; only Send/Edit of row 1 visible at 320 px; Delete/Duplicate need an unexplained menu | UXR-040 | Send and Edit visible per row; secondary actions discoverable by label; at least two rows visible at 390x844 |
| 4 | First-run Review button below the fold (about 975 px page at 320) | UXR-030 | Each step's primary action in the first viewport at 320x568 |
| 5 | Macro editor: sticky block fills about 500 px at 320; insert tools, validation and the reason Save is disabled are below the fold | UXR-042 | Save, Cancel and an insert-tools entry visible at 320x568; picker on its own subpage |
| 6 | No key-combo builder, no escaped-bracket button | UXR-042 | Builder shows generated source before insertion; bracket escapes insertable; compiles via `compileMacro` |
| 7 | Settings is one long page; destructive actions at the bottom | UXR-060 | One group per screen; destructive actions in a separate area |
| 8 | Snapshots: nav overlaps row Advanced at 320; Import/Export below fold; Replace two taps deep | UXR-051/052 | No overlap; import/export on its own screen |
| 9 | Execution recovery overlay covers about 60% of 320x568 and hides nav | UXR-041 | Cancel and release stay visible; overlay no taller than 40% of viewport |
| 10 | Raw API codes in errors ("invalid_credentials: ...") | UXR-062 | Primary error copy is plain language; code kept in secondary detail |
| 11 | Explanatory copy listed in the inventory above | UXR-070 | Each block removed or replaced with a decision/consequence/recovery message |
| 12 | Keycap, uppercase, "PBT and petrol" styling | UXR-012, UXR-072 | No bottom-edge buttons or uppercase legends remain in shared components |
| 13 | Remove station network has no confirmation | UXR-062 | Not changed unless `SPEC_V2.md` says so; decide in UXR-060 after reading it |
| 14 | Delete macro/package confirmations are inline, not dialogs; macro editor Cancel never prompts for unsaved edits | UXR-062 | Confirmation names object and consequence; editor behavior preserved unless spec says otherwise |
| 15 | No virtual-keyboard handling | UXR-021 | Focused field and its action remain visible with keyboard open in Chrome emulation |
| 16 | Visual baselines are host-font-specific | UXR-072 | Baselines regenerated once after redesign; limitation recorded |
| 17 | Stale `short:` and 42rem references | UXR-021 | Removed with the layout rewrite |

## Remaining Phase 0 limitations

- The mapping of each flow to `SPEC_V2.md` behavior is by test file, not by spec
  section. Section-level mapping happens per screen when each is redesigned.
- Real-device and hardware behavior are not exercised by any of this.
