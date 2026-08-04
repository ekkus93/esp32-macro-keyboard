# ESP32 Macro Keyboard v2 — Phase 1 Contract Closeout and Production Limits Repair TODO

**Document status:** Authoritative repair checklist; hardware closeout remains open  
**Date:** 2026-08-04  
**Target branch:** `master` only  
**Baseline SHA at review:** `077ea244a99e44f62da87222597dc7cb91bdeebb`  
**Software-repair evidence:**
`docs/implementation-v2/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_2026-08-04.md`  
**Companion spec:**
`docs/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_SPEC_2026-08-04.md`

## 0. Rules for this repair

- [x] Work directly on `master` unless the product owner explicitly changes the
      workflow.
- [x] Do not create a feature branch or pull request unless asked.
- [x] Do not weaken, skip, or suppress any existing gate.
- [x] Do not use `|| true`, ignored exit codes, hidden diagnostics, or warning
      suppression to make CI pass.
- [x] Do not begin Phase 2 deletion work in this repair.
- [x] Do not claim production API, parser, NVS, setup, or route-table v2 parity
      unless the active production implementation and tests prove it.
- [x] Do not check any item without implementation evidence.
- [x] Keep all evidence free of passwords, passphrases, setup codes, session
      tokens, private repository contents, and real macro source.

## P1C-000 — Confirm baseline and scope

- [x] Confirm current `master` SHA before starting implementation.
- [x] Record whether `master` still includes
      `077ea244a99e44f62da87222597dc7cb91bdeebb` in history.
- [x] Read the companion spec for this repair.
- [x] Read `docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md`.
- [x] Read `docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md`.
- [x] Confirm this repair is limited to Phase 1 contract closeout and the
      production `/api/v1/limits` contract repair.
- [x] Confirm Phase 2 deletion of package/macro repositories and CRUD routes is
      not part of this repair.

Evidence: the first implementation SHA was
`cbfa0c0ca3b07ac7592c1463a9b326dd6e2bf074`; GitHub compare confirmed current
`master` is a descendant of the reviewed baseline with no divergence.

## P1C-010 — Inspect current limits contract drift

- [x] Inspect `contracts/v2/limits.json`.
- [x] Inspect `firmware/components/app_contracts_v2/include/app_limits_v2.h`.
- [x] Inspect `webapp/src/v2/limits.ts`.
- [x] Inspect `scripts/check-v2-limits.py`.
- [x] Inspect `firmware/components/web_server/web_server_adapter_json.c`.
- [x] Inspect `contracts/v2/api/examples.json`.
- [x] Inspect `webapp/src/v2/apiGuards.ts`.
- [x] Inspect host tests for `web_adapter_build_limits_json()`.
- [x] Confirm whether the active production serializer currently omits any v2
      central limits.
- [x] Confirm whether TypeScript examples and guards currently omit any v2
      central limits.
- [x] Record the exact omitted keys, if any, before modifying code.

Finding: the serializer, canonical API example, TypeScript response type, and
runtime guard omitted `activeSessionsMax`, `sessionIdleLifetimeSeconds`,
`sessionAbsoluteLifetimeSeconds`, and `serialConfirmationTimeoutSeconds`.

## P1C-011 — Repair production `/api/v1/limits` serializer

- [x] Make `web_adapter_build_limits_json()` emit a top-level JSON object with
      every key from `contracts/v2/limits.json`.
- [x] Include `activeSessionsMax`.
- [x] Include `sessionIdleLifetimeSeconds`.
- [x] Include `sessionAbsoluteLifetimeSeconds`.
- [x] Include `serialConfirmationTimeoutSeconds`.
- [x] Preserve existing v2 fields and values.
- [x] Keep all values sourced from `APP_V2_*` firmware constants.
- [x] Ensure the response has no `ok` wrapper.
- [x] Ensure the response has no `data` wrapper.
- [x] Ensure the response has no retired v1-only fields such as
      `macrosPerPackage`, `packages`, or `importBytes`.
- [x] Ensure truncation or formatting failure still returns an error and clears
      the output buffer.

Implementation commits:
`49424659d9530f3139101a970245e2252f66e8fa` and
`c580238589c507aa04646a3e9d41932b3ce5a91c`.

## P1C-012 — Repair limits examples and TypeScript guards

- [x] Update `contracts/v2/api/examples.json` so `limits` contains every key from
      `contracts/v2/limits.json`.
- [x] Update `webapp/src/v2/apiGuards.ts` so `isLimitsResponse()` requires every
      v2 limits field.
- [x] Ensure `isLimitsResponse()` rejects missing session-limit fields.
- [x] Ensure `isLimitsResponse()` rejects missing serial-confirmation timeout.
- [x] Ensure `isLimitsResponse()` rejects extra fields.
- [x] Ensure `isLimitsResponse()` rejects changed central limit values.
- [x] Ensure public TypeScript exports still compile through
      `webapp/src/v2/apiContracts.ts`.

