# ESP32 Macro Keyboard v2 — Phase 1 Contract Closeout and Production Limits Repair Specification

**Document status:** Authoritative repair specification  
**Date:** 2026-08-04  
**Target branch:** `master` only  
**Baseline SHA at review:** `077ea244a99e44f62da87222597dc7cb91bdeebb`  
**Companion TODO:**
`docs/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_TODO_2026-08-04.md`

## 1. Purpose

This repair closes the remaining Phase 1 v2 contract-layer gaps found during
source review of current `master` after CI was restored to green. It is a narrow
closeout and production-contract repair. It must not begin the Phase 2 deletion
of the retired firmware-owned repository architecture.

The repair has four purposes:

1. make the active production `/api/v1/limits` response match the full v2 limits
   contract;
2. run the mandatory ESP32-S3R8 PBKDF2 benchmark and freeze the password
   iteration count;
3. update stale Phase 1 evidence so completed work is recorded accurately and
   remaining boundaries are not overclaimed;
4. preserve a clean phase boundary: Phase 1 contract infrastructure may close
   only when its evidence is complete, while production v1 architecture removal
   remains Phase 2 work.

## 2. Authority

The product authority remains:

- `docs/SPEC_V2.md`
- `docs/UI_UX_SPEC_V2.md`
- `docs/TODO_V2.md`
- `docs/PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md`
- `docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md`
- `docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md`

This specification supersedes only the ambiguous or stale parts of the Phase 1
repair closeout. It does not supersede Phase 2 deletion requirements in
`docs/TODO_V2.md`.

When this document conflicts with current production v1 behavior, this document
is the intended repair target for Phase 1 closeout. When this document conflicts
with `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`, stop and report the conflict
rather than silently changing the product contract.

## 3. Scope

### 3.1 In scope

This repair may change:

- `contracts/v2/limits.json` only if a real contract issue is discovered;
- v2 API examples and TypeScript guards;
- the active production `/api/v1/limits` serializer;
- host tests for the limits serializer;
- v2 contract tests and drift checkers;
- PBKDF2 settings/auth constants and their mirrors;
- ESP32-S3R8 benchmark evidence documentation;
- Phase 1 closeout reports, TODO checkboxes, and boundary documentation.

### 3.2 Out of scope

This repair must not delete or migrate the retired v1 production architecture.
The following remain Phase 2 or later unless a later spec explicitly changes the
boundary:

- firmware-owned package repository deletion;
- firmware-owned macro repository deletion;
- package and macro CRUD route deletion;
- repository import, export, replace, restore, revision, backup, and merge
  behavior deletion;
- production parser migration to `macro_compile_v2`;
- production NVS migration from legacy provisioning structures to the v2 device
  settings record;
- React setup-client migration away from legacy setup paths;
- full production API serializer migration beyond the specific limits response
  repaired here.

## 4. Non-negotiable constraints

- Work directly on `master` unless the product owner explicitly changes the
  workflow.
- Do not create a feature branch or pull request for this repair unless asked.
- Do not weaken CI, remove checks, suppress warnings, add `|| true`, redirect
  diagnostics to hide failures, or accept partial command success.
- Do not rename old behavior to make it look v2-compliant.
- Do not claim hardware evidence from a host fake, build-only job, or compiler
  success.
- Do not commit secrets, AP passphrases, admin passwords, setup codes, session
  tokens, real repository contents, or real macro source.
- Every completed task must have implementation evidence and a reproducible
  command, test, or hardware log.

## 5. Repair decisions

### 5.1 Limits endpoint decision

The active production `GET /api/v1/limits` endpoint must expose the full v2
limits object. It must not expose an undocumented subset.

The response must include every key in `contracts/v2/limits.json`:

- `packageNameMaxBytes`
- `macroNameMaxBytes`
- `macroSourceMaxBytes`
- `compiledActionsMax`
- `delayDirectiveMaxMs`
- `keyPressMaxMs`
- `interKeyMaxMs`
- `estimatedDurationMaxMs`
- `executorAbsoluteDeadlineMs`
- `jsonBodyMaxBytes`
- `blobMaxBytes`
- `activeSessionsMax`
- `sessionIdleLifetimeSeconds`
- `sessionAbsoluteLifetimeSeconds`
- `serialConfirmationTimeoutSeconds`
- `adminPasswordMinBytes`
- `adminPasswordMaxBytes`
- `snapshotRetentionTargetMax`

