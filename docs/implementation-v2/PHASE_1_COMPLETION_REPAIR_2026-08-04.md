# ESP32 Macro Keyboard v2 — Phase 1 Completion Repair

**Status:** In progress; completion not claimed  
**Date:** 2026-08-04  
**Target branch:** `master` only  
**Starting master SHA:** `de99e4a4b287f0553fdf7d931e4a067d7c63ac23`  
**Repair specification:**
[`docs/PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md`](../PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md)  
**Repair checklist:**
[`docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md`](../PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md)

## 1. Scope and phase boundary

This repair closes defects in the Phase 1 v2 contract layer. It does not delete
production v1 package repositories, macro repositories, CRUD routes, revision
behavior, serializers, or plural execution resources. Those deletions remain
Phase 2 work.

For this report, `Phase 1 complete` can mean only that the Phase 1 contract
infrastructure is complete, synchronized, tested, and truthfully documented. It
does not mean the production v1 architecture has been removed.

Phase 2 implementation has not begun during this repair.

## 2. Workflow constraints observed

- All changes were committed directly to `master`.
- No feature branch was created.
- No pull request was opened.
- No temporary workflow file was added.
- Existing gates were not weakened or bypassed.
- Failures were repaired at their root cause and rerun through permanent CI.

## 3. Starting evidence

The repair began after the two control documents were committed to `master` at
`de99e4a4b287f0553fdf7d931e4a067d7c63ac23`. The previously reviewed contract
baseline was `26920fe1917895f0ebd0ab4287285a09dcedde3e`.

At the start of the repair:

- no open Phase 1 repair pull request existed;
- the permanent CI status bridge used issues 19 through 22 for `master` push
  workflows;
- the prior Phase 1 checkpoint explicitly did not claim Phase 1 completion;
- production v1 package, macro, setup, parser, settings, and execution paths still
  existed and remained outside this contract-repair scope.

## 4. Task evidence

| Task group | Status | Implementation and evidence |
| --- | --- | --- |
| P1R-020 repository symbol cleanup | Implemented; final CI pending | Removed `RepositoryV1`, `RepositoryPackageV1`, `RepositoryMacroV1`, `validateRepositoryV1`, and `serializeRepositoryV1`; updated consumers and tests. |
| P1R-021/P1R-022 repository fixture corpus | Implemented; final CI pending | Added a checked-in corpus covering valid, boundary, malformed, duplicate-ID, unknown-field, active-package, source-size, timing, sparse-array, non-finite, and prototype cases. |
| P1R-030 sparse API arrays | Implemented; final CI pending | Replaced `Array.prototype.every()` density detection with explicit own-index checks; added sparse blob and diagnostics array regressions. |
| P1R-031/P1R-032 route manifest | Implemented; final CI pending | Added `contracts/v2/api/routes.json`, exact TypeScript validation, C mirror, Python drift checker, native C tests, and focused-gate registration. The manifest contains 21 v2 routes and no package, macro, validation, or plural execution path. |
| P1R-033/P1R-034 API contract evidence | Implemented in Phase 1 scope; final CI pending | Added warning-as-error C route/limit contract tests and nested unknown-field tests. C parser/serializer parity for all API examples is not claimed where no C parser or serializer exists. |
| P1R-041 production limits response | Implemented; host evidence observed | `web_adapter_build_limits_json()` now emits the exact v2 limits object without `ok/data` wrapping or v1 package/import fields. The host test asserts the full serialized response. |
| P1R-050 macro test truthfulness | Implemented; final CI pending | Split source-byte and compiled-action claims. The tests now state that schema-v1 source size bounds action count before a 4097th one-byte action can be compiled. |
| P1R-051 canonical directives | Implemented; final CI pending | Both C and TypeScript now explicitly reject standalone `{A}` and `{1}` while preserving valid chord behavior. |
| P1R-061 device settings | Implemented; final CI pending | Added unsupported credential/password-algorithm version rejection and stronger reset preservation assertions, including credential version. |
| P1R-080/P1R-081 traceability | Implemented; final CI pending | Replaced the retired `docs/SPEC.md` generator with a fail-closed v2 source-fingerprint report covering `SPEC_V2.md` and `UI_UX_SPEC_V2.md`. The report is nonzero and explicitly leaves both sources unmapped until deliberate v2 citations are added. |
| P1R-062 PBKDF2 measurement | Open | No reference ESP32-S3R8 timing run has been performed in this connector-only session. No iteration count is frozen or claimed. |
| P1R-010/P1R-011 phase-boundary docs | In progress | This report states the boundary. `docs/TODO_V2.md` and the prior checkpoint still require final synchronized wording. |
| P1R-090 through P1R-093 final validation | In progress | Permanent `master` CI is being used. Final exact-SHA results are not yet recorded. |

