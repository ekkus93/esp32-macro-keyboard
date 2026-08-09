# V2-110 through V2-116 — Snapshots, import, and export UI

**Phase:** 11 — Snapshots, import, and export UI
**Tasks:** V2-110 (Manual Save snapshot), V2-111 (Snapshot management),
V2-112 (Advisory retention target), V2-113 (Dirty-work protection during
load), V2-114 (Unreadable snapshot recovery), V2-115 (Import and export),
V2-116 (Advanced non-atomic replace), plus V2-103's remaining
snapshot-load/import-replacement trigger points.

**Commit:** `90f5eecadbc8c45d9f7b710921d85228d60bfd64` (worktree branch
`worktree-agent-abe90a556231d208a`), authored by an isolated worktree agent
(Track K).

**Toolchain used:** Node `v24.18.0` (nvm), npm `11.16.0`, host OS Ubuntu (via
`/home/phil/.nvm`), Chromium 150.0.7871.128 (snap package) for the real-browser
gate. Confirmed against `.nvmrc` before running anything.

## What this covers

Phase 7 (V2-073) had already built the full snapshot API/logic layer
(`webapp/src/v2/snapshotClient.ts`, `repositoryCodec.ts`, `gzip.ts`) with its
own Vitest coverage, and Phase 10 had built `UnsavedChangesPrompt.tsx` as a
tested-but-unwired primitive. This phase's job was almost entirely the
**UI** and the **remaining logic gaps** the TODO items call out explicitly:

1. A real Snapshots screen (`SnapshotsPage.tsx`) — Snapshot Management,
   the advisory retention indicator, dirty-work protection during load,
   unreadable-snapshot recovery, and import/export — wired into `AppV2.tsx`'s
   "Snapshots" bottom-navigation destination (previously a placeholder).
2. Two logic gaps V2-110 named explicitly that the Phase 7 client did not
   yet close:
   - **"Validate the entire repository"** before upload — the existing
     `saveWorkingCopyAsSnapshot` serialized and size-checked but never called
     `validateRepositoryForUse`. Fixed: it now validates first and throws a
     new `SnapshotValidationError` (with structural issues) on failure,
     before any network call.
   - **"Mark saved only after `201 Created`"** — `v2PostBinary` accepted any
     `2xx` status. Fixed: it now requires exactly `201`.
3. V2-116's advanced non-atomic replace flow
   (`replaceSnapshotWithWorkingCopy`), which did not exist anywhere yet.
4. Threading a `loadedBlobId` through the startup state machine
   (`RepositoryStartupReady`/`StartupResult`) so V2-111's "loaded snapshot
   indicator" is accurate immediately after sign-in, not just after a
   same-session Save/Load.
5. Wiring `UnsavedChangesPrompt.tsx` to its snapshot-load and
   import-replacement trigger points (2 of the 5 V2-103 names) — Sign Out,
   reset settings, and factory reset remain Phase 12's job (their screens
   don't exist yet).

## Files changed

- `webapp/src/v2/snapshotClient.ts` — added `SnapshotValidationError` and
  the validation call in `saveWorkingCopyAsSnapshot`; added
  `replaceSnapshotWithWorkingCopy` (V2-116).
- `webapp/src/v2/apiClient.ts` — `v2PostBinary` now requires exactly `201`.
- `webapp/src/v2/snapshotRetention.ts` (new) — `evaluateSnapshotRetention`,
  pure retention-target-vs-count evaluation (V2-112).