Implementation commits:
`75f29c7bec5c84895b52558ab3b782657184be44`,
`12dc26dbb0389fc72cacdb735e4d96fafa417410`, and
`ed19096a98cfe7232afaa31199f7e5243714cc73`.

## P1C-013 — Strengthen limits tests

- [x] Update the host serializer test to assert the full 18-field serialized
      limits response.
- [x] Add or update a host assertion that no `ok` wrapper appears.
- [x] Add or update a host assertion that no `data` wrapper appears.
- [x] Add or update host assertions that retired v1-only fields do not appear.
- [x] Add or update TypeScript tests proving all canonical limit values are
      accepted.
- [x] Add or update TypeScript tests proving any omitted field is rejected.
- [x] Add or update TypeScript tests proving changed values are rejected.
- [ ] Run the focused limits checker:

```bash
python3 scripts/check-v2-limits.py
```

The committed permanent Quality workflow is the authoritative execution path for
this command. Final exact-SHA evidence remains open below.

## P1C-020 — Confirm PBKDF2 benchmark harness

- [x] Inspect `firmware/test_app` for the PBKDF2 benchmark test.
- [x] Confirm the benchmark uses PBKDF2-HMAC-SHA-256.
- [x] Confirm the benchmark uses the intended salt length and verifier length.
- [x] Confirm the benchmark uses the same production authentication derivation
      implementation.
- [x] Confirm the benchmark prints candidate iteration count, sample count,
      median, p90, and worst-case timing.
- [ ] Confirm the benchmark covers enough candidates to select a 250-500 ms
      derivation target on ESP32-S3R8.
- [ ] Confirm the device-test firmware still builds after the documentation and
      closeout changes on the final exact SHA.

The harness measures 60,000, 90,000, 120,000, and 150,000 iterations with ten
samples. Candidate sufficiency cannot be established until the physical timings
are observed.

## P1C-021 — Run PBKDF2 benchmark on ESP32-S3R8

- [ ] Build the device-test firmware for the reference ESP32-S3R8.
- [ ] Flash the device-test firmware to the reference ESP32-S3R8.
- [ ] Run the PBKDF2 benchmark test.
- [ ] Capture the raw benchmark output.
- [ ] Record the board model.
- [ ] Record the serial port.
- [ ] Record the host operating system.
- [ ] Record the exact firmware build ID or commit SHA.
- [ ] Record the build, flash, monitor, and test commands.
- [ ] Record every candidate iteration count.
- [ ] Record sample count per candidate.
- [ ] Record median timing per candidate.
- [ ] Record p90 timing per candidate.
- [ ] Record worst-case timing per candidate.
- [ ] Select the highest measured candidate whose p90 is at or below 500 ms and
      whose median is at or above 250 ms.
- [ ] If no candidate exactly satisfies that rule, select the best measured value
      for the 250-500 ms target and document the rationale.
- [ ] Do not freeze an interpolated or unmeasured value unless the product owner
      explicitly accepts that decision in the report.

Open reason: no physical ESP32-S3R8 is connected to the GitHub connector session.

## P1C-022 — Freeze PBKDF2 iteration count in contracts and firmware

- [ ] Replace the placeholder
      `measured-and-frozen-before-v0.2-acceptance` in
      `contracts/v2/device-settings.json` with the selected integer.
- [ ] Add or update a firmware constant for the frozen PBKDF2 iteration count.
- [ ] Ensure the settings/auth path uses the frozen constant for newly
      provisioned credentials.
- [ ] Ensure schema or drift checks fail if the JSON contract and firmware
      constant disagree.
- [ ] Ensure no placeholder PBKDF2 iteration string remains in contracts,
      firmware, tests, or reports except as historical explanation.
- [x] Preserve existing PBKDF2 known-answer vector tests.
- [x] Preserve the PBKDF2 benchmark test for future recalibration.

The existing legacy value of 120,000 is not treated as a measured v2 value.

## P1C-023 — Strengthen PBKDF2/settings tests

- [ ] Add or update native tests proving provisioned records validate with the
      frozen PBKDF2 iteration count.
- [ ] Add or update native tests proving encoded provisioned records include the
      frozen nonzero iteration count.
- [ ] Add or update native tests proving decoded provisioned records preserve the
      frozen iteration count.
- [x] Preserve tests proving unprovisioned records contain no verifier metadata.
- [ ] Add or update tests proving reset-settings preserves the frozen iteration
      count.
- [ ] Run the focused settings schema checker:

```bash
python3 scripts/check-v2-settings-schema.py
```

Frozen-value assertions cannot be written truthfully until P1C-021 selects a
measured value.

## P1C-030 — Repair implementation report evidence

- [x] Update
      `docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md`.
- [x] Replace stale `final CI pending` language for tasks with recorded passing
      CI evidence.
- [ ] Record the exact final SHA for the closeout repair.
- [x] Record every changed file.
- [ ] Record final focused commands and pass/fail results.
- [ ] Record final permanent CI workflow names, run IDs, job IDs, and conclusions.
- [ ] Record PBKDF2 benchmark hardware evidence.
- [ ] If PBKDF2 hardware is explicitly deferred by the product owner, record the
      deferral and keep the task visibly open.
