# ESP32 Macro Keyboard v2 — Phase 1 Completion Repair

**Status:** In progress; hardware completion not claimed  
**Date:** 2026-08-04  
**Target branch:** `master` only  
**Starting master SHA:** `de99e4a4b287f0553fdf7d931e4a067d7c63ac23`  
**Repair specification:**
[`docs/PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md`](../PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md)  
**Repair checklist:**
[`docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md`](../PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md)  
**Closeout addendum:**
[`docs/implementation-v2/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_2026-08-04.md`](PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_2026-08-04.md)

## 1. Scope and phase boundary

This repair addresses the Phase 1 v2 contract layer. It does not delete the
production v1 package/macro repositories, CRUD routes, revision behavior,
production parser entry points, legacy production NVS structures, or legacy React
setup paths. Those remain Phase 2 or later work.

`Phase 1 complete` means only that the shared v2 contract infrastructure is
synchronized, tested, documented, and supported by required hardware evidence. It
does not mean production v1 architecture has been removed.

All work was committed directly to `master`. No feature branch, pull request, or
temporary workflow was created, and no gate was weakened.

## 2. Implemented task evidence

The following task groups are implemented and were validated on exact SHA
`077ea244a99e44f62da87222597dc7cb91bdeebb`:

- P1R-020: v2-neutral repository symbols;
- P1R-021/P1R-022: checked-in repository fixture corpus and fixture-driven tests;
- P1R-030: explicit dense-array validation and sparse-array regressions;
- P1R-031/P1R-032: reviewed 21-route v2 manifest, TypeScript validator, C mirror,
  Python drift checker, native tests, and focused-gate registration;
- P1R-033/P1R-034: warning-as-error native API contract evidence and strict
  unknown-field coverage;
- P1R-040: centralized limits mirrors and drift checking;
- P1R-050/P1R-051: truthful action/source limit tests and cross-language macro
  conformance, including standalone `{A}` and `{1}` rejection;
- P1R-060/P1R-061: settings schema drift checks and strengthened binary-record
  rejection/reset-preservation tests;
- P1R-070: setup-route policy synchronization and drift tests;
- P1R-080/P1R-081: v2 traceability generation and freshness gating.

The permanent workflows on SHA `077ea244a99e44f62da87222597dc7cb91bdeebb`
completed successfully:

| Workflow | Run ID | Job evidence | Conclusion |
| --- | ---: | --- | --- |
| Quality | `30950331418` | `92130511005` | success |
| Device Test Build | `30950331409` | `92130510699` | success |
| Host Tests | `30950331206` | all five jobs | success |
| Browser Tests | `30950331210` | Real Chrome Workflows | success |

The former `final CI pending` wording is therefore retired for the task groups
listed above.

## 3. Production limits closeout repair

A later source review found that `contracts/v2/limits.json` contained 18 values
while the production serializer, canonical API example, TypeScript response type,
and runtime guard exposed only 14. The omitted values were:

- `activeSessionsMax`;
- `sessionIdleLifetimeSeconds`;
- `sessionAbsoluteLifetimeSeconds`;
- `serialConfirmationTimeoutSeconds`.

The closeout repair now:

- emits all 18 fields from `web_adapter_build_limits_json()`;
- preserves a top-level object without `ok` or `data` wrapping;
- rejects truncation and clears the output buffer;
- excludes retired v1 package/import fields;
- expands the canonical API example and `LimitsResponse` type;
- requires all 18 fields and exact values in `isLimitsResponse()`;
- tests removal and mutation of every individual limits field;
- makes `scripts/check-v2-limits.py` compare the API example with the central
  limits contract.

Detailed commits, files, and current validation state are recorded in the
closeout addendum linked above.

## 4. Production integration boundaries

The following are explicitly not claimed complete:

- active production package/macro route-table migration;
- deletion of firmware-owned package/macro repositories;
- production execution migration from legacy parser entry points to
  `macro_compile_v2`;
- production provisioning/NVS migration to the v2 binary settings record;
- production React setup migration to the exact v2 setup route contract.

The v2 route manifest describes and protects the target surface. It does not
prove the active production route table has already been migrated.

## 5. Traceability status

`docs/SPEC_V2_TEST_TRACEABILITY.md` fingerprints both authoritative v2 source
documents. Both remain visibly unmapped because the current tests do not yet use
explicit `SPEC_V2 §...` or `UI_UX_SPEC_V2 §...` references. This is an honest
worklist, not a coverage claim.

## 6. PBKDF2 hardware blocker

P1R-062 remains open.

The device-test harness exists and uses the same production mbedTLS
PBKDF2-HMAC-SHA-256 derivation path. It measures 60,000, 90,000, 120,000, and
150,000 iterations with ten samples and prints median, p90, and worst-case timing.
The execution procedure is documented in `firmware/test_app/README.md`.

No reference ESP32-S3R8 timing run has been performed through this connector
session. No board model, serial port, host OS, raw timing distribution, or selected
iteration count has been recorded. The v2 settings contract therefore retains its
measurement placeholder, and no measured-and-frozen iteration value is claimed.

The existing legacy auth constant of 120,000 iterations is not accepted as v2
hardware evidence merely because it is already present in source.

## 7. Remaining work before Phase 1 closeout

1. Run the `[benchmark]` Unity test on the reference ESP32-S3R8.
2. Record the board, port, host OS, exact SHA, commands, and every
   `PBKDF2_BENCH` line.
3. Select a measured value meeting the 250–500 ms rule.
4. Freeze that value in the v2 settings/auth contract and add drift and binary
   record tests.
5. Synchronize `docs/TODO_V2.md`, the original Phase 1 repair checklist, and the
   closeout checklist after the hardware result exists.
6. Run all four permanent workflows on one final exact `master` SHA and record
   their run/job IDs.

Phase 1 completion and Phase 2 entry are not claimed while those items remain
open. No unchecked task is claimed complete.
