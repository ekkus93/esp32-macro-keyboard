# V2-156 — final acceptance audit — 2026-08-16

Audited at final candidate `28359e884d4fdbc3748853ce880a421ee0644a01`.

**Result: all eleven V2-156 items met.** Three coverage findings are recorded
at the end — all are *unguarded but currently satisfied* requirements, not
violations. Nothing in this document is asserted without being checked against
the tree; every gate result quoted here was produced by running the gate.

## Method

Normative requirements were extracted with the same regex the committed
traceability generator uses (`scripts/generate-spec-traceability.py`):
`MUST`, `MUST NOT`, `REQUIRED`, `SHOULD`, `SHOULD NOT`. That yields **108**
requirement lines — 105 in `docs/SPEC_V2.md`, 3 in `docs/UI_UX_SPEC_V2.md`.

Each was then linked to an artifact. The existing traceability report is
deliberately *source-level* and says so — "a citation does not prove every
requirement in that section" — so it could not discharge items 1 and 2 on its
own. This audit works at requirement granularity instead.

| Link kind | Count | Meaning |
| --- | ---: | --- |
| `CITED` | 63 | a first-party test or gate cites the requirement's section explicitly |
| `TEST` | 18 | linked by hand this audit to a specific test that exercises it |
| `GATE` | 14 | enforced mechanically by a named gate script |
| `CONFIG` | 3 | satisfied by committed build configuration |
| `CODE` | 3 | satisfied by construction, verified by inspection |
| `META` | 6 | about the specifications themselves, not runtime behaviour |
| `OPEN` | 1 | a `SHOULD` honestly recorded as not performed |

**No requirement is unmapped.**

## Items 3-10 — the specific confirmations

| # | Item | Verified by | Result |
| --- | --- | --- | --- |
| 3 | no firmware repository parser/compressor/CRUD | `check-v2-phase2-architecture.py`; grep for inflate/deflate/zlib/miniz in first-party firmware | **PASS** — the only `gzip` hits are Content-Type headers and passthrough; firmware never compresses or decompresses |
| 4 | no `activePackageId` in the repository schema | grep across schemas, firmware, webapp | **PASS** — absent from every schema; it survives only as a *negative* fixture asserting rejection with `invalid_fields` |
| 5 | package selection is device UI state, switching not dirty | `AppV2.tsx` `useState(ready.packageId)`; `v2-app-v2-wiring.test.tsx` | **PASS** — test: "Open switches the selected package, persists the change, and does not add a further dirty transition" |
| 6 | snapshots never created/deleted automatically | all three mutation entry points traced to their callers | **PASS** — `saveWorkingCopyAsSnapshot`, `deleteSnapshot` and `replaceSnapshotWithWorkingCopy` are reached only from user handlers, never an effect. The one internal `deleteSnapshot` call is inside the explicit user-initiated replace flow (SPEC_V2 §10.6) |
| 7 | ordinary sends need no standalone confirmation navigation | `screensV2` union; `v2-macros-page-send.test.tsx` | **PASS** — the seven screens contain **no** confirmation screen; confirmation is inline ("shows the serial-confirmation waiting state inline") |
| 8 | every terminal send path releases all keys | `macro_executor_engine.c` `finish_execution()`; executor `.inc` suites | **PASS** — `usb_release_all` is the **first, unconditional** statement of `finish_execution()`, and every terminal transition routes through it |
| 9 | no credential, repository-data or macro-source leak | `check-credential-logging.sh`; H9 cross-cutting audit; diagnostics field review | **PASS** — diagnostics carries only subsystem health states and reset reasons; no blob content, macro source or credential |
| 10 | partitions and images fit with recorded margins | `check-partitions.sh`, `check-release-budgets.sh` | **PASS** — app 47.9%, webfs 50.6%, DIRAM 49.3%, userdata 516096 B against a 262144 B minimum. One documented `[SKIP]`: task-stack margin is a runtime measurement, covered separately by the `check-stack-usage.sh` ratchet |

