# V2-035 — Recovery and firmware-artifact verification hardening

**Status:** Harness hardening complete; physical V2-035 evidence still required  
**Task:** V2-035  
**Implementation commits:**
`396dfa9e33e7ed9c75c00b36b7e07bc99af749cb`,
`5f319cf16842d6e80519379c619c74e042ec8ebb`  
**Target hardware:** ESP32-S3R8  
**Required toolchain:** ESP-IDF v5.5.5

## Scope

This work hardens the physical V2-035 evidence collector before it is used on
the reference board. It does not claim any of the seven hardware checklist
items complete.

The audit found three classes of fail-open or unrecoverable behavior in the
prepared harness:

1. the collector trusted the application-image and full ELF SHA-256 values
   copied into `flash-manifest.json` without rechecking the actual application
   image supplied beside that manifest;
2. mutating stages persisted their state only after a whole stage succeeded,
   so a host interruption or mid-stage error could strand collector-created
   blobs or partially delete sentinels without a safe, ownership-aware recovery
   path; and
3. even after immediate post-response journaling was added, a final blob could
   still commit in the narrow interval between beginning an upload request and
   persisting the returned blob ID.

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

The evidence state schema is version 3. The collector writes the state file
before the first mutation, records every collector-owned blob immediately after
successful creation, and records each successful deletion immediately.

The journal is used by:

- initial numeric-ordering and deletion setup;
- real-storage-exhaustion uploads and cleanup; and
- final sentinel cleanup.

### Pre-request, hash-bound creation intent

Before every request capable of committing a final blob, the collector now
persists a `pendingCreation` record containing:

- the exact byte-for-byte SHA-256 of a fresh, randomly namespaced payload;
- the complete pre-request blob ID-to-SHA-256 snapshot;
- the destination stage journal, when applicable; and
- the request start time.

This applies to:

- the three Stage 1 ordering uploads;
- the intentionally interrupted upload;
- every storage-exhaustion upload attempt; and
- any recovery path that must reconcile a request whose response was not
  durably journaled.

After a normal `201` response, the collector reloads the returned blob and
verifies its bytes before atomically converting the pending intent into an
owned-blob journal entry. After a non-`201` response, it clears the intent only
if the complete live blob snapshot is unchanged. A transport error or host
termination leaves the intent intact.

### Fail-closed recovery command

The recovery command is:

```bash
python3 scripts/run-v2-035-hardware.py recover-cleanup \
  --state "${V2_035_STATE}"
```

Recovery first reconciles a pending creation:

- zero new IDs means the request did not commit a final blob, so the intent can
  be cleared;
- exactly one new ID is adopted as collector-owned only when its reloaded bytes
  match the pre-request SHA-256 exactly; and
- multiple new IDs, a missing or changed pre-request blob, or any hash mismatch
  fails closed without deleting anything.

After reconciliation, recovery:

- verifies every baseline blob remains byte-identical;
- rejects any live blob ID that is neither baseline nor collector-owned;
- verifies every surviving collector-owned blob before deletion;
- tolerates only the narrow crash case where a journaled owned blob was already
  deleted immediately before the host process stopped;
- deletes only collector-owned blobs;
- requires the final device blob set to equal the original baseline exactly;
- writes a recovered state before removing the local state file; and
- never guesses ownership from ID ranges, ordering, payload size, or an
  unrecorded deterministic payload.

After successful recovery, the hardware run must restart from Stage 1 with a new
state path. Partial observations are not promoted into completion evidence.

### Interrupted-upload recovery binding

The interrupted-upload stage now changes phase and persists its hash-bound
creation intent before opening the slow raw HTTP upload. Successful verification
requires the post-reboot final-blob set to equal the exact pre-request snapshot
and requires that the stored intent matches that snapshot. The intent is cleared
in the same local evidence write that records the two interrupted-upload
scenarios.

If the upload unexpectedly committed a final blob or the host stopped after the
server committed it, the evidence stage fails. `recover-cleanup` can identify
and remove only the exact hash-bound pending blob before the entire V2-035 run
is restarted.

### Resumable finalization

Finalization enters and persists `finalize_in_progress` before deleting
sentinels. If the host stops after one sentinel deletion, rerunning `finalize`
verifies the baseline plus the remaining owned journal, completes the bounded
cleanup, and emits evidence only after `ownedBlobs` is empty, no pending
creation remains, and the exact baseline is restored.

## Regression coverage

`tests/scripts/test-v2-035-hardware.py` now covers:

- valid application-image and ELF verification;
- dirty-manifest rejection;
- wrong ESP-IDF version rejection;
- application-image tampering rejection;
- board build-ID mismatch rejection;
- ownership-journal recovery that restores an exact baseline;
- pending-creation adoption of exactly one new blob with the exact recorded
  payload hash;
- refusal to adopt an ambiguous set of multiple new blob IDs;
- successful creation-intent conversion into the owned and stage journals;
- finalization resumption after an injected deletion failure; and
- completed-evidence refusal while a creation intent remains unresolved.

The existing exact-gzip, V2-route, diagnostics-schema, interrupted-upload
header, complete-evidence, and mount-failure regression coverage remains in the
same suite.

## Scoped materialization evidence

### Artifact verification and owned-journal pass

The first temporary, self-removing materialization workflow failed closed three
times before publishing production changes:

- run `31108285028`, job `92639057080`: Python compilation rejected an
  incorrectly escaped generated newline in the collector source;
- run `31108391798`, job `92639428110`: collector compilation passed, then
  Python compilation rejected the equivalent generated newline in the test
  fixture; and
- run `31108456777`, job `92639654444`: both files compiled, then the regression
  suite detected that the generated ELF-SHA matcher looked for a literal
  backslash instead of whitespace.

No failed attempt committed generated production changes. The workflow then
succeeded:

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

### Pending-creation crash-window pass

The second temporary, self-removing workflow succeeded on its first run:

- run `31109181436`;
- job `92642166455`;
- the materializer itself compiled before execution;
- collector and test Python compilation passed;
- all V2-035 collector regression tests, including the new pending-intent tests,
  passed;
- command-line parser checks and `git diff --check` passed;
- the exact five-path staged scope was verified;
- both temporary files were removed; and
- implementation commit
  `5f319cf16842d6e80519379c619c74e042ec8ebb` was pushed to `master`.

## Permanent files changed

- `scripts/run-v2-035-hardware.py`
- `tests/scripts/test-v2-035-hardware.py`
- `docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`
- `docs/implementation-v2/V2_035_RECOVERY_AND_ARTIFACT_VERIFICATION_2026-08-06.md`

Temporary materialization files were deleted in their implementation commits
and are not part of the permanent repository.

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
