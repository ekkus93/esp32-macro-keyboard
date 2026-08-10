# V2-140 — Delete dead v1 code (webapp scope)

Date: 2026-08-09
Branch: `worktree-agent-a4c04e31ee1480a8c` (agent worktree off `master`,
`HEAD` at `e7a7546` before this work)
Scope: `webapp/`, plus doc/script fallout the deletion required. Firmware was
**not** audited in this pass — see "Left open" below.

## Method

1. Read `webapp/src/main.tsx` and traced `AppV2.tsx`'s full import graph by
   hand (every `import` line, followed transitively).
2. For every file suspected v1-only, ran `grep -rn` for its basename across
   the **whole** `webapp/` tree (`src/` and `tests/`, not just `App.tsx`), to
   find every importer before deleting anything. Where a file was imported by
   both v1 and v2 code, kept it and recorded why.
3. Deleted confirmed-dead production files and confirmed-dead test files
   (files whose only importer was another confirmed-dead file, or a test that
   exclusively exercised deleted code).
4. Re-ran `grep -rn` for every deleted file's import path afterward
   (repository-wide, not just `webapp/`) to catch stray references in
   scripts, docs, and contracts.
5. Ran the full verification chain (below) and fixed every fallout it found.

## Reachability audit (before deleting anything)

`webapp/src/main.tsx` imports only `AppV2.tsx` (plus `styles.css`,
`management.css`). `AppV2.tsx`'s imports are exclusively: `features/*/v2/*`,
`components/ErrorBanner.tsx`, and `v2/*`. Grepping every `v2/` and
`features/*/v2/` file for `from "../api/`, `from "./api/`, `from "../../api/`
found zero hits — the entire `v1` `api/` client, and everything importing it,
is unreachable from `AppV2`.

## Deleted — production code (18 files/dirs, confirmed unreachable)

| Path | Only importer(s) before deletion |
| --- | --- |
| `webapp/src/App.tsx` | none (was the v1 entry point, never imported by `AppV2`) |
| `webapp/src/routing.ts` | `App.tsx`, `components/AppShell.tsx`, v1 feature pages |
| `webapp/src/api/` (`client.ts`, `errors.ts`, `executionGuards.ts`, `guards.ts`, `managementGuards.ts`, `packages.ts`, `routes.ts`, `README.md`) | `App.tsx` and v1 feature pages only — no `v2/` file imports from `api/` |
| `webapp/src/types/models.ts` | `App.tsx`, `api/*`, v1 feature pages only |
| `webapp/src/components/AppShell.tsx` | `App.tsx` only |
| `webapp/src/components/ConnectivityBanner.tsx` | `components/AppShell.tsx` only |
| `webapp/src/components/AccessibleDialog.tsx` | v1 `PackageManagementPage.tsx`, `PackageOperationsPage.tsx`, `SettingsPage.tsx` only — grepped every `features/*/v2/` file and `v2/`, zero hits |
| `webapp/src/features/auth/{LoginPage,SetupPage,SessionBoundary}.tsx`, `README.md` | `SessionBoundary` only imported by `App.tsx`; `LoginPage`/`SetupPage` only by `SessionBoundary` |
| `webapp/src/features/execution/` (`ConfirmExecutionPage.tsx`+`.css`, `ExecutionPage.tsx`, `ExecutionResultPage.tsx`, `executionResult.ts`, `README.md`) | `App.tsx` and each other only |
| `webapp/src/features/package/` (`PackageManagementPage.tsx`, `PackageSelectionPage.tsx`, `README.md`) | `App.tsx` only — distinct from the *kept* `features/macros/v2/PackageManagementPage.tsx`, a different file with the same base name in a different directory |
| `webapp/src/features/macros/MacroEditorPage.tsx`, `MacroLibraryPage.tsx`, `macroDraft.ts`, `README.md` | `App.tsx` and each other only — distinct from the *kept* `features/macros/v2/MacroEditorPage.tsx` |
| `webapp/src/features/settings/{DiagnosticsPage,PackageOperationsPage,SettingsPage}.tsx`, `README.md` | `App.tsx` only — distinct from the *kept* `features/settings/v2/{DiagnosticsPage,SettingsPage}.tsx` |
| `docs/schemas/{all-data-backup,diagnostic-report,macro-set-package}.schema.json` | Identified as v1-era, not-part-of-v2-contracts by `docs/implementation-v2/V2_000_002_BASELINE_INVENTORY_COMPLETION_2026-08-09.md`; grepped repo-wide, referenced only by historical evidence docs and `scripts/check-docs.sh`'s `jq empty` syntax check (fixed, see below) |

## Deleted — dead tests (17 files, 129 tests)

