# ESP32 Macro Keyboard v2 — Phase 1 Contract Closeout and Production Limits Repair TODO

**Document status:** Authoritative repair checklist  
**Date:** 2026-08-04  
**Target branch:** `master` only  
**Baseline SHA at review:** `077ea244a99e44f62da87222597dc7cb91bdeebb`  
**Companion spec:**
`docs/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_SPEC_2026-08-04.md`

## 0. Rules for this repair

- [ ] Work directly on `master` unless the product owner explicitly changes the
      workflow.
- [ ] Do not create a feature branch or pull request unless asked.
- [ ] Do not weaken, skip, or suppress any existing gate.
- [ ] Do not use `|| true`, ignored exit codes, hidden diagnostics, or warning
      suppression to make CI pass.
- [ ] Do not begin Phase 2 deletion work in this repair.
- [ ] Do not claim production API, parser, NVS, setup, or route-table v2 parity
      unless the active production implementation and tests prove it.
- [ ] Do not check any item without implementation evidence.
- [ ] Keep all evidence free of passwords, passphrases, setup codes, session
      tokens, private repository contents, and real macro source.

## P1C-000 — Confirm baseline and scope

- [ ] Confirm current `master` SHA before starting implementation.
- [ ] Record whether `master` still includes
      `077ea244a99e44f62da87222597dc7cb91bdeebb` in history.
- [ ] Read the companion spec for this repair.
- [ ] Read `docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md`.
- [ ] Read `docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md`.
- [ ] Confirm this repair is limited to Phase 1 contract closeout and the
      production `/api/v1/limits` contract repair.
- [ ] Confirm Phase 2 deletion of package/macro repositories and CRUD routes is
      not part of this repair.

## P1C-010 — Inspect current limits contract drift

- [ ] Inspect `contracts/v2/limits.json`.
- [ ] Inspect `firmware/components/app_contracts_v2/include/app_limits_v2.h`.
- [ ] Inspect `webapp/src/v2/limits.ts`.
- [ ] Inspect `scripts/check-v2-limits.py`.
- [ ] Inspect `firmware/components/web_server/web_server_adapter_json.c`.
- [ ] Inspect `contracts/v2/api/examples.json`.
- [ ] Inspect `webapp/src/v2/apiGuards.ts`.
- [ ] Inspect host tests for `web_adapter_build_limits_json()`.
- [ ] Confirm whether the active production serializer currently omits any v2
      central limits.
- [ ] Confirm whether TypeScript examples and guards currently omit any v2
      central limits.
- [ ] Record the exact omitted keys, if any, before modifying code.

## P1C-011 — Repair production `/api/v1/limits` serializer

- [ ] Make `web_adapter_build_limits_json()` emit a top-level JSON object with
      every key from `contracts/v2/limits.json`.
- [ ] Include `activeSessionsMax`.
- [ ] Include `sessionIdleLifetimeSeconds`.
- [ ] Include `sessionAbsoluteLifetimeSeconds`.
- [ ] Include `serialConfirmationTimeoutSeconds`.
- [ ] Preserve existing v2 fields and values.
- [ ] Keep all values sourced from `APP_V2_*` firmware constants.
- [ ] Ensure the response has no `ok` wrapper.
- [ ] Ensure the response has no `data` wrapper.
- [ ] Ensure the response has no retired v1-only fields such as
      `macrosPerPackage`, `packages`, or `importBytes`.
- [ ] Ensure truncation or formatting failure still returns an error and clears
      the output buffer.

## P1C-012 — Repair limits examples and TypeScript guards

- [ ] Update `contracts/v2/api/examples.json` so `limits` contains every key from
      `contracts/v2/limits.json`.
- [ ] Update `webapp/src/v2/apiGuards.ts` so `isLimitsResponse()` requires every
      v2 limits field.
- [ ] Ensure `isLimitsResponse()` rejects missing session-limit fields.
- [ ] Ensure `isLimitsResponse()` rejects missing serial-confirmation timeout.
- [ ] Ensure `isLimitsResponse()` rejects extra fields.
- [ ] Ensure `isLimitsResponse()` rejects changed central limit values.
- [ ] Ensure public TypeScript exports still compile through
      `webapp/src/v2/apiContracts.ts`.

## P1C-013 — Strengthen limits tests

- [ ] Update the host serializer test to assert the full 18-field serialized
      limits response.
- [ ] Add or update a host assertion that no `ok` wrapper appears.
- [ ] Add or update a host assertion that no `data` wrapper appears.
- [ ] Add or update host assertions that retired v1-only fields do not appear.
- [ ] Add or update TypeScript tests proving all canonical limit values are
      accepted.
- [ ] Add or update TypeScript tests proving any omitted field is rejected.
- [ ] Add or update TypeScript tests proving changed values are rejected.
- [ ] Run the focused limits checker:

```bash
python3 scripts/check-v2-limits.py
```

## P1C-020 — Confirm PBKDF2 benchmark harness