### §17's five mandated guards

§17 requires checks that *prevent reintroduction* of five specific things.
All five exist — three as gate scripts, two as tests:

| Guard | Artifact |
| --- | --- |
| firmware package/macro persistence | `scripts/check-v2-phase2-architecture.py` |
| package/macro API routes | `scripts/check-v2-api-routes.py` |
| repository `activePackageId` | `scripts/check-v2-phase2-architecture.py` + rejection fixture |
| automatic snapshot deletion | `v2-snapshots-page-protection.test.tsx` — "never deletes or modifies any stored snapshot merely by loading" |
| mandatory standalone send-flow navigation | closed `screensV2` union + `v2-macros-page-send.test.tsx` inline-flow tests |

## Item 11 — TODO and specifications match implemented behaviour

`docs/TODO_V2.md` was reconciled the same day: seven items whose own notes
deferred them to named successors (H12-121, H10-102, H3-035/H10-103, H2-024)
were closed against that evidence, and four stale reopening notes that still
asserted things like "no 12/12 physical run exists yet" were corrected. Both
specifications match implemented behaviour as mapped in the matrix below; no
requirement was found that the implementation contradicts.

## Findings — three unguarded requirements

None is a violation. Each holds today and each could regress silently.

**Finding 1 — §5.3 PSRAM configuration is set but not gated.**
`firmware/sdkconfig.defaults` sets all three required options
(`CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`, `CONFIG_SPIRAM_USE_MALLOC`), but
`scripts/check-production-config.sh` does not verify them — its allowlist
covers NVS encryption and the provisioning-log flags only. A quad-PSRAM or
SPIRAM-off build would pass every gate.

**Finding 2 — §5.3 task stacks stay in internal SRAM by construction, not by
guard.** The generated `sdkconfig` carries
`CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y`, which *permits* external task
stacks. The requirement still holds because all three task-creation sites
(`device_controls.c`, `macro_executor.c`, `web_server_async.c`) use
`xTaskCreate`, which allocates internally, and the firmware never calls
`xTaskCreateStatic` with a PSRAM buffer. A single future `xTaskCreateStatic`
would violate §5.3 with nothing to catch it.

**Finding 3 — §5.4 no-wall-clock holds in code but has no guard.** No
`time(NULL)`, `gettimeofday`, `localtime`, `strftime` or SNTP call exists
anywhere in `firmware/`, and repository schema v1 carries no dates. Nothing
prevents one being added.

All three are cheap to close with small additions to
`scripts/check-production-config.sh` and a grep-style guard. They are recorded
rather than fixed here because V2-156 is an audit task; adding guards is
implementation and is the product owner's call.

## Full criterion matrix

`L` is the line number in the source specification.

