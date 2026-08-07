# V2-035 — Finalization crash-recovery hardening

**Status:** Hosted harness hardening implemented and regression-validated; physical V2-035 evidence still required  
**Task:** V2-035  
**Implementation commit:** `3fd93a01b1fa825fc674166d522c5744cb5702c8`  
**Materialization trigger commit:** `d8bdbc0b5957f2bb7187543add5baec493c1c10a`  
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

## Scoped materialization and regression evidence

The one-time, self-removing materializer ran as GitHub Actions run `31157019104`, job `92798622525`, from trigger commit `d8bdbc0b5957f2bb7187543add5baec493c1c10a`.

The run completed successfully and, before publishing production changes, required:

```text
python3 -m py_compile scripts/run-v2-035-hardware.py tests/scripts/test-v2-035-hardware.py
python3 tests/scripts/test-v2-035-hardware.py
git diff --check
```

Observed regression output included:

```text
PASS: failed mount left the corrupt userdata partition byte-identical and backup restoration matched
PASS: collector-owned blobs were removed and the original baseline was restored
PASS: complete V2-035 evidence written to <temporary path>/evidence.json
PASS: <temporary path>/evidence.json contains all seven required physical scenarios
PASS: V2-035 hardware evidence collector regression tests
```

The staged scope was limited to:

- `scripts/run-v2-035-hardware.py`;
- `tests/scripts/test-v2-035-hardware.py`;
- this implementation report; and
- deletion of the one-time materializer workflow.

The materializer then committed and pushed implementation commit `3fd93a01b1fa825fc674166d522c5744cb5702c8`. The temporary workflow is not present in the resulting repository.

## Permanent-CI boundary

This report update is a normal GitHub API commit so the permanent Browser Tests, Host Tests, Device Test Build, and Quality workflows run on the resulting exact repository head. Those runs must all succeed before hosted V2-035 preparation is considered settled. Any failure remains a blocker and must be repaired rather than documented away.

## Completion boundary

This repair does not satisfy any physical V2-035 checklist item. V2-035 remains open until the seven scenarios are executed on the reference ESP32-S3R8 and sanitized finalized evidence is committed. No unchecked V2-035 task is claimed complete by this report.
