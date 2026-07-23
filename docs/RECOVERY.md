# Recovery policy

Recovery must preserve evidence and must never guess.

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
