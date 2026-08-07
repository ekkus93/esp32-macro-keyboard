# V2-035 — Finalization crash-recovery hardening

**Status:** Hosted harness hardening implemented; physical V2-035 evidence still required  
**Task:** V2-035  
**Target hardware:** ESP32-S3R8

## Finding

A fail-closed audit of the prepared V2-035 physical collector found a real crash-window bug in finalization. The collector documented that `finalize` was resumable if the host stopped after deleting a sentinel, but the resume precheck required every journaled collector-owned blob to still exist. A host failure after the device accepted a delete and before the local ownership journal was rewritten therefore made finalization unrecoverable.

The audit also found that the standalone `validate` command accepted any state phase allowed by the internal pre-finalization validator and did not verify the emitted `evidenceSha256`. That was stricter than no validation, but weaker than the finalized-evidence contract.

## Repair

- `finalize_in_progress` now uses the ownership-aware recovery snapshot check. Baseline blobs must all exist and remain byte-identical, unowned IDs are rejected, and collector-owned blobs may already be absent because deletion is the intended operation.
- The delete loop then journals already-missing collector-owned blobs and completes the bounded cleanup.
- Finalized evidence validation now requires `phase == complete`.
- Finalized evidence validation recomputes and verifies `evidenceSha256`.
- Common complete-state validation now requires the exact state schema, task ID, and ESP-IDF version.
- Regression coverage now injects the exact device-delete/local-journal crash window and proves a second `finalize` invocation recovers it.
- Regression coverage also proves a hash-consistent but non-finalized evidence object is rejected by the standalone validator.

## Completion boundary

This repair does not satisfy any physical V2-035 checklist item. V2-035 remains open until the seven scenarios are executed on the reference ESP32-S3R8 and sanitized finalized evidence is committed.
