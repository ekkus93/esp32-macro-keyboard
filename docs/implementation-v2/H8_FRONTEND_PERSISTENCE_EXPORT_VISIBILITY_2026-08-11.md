# H8 — Frontend persistence and export failure visibility evidence

**Date:** 2026-08-11  
**Phase:** `H8 — Frontend persistence and export failure visibility`  
**Existing H8 implementation:** `1445ae6f35502ece15824a04805d050e7d7baa4f` (`fix(webapp): surface persistence and export failures`)  
**H8 reconciliation/correction SHA:** `4c2eab2d2c06609c4862fb4da82c8359de7f9045` (`fix(webapp): preserve durable package selection on snapshot replace`)  
**Targeted validation:** workflow run `31542963700`, job `93949257708`

## Result

The earlier H8 implementation already provided the intended visible package-selection persistence warning, local continuation, retry path, export error banner, and export `try/catch/finally` cleanup. A literal H8 call-site audit found one remaining correctness defect in snapshot load/import selection resolution, fixed at the reconciliation SHA above.

`lastSelectedPackageId` is the durable device-wide selection preference. The running shell separately has a local selected package, which may intentionally advance even when persisting that preference fails. Before this H8 reconciliation, `SnapshotsPage.afterWorkingCopyReplaced()` resolved a newly loaded/imported repository from the transient local package rather than from the last known successfully persisted package ID. If a previous selection write had failed and the transient package happened to exist in the replacement repository, the code could report selection-persistence success and clear the warning without performing a successful settings write. That falsely implied persistence had recovered.

`4c2eab2d2c06609c4862fb4da82c8359de7f9045` removes that second, transient input from `SnapshotsPage` and resolves snapshot load/import only from `persistedPackageId`. Two regressions cover a multi-package snapshot load and a multi-package repository import where the durable preference differs from the prior local package. Both require the durable package to be selected, require no redundant settings PUT, and require the success callback to refer to the durable package. The snapshot-load regression also proves the clean working copy remains non-dirty.

## H8-080 — Package-selection persistence warning

All production call sites that can change or resolve selection were audited.

### Startup package chooser

`RepositoryStartupScreen` uses `tryPersistSelectedPackageId()` for chooser selection. The helper returns an explicit `persisted` or `failed` result instead of throwing away a failed settings write. On failure, startup still hands the locally selected package and working-copy store to the shell while carrying `selectionPersistenceFailure` forward for display.

### First-package flow

The first-package path uses the same explicit persistence attempt. Package creation itself is a repository content edit and therefore legitimately creates a dirty working copy, but failure to save the package-selection preference does not add or hide any repository mutation. Local entry into the new package remains possible and the persistence failure is handed to the shell.

### Package management Open

`PackageManagementPage` attempts selection persistence, reports success/failure through the shell callbacks, then updates the local selected package and opens Macros. The existing regression proves a failed preference write still opens locally and leaves an otherwise-clean repository non-dirty.

### Selected-package deletion resolution

After deleting the selected package, `PackageManagementPage` resolves the replacement selection from `persistedPackageId` and reports any preference-write failure separately. The repository is dirty because the user explicitly deleted a package; selection itself does not create a second repository edit or hide the persistence result.

### Snapshot load/import resolution

`SnapshotsPage.afterWorkingCopyReplaced()` now resolves from `persistedPackageId`, not the potentially divergent local package. If the durable preference is absent from the replacement repository and exactly one package exists, it attempts to persist that sole-package resolution. Failure is surfaced through the shell warning while the local package can still open. If several packages remain unresolved, the package chooser opens instead of inventing a selection.

### Visible warning and Retry

`packageSelectionPersistenceWarning` says:

> The package opened locally, but the device could not save it as the selected package. This selection may not survive a reload.

`AuthenticatedShell` renders that warning through the common `ErrorBanner`, retains the underlying sanitized error detail, and provides **Retry saving selection**. Retry uses the last known successfully persisted package ID as the compare point. Only a successful persistence attempt advances `persistedPackageId` and clears the warning.