## 5. Failure-driven repair record

### 5.1 Quality formatting failure

Quality run `30940176253` on `993a5ba03f7d694405f4d3704fc3bc757babb13b`
failed the authoritative formatting gate for the first C route mirror, its native
test, and the limits serializer.

The repair replaced the fragile X-macro mirror with a typed static C contract
array, updated the native consumer and drift checker, and normalized the limits
serializer argument layout. No formatter or warning gate was weakened.

### 5.2 Host include integration failure

Host run `30942096626` on
`911b70660d2cf35bff22206887bc59233f646b08` failed because the host build of
`web_server_adapter_json.c` could not resolve `app_limits_v2.h`.

The source now uses a location-stable sibling-component include, so the same
production file builds under ESP-IDF, ordinary host tests, native coverage, and
sanitizers without broad target-specific include leakage.

On run `30942349476` for
`7befd65ce8daa95e091803c4f95d7424166608c3`:

- Host Tests passed;
- Host ASan and UBSan passed;
- native coverage was no longer reported as a failing completed job at the last
  observed status update.

### 5.3 Frontend fixture type and lint failures

The frontend initially inferred checked-in empty diagnostics arrays as `never[]`,
which made sparse `string[]` fixtures fail type checking. The fixtures now use
explicit mutable test-only types.

A subsequent strict ESLint run found one unnecessary assertion in the route
manifest test. It was removed rather than suppressed.

### 5.4 Exact route metadata validation failure

Frontend coverage exposed that the first TypeScript route validator rejected
changed identities but accepted otherwise valid changed success status and
response content type values.

The validator now compares every reviewed route field:

- ID;
- method;
- path;
- authentication policy;
- request body contract;
- request content type;
- request maximum;
- response content type;
- success status;
- exact ordered error-status set.

The failing mutations are therefore repaired in implementation, not hidden by
changing their expected test result.

## 6. Production integration boundaries

The following boundaries remain true and must not be described as completed
Phase 1 production migration:

- production still contains firmware-owned package and macro repositories;
- production still contains package/macro CRUD and related v1 routes;
- production execution/validation still contains legacy parser entry paths;
- production provisioning/NVS still contains legacy settings structures;
- production React setup paths remain legacy until their later integration phase;
- deleting those paths is Phase 2 or later work under `docs/TODO_V2.md`.

The repaired v2 route manifest defines the intended target surface and prevents
contract drift. It does not claim the active production route table has already
been migrated.

## 7. Traceability status

`docs/SPEC_V2_TEST_TRACEABILITY.md` now fingerprints both authoritative v2 source
documents and reports a nonzero source count. The conservative initial report
marks both source documents unmapped because existing tests predominantly cite
retired `SPEC` section labels rather than explicit `SPEC_V2` or
`UI_UX_SPEC_V2` labels.

This is intentional. The report is a visible worklist, not a coverage score, and
it does not infer coverage from legacy citations.

## 8. Hardware evidence

No hardware command was run during this repair session. In particular:

- no ESP32-S3R8 PBKDF2 benchmark was run;
- no board model, serial port, build ID, timing distribution, or selected
  iteration count is available;
- P1R-062 remains open;
- Phase 1 repair completion is not claimed while that mandatory item remains open,
  unless the product owner explicitly accepts a documented deferral.

## 9. Validation still required

Before this report may claim completion:

1. all focused v2 checkers and CTest/Vitest suites must pass;
2. `./scripts/check-all.sh` must pass through the permanent Quality workflow;
3. Browser Tests, Host Tests, Device Test Build, and Quality must pass on one exact
   final `master` SHA;
4. `docs/TODO_V2.md`, the prior Phase 1 checkpoint, this report, and the repair
   TODO must agree on the phase boundary;
5. all completed repair-TODO items must be checked with evidence;
6. P1R-062 must either have real ESP32-S3R8 evidence or remain explicitly open
   under a product-owner-approved deferral.

No unchecked task is claimed complete in this report.