The response must be a top-level JSON object. It must not use an `ok/data`
wrapper. It must not include retired v1-only fields such as `macrosPerPackage`,
`packages`, `importBytes`, validation-route limits, or repository-CRUD-only
limits.

### 5.2 PBKDF2 decision

The v2 settings/auth contract must not ship with a placeholder PBKDF2 iteration
count. The placeholder value
`measured-and-frozen-before-v0.2-acceptance` must be replaced with a concrete
positive integer chosen from reference ESP32-S3R8 measurement.

The selected value must target an approximately 250-500 ms derivation time on
the reference ESP32-S3R8. The selection must be based on observed benchmark
output, not host timing or theoretical estimates.

### 5.3 Phase boundary decision

Phase 1 may close only as a contract-layer milestone. It must not be described as
production v2 migration complete. If production v1 routes, repositories, parser
entry points, settings paths, or React setup paths remain, the reports must say
so explicitly and point to Phase 2 or later work.

## 6. Functional requirements

## 6.1 `/api/v1/limits` production response

### LIM-001 — Full v2 field set

`web_adapter_build_limits_json()` must emit every key from
`contracts/v2/limits.json` with values equal to `APP_V2_*` constants from
`firmware/components/app_contracts_v2/include/app_limits_v2.h`.

### LIM-002 — v2 shape

The serialized response must be a single JSON object containing only v2 limit
fields. It must not be wrapped in `ok`, `data`, or any other envelope.

### LIM-003 — No retired v1 fields

The serialized response must not contain package-count, macro-count,
import/export, validation-route, revision, ETag, repository index, restore, or
plural execution fields.

### LIM-004 — TypeScript response guard parity

`isLimitsResponse()` must require the same full field set as the firmware
serializer and the shared JSON contract. It must reject:

- missing v2 fields;
- extra fields;
- changed values;
- non-integer values;
- non-plain objects.

### LIM-005 — Example parity

`contracts/v2/api/examples.json` must contain a canonical `limits` example with
all fields and values from `contracts/v2/limits.json`.

### LIM-006 — Test parity

Host and TypeScript tests must fail if any of the following drift:

- a central limit value changes without the serializer/guard/example changing;
- a serializer field is removed;
- one of the four previously omitted fields is omitted again;
- retired v1 fields reappear;
- an `ok/data` wrapper reappears.

## 6.2 PBKDF2 benchmark and frozen iteration count

### PBK-001 — Measurement harness

The repair must identify or add a hardware measurement harness under the
existing firmware test-app flow. The harness must compile in the device-test
firmware and print enough timing data to choose the PBKDF2 iteration count.

The harness must exercise PBKDF2-HMAC-SHA-256 with the same implementation,
salt length, verifier length, and intended credential path as production v2
settings/auth.

### PBK-002 — Reference hardware run

The benchmark must be run on the reference ESP32-S3R8 device. The committed
evidence must record:

- board model;
- serial port;
- host operating system;
- firmware build ID or exact SHA;
- command used to build, flash, monitor, and run the benchmark;
- candidate iteration counts;
- sample count per candidate;
- median, p90, and worst-case timings where available;
- selected iteration count;
- rationale for the selected value.

### PBK-003 — Selection rule

The preferred selected value is the highest tested iteration count whose p90 is
at or below 500 ms and whose median is at or above 250 ms.

If no tested value satisfies that exact rule, select the best measured value for
the 250-500 ms target and document why. Do not invent a value without measuring
or interpolating from observed data. If measurement quality is ambiguous, rerun
the benchmark before freezing the count.

### PBK-004 — Contract freeze

The frozen iteration count must be committed into the v2 settings/auth contract
as a concrete integer. No placeholder string may remain in
`contracts/v2/device-settings.json` or any generated mirror.

### PBK-005 — Firmware constants

