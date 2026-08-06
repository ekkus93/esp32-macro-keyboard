# V2-035 — Recovery and firmware-artifact verification hardening

**Status:** Harness hardening complete; physical V2-035 evidence still required  
**Task:** V2-035  
**Implementation commit:** `396dfa9e33e7ed9c75c00b36b7e07bc99af749cb`  
**Target hardware:** ESP32-S3R8  
**Required toolchain:** ESP-IDF v5.5.5

## Scope

This change hardens the physical V2-035 evidence collector before it is used on
the reference board. It does not claim any of the seven hardware checklist
items complete.

The audit found two classes of fail-open or unrecoverable behavior in the
prepared harness:

1. the collector trusted the application-image and full ELF SHA-256 values
   copied into `flash-manifest.json` without rechecking the actual application
   image supplied beside that manifest; and
2. mutating stages persisted their state only after a whole stage succeeded,
   so a host interruption or mid-stage error could strand collector-created
   blobs or partially delete sentinels without a safe, ownership-aware recovery
   path.

## Implemented hardening

### Exact firmware-artifact verification

Before reading the device blob baseline or performing any mutation, `start` now:

- requires the manifest to record exactly `ESP-IDF v5.5.5`;
- requires `appImage` to be a relative path contained by the manifest's build
  directory;
- requires the named application image to exist;
- recomputes the application-image SHA-256 and compares it with
  `appImageSha256`;
- reruns `esptool.py image_info` against that exact image;
- extracts and verifies the full 64-character ELF SHA-256;
- verifies the manifest's 39-character diagnostics build ID is the prefix of
  that independently verified ELF SHA-256; and
- compares that verified build ID with the physical board's diagnostics
  `buildId` before any blob creation or deletion.

The sanitized evidence therefore cannot claim a full application-image or ELF
hash solely because those strings were present in a JSON manifest.

### Durable collector-owned blob journal

The evidence state schema is now version 3. The collector writes the state file
before the first mutation and records every collector-owned blob immediately
after successful creation. It also records each successful deletion
immediately.

This journal is used by:

- initial numeric-ordering and deletion setup;
- real-storage-exhaustion uploads and cleanup; and
- final sentinel cleanup.

A failure after a device mutation but before the next host operation therefore
retains the exact blob ID and SHA-256 required for bounded recovery.

### Fail-closed recovery command

The new command is:

```bash
python3 scripts/run-v2-035-hardware.py recover-cleanup \
  --state "${V2_035_STATE}"
```

Recovery:

- verifies every baseline blob remains byte-identical;
- rejects any live blob ID that is neither part of the baseline nor recorded as
  collector-owned;
- verifies every surviving collector-owned blob before deletion;
- tolerates only the narrow crash case where a journaled owned blob was already
  deleted immediately before the host process stopped;
- deletes only collector-owned blobs;
- requires the final device blob set to equal the original baseline exactly;
- writes a recovered state before removing the local state file; and
- never guesses ownership from ID ranges, ordering, payload size, or content.

After successful recovery, the hardware run must restart from Stage 1 with a new
state path. Partial observations are not promoted into completion evidence.

### Resumable finalization

Finalization now enters and persists `finalize_in_progress` before deleting
sentinels. If the host stops after one sentinel deletion, rerunning `finalize`
verifies the baseline plus the remaining owned journal, completes the bounded
cleanup, and emits evidence only after `ownedBlobs` is empty and the exact
baseline is restored.

## Regression coverage

`tests/scripts/test-v2-035-hardware.py` now covers:

- valid application-image and ELF verification;
- dirty-manifest rejection;
- wrong ESP-IDF version rejection;
- application-image tampering rejection;
- board build-ID mismatch rejection;
- ownership-journal recovery that restores an exact baseline; and
- finalization resumption after an injected deletion failure.

The existing exact-gzip, V2-route, diagnostics-schema, interrupted-upload
header, complete-evidence, and mount-failure regression coverage remains in the
same suite.

## Scoped materialization evidence

The repository's temporary, self-removing materialization workflow failed
closed three times before the implementation commit was published:

- run `31108285028`, job `92639057080`: Python compilation rejected an
  incorrectly escaped generated newline in the collector source;
- run `31108391798`, job `92639428110`: collector compilation passed, then
  Python compilation rejected the equivalent generated newline in the test
  fixture; and
- run `31108456777`, job `92639654444`: both files compiled, then the regression
  suite detected that the generated ELF-SHA matcher looked for a literal
  backslash instead of whitespace.

No failed attempt committed generated production changes. The materialization
and publishing workflow succeeded only after all corrections:

- run `31108590384`;
- job `92640118254`;
- Python compilation passed;
- the complete V2-035 collector regression suite passed;
- collector and recovery command help parsing passed;
- `git diff --check` passed;
- the exact five-path staged scope was verified;
- both temporary materialization files were removed; and
- implementation commit
  `396dfa9e33e7ed9c75c00b36b7e07bc99af749cb` was pushed to `master`.

## Files changed

- `scripts/run-v2-035-hardware.py`
- `tests/scripts/test-v2-035-hardware.py`
- `docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`

Temporary materialization files were deleted in the implementation commit and
are not part of the permanent repository.

## Remaining hardware gate

The current execution environment exposes no reference ESP32-S3 serial device
or device network endpoint. No physical observation has been fabricated or
inferred from hosted tests.

V2-035 remains open until the runbook is executed against the ESP32-S3R8 and a
sanitized finalized evidence JSON proves all seven scenarios:

1. power-cycle persistence;
2. numeric ordering;
3. deletion preservation;
4. interrupted upload without a partial final blob;
5. reboot cleanup of temporary files;
6. real HTTP 507 storage exhaustion preserving all committed blobs; and
7. mount failure without formatting userdata.

No unchecked V2-035 task is claimed complete by this report.
