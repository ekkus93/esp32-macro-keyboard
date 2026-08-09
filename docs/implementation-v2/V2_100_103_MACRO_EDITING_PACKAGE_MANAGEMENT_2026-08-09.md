# V2-100 through V2-103 — Macro editing and package management

**Phase:** 10 — Macro editing and package management
**Tasks:** V2-100 (Macro editor), V2-101 (Macro CRUD and ordering), V2-102
(Package management), V2-103 (Unsaved-change protection), plus V2-094's
"Honor Always Preview when configured" gap left open by Phase 9.

**Commits:**

- `606fa02` — feature implementation (macro editor, package management,
  unsaved-change protection, Always Preview wiring) and its Vitest coverage.
  Authored by an isolated worktree agent (Track H).
- `24ba7cf` — `npm run format:write` applied to the files `606fa02` committed;
  `check-webapp.sh`'s `format:check` step caught pre-existing Prettier drift.
  No semantic change.
- `383f66c` — fix to `webapp/tests/browser/run-browser-tests.mjs`, described
  below. Not part of the feature; a shared-test-infrastructure defect this
  work exposed.

Track H's own agent session was interrupted (by an explicit user stop, not a
crash) before it reached the `docs/TODO_V2.md` checkbox/evidence step or ran
the full `check-webapp.sh` gate. The remainder of this report — verification,
the dialog-hang diagnosis and fix, and the checkbox evidence below — was done
directly by the coordinating session picking up Track H's already-committed
work (`606fa02`), not by a subagent.

## What Track H built (606fa02)

- `webapp/src/v2/repositoryEditing.ts` — pure macro/package CRUD helpers
  (create, edit, duplicate, delete, move/reorder), `crypto.randomUUID()` ID
  generation, equality checks so no-op edits never dirty the working copy.
- `webapp/src/features/macros/v2/MacroEditorPage.tsx` — name/source/timing
  fields, UTF-8 byte counts, directive insertion (named keys, delay, chord
  builder), live validation via the shared `macroCompiler`, exact error
  location and "Go to error", action count/duration when valid, save only to
  the working copy, cancel without touching the store.
- `webapp/src/features/macros/v2/MacrosPage.tsx` (extended) — an overflow
  menu (Preview and send, Duplicate, Delete with name-bearing confirmation)
  closing the gap Phase 9 left open; Quick Send now honors `sendMode:
  "preview"` by opening Preview and Send instead of sending directly.
- `webapp/src/features/macros/v2/PackageManagementPage.tsx` —
  create/rename/duplicate/reorder/delete packages, canonical UUID v4 IDs,
  non-dirtying Open with persisted selection, name-bearing delete
  confirmation, selection resolution after deleting the selected package.
- `webapp/src/features/shell/v2/useBeforeUnloadGuard.ts` + `AppShellV2`
  wiring — registers a `beforeunload` warning while the repository is dirty.
- `webapp/src/features/shell/v2/UnsavedChangesPrompt.tsx` — a reusable
  Cancel/Export working copy/Save snapshot/Discard changes dialog for Phase
  11/12 call sites (Sign Out, snapshot load, import replacement, reset
  settings, factory reset). **Not yet wired to a real trigger** — none of
  those screens exist before Phase 11/12 — so this component is built and
  unit-tested in isolation but does not yet fire from a real user action.
  Left open below.
- `webapp/src/v2/routingV2.ts` — macro-editor create/edit hash targets.
- `webapp/src/AppV2.tsx` — wires the above into real routes;
  `MacroEditorPage`/`PackageManagementPage` render inside `<AppShellV2>`, so
  the shell's dirty-state/Save-snapshot indicator (V2-090) covers them too.

Tests added or extended: `v2-repository-editing.test.ts`,
`v2-macro-editor-page.test.tsx`, `v2-package-management-page.test.tsx`,
`v2-before-unload-guard.test.tsx`, `v2-unsaved-changes-prompt.test.tsx`
(new); `v2-macros-page.test.tsx`, `v2-routing.test.ts`, `v2-app-v2.test.tsx`
(extended).