All exclusively imported deleted v1 production files (verified by reading
each file's imports before deletion):

`tests/app-auth.test.tsx`, `app-execution.test.tsx`, `app-macros.test.tsx`,
`app-packages.test.tsx`, `app-routing.test.tsx`, `execution-confirmation.test.tsx`,
`execution-identity.test.tsx`, `management-screens.test.tsx`,
`management-api.test.ts`, `package-management.test.tsx`, `spec-screens.test.tsx`,
`api-execution-submit.test.ts`, `api.test.ts`, `api-timeout.test.ts`,
`guards.test.ts`, `routing-confirmation.test.ts`, and the now-orphaned fixture
helper `appFixtures.ts` (used only by the 16 test files above — confirmed by
grepping every remaining test file for `appFixtures` before deleting it).

**Not deleted despite the v1-looking name**: `tests/app.test.ts` imports only
`../src/types/limits` (kept, live v2 code) and asserts firmware-mirrored limit
constants — it is not v1 test debt, just a pre-v2-naming test file. Kept.

## Kept despite looking like candidates — and why

| Path | Why kept |
| --- | --- |
| `webapp/src/components/ErrorBanner.tsx` | Imported by 12+ `v2/`-reachable files (`AppV2.tsx`, `AppShellV2.tsx`, `FirstRunSetupPage.tsx`, `SignInPage.tsx`, `SnapshotsPage.tsx`, `MacroPreviewPage.tsx`, `RepositoryStartupScreen.tsx`, `DiagnosticsPage.tsx` (v2), `DeviceReconnectScreen.tsx`, `SettingsPage.tsx` (v2), and others) |
| `webapp/src/components/StatusBadge.tsx` | Imported by `features/macros/v2/MacroPreviewPage.tsx` and `features/shell/v2/AppShellV2.tsx` |
| `webapp/src/types/limits.ts` | Imported throughout `v2/` (`repositoryEditing.ts`, `apiRequestGuards.ts`, `apiGuards.ts`, `snapshotClient.ts`, `macroCompiler.ts`, `apiContracts.ts`) and several `features/*/v2/` files |
| `webapp/tests/{render.tsx,fakeFetch.ts,fakeLocation.ts,setup.ts}` | Shared test infrastructure still used by the kept `v2-*` test files (`setup.ts` is the global Vitest `setupFiles` entry; `fakeLocation.ts` is also used directly by `tests/v2-routing.test.ts`) |
| `webapp/src/pages/` | Per task instructions, out of scope to delete (placeholder dir, not part of the v1 route tree) — but its `README.md` referenced the now-deleted `App.tsx` as fact, so that one file was corrected in place, not removed |

## Firmware, and other things explicitly left open

- **Firmware files/build registrations** — not audited in this pass; this
  task instance was scoped to the webapp (every file category named in the
  task prompt was a `webapp/src/` path). `scripts/check-v2-phase2-architecture.py`
  (CI-enforced, passed both times it was run below) is evidence firmware
  carries no package/macro repository model, but that is not the same as a
  full sweep for other obsolete v1 firmware remnants.
- **Compatibility types/migrations with no released v2 input** — grepped
  `webapp/src/v2/` and `features/*/v2/` for migration/compat-shim code
  reading v1-shaped data; found none (this product never shipped v1 to real
  users, so there is nothing to migrate from in the webapp). Firmware NVS
  settings-schema migration code was not audited.
- **Host/on-device test suites** — not audited for v1 debt in this pass.

## docs/API.md — rewritten, not just banner-patched

The file previously carried a retirement banner at the top but its ~200-line
body still documented the full v1 route surface as if it were the reference.
Rewrote it: a new "Current v2 API" section leads with the general rules,
error envelope (cross-checked against `webapp/src/v2/apiClient.ts`'s actual
behavior), and the complete 21-route table (from `contracts/v2/api/routes.json`,
the authoritative machine-readable source), each row pointing at its
`docs/SPEC_V2.md` §13.x subsection for the byte-exact contract. The old v1
content was moved — not deleted — into a clearly headed "Archived: retired
v1 API (historical reference only)" section at the bottom, with a corrected
banner (the original banner over-claimed retirement of a few routes —
session validation, settings read/update, password change, restart,
reset-settings, factory-reset — that actually survived into v2 unchanged;
the new banner says so explicitly and points to where they now live).

No content in the new "Current v2 API" section was invented: the route table
is copied verbatim (method/path) from `contracts/v2/api/routes.json`, the
error envelope shape and general rules are cross-checked against
`docs/SPEC_V2.md` §13.1–§13.2, and the device-actions confirmation
requirements were verified against `docs/SPEC_V2.md` §13.12 rather than
reused from the (different, `physical confirmation`-conflating) old v1 text.

## Other documentation fixed as direct fallout of the deletion

- `webapp/README.md` — removed the `App.tsx`/`api/routes.ts`/`types/models.ts`
  references (files no longer exist) and corrected a stale claim that
  Settings/Diagnostics were unimplemented v2 placeholders — they shipped in
  Phase 12 (`V2_120_122_SETTINGS_DIAGNOSTICS_DESTRUCTIVE_2026-08-09.md`),
  before this task ran, but the README was never updated.
- `CLAUDE.md` — rewrote the "Webapp (`webapp/src/`)" architecture section to
  match the post-deletion tree, including the same stale Settings/Diagnostics
  claim; updated the frontend test-file count (35 → 43); removed the "JSON
  schemas" mention from the `docs/` bullet (schemas now live only in
  `contracts/v2/`, not `docs/schemas/`, which is now empty).
