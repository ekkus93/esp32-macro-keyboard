# H5-052 — Atomic-write error provenance — 2026-08-13

## Scope

This document records the H5-052 implementation from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.

Starting SHA: `4b52066a15a9a3840e9ca8f683ae96baaf38c26d`.
H5-052 regression candidate before this evidence commit:
`dec8cdfbea4a1a57a4d53627b3f6d61a81489f50`.

H5-051 introduced the shared `app_operation_result_t` commit-state vocabulary and
wired the atomic-write state machine to it. H5-052 closes the atomic-write
provenance proof at each required failure boundary. No public storage API
signature changes are introduced here.

## Required invariants

The atomic-write detailed result now has deterministic regression coverage for
all four H5-052 requirements:

1. write failure remains primary when temporary cleanup also fails;
2. verify/read-back failure remains primary when temporary cleanup also fails;
3. rename failure remains primary when temporary cleanup also fails; and
4. a parent-directory sync failure after successful rename is represented as
   commit/durability uncertain rather than as an ordinary uncommitted failure.

The first three cases must report `APP_OPERATION_NOT_COMMITTED`. The fourth must
report `APP_OPERATION_COMMIT_UNCERTAIN` because the canonical rename already
occurred and the destination contains the new bytes.

## Production state-machine audit

`firmware/components/storage/storage_atomic.c` already implements the required
state transitions from H5-051:

- staging/write/verify failure returns the initiating error and separately records
  failed `.tmp` cleanup;
- the caller marks any failed staging result `APP_OPERATION_NOT_COMMITTED`;
- rename failure records the rename failure as primary, records failed `.tmp`
  cleanup independently, and reports `APP_OPERATION_NOT_COMMITTED`;
- rename success followed by parent sync failure returns the sync error with
  `APP_OPERATION_COMMIT_UNCERTAIN` and does not rename the canonical object away;
- full rename + parent sync success reports `APP_OPERATION_COMMITTED`.

The legacy `app_error_code_t` wrapper continues to map through
`app_operation_result_error()`, which keeps `primary_error` authoritative when
both primary and cleanup errors exist.

No production algorithm change was required by H5-052 after that audit; the
missing work was deterministic proof of the complete failure matrix.

## Regression coverage

`tests/host/test_storage_atomic.c` now proves the required matrix.

### Write + unlink failure

The fake filesystem injects:

- `FAKE_FS_WRITE` failure with `ENOSPC`; and
- `FAKE_FS_UNLINK` failure with `EIO`.

Assertions require:

- `primary_error == APP_ERROR_STORAGE_FULL`;
- `cleanup_error == APP_ERROR_IO`;
- `cleanup_incomplete == true`;
- `commit_state == APP_OPERATION_NOT_COMMITTED`;
- no canonical destination was created; and
- the failed-cleanup temporary remains visible.

The legacy wrapper is separately exercised and must return the initiating
`APP_ERROR_STORAGE_FULL`, not the cleanup error.

### Verify + unlink failure

A new deterministic regression injects:

- `FAKE_FS_READ` failure with `ENOSPC` during the read-back verification stage;
  and
- `FAKE_FS_UNLINK` failure with `EIO` while cleaning the staged temporary.

The same primary/cleanup/`NOT_COMMITTED` assertions are required, and the legacy
wrapper again must return the initiating verification failure.

This closes the H5-052 verify-provenance gap that was not covered by the prior
write and rename regressions.

### Rename + unlink failure

The existing combined-failure regression is strengthened with an explicit
`APP_OPERATION_NOT_COMMITTED` assertion. It continues to prove that the old
canonical destination remains byte-for-byte intact while the failed-cleanup
`.tmp` remains visible.

### Rename success + parent sync failure

A focused parent-sync adapter now routes the detailed atomic-write call through
the fake filesystem's `FAKE_FS_SYNC_PARENT` failure seam.

The regression requires:

- `primary_error == APP_ERROR_IO`;
- no cleanup error;
- `cleanup_incomplete == false`;
- `commit_state == APP_OPERATION_COMMIT_UNCERTAIN`;
- exactly one successful rename;
- exactly one failed parent sync;
- the canonical path contains the new bytes; and
- no temporary file remains.

This distinguishes activation from final durability acknowledgement and prevents
callers from treating the result as ordinary `NOT_COMMITTED`.

## Validation status

The repository checkout is not available inside this sandbox and direct GitHub
DNS resolution is unavailable, so the literal host/sanitizer/Quality commands
cannot be executed locally here. Connector-backed source inspection was used to
verify the production state-machine paths and the fake filesystem failure seams.

Authoritative validation for the exact evidence descendant remains:

```text
./scripts/run-tests.sh storage
./scripts/run-tests.sh --sanitizers storage
./scripts/check-all.sh
```

The H5-052 checkboxes should be marked complete only after those gates pass on
an exact descendant containing `dec8cdfbea4a1a57a4d53627b3f6d61a81489f50`.

## Remaining H5 work

After H5-052 validation, the next substantive task is H5-053: define and
implement end-to-end reconciliation/retry semantics for `COMMIT_UNCERTAIN`, in
particular preventing a blob-create retry from silently creating a duplicate
snapshot when activation may already have occurred.

H5-054 then completes the broader storage failure-injection matrix, and H5-055
remains the physical interrupted-write/power-cycle durability sanity pass.