## Real defect found and fixed during verification (383f66c)

Track H's `useBeforeUnloadGuard` registers a genuine `window.beforeunload`
listener while the working copy is dirty. A native `beforeunload`
confirmation dialog blocks the page's JS execution thread until dismissed.
`webapp/tests/browser/run-browser-tests.mjs`'s `Cdp` class explicitly
discarded any WebSocket message with no `id` field — which is exactly how
unsolicited CDP events (including `Page.javascriptDialogOpening`) arrive — so
no dialog was ever dismissed, and every subsequent `Runtime.evaluate()` call
hung forever rather than failing.

This was latent, not something Track H's own new scenarios triggered
directly: the *existing* Phase 9 browser-test scenario reorders a macro
("Move Open terminal down"/"up") then later reloads the page. Reordering now
dirties the working copy (per V2-101's rule, active app-wide once Track H's
dirty-tracking landed), so the pre-existing reload step now hits an unhandled
native dialog.

Reproduced directly before fixing: running the unmodified harness against
`606fa02` with an explicit `timeout 90` wrapper produced exit code `124`
(timeout) and zero stdout/stderr — consistent with a page-level JS hang, not
a crash or assertion failure.

Fix: the `Cdp` class's message handler now recognizes
`Page.javascriptDialogOpening` and responds with
`Page.handleJavaScriptDialog({ accept: true })` before falling through to the
existing request/response matching logic. No current scenario asserts on the
dialog's own presence, only on app state before/after it, so auto-accepting
is safe and consistent with every other CDP interaction in this harness (act,
then assert on resulting DOM state).

Verified: 6 consecutive clean runs of
`node tests/browser/run-browser-tests.mjs` after the fix (0/1 before it, with
the failure being a reproducible hang, not a flake).

## Checkbox evidence

**V2-100 — Macro editor**: all items closed. `MacroEditorPage.tsx` implements
name/source/key-press/inter-key fields, UTF-8 byte counts, directive
insertion, live validation against the shared `macroCompiler`, exact error
location with "Go to error", action count/duration, save-to-working-copy-only,
and cancel-without-mutation — covered by
`tests/v2-macro-editor-page.test.tsx`'s four `describe` blocks (fields/byte
counts, live validation/error location, directive insertion, save/cancel).

**V2-101 — Macro CRUD and ordering**: all items closed. Create, edit,
duplicate, move/reorder, and delete are implemented in
`repositoryEditing.ts` and exercised through `MacroEditorPage`/`MacrosPage`'s
new overflow menu; IDs use `crypto.randomUUID()`; global macro-ID uniqueness
and no-op-edit-never-dirties are both directly tested in
`tests/v2-repository-editing.test.ts::"repositoryEditing — macro CRUD and
ordering (V2-101)"` and `"repositoryEditing — ID generation"`. The overflow
menu itself (the gap Phase 9 explicitly deferred here) is tested in
`tests/v2-macros-page.test.tsx::"MacrosPage — V2-101 overflow menu
(Duplicate/Delete)"`.

**V2-102 — Package management**: all items closed. `PackageManagementPage.tsx`
implements create/rename/duplicate/reorder/delete with canonical UUID v4
IDs, name-bearing destructive confirmation, and selection resolution after
deleting the selected package — covered by
`tests/v2-package-management-page.test.tsx`'s four `describe` blocks
(create/rename/duplicate/reorder/delete, selected-package deletion, ordinary
switching never dirties, search) and
`tests/v2-repository-editing.test.ts::"repositoryEditing — package
management (V2-102)"`.

**V2-103 — Unsaved-change protection**: 3 of 5 items closed.

- [x] "Keep Unsaved changes and Save snapshot visible on all operational
  screens" — `MacroEditorPage`/`PackageManagementPage` render inside
  `AppShellV2` (see `AppV2.tsx`), which already shows this per V2-090; no new
  code needed, verified by reading the render tree.
