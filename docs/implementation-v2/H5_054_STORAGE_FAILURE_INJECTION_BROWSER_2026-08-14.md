# H5-054 — Storage failure-injection and browser reconciliation — 2026-08-14

## Scope

This document records the H5-054 implementation from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.

H5-054 is a proof/coverage task. H5-051 through H5-053 established the structured
operation-result contract, atomic-write provenance, and commit-uncertain retry
semantics. H5-054 aggregates deterministic failure injection around those
boundaries and adds a real-Chrome regression that proves the UI cannot turn a
lost final durability acknowledgement into a duplicate blob POST.

The H5-054 source-level candidate is the descendant ending at the commit that
adds this evidence. The implementation immediately before this evidence commit
includes:

- `5d917cf44788fabf36283676c61f41d062851d40` — formatter-only correction for
  the H5 storage commit-result declaration/assertion;
- `bab44e9fb8e85fcb7397f0e461e727211d77ce19` — dedicated H5 real-browser
  reconciliation runner;
- `c196a56a825b5777b9e789ff79969276fb9bdbab` — integration of the H5 runner
  into `npm run test:browser`;
- `7a1b01bab9975783fd8b03685178b209d2ce41f9` — browser-runner formatting
  normalization.

No new storage semantics are introduced by H5-054.

## Authoritative H5-054 matrix

### Primary write failure + unlink failure

`tests/host/test_storage_atomic.c::test_stage_failure_retains_primary_and_cleanup_errors()`
forces the primary write to fail with `ENOSPC` and temporary unlink cleanup to
fail with `EIO`.

It proves:

- `primary_error == APP_ERROR_STORAGE_FULL`;
- `cleanup_error == APP_ERROR_IO`;
- `cleanup_incomplete == true`;
- `commit_state == APP_OPERATION_NOT_COMMITTED`;
- the compatibility wrapper still returns the primary error;
- the failed cleanup leaves only the temporary file, never a canonical target.

### Verify failure + unlink failure

`tests/host/test_storage_atomic.c::test_verify_failure_retains_primary_and_cleanup_errors()`
forces read-back verification to fail with `ENOSPC` and temporary unlink cleanup
to fail with `EIO`.

It proves both errors remain separately observable, verification remains primary,
and the result is `APP_OPERATION_NOT_COMMITTED`.

### Rename failure + unlink failure

`tests/host/test_storage_atomic.c::test_rename_failure_retains_primary_and_cleanup_errors()`
forces the activation rename to fail with `ENOSPC` and temporary cleanup to fail
with `EIO`.

It proves:

- rename failure remains primary;
- unlink failure is retained as cleanup detail;
- the old destination remains byte-for-byte intact;
- the result is `APP_OPERATION_NOT_COMMITTED`.

### Rename success + parent sync failure

`tests/host/test_storage_atomic.c::test_parent_sync_failure_is_commit_uncertain()`
forces the final parent-directory durability acknowledgement to fail after a
successful rename.

It proves:

- canonical bytes are already the new bytes;
- no temporary file remains;
- `commit_state == APP_OPERATION_COMMIT_UNCERTAIN`;
- the failure is not represented as ordinary not-committed state.

The blob-specific equivalent is covered by
`tests/host/test_storage_blob_upload.c::test_directory_sync_failure_is_uncertain_and_retained()`
and `test_public_wrapper_records_only_durable_commit()`:

- final-path ownership remains true after activation;
- abort cannot unlink the activated final blob;
- the public wrapper returns `APP_ERROR_COMMIT_UNCERTAIN`;
- the in-memory durable-commit shortcut is not falsely advanced.

### Retry/reconcile does not silently duplicate data

The unit-level state machine remains covered by:

- `webapp/tests/v2-snapshot-client.test.ts`;
- `webapp/tests/v2-snapshot-commit-reconciliation.test.ts`.

H5-054 adds a real-Chrome proof in:

`webapp/tests/browser/run-h5-storage-reconciliation-tests.mjs`

The dedicated fixture performs a real built-app workflow:

1. Chrome loads a real gzip repository through the normal v2 startup flow.
2. A keyboard reorder dirties the in-memory working copy.
3. The first `Save snapshot` POST is accepted into the fixture's canonical blob
   store, but the server returns HTTP 503 with machine code `commit_uncertain`.
4. The client refreshes the canonical blob list and downloads the newly observed
   blob as raw gzip bytes.
5. The runner verifies the retained blob decompresses to the exact reordered
   working copy.
6. The UI reports that no duplicate upload was sent and remains dirty.
7. The user presses `Save snapshot` again.
8. The runner waits for a new canonical list GET and candidate-byte GET.
9. It proves the total `POST /api/v1/blob` count is still exactly one and the
   canonical blob count is unchanged.

This is a real browser interaction rather than a direct snapshot-client call, so
it covers the shell Save button, React saving/error state, browser gzip support,
network request sequencing, and the per-working-copy uncertainty latch together.

The runner is included in the existing browser gate through:

```text
npm run test:browser
```

after the established general browser suite and H4 recovery runner.

### Mount rollback preserves initiating error and cleanup detail

`tests/host/test_storage_mount.c::test_mount_data_failure_preserves_rollback_error()`
proves a data-mount failure remains primary when web-mount rollback also fails.
The rollback error is retained as cleanup detail and ownership remains accurate.

`tests/host/test_storage_mount.c::test_prepare_failure_preserves_first_rollback_error()`
proves a directory-preparation failure remains primary when both data and web
rollback unmounts fail. The first rollback failure is retained separately and the
state continues to report resources whose unmount did not succeed.

## CI formatter correction observed during H5-054

The Quality output supplied during this task exposed three formatter-only
violations in the H5-053 storage changes. They were corrected without semantic
changes:

- `storage_blob_upload_core.c` now uses the repository's canonical LLVM-style
  return-type break for the long detailed-result function;
- `storage_blob_upload_internal.h` uses the matching declaration layout;
- the 98-column `TEST_CHECK_APP_ERROR(APP_ERROR_COMMIT_UNCERTAIN, ...)`
  assertion is a single line as required by the 100-column clang-format policy.

The formatter correction ends at
`5d917cf44788fabf36283676c61f41d062851d40` before the H5-054 browser commits.

## Validation status

The repository checkout is not available in this sandbox and direct GitHub DNS
resolution is unavailable, so the literal repository gates were not executed
locally. Source, target wiring, and the exact failure matrix were inspected using
the GitHub connector. No CI success is inferred from that inspection.

Authoritative validation for the exact H5-054 descendant is:

```text
./scripts/run-tests.sh storage
./scripts/run-tests.sh --sanitizers storage
npm --prefix webapp run test -- --run \
  tests/v2-snapshot-client.test.ts \
  tests/v2-snapshot-commit-reconciliation.test.ts
npm --prefix webapp run test:browser
npm --prefix webapp run typecheck
npm --prefix webapp run lint
npm --prefix webapp run format:check
./scripts/check-all.sh
```

The browser gate must run with the repository-pinned Node.js 24.18.0 and its
pinned Playwright/Chromium dependencies.

Do not mark the authoritative H5-054 checkboxes complete until these gates pass
on an exact descendant containing the H5-054 implementation.

## Remaining H5 work

After the validation above is green, H5-054 can be formally closed. H5-055 then
remains the physical durability sanity pass:

- interrupted upload/power-cycle after the changed storage semantics;
- no formatting-on-mount-failure regression;
- byte identity and blob-list behavior on the real device.