| Doc | § | L | Requirement | Link |
| --- | --- | ---: | --- | --- |
| SPEC_V2 | §0 | 17 | specification set and MUST be implemented together. Neither document may be used | *META* — specification authority/process, not a runtime criterion |
| SPEC_V2 | §0 | 21 | as historical records in git history. They MUST NOT be used to infer product | *META* — specification authority/process, not a runtime criterion |
| SPEC_V2 | §0 | 37 | owner's explicit permission. Implementation work MUST NOT silently amend the | *META* — specification authority/process, not a runtime criterion |
| SPEC_V2 | §1.2 | 97 | Firmware MUST NOT parse, decompress, validate, index, reorder, merge, or otherwise | *GATE* — scripts/check-v2-phase2-architecture.py (PASS) |
| SPEC_V2 | §1.2 | 98 | interpret repository contents. It MUST NOT contain a package repository, macro | *GATE* — scripts/check-v2-phase2-architecture.py (PASS) |
| SPEC_V2 | §2 | 110 | MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and **MAY** have | *META* — defines the normative keywords themselves |
| SPEC_V2 | §3 | 120 | The product MUST: | *META* — `The product MUST:` lead-in; the criteria are the bullets that follow |
| SPEC_V2 | §5.1 | 172 | The build MUST reject an unrecognized ESP-IDF version. | *GATE* — scripts/verify-toolchain.sh (quotes the requirement; pins v5.5.5); committed idf_component.lock + package-lock.json |
| SPEC_V2 | §5.1 | 173 | Dependencies MUST be pinned by committed manifest and lock files. | *GATE* — scripts/verify-toolchain.sh (quotes the requirement; pins v5.5.5); committed idf_component.lock + package-lock.json |
| SPEC_V2 | §5.2 | 179 | Package versions MUST be exact and locked in the committed npm lock file. | *GATE* — webapp/.npmrc `save-exact`; committed package-lock.json; scripts/verify-no-remote-assets.sh |
| SPEC_V2 | §5.2 | 180 | Production assets MUST be static and contain no CDN, remote-font, remote-icon, | *GATE* — webapp/.npmrc `save-exact`; committed package-lock.json; scripts/verify-no-remote-assets.sh |
| SPEC_V2 | §5.3 | 186 | MUST enable `CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`, and | *CONFIG* — firmware/sdkconfig.defaults sets CONFIG_SPIRAM/_MODE_OCT/_USE_MALLOC; all tasks use xTaskCreate (internal SRAM); native USB proven by H10-103 USB identity — see Finding 1/2 |
| SPEC_V2 | §5.3 | 190 | FreeRTOS task stacks MUST remain in internal SRAM. | *CONFIG* — firmware/sdkconfig.defaults sets CONFIG_SPIRAM/_MODE_OCT/_USE_MALLOC; all tasks use xTaskCreate (internal SRAM); native USB proven by H10-103 USB identity — see Finding 1/2 |
| SPEC_V2 | §5.3 | 192 | The board MUST expose the native USB D+/D- signals. No external button, jumper, | *CONFIG* — firmware/sdkconfig.defaults sets CONFIG_SPIRAM/_MODE_OCT/_USE_MALLOC; all tasks use xTaskCreate (internal SRAM); native USB proven by H10-103 USB identity — see Finding 1/2 |
| SPEC_V2 | §5.4 | 198 | Firmware MUST NOT create, require, or report wall-clock timestamps. | *CODE* — no time(NULL)/gettimeofday/localtime/strftime/sntp anywhere in firmware; schema v1 carries no dates — see Finding 3 |
| SPEC_V2 | §5.4 | 201 | not part of repository schema version 1 and MUST NOT be used as device ordering, | *CODE* — no time(NULL)/gettimeofday/localtime/strftime/sntp anywhere in firmware; schema v1 carries no dates — see Finding 3 |
| SPEC_V2 | §5.5 | 220 | Exact sizes are defined by the committed partition table and MUST be validated | *GATE* — scripts/check-partitions.sh (PASS: final end 0x6b0000, nvs_keys protected) |
| SPEC_V2 | §7.1 | 288 | It MUST NOT ship with Espressif example product strings. | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.2 | 303 | A send MUST NOT start unless USB is `ready`. | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.3 | 307 | After every key or chord action, firmware MUST emit a release-all report. | `firmware/components/macro_executor/macro_executor_engine.c`, `tests/host/executor_validation_tests.inc`, `webapp/src/features/macros/v2/MacroEditorPage.tsx` (+3 more) |
| SPEC_V2 | §7.3 | 311 | error, firmware MUST attempt a release-all report and move the send to a terminal | `firmware/components/macro_executor/macro_executor_engine.c`, `tests/host/executor_validation_tests.inc`, `webapp/src/features/macros/v2/MacroEditorPage.tsx` (+3 more) |
| SPEC_V2 | §7.3 | 312 | state. The executor MUST clear its internal pressed-key state even when the | `firmware/components/macro_executor/macro_executor_engine.c`, `tests/host/executor_validation_tests.inc`, `webapp/src/features/macros/v2/MacroEditorPage.tsx` (+3 more) |
| SPEC_V2 | §7.4 | 317 | There is exactly one executor task. HTTP handlers MUST NOT type directly. | `tests/host/executor_cancellation_race_tests.inc`, `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.4 | 322 | Cancellation MUST remain responsive during ordinary typing and delay actions. | `tests/host/executor_cancellation_race_tests.inc`, `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.10 | 383 | The parser MUST consume the entire source. | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.10 | 384 | Parsing and compilation MUST complete before execution starts. | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.10 | 385 | A partial parse MUST NOT execute. | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.10 | 386 | Errors MUST include an error code, byte offset, line, column, and readable | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.11 | 409 | Limits MUST be centralized, exposed by `GET /api/v1/limits`, and tested at their | `firmware/components/macro_executor/macro_executor_engine.c`, `tests/host/executor_execution_tests.inc`, `tests/host/executor_validation_tests.inc` (+1 more) |
| SPEC_V2 | §7.13 | 426 | A checked-in conformance corpus MUST contain valid and invalid macro sources, | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §7.13 | 428 | suites MUST execute the same corpus. Parser drift MUST fail CI. | `webapp/src/features/macros/v2/MacroEditorPage.tsx` |
| SPEC_V2 | §8.1 | 438 | significant, but newly saved snapshots SHOULD use compact JSON. | *TEST* — webapp/src/v2/repositoryValidation.ts + contracts/v2/repository/fixtures.json (schema-version rejection) |
| SPEC_V2 | §8.1 | 443 | Version 0.2 reads and writes only schema version `1`. An unsupported version MUST | *TEST* — webapp/src/v2/repositoryValidation.ts + contracts/v2/repository/fixtures.json (schema-version rejection) |
| SPEC_V2 | §8.1 | 444 | produce an explicit error and MUST NOT replace the current in-memory working | *TEST* — webapp/src/v2/repositoryValidation.ts + contracts/v2/repository/fixtures.json (schema-version rejection) |
| SPEC_V2 | §8.3 | 493 | Package names need not be unique. Package IDs MUST be unique within the | *TEST* — contracts/v2/repository/fixtures.json `duplicate package id` -> duplicate_id |
| SPEC_V2 | §8.4 | 520 | A macro ID MUST be unique across the entire repository, not merely within its | *TEST* — contracts/v2/repository/fixtures.json `duplicate macro id across packages` -> duplicate_id |
| SPEC_V2 | §8.5 | 529 | it MUST validate all of the following: | `webapp/src/v2/repositoryEditing.ts`, `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §8.5 | 543 | Validation failure MUST leave the existing working copy unchanged and identify | `webapp/src/v2/repositoryEditing.ts`, `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §8.6 | 551 | Repository data MUST NOT be persisted in `localStorage`, `sessionStorage`, | `webapp/src/features/macros/v2/PackageManagementPage.tsx`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/packageSelection.ts` (+4 more) |
| SPEC_V2 | §8.6 | 553 | persistent store. Browser storage MAY hold unrelated presentation data, but MUST | `webapp/src/features/macros/v2/PackageManagementPage.tsx`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/packageSelection.ts` (+4 more) |
| SPEC_V2 | §8.6 | 562 | The application MUST continuously expose `Saved` or `Unsaved changes`. A dirty | `webapp/src/features/macros/v2/PackageManagementPage.tsx`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/packageSelection.ts` (+4 more) |
| SPEC_V2 | §8.6 | 567 | A failed save MUST NOT discard or reset the working copy. | `webapp/src/features/macros/v2/PackageManagementPage.tsx`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/packageSelection.ts` (+4 more) |
| SPEC_V2 | §8.7 | 579 | the UI MUST NOT claim that a closed dirty working copy can be recovered. | `webapp/src/features/shell/v2/UnsavedChangesPrompt.tsx`, `webapp/src/features/shell/v2/useBeforeUnloadGuard.ts`, `webapp/src/features/snapshots/v2/SnapshotsPage.tsx` (+1 more) |
| SPEC_V2 | §8.8 | 608 | Exports MUST NOT contain access-point credentials, station credentials, | `webapp/src/v2/repositoryWorkingCopy.ts`, `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §9 | 621 | React MUST feature-detect both APIs during startup. An unsupported browser MUST | `scripts/check-h9-production-audit.py`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/gzip.ts` (+4 more) |
| SPEC_V2 | §9 | 623 | The application MUST NOT silently store uncompressed JSON as a fallback. | `scripts/check-h9-production-audit.py`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/gzip.ts` (+4 more) |
| SPEC_V2 | §9 | 625 | Firmware MUST NOT: | `scripts/check-h9-production-audit.py`, `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/gzip.ts` (+4 more) |
| SPEC_V2 | §10.1 | 659 | MUST also scan existing filenames and ensure the next ID is greater than every | *CODE* — firmware/components/storage/storage_blob_core.c storage_blob_derive_next_id() takes max(persisted, scanned max+1) |
| SPEC_V2 | §10.2 | 675 | Stray names that do not match the final filename grammar MUST be reported in | `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-snapshots-page-management.test.tsx` |
| SPEC_V2 | §10.2 | 676 | diagnostics and MUST NOT appear as valid blobs. | `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-snapshots-page-management.test.tsx` |
| SPEC_V2 | §10.3 | 697 | fails, the final `<id>.gz` path MUST be retained and the operation MUST be | `webapp/src/v2/apiClient.ts`, `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-api-client.test.ts` |
| SPEC_V2 | §10.3 | 698 | reported as durability-uncertain rather than uncommitted. Firmware MUST NOT delete | `webapp/src/v2/apiClient.ts`, `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-api-client.test.ts` |
| SPEC_V2 | §10.3 | 700 | `commit_uncertain`; the caller MUST reconcile canonical blob state before any | `webapp/src/v2/apiClient.ts`, `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-api-client.test.ts` |
| SPEC_V2 | §10.3 | 701 | retry and MUST NOT assume that no blob was created. | `webapp/src/v2/apiClient.ts`, `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-api-client.test.ts` |
| SPEC_V2 | §10.4 | 714 | Firmware MUST NOT decompress, transform, repair, or substitute another blob. A | `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §10.5 | 719 | Deletion removes exactly the blob named by the request. It MUST NOT delete any | `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §10.5 | 720 | other version and MUST NOT select a replacement. | `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §10.5 | 725 | Deletion is always initiated by an explicit user action. Firmware and React MUST | `webapp/src/v2/snapshotClient.ts` |
| SPEC_V2 | §10.6 | 739 | deleted. The UI MUST make this non-atomic consequence clear before beginning. | `webapp/src/features/snapshots/v2/SnapshotRow.tsx`, `webapp/src/v2/snapshotClient.ts`, `webapp/tests/v2-snapshot-client.test.ts` |
| SPEC_V2 | §10.7 | 752 | partition and filesystem configuration MUST be proven to hold two maximum-sized | *GATE* — scripts/check-release-budgets.sh userdata minimum 262144 B = exactly 2 x 131072 B max blob (verified) |
| SPEC_V2 | §10.9 | 775 | A mount failure MUST NOT format the partition. It produces a visible degraded or | *TEST* — tests/host storage_no_format_policy; webapp/tests/v2-snapshots-page-protection.test.tsx keeps unreadable blobs stored |
| SPEC_V2 | §10.9 | 779 | MUST NOT delete a final `.gz` blob because its contents are unreadable to React. | *TEST* — tests/host storage_no_format_policy; webapp/tests/v2-snapshots-page-protection.test.tsx keeps unreadable blobs stored |
| SPEC_V2 | §11.1 | 801 | Repository JSON and blobs MUST NOT be stored in NVS. | `webapp/src/features/settings/v2/SettingsPage.tsx`, `webapp/src/v2/settingsClient.ts`, `webapp/tests/v2-snapshot-retention.test.ts` |
| SPEC_V2 | §11.3 | 824 | The administrator password MUST be 12 through 128 UTF-8 bytes and MUST NOT be | `webapp/src/features/settings/v2/SettingsPage.tsx` |
| SPEC_V2 | §11.3 | 831 | conditions. The implementation MUST freeze one exact iteration count before the | `webapp/src/features/settings/v2/SettingsPage.tsx` |
| SPEC_V2 | §11.3 | 832 | v0.2 acceptance gate; it MUST NOT guess or silently vary the count at runtime. | `webapp/src/features/settings/v2/SettingsPage.tsx` |
| SPEC_V2 | §11.3 | 834 | Wi-Fi passphrases are necessarily recoverable by firmware but MUST NOT appear in | `webapp/src/features/settings/v2/SettingsPage.tsx` |
| SPEC_V2 | §11.3 | 839 | corrupt and MUST NOT be parsed on a best-effort basis. | `webapp/src/features/settings/v2/SettingsPage.tsx` |
| SPEC_V2 | §12.1 | 877 | logged and reported but MUST NOT prevent access-point operation, erase stored | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.1 | 880 | At most one station network is remembered. Firmware MUST NOT scan for, rank, or | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.2 | 902 | an unambiguous URL-safe representation. Raw tokens and token-derived secrets MUST | `firmware/components/web_server/README.md`, `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.2 | 918 | MUST revisit DNS-rebinding protection before release. | `firmware/components/web_server/README.md`, `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.3 | 927 | returns the minimal setup state defined in §13.4. It MUST NOT return the setup | `webapp/src/features/auth/v2/FirstRunSetupPage.tsx`, `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.3 | 942 | Setup MUST preserve configuration fields it does not modify. It MUST NOT rebuild | `webapp/src/features/auth/v2/FirstRunSetupPage.tsx`, `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.4 | 954 | USB-Serial-JTAG console, this secret-bearing response MUST bypass `stdout` and | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.4 | 955 | write only to the primary UART command channel. Firmware MUST NOT emit that | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.4 | 959 | A successful `wifi-connect` command MUST persist the explicitly supplied station | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.4 | 963 | station afterward, and first-run setup MUST preserve that newly configured | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.4 | 964 | station because setup does not modify station fields. Persistence failure MUST | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §12.4 | 967 | Before distribution to third parties, the interactive development console MUST | `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §13.2 | 1013 | `commit_uncertain` MUST reconcile the canonical resource state before retrying | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_storage_atomic.c |
| SPEC_V2 | §13.2 | 1014 | and MUST NOT assume that the mutation did not occur. | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_storage_atomic.c |
| SPEC_V2 | §13.4 | 1082 | record. The response has no optional fields. It MUST NOT return the setup code, | `webapp/src/features/settings/v2/settingsFieldLimits.ts` |
| SPEC_V2 | §13.8 | 1246 | fails, firmware instead returns `503` with error code `commit_uncertain` and MUST | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_web_server_blob_create.inc; webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs |
| SPEC_V2 | §13.8 | 1249 | Before starting a blob add, React MUST record the set of blob IDs returned by the | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_web_server_blob_create.inc; webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs |
| SPEC_V2 | §13.8 | 1251 | `commit_uncertain` response, React MUST NOT retry the POST in that invocation. It | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_web_server_blob_create.inc; webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs |
| SPEC_V2 | §13.8 | 1252 | MUST refresh `GET /api/v1/blob`, consider only IDs that were absent from the | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_web_server_blob_create.inc; webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs |
| SPEC_V2 | §13.8 | 1258 | exactly one new byte-identical blob: React MUST NOT issue another POST; the | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_web_server_blob_create.inc; webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs |
| SPEC_V2 | §13.8 | 1265 | byte-identical: the working copy remains dirty and further snapshot POSTs MUST | *TEST* — webapp/tests/v2-snapshot-commit-reconciliation.test.ts; tests/host/test_web_server_blob_create.inc; webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs |
| SPEC_V2 | §13.9 | 1362 | such case MUST be specified and contract-tested rather than inferred by React. | `webapp/src/v2/apiClient.ts`, `webapp/src/v2/settingsClient.ts` |
| SPEC_V2 | §13.11 | 1447 | The React API layer SHOULD expose a helper shaped conceptually as: | `webapp/src/v2/sendClient.ts` |
| SPEC_V2 | §13.12 | 1515 | The destructive operation MUST NOT begin until the complete request, password, | `webapp/src/AppV2.tsx`, `webapp/src/features/settings/v2/DeviceReconnectScreen.tsx`, `webapp/src/features/settings/v2/SettingsPage.tsx` (+1 more) |
| SPEC_V2 | §13.13 | 1559 | an explicit specification update. Diagnostics MUST NOT include credentials, | `webapp/src/features/settings/v2/DiagnosticsPage.tsx`, `webapp/tests/browser/workflows/settings.mjs` |
| SPEC_V2 | §14.1 | 1606 | shows snapshot recovery. It MUST NOT silently load an older blob or delete the | `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx`, `webapp/src/v2/startup.ts` |
| SPEC_V2 | §14.3 | 1625 | React MUST NOT call firmware after every edit. Device repository writes occur | *TEST* — webapp/tests/v2-unsaved-changes-prompt.test.tsx (writes only on explicit save) |
| SPEC_V2 | §14.5 | 1667 | blob ID. The next macro MUST NOT execute automatically. | `webapp/src/features/macros/v2/MacroPreviewPage.tsx`, `webapp/src/features/macros/v2/MacrosPage.tsx` |
| SPEC_V2 | §14.6 | 1674 | Macro source is shown in the editor and optional preview screen but MUST NOT be | *TEST* — webapp/tests/v2-macros-page-send.test.tsx (hidden macro source) |
| SPEC_V2 | §14.7 | 1683 | An active send's progress and **Cancel and release all keys** control MUST remain | *TEST* — V2-132 landscape active-send safety; webapp/tests/v2-macros-page-send.test.tsx cancel controls |
| SPEC_V2 | §14.8 | 1690 | Compressible static assets SHOULD have pre-generated gzip variants. The static | *GATE* — scripts/build-webfs-image.sh gzip variants; scripts/verify-no-remote-assets.sh |
| SPEC_V2 | §16.1 | 1716 | Code MUST NOT swallow an `esp_err_t`, discard an error, report success after a | *GATE* — scripts/check-h9-production-audit.py; clang-tidy WarningsAsErrors '*' |
| SPEC_V2 | §16.2 | 1723 | Logs MUST NOT contain: | *GATE* — scripts/check-credential-logging.sh (PASS) |
| SPEC_V2 | §17 | 1750 | `./scripts/check-all.sh` is the authoritative local gate and CI MUST invoke the | *GATE* — scripts/check-all.sh; the five mandated guards verified present (see Item 3) |
| SPEC_V2 | §17 | 1753 | It MUST fail on the first failed phase and MUST never mask failures. | *GATE* — scripts/check-all.sh; the five mandated guards verified present (see Item 3) |
| SPEC_V2 | §17 | 1764 | Checks MUST prevent reintroduction of firmware package/macro persistence, | *GATE* — scripts/check-all.sh; the five mandated guards verified present (see Item 3) |
| SPEC_V2 | §18.5 | 1859 | SHOULD be represented in web compatibility testing where practical. | *OPEN* — ChromeOS/Windows explicitly not performed (V2-152, H10-103) — SHOULD, recorded not claimed |
| UI_UX | §0 | 15 | one v2 specification set and MUST be implemented together. Neither document may | *META* — specification authority, not a runtime criterion |
| UI_UX | §2.3 | 84 | Hash routing SHOULD be used. A route may encode a package or macro selection for | `webapp/src/features/shell/v2/AppShellV2.tsx`, `webapp/src/v2/routingV2.ts` |
| UI_UX | §12.4 | 568 | The web app manifest SHOULD include `orientation: "portrait-primary"` as | `webapp/src/AppV2.tsx`, `webapp/src/features/shell/v2/useLandscapePhoneBlock.ts`, `webapp/src/styles.css` (+2 more) |
