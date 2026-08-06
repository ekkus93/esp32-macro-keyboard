# V2-035 — Hardware evidence harness preparation

**Status:** Harness prepared; physical evidence still required  
**Task:** V2-035  
**Target hardware:** ESP32-S3R8  
**Harness implementation commit:** `bb61b57207dc01510dafd84f26115ea50d0b63fd`  
**Harness hardening commit:** `15ad1a2aa0e913c03338abeb016f7aa20acb05ec`

## Prepared capability

The repository now contains a fail-closed physical storage evidence collector at
`scripts/run-v2-035-hardware.py`, its regression suite at
`tests/scripts/test-v2-035-hardware.py`, and the execution runbook at
`docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`.

The collector stages and verifies all seven V2-035 scenarios:

1. blob persistence through a real power cycle;
2. numeric blob ordering on the physical board;
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
- The interrupted-upload request emits exactly one `Host` header and declares
  the production 131,072-byte content length.
- The destructive mount-failure procedure requires a full partition backup,
  proves the failed-boot partition is byte-identical to the injected corrupt
  image, and proves restoration is byte-identical to the backup.
- Finalization refuses to succeed unless all seven scenarios have explicit
  passing observations and collector-created blobs are cleaned up.

## Validation already performed

The scoped hardening workflow compiled the collector and regression test,
executed `tests/scripts/test-v2-035-hardware.py`, checked the required
`--firmware-sha` CLI option, passed `git diff --check`, verified the exact staged
path set, removed its temporary patch/workflow files, and committed the
permanent result.

## Remaining gate

No physical ESP32-S3 serial device or network endpoint is exposed to the current
execution environment. Therefore no hardware result has been invented or
inferred from hosted CI. V2-035 and the Phase 3 hardware exit gate remain open
until the runbook is executed on the reference board, the finalized evidence
JSON is reviewed and committed, and the seven checklist items are then marked
complete.