Firmware must expose the frozen iteration count through an audited constant used
by the settings/auth path. Any C header mirror or schema checker must fail if
the constant drifts from the JSON contract.

### PBK-006 — Tests

Native tests must prove that provisioned settings encode, decode, and validate a
nonzero frozen iteration count. Existing PBKDF2 known-answer tests must remain
intact and must not be weakened to accommodate timing work.

## 6.3 Evidence repair and documentation synchronization

### DOC-001 — Implementation report update

`docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md` must be
updated so it no longer says `final CI pending` for items that have passed on a
recorded exact SHA.

The report must identify:

- exact final SHA for this closeout repair;
- all files changed;
- commands and workflow runs used as evidence;
- focused validation status;
- full CI status;
- PBKDF2 hardware evidence or explicit product-owner deferral;
- remaining Phase 2 production-migration boundaries.

### DOC-002 — Repair TODO update

`docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md` must be updated so checked
items match real implementation and evidence. Do not check P1R-062 unless the
ESP32-S3R8 timing evidence and frozen iteration count are committed.

### DOC-003 — `TODO_V2.md` synchronization

`docs/TODO_V2.md` must distinguish:

- Phase 1 contract-layer closeout;
- Phase 2 deletion of retired production architecture;
- later production integration tasks for parser, settings/NVS, route table, and
  React setup paths.

### DOC-004 — No overclaiming

All closeout reports must explicitly avoid these claims unless the code and tests
actually prove them:

- production API is fully v2-compliant;
- production route table has removed all v1 routes;
- production parser uses `macro_compile_v2`;
- production NVS uses the v2 device-settings record;
- production React setup uses only the v2 setup contract;
- Phase 2 deletion has begun or completed.

## 7. Validation requirements

### VAL-001 — Focused local gates

At minimum, the following focused gates must pass after implementation:

```bash
python3 scripts/check-v2-limits.py
python3 scripts/check-v2-settings-schema.py
python3 scripts/check-v2-setup-route-policy.py
python3 scripts/check-v2-api-routes.py
./scripts/check-v2-contracts.sh
```

If the environment cannot run the full web dependency install, run
`./scripts/check-v2-contracts.sh --native-only` and record the limitation. That
limitation does not replace CI evidence.

### VAL-002 — Host and frontend evidence

The repair must run or obtain CI evidence for:

- native v2 CTest suite;
- host tests that include the limits serializer;
- focused v2 Vitest suites;
- TypeScript typecheck and lint as part of the normal webapp gate.

### VAL-003 — Device-test build evidence

The device-test firmware must build successfully after PBKDF2 and settings/auth
changes. If a hardware benchmark was added or changed, the same target must be
built by the permanent Device Test Build workflow.

### VAL-004 — Full permanent CI evidence

The final closeout SHA must pass the permanent `master` CI set:

- Quality;
- Host Tests;
- Browser Tests;
- Device Test Build.

Do not claim closeout based on older workflow runs from a previous SHA.

## 8. Acceptance criteria

This repair is complete only when all of the following are true:

- `/api/v1/limits` emits the full v2 limits object with all 18 fields;
- v2 examples, TypeScript guards, firmware serializer, and tests agree;
- the PBKDF2 iteration count is measured on ESP32-S3R8 and frozen as an integer;
- native tests prove provisioned settings carry the frozen PBKDF2 count;
- Phase 1 reports are updated and no longer contain stale pending-CI language;
- Phase 1 closeout and Phase 2 production-deletion boundaries are synchronized;
- focused gates pass;
- full permanent CI passes on one exact final `master` SHA;
- no report claims Phase 2 behavior that has not been implemented.

## 9. Failure handling

If a validation gate fails, fix the root cause. Do not weaken the gate or mark the
item complete with a known failure.

If the ESP32-S3R8 benchmark cannot be run because hardware is unavailable, the
implementation report must keep P1R-062 open unless the product owner explicitly
accepts a documented deferral. A deferral must name the missing hardware step and
must not freeze an unmeasured iteration count.

If the limits contract proves intentionally smaller than `contracts/v2/limits.json`,
stop and write a product decision before changing the implementation. The default
repair target in this specification is the full 18-field v2 limits object.