- [ ] Inspect `firmware/test_app` for the PBKDF2 benchmark test.
- [ ] Confirm the benchmark uses PBKDF2-HMAC-SHA-256.
- [ ] Confirm the benchmark uses the intended salt length and verifier length.
- [ ] Confirm the benchmark uses the same implementation as the settings/auth
      path.
- [ ] Confirm the benchmark prints candidate iteration count, sample count,
      median, p90, and worst-case timing, or add those outputs.
- [ ] Confirm the benchmark covers enough candidates to select a 250-500 ms
      derivation target on ESP32-S3R8.
- [ ] Confirm the device-test firmware still builds after any harness changes.

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
- [ ] Preserve existing PBKDF2 known-answer vector tests.
- [ ] Preserve the PBKDF2 benchmark test for future recalibration.

## P1C-023 — Strengthen PBKDF2/settings tests

- [ ] Add or update native tests proving provisioned records validate with the
      frozen PBKDF2 iteration count.
- [ ] Add or update native tests proving encoded provisioned records include the
      frozen nonzero iteration count.
- [ ] Add or update native tests proving decoded provisioned records preserve the
      frozen iteration count.
- [ ] Add or update tests proving unprovisioned records still contain no verifier
      metadata.
- [ ] Add or update tests proving reset-settings preserves the frozen iteration
      count.
- [ ] Run the focused settings schema checker:

```bash
python3 scripts/check-v2-settings-schema.py
```

## P1C-030 — Repair implementation report evidence

- [ ] Update
      `docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md`.
- [ ] Replace stale `final CI pending` language for tasks with recorded passing
      CI evidence.
- [ ] Record the exact SHA for the closeout repair.
- [ ] Record every changed file.
- [ ] Record focused commands and pass/fail results.
- [ ] Record permanent CI workflow names, run IDs, job IDs, and conclusions.
- [ ] Record PBKDF2 benchmark hardware evidence.
- [ ] If PBKDF2 hardware is explicitly deferred by the product owner, record the
      deferral and keep the task visibly open.
- [ ] State that Phase 1 closeout is contract-layer closeout only.
- [ ] State that production v1 architecture deletion remains Phase 2.
- [ ] State that no unchecked task is being claimed complete.

## P1C-031 — Synchronize Phase 1 TODOs and boundaries

- [ ] Update `docs/PHASE_1_COMPLETION_REPAIR_TODO_2026-08-04.md`.
- [ ] Check only items that have implementation evidence.
- [ ] Keep P1R-062 unchecked unless ESP32-S3R8 timing evidence and frozen value
      are committed.
- [ ] Update `docs/TODO_V2.md` to distinguish Phase 1 contract closeout from
      Phase 2 production architecture deletion.
- [ ] Update any prior Phase 1 checkpoint or handoff document that still says
      Phase 1 is complete without the PBKDF2 evidence.
- [ ] Confirm docs do not claim production parser v2 integration unless active
      production execution uses `macro_compile_v2`.
- [ ] Confirm docs do not claim production NVS v2 integration unless active
      production provisioning reads and writes the v2 settings record.
- [ ] Confirm docs do not claim production setup v2 integration unless active
      production routes and React clients use the exact v2 setup contract.

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

- [ ] Run or obtain CI evidence for the host limits serializer test.
- [ ] Run or obtain CI evidence for focused v2 Vitest suites.
- [ ] Run or obtain CI evidence for native v2 CTest.

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

- [ ] Review the diff for accidental Phase 2 deletions.
- [ ] Review the diff for stale placeholder PBKDF2 strings.
- [ ] Review the diff for v1-only limits response fields.
- [ ] Review the diff for `ok/data` reintroduction in the limits response.
- [ ] Review the diff for warning suppressions or weakened gates.
- [ ] Review the diff for committed secrets or sensitive benchmark output.
- [ ] Review the docs for overclaiming production v2 parity.
- [ ] Confirm every checked task has evidence.
- [ ] Confirm every unchecked task is called out as open or explicitly deferred.

## Exit gate

This repair is complete only when all of the following are true:

- [ ] Production `/api/v1/limits` emits the full 18-field v2 limits object.
- [ ] Firmware serializer, TypeScript guards, API examples, and tests agree.
- [ ] No retired v1-only limits fields appear in the v2 limits response.
- [ ] No `ok/data` wrapper appears in the v2 limits response.
- [ ] PBKDF2 was benchmarked on ESP32-S3R8.
- [ ] The PBKDF2 iteration count is frozen as a concrete integer.
- [ ] Native tests prove provisioned settings encode, decode, validate, and reset
      while preserving the frozen iteration count.
- [ ] Phase 1 evidence docs are current and no longer contain stale pending-CI
      language.
- [ ] Phase 1 and Phase 2 boundaries are synchronized across docs.
- [ ] Focused v2 contract gates pass.
- [ ] Quality, Host Tests, Browser Tests, and Device Test Build pass on one exact
      final `master` SHA.
- [ ] No report claims production v2 migration or Phase 2 deletion that has not
      been implemented.