- `webapp/src/pages/README.md` — no longer describes `App.tsx` as the router;
  points at `AppV2.tsx`/`routingV2.ts` instead. Left the directory itself
  alone per the task's explicit instruction.
- `webapp/tests/README.md` — was describing pre-v1-numbered phases ("Phase
  17.7", "Phase 18", "Phase 19") that no longer correspond to anything;
  rewritten to describe the actual current suite (v2 contract tests +
  Playwright browser suite).
- `webapp/src/AppV2.tsx` — its file-header comment describing "the retired v1
  `App`" as still present in the tree was corrected.
- Root `README.md` — corrected the already-stale frontend vitest count line
  (37 files/352 tests, which didn't match the pre-deletion baseline of 59/577
  either) to the accurate post-deletion 43/448.
- `scripts/bootstrap-repo.sh` — its tracked-directory list still named
  `webapp/src/api`, `webapp/src/features/execution`, and two directories that
  were already gone before this task (`webapp/src/features/procedures`,
  `webapp/src/features/sets`); removed all four so re-running the script
  after this deletion doesn't resurrect empty stale directories. Added
  `webapp/src/v2`, which the list never had.
- `scripts/check-docs.sh` — its `for schema in docs/schemas/*.json` loop
  broke (`jq: error: Could not open file docs/schemas/*.json`) once the
  directory's only three files were deleted, because bash doesn't expand a
  no-match glob to nothing by default. Added `shopt -s nullglob` /
  `shopt -u nullglob` around the loop.

## User-visible wording sweep

Grepped `webapp/src` for `\bv1\b`, `\blegacy\b`, `\brevision\b`,
`\bprocedure\b`, and stray "Set"/"set" wording (the v1 term where v2's
glossary term is "package", `docs/SPEC_V2.md` §6.3) after deletion. All
`v1`/`legacy` hits were in source comments referencing `/api/v1/*` route
paths (correct — that's the real URL prefix) or the one `AppV2.tsx` comment
fixed above. No `revision`, `procedure`, or stray `set`-for-package wording
found anywhere in the surviving tree — including in user-visible JSX text.

## Verification

Toolchain: Node v24.18.0 (`~/.nvm/versions/node/v24.18.0/bin` on `PATH`,
matching `.nvmrc` exactly), `npm ci` in `webapp/`.

- `npm --prefix webapp run typecheck` — clean, no dangling imports.
- `npm --prefix webapp run test` — **before: 59 files / 577 tests. After: 43
  files / 448 tests.** All 448 passing.
- `npm --prefix webapp run lint` — clean (`--max-warnings=0`).
- `npm --prefix webapp run stylelint` — clean (`--max-warnings=0`).
- `npm --prefix webapp run build` — clean production build
  (`dist/assets/index-B7T8JvQC.css`, `dist/assets/index-BySs7KsJ.js`).
- `./scripts/check-webapp.sh` (full chain: `npm ci` → format:check →
  typecheck → lint → stylelint → test → test:coverage → build → real-Chrome
  Playwright `test:browser` → `verify-no-remote-assets.sh`) — run 4 times
  total. 3 of 4 exited 0. The one failure was `tests/v2-snapshot-client.test.ts`
  timing out at exactly 5000ms only under `test:coverage`'s v8-instrumented
  run, in a test file this task never touched; an isolated immediate rerun of
  `npm run test:coverage` passed all 448/448 with no failure, confirming a
  load-induced timing flake rather than a regression. The two runs
  immediately after (and the two immediately before) all exited 0 cleanly,
  including the real-Chrome browser suite:
  - "Real Chrome v2 Macros page/Quick Send workflows passed."
  - "Real Chrome v2 Snapshots/import-export workflows passed."
  - "Real Chrome v2 Settings/Diagnostics workflows passed."
- `./scripts/check-docs.sh` — passes after the `nullglob` fix (one unrelated
  pre-existing failure remains: `docs/SPEC_V2_TEST_TRACEABILITY.md is out of
  date`, confirmed present on the unmodified baseline via `git stash` before
  this task made any change, so it is not caused by this work and was left
  alone rather than regenerated as out-of-scope).
- `./scripts/check-frontend-persisted-state.sh` — passes.
- `python3 scripts/check-v2-phase2-architecture.py` — passes.
- `bash scripts/bootstrap-repo.sh` — runs cleanly against the post-deletion
  tree, creates no new tracked artifacts.
- `shellcheck scripts/bootstrap-repo.sh scripts/check-docs.sh` and
  `shfmt -d scripts/bootstrap-repo.sh scripts/check-docs.sh` — both clean.

## Files changed

62 files changed: 18 production files/directories and 3 v1-era JSON schemas
deleted, 17 test files deleted, 10 documentation/script files edited, plus
this evidence file and the `docs/TODO_V2.md` checkbox updates.