The success-path AppV2 regression now explicitly asserts that the persistence warning is absent. The existing failure/retry AppV2 regression proves local package access remains available, the warning is visible with the settings-write error, Retry is offered, and a successful Retry clears the warning without losing the local package.

## H8-081 — Snapshot export error handling

`SnapshotsPage.exportWorkingCopy()` is an internally-contained async operation:

1. it sets the export busy flag and clears the prior export error;
2. it awaits repository serialization/compression;
3. it invokes the shared file-save helper;
4. it catches both asynchronous export/compression failures and synchronous file-save failures and converts them to the visible `ErrorBanner` message;
5. it clears the busy flag in `finally` on every path.

The click handler intentionally discards the returned promise only because `exportWorkingCopy()` catches its complete export/file-save failure surface internally; rejected export promises are not left unhandled.

The export regressions prove failure does not mutate repository content, dirty state, package selection, or loaded-snapshot association:

- compression/export failure begins with a dirty renamed working copy and proves the identical repository object remains selected and dirty after the visible error;
- file-save failure begins clean and proves the repository stays identical and clean;
- both tests prove selection and working-copy-origin callbacks are not invoked;
- both prove the Export button is enabled again after failure, demonstrating busy cleanup.

## H8-082 — Required frontend regression matrix

The permanent frontend suite now covers every required row:

- **package selection persists successfully -> no warning:** AppV2 successful startup explicitly asserts the H8 warning is absent; existing package-management success tests also exercise the persisted success callback.
- **persistence fails -> local open + warning + non-dirty state:** package-management failure proves local open/non-dirty behavior; AppV2 proves the common warning remains visible after local continuation.
- **retry succeeds -> warning clears:** AppV2 failure/retry regression.
- **export compression fails -> visible error:** `v2-snapshots-page.test.tsx` export/compression regression.
- **save-as-file fails -> visible error:** `v2-snapshots-page.test.tsx` file-save regression.
- **state remains unchanged after export failure:** the same two export regressions assert repository identity, dirty-state preservation, and no selection/origin callback.
- **replacement resolution cannot falsely clear an earlier warning:** new H8 snapshot-load/import durable-preference regressions at `4c2eab2d2c06609c4862fb4da82c8359de7f9045`.

## Validation

Targeted run `31542963700`, job `93949257708`, used the repository-pinned Node **24.18.0** and npm **11.16.0**. It passed:

- `npm --prefix webapp run format:check`
- `npm --prefix webapp run typecheck`
- `npm --prefix webapp run lint`
- `npm --prefix webapp run stylelint`
- `npm --prefix webapp run test` — **46/46 test files, 517/517 tests passed**
- `npm --prefix webapp run test:coverage` — **46/46 test files, 517/517 tests passed**; aggregate **87.43% statements / 83.35% branches / 91.43% functions / 87.51% lines**
- `npm --prefix webapp run build`
- `git diff --check`

The H8-specific snapshot page file contains **41 passing tests** in this run; AppV2 contains **14**, RepositoryStartupScreen **17**, and PackageManagementPage **17**.

The first temporary validator attempt (`31542807853`) failed before product modification because its self-applying patch used an overly strict whitespace anchor for `AppV2.tsx`. It did not reach the test gate and did not push any product change. The repaired targeted validator above is the behavior evidence used for H8 completion.

## Phase H8 disposition

All H8-080, H8-081, H8-082, and Phase H8 exit requirements are satisfied by current source plus permanent regressions. No reviewed package-selection persistence or snapshot-export failure in this phase is intentionally invisible, and noncritical local continuation never clears the persistence warning until a successful persistence result is established.

This phase does **not** claim the later H10 full real-Chrome/release gate; H10 remains responsible for running the complete browser and final regression matrix on the eventual candidate SHA.