- [x] State that Phase 1 closeout is contract-layer closeout only.
- [x] State that production v1 architecture deletion remains Phase 2.
- [x] State that no unchecked task is being claimed complete.

The prior exact-SHA CI evidence for
`077ea244a99e44f62da87222597dc7cb91bdeebb` is recorded. The current software
repair still requires final-workflow evidence.

## P1C-031 — Synchronize Phase 1 TODOs and boundaries

- [ ] Update `docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md` with final
      closeout evidence.
- [x] Check only items that have implementation evidence in this checklist.
- [x] Keep P1R-062 unchecked unless ESP32-S3R8 timing evidence and a frozen value
      are committed.
- [ ] Update `docs/TODO_V2.md` to distinguish Phase 1 contract closeout from
      Phase 2 production architecture deletion.
- [x] Update the prior Phase 1 implementation report so it does not claim
      completion without PBKDF2 evidence.
- [x] Confirm docs do not claim production parser v2 integration unless active
      production execution uses `macro_compile_v2`.
- [x] Confirm docs do not claim production NVS v2 integration unless active
      production provisioning reads and writes the v2 settings record.
- [x] Confirm docs do not claim production setup v2 integration unless active
      production routes and React clients use the exact v2 setup contract.

Final synchronization remains open until the hardware-selected iteration value
can be represented consistently in every controlling document.

## P1C-040 — Run focused contract validation

- [ ] Run the limits drift checker:

```bash
python3 scripts/check-v2-limits.py
```

- [ ] Run the device-settings schema checker:

```bash
python3 scripts/check-v2-settings-schema.py
```

- [ ] Run the setup-route policy checker:

```bash
python3 scripts/check-v2-setup-route-policy.py
```

- [ ] Run the API route drift checker:

```bash
python3 scripts/check-v2-api-routes.py
```

- [ ] Run the full focused v2 contract gate:

```bash
./scripts/check-v2-contracts.sh
```

- [ ] If local web dependency installation is unavailable, run the native-only
      contract gate and record that this is not a substitute for CI:

```bash
./scripts/check-v2-contracts.sh --native-only
```

- [ ] Run or obtain final-SHA CI evidence for the host limits serializer test.
- [ ] Run or obtain final-SHA CI evidence for focused v2 Vitest suites.
- [ ] Run or obtain final-SHA CI evidence for native v2 CTest.

## P1C-041 — Run full validation gates

- [ ] Run or obtain CI evidence for `./scripts/check-all.sh` through the Quality
      workflow.
- [ ] Run or obtain CI evidence for Host Tests.
- [ ] Run or obtain CI evidence for Browser Tests.
- [ ] Run or obtain CI evidence for Device Test Build.
- [ ] Confirm all four permanent `master` workflows pass on the same exact final
      SHA.
- [ ] Confirm no failed, cancelled, skipped-required, pending, or running job is
      hidden behind an older status issue.
- [ ] Record workflow run IDs and job IDs in the implementation report.

## P1C-050 — Final source review before closeout

- [x] Review the diff for accidental Phase 2 deletions.
- [x] Review the diff for stale placeholder PBKDF2 strings.
- [x] Review the diff for v1-only limits response fields.
- [x] Review the diff for `ok/data` reintroduction in the limits response.
- [x] Review the diff for warning suppressions or weakened gates.
- [x] Review the diff for committed secrets or sensitive benchmark output.
- [x] Review the docs for overclaiming production v2 parity.
- [x] Confirm every checked task has evidence.
- [x] Confirm every unchecked task is called out as open or explicitly deferred.

The compare from the reviewed baseline to the software-repair SHA contains only
the intended contract, test, documentation, and evidence changes; no Phase 2
production deletion appears.

## Exit gate

This repair is complete only when all of the following are true:

- [x] Production `/api/v1/limits` emits the full 18-field v2 limits object.
- [x] Firmware serializer, TypeScript guards, API examples, and tests agree in
      source.
- [x] No retired v1-only limits fields appear in the v2 limits response.
- [x] No `ok/data` wrapper appears in the v2 limits response.
- [ ] PBKDF2 was benchmarked on ESP32-S3R8.
- [ ] The PBKDF2 iteration count is frozen as a concrete integer.
- [ ] Native tests prove provisioned settings encode, decode, validate, and reset
      while preserving the frozen iteration count.
- [x] Phase 1 evidence docs no longer contain stale pending-CI language for the
      previously validated baseline.
- [ ] Phase 1 and Phase 2 boundaries are synchronized across all controlling
      docs.
- [ ] Focused v2 contract gates pass on the final exact SHA.
- [ ] Quality, Host Tests, Browser Tests, and Device Test Build pass on one exact
      final `master` SHA.
- [x] No report claims production v2 migration or Phase 2 deletion that has not
      been implemented.

**Current result:** software limits repair implemented; Phase 1 closeout remains
open for physical PBKDF2 evidence, frozen-value implementation/tests, final
cross-document synchronization, and exact-SHA validation.