- [x] "Register `beforeunload` while dirty where supported" —
  `useBeforeUnloadGuard.ts`, tested in `tests/v2-before-unload-guard.test.tsx`
  and (indirectly, at the integration level) confirmed real by this report's
  own dialog-hang finding above — the listener unambiguously fires in a real
  browser.
- [ ] "Warn before Sign Out, snapshot load, import replacement, reset
  settings, and factory reset" — **not closed**. `UnsavedChangesPrompt.tsx`
  exists and is unit-tested in isolation
  (`tests/v2-unsaved-changes-prompt.test.tsx`), but none of its five named
  trigger points are wired to it yet, because none of those screens (Sign
  Out control, Snapshots UI, Import UI, Settings UI) exist before Phase
  11/12. Deliberately left open rather than claimed from a component that
  isn't reachable by any real user action yet.
- [x] "Offer context-appropriate Cancel, Export working copy, Save snapshot,
  and Discard options" — `UnsavedChangesPrompt.tsx` implements all four
  actions, tested in `tests/v2-unsaved-changes-prompt.test.tsx`. Its buttons
  exist and work; only its call sites (the item above) are missing.
- [x] "Never claim closed unsaved work can be recovered" — verified by
  reading `UnsavedChangesPrompt.tsx` and `useBeforeUnloadGuard.ts`'s copy;
  neither claims recoverability.

**V2-094 — Optional Preview and Send**: the two items Phase 9 left open are
now closed — see the `docs/TODO_V2.md` entries for exact evidence citations
(`tests/v2-macros-page.test.tsx`'s "the overflow menu still offers Preview
and send" and "MacrosPage — V2-094 honoring Always Preview" describe blocks).

**Phase 10 exit gate**:

- [ ] "Editing and package-management unit and browser tests pass" — **not
  fully closed**. Unit tests pass in full (see Commands below). No browser
  (real-Chrome/CDP) scenario exists yet for macro editing or package
  management specifically — `run-browser-tests.mjs` still only exercises the
  Quick Send / Macros-list flow inherited from Phases 8/9.
- [x] "No edit calls a firmware package or macro route" — true by
  construction: v2 firmware has no package/macro CRUD routes at all (deleted
  in Phase 2, continuously enforced by `check-v2-phase2-architecture.py` and
  `check-removed-features.sh`, both of which pass on every `check-all.sh`
  run), and `repositoryEditing.ts` only mutates the in-memory working copy —
  it makes no HTTP calls at all.
- [x] "Dirty-state transition matrix is fully tested" — every dirty/no-op
  transition named by V2-101/V2-102 has a direct test: content/order changes
  dirty, no-op edits don't, ordinary package switching doesn't (all three
  `describe` blocks cited above by name).

## Commands run and results (this verification pass, on commit `383f66c`)

```bash
npm --prefix webapp run test
```

478/478 passed (49 test files) — up from 407/407 before this work (`606fa02`
added ~71 new/extended test cases).

```bash
npm --prefix webapp run typecheck
npm --prefix webapp run lint
npm --prefix webapp run stylelint
npm --prefix webapp run build
```

All clean (no output beyond the command banners).

```bash
cd webapp && node tests/browser/run-browser-tests.mjs
```

Run 6 times consecutively: `EXIT=0`, "Real Chrome v2 Macros page/Quick Send
workflows passed." every time, after the dialog-handler fix. Before the fix:
`EXIT=124` (timeout) on the same unmodified command against `606fa02`.

```bash
./scripts/check-webapp.sh
```

`EXIT=0` — full chain (ci → typecheck → lint → stylelint → test → build →
local-assets, including the real-Chrome browser test) passes.

## Explicit completion statement

No unchecked task above is being claimed complete. Left open, with reasons
stated inline: V2-103's "warn before Sign Out/snapshot load/import/reset/
factory reset" (component built, not yet wired — no trigger screens exist
yet), and Phase 10's exit-gate browser-test line (unit tests solid, no
macro-editing/package-management browser scenarios exist yet). No hardware
was used or claimed anywhere in this task.