- `webapp/src/v2/startup.ts` — `StartupResult`'s `"ready"` variant now
  carries `blobId: string` (the loaded blob's ID).
- `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx` —
  `RepositoryStartupReady` now carries `loadedBlobId: string | null`,
  threaded through every path that reaches `onReady` (first-package,
  package-chooser, resolved, and the snapshot-recovery view's equivalents).
- `webapp/src/features/shell/v2/UnsavedChangesPrompt.tsx` — added an
  optional `discardLabel` prop (default `"Discard changes"`) so a caller can
  use UI_UX_SPEC_V2 §9.4's more specific "Discard changes and load" without
  forking the component; updated its module doc to reflect the two newly
  wired trigger points.
- `webapp/src/features/snapshots/v2/SnapshotsPage.tsx` (new, ~760 lines) —
  the Snapshots screen: blob list with Load/Download/Delete/Advanced-replace
  per row, storage usage and retention-target header, Save current snapshot,
  the dirty-load `UnsavedChangesPrompt`, and the Import/Export panel with its
  own dirty-import `UnsavedChangesPrompt`.
- `webapp/src/AppV2.tsx` — wires `SnapshotsPage` into the `"snapshots"`
  route (replacing `PlaceholderPage`); adds `loadedBlobId` state, seeded
  from `ready.loadedBlobId` and updated by the shared `saveSnapshot`
  handler and by `SnapshotsPage`'s own actions.
- `webapp/tests/browser/run-browser-tests.mjs` — a real multi-blob fixture
  server (`POST`/`GET`/`DELETE /api/v1/blob[/:id]`, dynamic list/storage
  accounting) and a new `runSnapshotsWorkflows` scenario (see below).
- Tests: `webapp/tests/v2-snapshot-client.test.ts` (extended),
  `webapp/tests/v2-snapshot-retention.test.ts` (new),
  `webapp/tests/v2-snapshots-page.test.tsx` (new, 33 tests),
  `webapp/tests/v2-unsaved-changes-prompt.test.tsx` (extended),
  `webapp/tests/v2-api-client.test.ts` (extended),
  `webapp/tests/v2-repository-startup-screen.test.tsx` (updated for the new
  `loadedBlobId` field).

## Commands run and results

```text
node --version                 # v24.18.0 (via nvm, matches .nvmrc)
npm ci                         # clean install, 430 packages, 0 vulnerabilities
npm run format:check           # pass (after one npm run format:write pass)
npm run typecheck              # pass
npm run lint                   # pass (eslint --max-warnings=0)
npm run stylelint              # pass
npm run test                   # 51 files, 522/522 tests pass
npm run test:coverage          # pass; 522/522, no new gap in policy files
npm run build                  # tsc -b && vite build, succeeds
npm run test:browser           # build + real-Chrome run, both workflows pass
./scripts/verify-no-remote-assets.sh webapp/dist   # pass
./scripts/check-webapp.sh      # full chain, pass end to end
```

Vitest: 51 test files, 522 tests, 522 passed, 0 failed (up from 478 tests /
49 files at the start of this session — 44 new tests: 33 in
`v2-snapshots-page.test.tsx`, 4 in `v2-snapshot-retention.test.ts`, 5 added to
`v2-snapshot-client.test.ts`, 1 added to `v2-unsaved-changes-prompt.test.tsx`,
1 added to `v2-api-client.test.ts`).

### Real-browser stability

`node tests/browser/run-browser-tests.mjs` (and `npm run test:browser`, and
the `test:browser` step inside `./scripts/check-webapp.sh`) were run
**6 consecutive times** after the fixes below were in place, all clean:

```text
Real Chrome v2 Macros page/Quick Send workflows passed.
Real Chrome v2 Snapshots/import-export workflows passed.
```

Getting there required diagnosing and fixing three real, reproducible
problems in the harness itself, not the application:

1. **A navigation race the assertions themselves caused.** Two of the new
   scenario's waits used `document.body.innerText.includes('Lab bench
   workflow')` (or `'Imported bench'`) as a "the async load/import finished
   and navigated back to Macros" signal. That text is also the *selected
   package name shown in the app header on every route* — it (and "Unsaved
   changes" clearing) becomes true the instant the synchronous
   `store.discardChanges()` / `store.applyImport()` runs, **before** the
   async `performLoad()`/`applyImport()` chain that follows it has actually
   called `onOpenMacros()`. The test would then click the next button while
   still on the Snapshots route, and the in-flight navigation would land a
   moment later, mid-assertion. Fixed by waiting for a Macros-page-only
   marker (`"Add macro"`) instead of header-only text.
2. **A snap-confined Chromium's private `/tmp`.** Real download/import
   plumbing (`Browser.setDownloadBehavior`, `DOM.setFileInputFiles`) worked
   from Chrome's own point of view — its `Browser.downloadProgress` event
   reported `state: "completed"` with a `filePath` — but the file was
   invisible to this (unconfined) Node process when that path was under
   `os.tmpdir()`. `snap-confine` gives the browser its own private `/tmp`
   mount namespace; the two processes were looking at different
   filesystems. Fixed by moving the download/import-fixture exchange
   directories under `$HOME` instead.
3. **The same snap's AppArmor profile excludes top-level hidden
   directories under `$HOME`.** The first `$HOME`-based fix used a
   `.`-prefixed directory name and hit the identical invisible-file symptom
   again — `snap.chromium.chromium`'s profile grants read/write under
   `@{HOME}` explicitly *except* toplevel hidden (dot) directories. Fixed by
   using a non-hidden directory name.
4. (Minor, same root cause as #2/#3) The download-completion CDP event was
   also observed to arrive very slightly before the file was visible to the
   separate Node process even once directories were fixed; a short bounded
   retry (up to 2s) around the post-event `readFile` absorbs that.

None of these three are application bugs — the app's own behavior (gzip
compress/decompress, real file download, real file selection) was correct
throughout; all three were harness/environment artifacts specific to running
a snap-packaged Chromium. They are documented in code comments at each fix
site (`run-browser-tests.mjs`) for the next person who hits the same thing.

### What the new real-Chrome scenario covers (`runSnapshotsWorkflows`)

Against the real built app, real gzip (`CompressionStream`/
`DecompressionStream` — unavailable to jsdom, so this is the one thing the
Vitest suite structurally cannot exercise), and a real multi-blob fixture
server:

- Snapshot list rendering (blob ID, size, retention target).
- Manual Save (V2-110): an explicit click adds a new blob; the original
  blob is asserted still present (additive, not automatically pruned).
- Dirty-work protection during load (V2-113): dirtying the working copy via
  a real reorder, then Load showing the warning with all four spec'd
  choices (`Cancel`, `Export working copy`, `Save snapshot`, `Discard
  changes and load`), then `Discard changes and load` actually discarding
  and loading.
- Export (V2-115): a real Chrome download via `Browser.setDownloadBehavior`,
  the downloaded file's name asserted to end with
  `.emk-repository.json.gz`, and its bytes gunzipped and JSON-parsed on the
  Node side to confirm they match the current working copy.
- Import (V2-115): a real file selected via CDP `DOM.setFileInputFiles`
  (scripts cannot assign `HTMLInputElement.files` directly, so this is the
  only way to exercise real file selection at all), the package/macro-count
  confirmation panel, and the resulting dirty, replaced working copy.
- Manual deletion (V2-111): exact-blob-ID-confirmed delete, asserted against
  the fixture server's own state (not just the DOM) to confirm exactly one
  blob was removed and the other left untouched.

**Deliberately not covered in the real-browser scenario** (see "Deferred /
not covered" below): the Advanced non-atomic-replace flow (V2-116), the
unreadable-snapshot-recovery inline error (V2-114), and the
import-while-dirty warning's own four choices. All three have thorough
Vitest coverage (dependency-injected, deterministic); adding them to the
already-long real-Chrome scenario was judged lower value than keeping that
scenario's runtime and flakiness surface bounded, given the explicit
instruction to prioritize verified stability over maximum real-browser
surface area.

## Test names and counts (new/changed files)

- `webapp/tests/v2-snapshots-page.test.tsx` — 33 tests across V2-111
  (management surface), V2-112 (retention indicator + no-delete-on-save),
  V2-110/V2-111 (manual load + package resolution), V2-113 (dirty-load
  warning, all four choices, discard-and-load, save-then-load), V2-114
  (unreadable/invalid snapshot recovery), V2-111 (download/delete/no
  auto-delete), V2-116 (replace success/add-failure/delete-failure), V2-115
  (export, import counts/confirm/dirty-warn/discard-and-import/error/cancel).
- `webapp/tests/v2-snapshot-retention.test.ts` — 4 tests for
  `evaluateSnapshotRetention` (at/below target, over target, target `0`
  disables, the spec'd "sixth snapshot" boundary).
- `webapp/tests/v2-snapshot-client.test.ts` — 5 new tests: validation blocks
  save (V2-110), no-DELETE-on-save (V2-116), and the three
  `replaceSnapshotWithWorkingCopy` outcomes (delete+add success,
  delete-only failure, delete-success/add-failure).
- `webapp/tests/v2-api-client.test.ts` — 1 new test: `v2PostBinary` rejects
  a non-`201` success status.
- `webapp/tests/v2-unsaved-changes-prompt.test.tsx` — 1 new test: the
  `discardLabel` override renders "Discard changes and load".
- `webapp/tests/v2-repository-startup-screen.test.tsx` — updated one
  existing exact-match assertion to include the new `loadedBlobId` field;
  no behavior change to the test itself.

## TODO_V2.md checkboxes closed

Checked (with evidence references) in `docs/TODO_V2.md`:

- V2-110 — all 8 items.
- V2-111 — all 6 items.
- V2-112 — all 5 items.
- V2-113 — all 5 items.
- V2-114 — all 4 items.
- V2-115 — all 6 items.
- V2-116 — all 4 items.
- Phase 11 exit gate — all 3 items.

**Left unchecked, honestly, and why:**

- V2-103's "Warn before Sign Out, snapshot load, import replacement, reset
  settings, and factory reset" — text updated to state exactly 2 of 5
  trigger points are now wired (snapshot load, import replacement). Sign
  Out, reset settings, and factory reset stay unchecked: their screens
  (Sign Out control, Settings UI) don't exist before Phase 12
  (V2-120/V2-121), which is out of this track's scope.

No other TODO_V2.md lines were touched — the diff is confined to the V2-103
item's text, the V2-110 through V2-116 checkbox blocks, and the Phase 11
exit gate, per the file-surface constraint (another track is editing
`docs/TODO_V2.md` in its own worktree concurrently).

## Deferred / not covered

- The Advanced non-atomic-replace UI (V2-116) and unreadable-snapshot
  recovery (V2-114) are Vitest-only, not real-Chrome — a deliberate scope
  cut explained above, not an oversight. Their logic (delete-then-add
  ordering, the three outcome states, decode/schema error surfacing) is
  fully covered by dependency-injected unit tests at both the client
  (`snapshotClient.ts`) and page (`SnapshotsPage.tsx`) layers.
- V2-115's "exclude every device credential, session, key, and diagnostic
  field" is satisfied by construction of the frozen `Repository` schema
  type (Phase 1) rather than by a new dedicated test asserting the absence
  of such fields — the type has no such fields to exclude, and
  `validateRepositoryForUse`'s exact-key check would reject any extra field
  a malformed import file tried to add.
- Hardware-in-the-loop evidence (a physical ESP32-S3R8 exercising real
  `POST`/`GET`/`DELETE /api/v1/blob` under this UI) was not attempted — this
  track's evidence is host-only (Vitest + a real browser against a Node
  fixture server), consistent with `docs/TODO_V2.md` §0.1's distinction
  between host/browser evidence and hardware evidence. No task here is
  marked complete on the strength of hardware validation it does not have.

No unchecked TODO_V2.md task in this track's scope is being claimed
complete.
