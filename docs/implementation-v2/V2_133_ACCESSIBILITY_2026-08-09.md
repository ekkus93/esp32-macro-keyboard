# V2-133 — Accessibility

**Phase:** 13 — Adaptive layout, portrait guard, and accessibility.
**Task:** V2-133 only. V2-130/V2-131/V2-132 (layout breakpoints, portrait
guard, landscape active-send safety) were explicitly out of scope for this
task — a separate agent was assigned those in parallel — so no CSS
breakpoint or orientation-detection logic was touched here.

**Branch:** `worktree-agent-a96589bd33b7a7b43` (isolated worktree). Not
pushed, not merged to `master`, per the task's standing instruction not to
touch `master` directly.

**Toolchain used:** Node `v24.18.0` via `~/.nvm/versions/node/v24.18.0/bin`
on `PATH` (the ambient shell defaulted to `v22.12.0`; switched explicitly
before every command, matching `.nvmrc`/`engine-strict=true`), npm `11.16.0`,
Playwright's bundled Chromium (cached at `~/.cache/ms-playwright`).

## What this covers

Per the parent task's audit summary, three of the nine `UI_UX_SPEC_V2` §14
requirements already held incidentally before this task and six did not.
This task closed the six gaps that were actually closeable within V2-133's
scope, left the two genuinely out-of-scope/unverifiable ones honestly
unchecked, and re-verified the three that already held.

### 1. Trap and restore focus in dialogs — now real, everywhere

`webapp/src/components/AccessibleDialog.tsx` (the retired v1 tree) had a
real implementation, but the actual v2 UI's 7 `role="alertdialog"` surfaces
were plain markup with no focus management. Extracted the trap logic into a
reusable v2-tree hook and wired it into all 7:

- `webapp/src/features/shell/v2/useFocusTrap.ts` (new) — initial focus onto
  the first focusable element, `Tab`/`Shift+Tab` wrap, `Escape`→`onClose`,
  and focus restoration when the trap deactivates or unmounts.
- Wired into: `MacrosPage.tsx`'s macro-delete confirmation,
  `PackageManagementPage.tsx`'s package-delete confirmation,
  `SnapshotsPage.tsx`'s snapshot-delete confirmation and its
  import-replace-working-copy confirmation, `SettingsPage.tsx`'s Restart
  confirmation and `ConfirmPhraseDialog` (reset-settings/factory-reset), and
  the shared `UnsavedChangesPrompt.tsx`.

**A real subtlety found and fixed during this work:** `MacrosPage.tsx`,
`PackageManagementPage.tsx`, and `SnapshotsPage.tsx`'s row-level delete
confirmations use a ternary that *unmounts* the "Delete" trigger button and
mounts the confirmation panel in its place (rather than layering the
confirmation over a persistent trigger, the way `SettingsPage.tsx`'s dialogs
do). `useFocusTrap`'s automatic "whatever had focus when it activated"
capture runs in an effect *after* that same render already removed the
trigger from the DOM — in a real browser, removing the focused element
resets `document.activeElement` to `document.body`, so the naive capture
would have restored focus to nowhere useful. Added an explicit
`restoreFocusRef` option: the caller attaches it to the *replacement*
trigger button (the one that remounts once the trap deactivates); React
assigns that ref during the same commit that removes the dialog, before the
hook's effects run, so it already points at the right node. Wired into all
three row-level delete confirmations. Verified with a test asserting
`document.activeElement` is the *reappeared* button (not object-identity
with the original, now-gone, node) after `Escape`.

- `webapp/src/features/shell/v2/useDismissibleOverlay.ts` (new, separate
  hook — see item 2) has no focus-trap responsibility; it only handles
  `Escape`/outside-click dismissal for the non-modal overflow menu.

### 2. Make all controls keyboard accessible — the one identified gap fixed

The audit found no bare-`onClick` `<div>`/`<span>` and no custom
`role="button"` anywhere (re-confirmed, still true), but flagged that
`MacrosPage.tsx`'s "More actions" overflow menu had no `Escape`-to-close or
outside-click dismissal. Fixed via `useDismissibleOverlay.ts`, wired into
`MacroOverflowMenu`. The dismiss-hook and the delete-confirmation's
focus-trap are deliberately only mutually exclusive while confirming
(`open && !confirmingDelete`), so `Escape` while the delete confirmation is
open cancels the confirmation, not the whole menu.

