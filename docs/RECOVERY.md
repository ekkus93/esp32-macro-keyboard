# Recovery policy

Recovery must preserve evidence and must never guess.

**Validation status:** every rule below is implemented and host-tested
(`tests/host/test_storage_mount.c`, `tests/host/test_storage_atomic.c`, and
`tests/host/test_storage_blob.c`, including
`test_boot_recovery_removes_only_canonical_regular_temporaries` and
`test_boot_recovery_reports_unlink_failure`). None has been validated against
a real power interruption on physical hardware - see
`docs/HARDWARE_TEST_PLAN.md`'s "Persistence and fault tests" (currently "Not
run") and `docs/TODO_V2.md` Phase 15 (V2-153).

Firmware stores each repository snapshot as one opaque blob under
`/data/repository/<fixed-width-id>.gz` (`docs/SPEC_V2.md` §10.1). It does not
parse, index, or otherwise understand blob contents; there is no repository
index file, backup directory, staging directory, transaction directory, trash
directory, or quarantine directory (§10.1, §10.9).

- LittleFS mount failures are visible. Firmware does not auto-format.
- Adding a blob is atomic: the request body streams to `<id>.gz.tmp`, every
  write/flush/close/synchronization failure is rejected, the temporary file is
  synchronized, then renamed to `<id>.gz` (the commit point), then the
  directory is synchronized when supported (§10.3).
- An interrupted add leaves all existing final `.gz` blobs intact. Boot
  recovery removes only `*.tmp` files left by an interrupted add; it MUST NOT
  delete a final `.gz` blob, because React - not firmware - can read its
  contents (§10.9).
- Deletion removes exactly the blob named by the request. It MUST NOT delete
  any other blob and MUST NOT select a replacement. Deletion is always an
  explicit user action; firmware and React MUST NOT automatically delete
  snapshots (§10.5).
- There is no atomic blob-replace operation. A user-requested replacement is
  two independent React-driven operations - delete, then add - and is not
  atomic across the pair (§10.6).
- Stray names that do not match the final-blob or temporary-file grammar are
  reported in diagnostics and MUST NOT be treated as valid blobs.

Erasing repository blobs is only reachable through `POST
/api/v1/device/factory-reset`, an explicit destructive action that requires
the current administrator password and the exact typed confirmation phrase
`FACTORY RESET` (`docs/SPEC_V2.md` §11.4, §13.9). There is no boot-time format
gesture; `POST /api/v1/device/reset-settings` restores non-credential defaults
and explicitly preserves repository blobs.
