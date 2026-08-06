# V2-035 — Hardware evidence harness preparation

**Status:** Harness prepared; physical evidence still required  
**Task:** V2-035  
**Target hardware:** ESP32-S3R8  
**Harness implementation commit:** `bb61b57207dc01510dafd84f26115ea50d0b63fd`  
**Provenance hardening commit:** `15ad1a2aa0e913c03338abeb016f7aa20acb05ec`  
**V2 API and diagnostics alignment commit:** `5d6e8ef152e6db9a318844915f6506b4f8e31f34`

## Prepared capability

The repository now contains a fail-closed physical storage evidence collector at
`scripts/run-v2-035-hardware.py`, its regression suite at
`tests/scripts/test-v2-035-hardware.py`, and the execution runbook at
`docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`.

The collector stages and verifies all seven V2-035 scenarios:

1. blob persistence through a real power cycle;
2. newest-first numeric blob ordering on the physical board;
3. deletion without collateral mutation;
4. no partial final blob after power loss during upload;
5. temporary-file cleanup after reboot;
6. HTTP 507 on real within-limit storage exhaustion while preserving committed data; and
7. mount failure without silent formatting of the `userdata` partition.

## Fail-closed properties

- The administrator password is read from an environment variable and is never
  written to the evidence record.
- Every preserved blob is verified byte-for-byte with SHA-256.
- Final evidence is bound to an exact 40-character firmware commit and the
  `ESP32-S3R8` target.
- The HTTP collector uses the current V2 production paths:
  `/api/v1/auth/login`, `/api/v1/blob`, `/api/v1/blob/{id}`, and
  `/api/v1/diagnostics`.
- The interrupted-upload request emits exactly one `Host` header and declares
  the production 131,072-byte content length.
- Power-cycle and interrupted-upload stages require diagnostics to report the
  same firmware build ID and a `power-on` reset.
- Reboot cleanup is read from the exact diagnostics schema:
  `blobScan.temporaryFileCount` must be zero and `blobScan.temporaryFiles` must
  be empty.
- The destructive mount-failure procedure requires a full partition backup,
  proves the failed-boot partition is byte-identical to the injected corrupt
  image, and proves restoration is byte-identical to the backup.
- Finalization refuses to succeed unless all seven scenarios have explicit
  passing observations and collector-created blobs are cleaned up.

## Pre-hardware audit corrections

The first collector draft was not accepted as ready merely because its isolated
unit tests passed. A source-to-production audit before physical execution found
and corrected these defects:

- the raw interrupted upload could emit duplicate `Host` headers;
- evidence was not bound to an exact firmware commit;
- login used the nonexistent `/api/v1/login` path instead of
  `/api/v1/auth/login`;
- blob operations used nonexistent plural `/api/v1/blobs` paths instead of the
  singular V2 resource;
- list validation expected ascending order although V2 requires newest-first
  numeric order; and
- reboot cleanup queried `/api/v1/status` for fields that exist only under the
  full diagnostics route and used the wrong field shape.

All repair workflows failed closed when a patch, obsolete test, or scope check
was wrong. No failed attempt committed partial production changes.

## Validation already performed

The provenance-hardening workflow:

- run `31093306125`, job `92589209791`;
- compiled the collector and regression test;
- executed `tests/scripts/test-v2-035-hardware.py`;
- checked the required `--firmware-sha` option;
- passed `git diff --check`;
- verified the exact staged path set; and
- removed its temporary patch and workflow files.

The V2 route and diagnostics alignment workflow:

- run `31094015484`, job `92591536719`;
- compiled and executed the repaired collector regression suite;
- verified all required paths against `contracts/v2/api/routes.json`;
- rejected legacy plural blob and legacy login paths;
- validated the exact diagnostics schema used for temporary-file cleanup;
- passed `git diff --check`;
- verified the exact staged path set; and
- removed its temporary patch and workflow files.

## Remaining gate

No physical ESP32-S3 serial device or network endpoint is exposed to the current
execution environment. Therefore no hardware result has been invented or
inferred from hosted CI. V2-035 and the Phase 3 hardware exit gate remain open
until the runbook is executed on the reference board, the finalized evidence
JSON is reviewed and committed, and the seven checklist items are then marked
complete.
