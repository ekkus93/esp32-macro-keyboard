# Recovery policy

Recovery must preserve evidence and must never guess.

**Validation status:** every rule below is implemented and host-tested
(`tests/host/test_storage_atomic_recovery.c`,
`tests/host/test_storage_restore_transactions.c`, and the storage-package
test suites), including deterministic crash-consistency across every
interrupted phase. None has been validated against a real power interruption
on physical hardware - see `docs/HARDWARE_TEST_PLAN.md`'s "Persistence and
fault tests" (currently "Not run") and FIX1 TODO §20.4.

- LittleFS mount failures are visible. Firmware does not auto-format.
- An invalid persistent object is preserved and recorded in quarantine with its
  original path and parse reason.
- Atomic single-file updates write a unique exclusive temporary file, check all
  writes, flush, optionally sync, close, validate full byte-for-byte readback,
  and rename.
- Multi-file operations use a durable transaction manifest and idempotent phases.
- Deleting a set first renames it into `/data/trash/`; permanent cleanup occurs
  only after committed index state is verified.
- Replacement keeps the previous set in a backup location until staged data and
  indexes are committed.
- Unknown or malformed transaction manifests are preserved for diagnostics and
  cause a degraded or fatal state. They are not silently discarded.

Formatting userdata is an explicit destructive action that requires physical
confirmation or the documented boot gesture.