### 3. Move first/up/down/last — direct target-index reordering

`webapp/src/v2/repositoryEditing.ts` gained `moveMacroToIndex` and
`movePackageToIndex` (direct move to an arbitrary index — the existing
`moveMacro`/`movePackage` only ever swap adjacent items, which cannot
express "jump to the end of a 20-item list" without 19 clicks).
`MacrosPage.tsx` and `PackageManagementPage.tsx` (the real v2 pages; the
pre-v2 `features/package/PackageManagementPage.tsx` that already had all
four buttons is dead code pending V2-140 and was left alone) now render
"Move first", "Move up", "Move down", "Move last" for every row, each with
an exact, name-bearing `aria-label` (`Move first`/`Move up`/`Move down`/
`Move last` visible text stays constant per UI_UX_SPEC_V2's own wording;
`aria-label`s read "Move {name} to first" / "Move {name} up" / "Move {name}
down" / "Move {name} to last" — the existing "Move {name} up"/"Move {name}
down" labels are byte-for-byte unchanged, so no existing test broke).

### 4. Reduced motion — a standing CSS policy

`webapp/src/styles.css` gained a `@media (prefers-reduced-motion: reduce)`
block collapsing `animation-duration`/`animation-iteration-count`/
`transition-duration`/`scroll-behavior` for every element. No
`transition`/`animation`/`@keyframes` exists anywhere in `webapp/src/*.css`
today, so this guard is currently vacuous in effect — but it is now a real,
`stylelint`-checked rule rather than the prior absence of one, so a future
motion addition is disabled by construction unless it explicitly opts out.

### 5. Automated accessibility checks — real axe-core, scoped honestly

Added `@axe-core/playwright` as an exact-pinned devDependency
(`webapp/package.json`, `4.12.1`) and a narrow, clearly-separated addition to
`webapp/tests/browser/run-browser-tests.mjs` (its own function,
`runAccessibilityScan`, plus three one-line call sites after the existing
workflow functions — deliberately not folded into
`runBrowserWorkflows`/`runSettingsWorkflows`, per the task's instruction to
keep this addition narrow since another agent may be adding orientation
scenarios to the same file in parallel). It scans the Macros, Snapshots, and
Settings pages (the three pages the harness already navigates) with axe-core's
`wcag2a`/`wcag2aa`/`best-practice` rule sets and fails the build on any
`serious`/`critical` finding.

**Honest scope note:** a temporary unfiltered run during this task (not part
of the committed code) found **zero axe-core violations at any impact
level** — `moderate`/`minor` included — across all three scanned pages as of
2026-08-09. The `serious`/`critical` filter is deliberate forward-looking
policy (a future color-contrast `moderate` finding against an unfinished
placeholder palette shouldn't block unrelated work), not something currently
hiding a real finding. The scan does **not** yet cover Sign-in, First-run
setup, Macro editor/preview, or Diagnostics — those screens' keyboard/ARIA
logic is exercised by Vitest component tests instead, not by axe-core
against a real rendered DOM.

### 6. Prevent hidden source from leaking through accessible names — left as moot

Re-verified: `features/macros/v2/MacroPreviewPage.tsx` still renders
`macro.source` unconditionally (no hide/reveal toggle exists there — that's
Phase 12's `V2-120` "source-preview preference" feature, not built yet).
`MacrosPage.tsx`'s own reveal/hide toggle (already shipped, V2-092) already
avoids leaking source through its `aria-label`s (`Reveal source for
{name}`/`Hide source for {name}`, never the source text itself). Since
there is no hidden state to leak from in `MacroPreviewPage.tsx`, this
requirement is moot rather than satisfied there — left unchecked rather than
inventing a source-hiding feature that is explicitly out of this phase's
scope.

### Already true, re-verified unchanged

- **Live regions without re-announcing every poll tick** — unaffected by
  this task's changes; re-confirmed still true by the full Vitest suite
  passing (`activeStatusText()` in `MacrosPage.tsx` is untouched).
- **Never use color as the only state indicator** — unaffected; `StatusBadge`
  untouched.
- **Source-editor labels and exact validation locations** — unaffected;
  `MacroEditorPage.tsx` untouched.

### Left honestly unchecked

- **Preserve logical focus order** — re-confirmed via grep (no CSS `order`,
  `row-reverse`/`column-reverse`, or positive `tabIndex` anywhere in
  `webapp/src`, including the new Move-first/last buttons, which render in
  plain DOM order) but still not manually or screen-reader verified.
- **Manual keyboard and screen-reader checks are recorded** (Phase 13 exit
  gate) — this is fundamentally a human-verification item. What it would
  need: a keyboard-only pass (no mouse) through every authenticated screen —
  Macros (send, reorder, overflow menu, delete confirm), Package management,
  Snapshots (including the exact-ID delete confirmation and the
  dirty-work-during-load prompt), Settings (all forms plus
  Restart/Reset settings/Factory reset), and the Macro editor/preview
  screens — plus a screen-reader pass (NVDA/JAWS/VoiceOver/Orca) confirming
  live-region announcements, dialog labeling, and that hidden macro source
  is never announced. Not done; not claimable from automation.

## Files changed

- `webapp/src/features/shell/v2/useFocusTrap.ts` (new) — shared dialog
  focus-trap-and-restore hook, with the `restoreFocusRef` escape hatch
  described above.
- `webapp/src/features/shell/v2/useDismissibleOverlay.ts` (new) — shared
  `Escape`/outside-click dismissal hook for non-modal overlays (the overflow
  menu).
- `webapp/src/features/macros/v2/MacrosPage.tsx` — `MacroOverflowMenu` now
  uses both hooks; `MacroRow` gained Move first/last buttons;
  `moveMacro` rewritten to route "up"/"down" through the existing swap
  helper and "first"/"last" through the new target-index helper.
- `webapp/src/features/macros/v2/PackageManagementPage.tsx` — `PackageRow`'s
  delete confirmation gained the focus trap (with `restoreFocusRef`); gained
  Move first/last buttons; `movePackageRow` rewritten analogously.
- `webapp/src/features/snapshots/v2/SnapshotsPage.tsx` — `SnapshotRow`'s
  delete confirmation gained the focus trap (with `restoreFocusRef`); the
  top-level import-ready confirmation gained the focus trap.
- `webapp/src/features/settings/v2/SettingsPage.tsx` — the restart
  confirmation and `ConfirmPhraseDialog` gained the focus trap.
- `webapp/src/features/shell/v2/UnsavedChangesPrompt.tsx` — gained the focus
  trap (this dialog's trigger is never itself unmounted, so no
  `restoreFocusRef` was needed).
- `webapp/src/v2/repositoryEditing.ts` — added `moveMacroToIndex`,
  `movePackageToIndex`.
- `webapp/src/styles.css` — added the `prefers-reduced-motion` guard.
- `webapp/tests/browser/run-browser-tests.mjs` — added the axe-core import
  and `runAccessibilityScan`, called after each of the three existing
  workflow functions.
- `webapp/package.json`/`webapp/package-lock.json` — added
  `@axe-core/playwright` `4.12.1` (exact-pinned devDependency).
- New tests: `webapp/tests/v2-focus-trap.test.tsx`,
  `webapp/tests/v2-dismissible-overlay.test.tsx`.
- Extended tests: `webapp/tests/v2-macros-page.test.tsx`,
  `webapp/tests/v2-package-management-page.test.tsx`,
  `webapp/tests/v2-snapshots-page.test.tsx`,
  `webapp/tests/v2-settings-page.test.tsx`,
  `webapp/tests/v2-unsaved-changes-prompt.test.tsx`,
  `webapp/tests/v2-repository-editing.test.ts`.
- `docs/TODO_V2.md` — V2-133 checkboxes and the two accessibility-specific
  Phase 13 exit-gate lines updated to match reality; the three
  layout/orientation exit-gate lines (Android, tablet/desktop, landscape
  cancellation) deliberately left untouched — out of this task's scope.
- `docs/SPEC_V2_TEST_TRACEABILITY.md` — regenerated
  (`python3 scripts/generate-spec-traceability.py`) because this task's new
  `UI_UX_SPEC_V2 §14`/`§4` and pre-existing-but-previously-uncounted
  `SPEC_V2 §8.7`/`§13.13` citations in `webapp/tests/**` shifted the citation
  set `check-docs.sh` fingerprints; regenerating (not hand-editing) is the
  script's own documented fix.

## Commands run and results

```text
# Node version pinned explicitly (ambient shell defaulted to v22.12.0):
export PATH="$HOME/.nvm/versions/node/v24.18.0/bin:$PATH"
node --version   # v24.18.0
npm --version    # 11.16.0

cd webapp
npm install --save-dev --save-exact @axe-core/playwright
  # added 434 packages (transitive tree recount), 24-line package-lock.json diff, 0 vulnerabilities

npm run format:check   # All matched files use Prettier code style!
npm run typecheck      # clean
npm run lint           # clean (eslint --max-warnings=0)
npm run stylelint      # clean (stylelint --max-warnings=0)
npm run test           # 61 test files, 610 tests passed (before this task: 577 tests)
npm run test:coverage  # 61 files, 610 tests passed; overall 83.55% stmts / 80.8% branch
npm run build           # tsc -b && vite build — clean
node tests/browser/run-browser-tests.mjs   # run 5 times total across this session, all green:
  # Real Chrome v2 Macros page/Quick Send workflows passed.
  # Real Chrome v2 Snapshots/import-export workflows passed.
  # Real Chrome v2 Settings/Diagnostics workflows passed.
  # Real Chrome axe-core accessibility scans passed.

cd ..
./scripts/check-webapp.sh   # full chain (npm ci -> format:check -> typecheck ->
                             # lint -> stylelint -> test -> test:coverage -> build ->
                             # test:browser -> verify-no-remote-assets.sh) — exit 0
./scripts/check-docs.sh     # markdownlint-cli2 (0 issues), yamllint (pre-existing
                             # warnings only, unrelated to this task),
                             # jq schema validation, spec-traceability --check — exit 0
                             # (required regenerating SPEC_V2_TEST_TRACEABILITY.md, see above)
```

Vitest count before this task: 577 tests (59 files), confirmed by running the
suite before any change. After: 610 tests (61 files) — 32 new tests, 1 file
adjusted for a new assertion added to an existing test (accounted for in the
610 total, not double-counted).

`./scripts/check-all.sh`'s firmware/host-C/`check-v2-contracts.sh` legs were
**not** run — this task touched only `webapp/`, `docs/TODO_V2.md`, and
`docs/SPEC_V2_TEST_TRACEABILITY.md`; nothing in `firmware/`, `tests/host/`,
or `tests/v2_contracts/`. Confirmed no reference to
`repositoryEditing`/`moveMacroToIndex`/`movePackageToIndex` (the one
non-purely-additive source change) exists in any `scripts/check-v2-*` gate.

## What's left open, and why

- **Manual keyboard and screen-reader checks** (Phase 13 exit gate) — not
  done; needs a human with real assistive technology, as described above.
  Not something this task, or any automated agent, can substitute for.
- **Logical focus order** — believed true (no violating CSS/`tabIndex`
  pattern found), but "believed true by absence of a known violation" is not
  the same bar as "verified" per the task's own instruction not to overclaim
  this specific item; left unchecked.
- **Hidden-source accessible-name leakage in `MacroPreviewPage.tsx`** — moot
  until Phase 12's source-preview-preference feature exists there; building
  that feature was out of V2-133's scope (it belongs to `V2-120`) and was
  not attempted.
- **axe-core scan coverage** — 3 of the app's ~10 screens. Extending it to
  Sign-in/First-run setup/Macro editor/Diagnostics would need the
  fixture server in `run-browser-tests.mjs` to support those flows (e.g. an
  unauthenticated/unprovisioned state for Sign-in and First-run setup, which
  the current fixture always serves as already-provisioned/authenticated) —
  a larger change than this task's narrow-addition instruction allowed.
- **V2-130/V2-131/V2-132** (layout, portrait guard, landscape safety) — not
  this task's scope; a separate agent's responsibility per the task
  instructions. No CSS breakpoint or orientation-detection code was touched.
