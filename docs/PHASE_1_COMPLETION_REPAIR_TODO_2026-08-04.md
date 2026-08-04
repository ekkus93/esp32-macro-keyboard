# ESP32 Macro Keyboard v2 — Phase 1 Completion Repair TODO

**Document status:** Authoritative repair checklist for finishing Phase 1 before Phase 2  
**Created:** 2026-08-04  
**Target branch:** `master` only  
**Companion spec:** [`PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md`](PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md)  
**Reviewed baseline:** `26920fe1917895f0ebd0ab4287285a09dcedde3e`

## 0. Operating rules

- [ ] Work directly on `master`.
- [ ] Do not create a feature branch.
- [ ] Do not open a pull request.
- [ ] Do not add temporary workflow files.
- [ ] Do not start Phase 2 implementation until every task in this file is
      complete.
- [ ] Do not mark any task complete without implementation and reproducible
      evidence.
- [ ] Do not weaken `./scripts/check-all.sh`, `check-v2-contracts.sh`, or any
      existing CI gate.
- [ ] Add tests or deterministic checks in the same change as each behavior fix.
- [ ] Preserve all existing passing behavior unless this TODO explicitly replaces
      it with the v2 contract.

## 1. Baseline confirmation

### P1R-000 — Confirm clean master starting point

- [ ] Confirm local or connector-visible `master` starts from the current repair
      baseline or a descendant of it.
- [ ] Record the starting commit SHA in the implementation report.
- [ ] Confirm there are no open Phase 1 repair PRs.
- [ ] Confirm no temporary workflow files exist under `.github/workflows/`.
- [ ] Confirm existing CI state for Browser Tests, Host Tests, Device Test Build,
      and Quality on the starting SHA.

### P1R-001 — Create the repair implementation report

- [ ] Create `docs/implementation-v2/PHASE_1_COMPLETION_REPAIR_2026-08-04.md`.
- [ ] Record this repair TODO path and companion spec path.
- [ ] Record the starting SHA.
- [ ] Maintain a task-by-task evidence table while implementing.
- [ ] Record exact files changed for every task group.
- [ ] Record all commands run and their pass/fail results.
- [ ] Record any hardware evidence separately from host/CI evidence.
- [ ] Include an explicit statement that Phase 2 deletion work has not begun.

## 2. Phase-boundary documentation repair

### P1R-010 — Resolve the Phase 1 / Phase 2 boundary contradiction

- [ ] Update `docs/TODO_V2.md` so Phase 1 does not overclaim Phase 2 deletion
      work.
- [ ] Preserve the requirement that production v1 package/macro repository
      architecture must be removed before the product is v2-complete.
- [ ] Make clear that full deletion of firmware-owned package/macro repositories,
      package/macro CRUD routes, revision/ETag behavior, and plural execution
      resources remains Phase 2 unless the product owner explicitly changes the
      phase plan.
- [ ] Add a note that the Phase 1 repair closes v2 contract correctness, drift
      checks, traceability, and evidence only.
- [ ] Ensure the Phase 1 checkpoint and repair report agree on what is complete
      and what remains deferred.

### P1R-011 — Prevent future Phase 1 overclaims

- [ ] Add wording to the repair report that `Phase 1 complete` means the Phase 1
      contract layer is complete, not that production v1 architecture is deleted.
- [ ] Add a checklist item requiring Phase 2 entry review before any production
      v1 route deletion work starts.
- [ ] Confirm no documentation says production v1 package/macro routes already
      satisfy the v2 API contract.

## 3. Repository contract repair

### P1R-020 — Rename misleading v1 repository symbols

- [ ] Rename `RepositoryV1` to a v2-neutral type name.
- [ ] Rename `RepositoryPackageV1` to a v2-neutral type name.
- [ ] Rename `RepositoryMacroV1` to a v2-neutral type name.
- [ ] Rename `validateRepositoryV1` to a v2-neutral function name.
- [ ] Rename `serializeRepositoryV1` to a v2-neutral function name.
- [ ] Update all imports, tests, and documentation references.
- [ ] Do not leave permanent compatibility aliases with `V1` names.
- [ ] Run TypeScript typecheck and the focused repository tests.

### P1R-021 — Add checked-in repository fixture corpus

- [ ] Add `contracts/v2/repository/fixtures.json` or an equivalent reviewed
      fixture corpus.
- [ ] Include at least one valid canonical fixture.
- [ ] Include empty repository fixture.
- [ ] Include package name 64-byte boundary and 65-byte failure cases.
- [ ] Include macro name 64-byte boundary and 65-byte failure cases.
- [ ] Include macro source 4096-byte boundary and 4097-byte failure cases.
- [ ] Include key press `0`, `10000`, `-1`, and `10001` cases.
- [ ] Include inter-key `0`, `10000`, `-1`, and `10001` cases.
- [ ] Include malformed root, package, and macro objects.
- [ ] Include duplicate package ID case.
- [ ] Include duplicate macro ID across different packages.
- [ ] Include `activePackageId` rejection case.
- [ ] Include unknown root, package, and macro field cases.
- [ ] Include non-finite number cases.
- [ ] Include sparse array cases.
- [ ] Include prototype-bearing object cases where possible in tests.

### P1R-022 — Consume repository fixtures in tests

- [ ] Update `webapp/tests/v2-repository.test.ts` to iterate over the checked-in
      fixture corpus.
- [ ] Keep direct targeted tests where they add clearer regression coverage.
- [ ] Ensure every fixture has expected `ok` status and expected issue code/path
      where applicable.
- [ ] Verify canonical serialization still equals the checked-in compact JSON.
- [ ] Verify `createEmptyRepository()` still returns the exact empty object.

## 4. API contract repair

### P1R-030 — Fix sparse-array validation bug

- [ ] Replace `apiGuards.ts` dense-array logic that relies on
      `Array.prototype.every()` with explicit index ownership checks.
- [ ] Apply the same helper style used by the repository validator.
- [ ] Audit all API guards for arrays that could accept holes.
- [ ] Add rejection tests for sparse `blobList.blobs`.
- [ ] Add rejection tests for sparse diagnostics `invalidNames`.
- [ ] Add rejection tests for sparse diagnostics `temporaryFiles`.
- [ ] Add rejection tests for sparse diagnostics `subsystems`.
- [ ] Add rejection tests for any other API array field found during audit.

### P1R-031 — Add reviewed v2 API route manifest

- [ ] Add `contracts/v2/api/routes.json`.
- [ ] Include every intended v2 `/api/v1` route.
- [ ] For each route, define method and path.
- [ ] For each route, define authentication requirement.
- [ ] For each route, define request body shape or `none`.
- [ ] For each JSON route, define request content type.
- [ ] For each route, define response content type.
- [ ] For each route, define request body maximum, where applicable.
- [ ] For each route, define success status code.
- [ ] For each route, define primary error status codes.
- [ ] Include setup, session, status, limits, settings, password-change, restart,
      reset-settings, factory-reset, diagnostics, blob, and send routes.
- [ ] Do not include v1 package/macro CRUD routes in the v2 manifest.
- [ ] Do not include v1 plural execution routes in the v2 manifest.

### P1R-032 — Add route-manifest drift checks

- [ ] Add a TypeScript validator for `contracts/v2/api/routes.json`.
- [ ] Add a C header or constants file mirroring the reviewed route manifest.
- [ ] Add a Python drift checker proving the C mirror matches the JSON manifest.
- [ ] Add Vitest coverage for route reordering, unknown fields, changed method,
      changed path, changed status, changed content type, changed auth, and
      changed body limit.
- [ ] Register the route-manifest checker in `scripts/check-v2-contracts.sh`.
- [ ] Register the route-manifest checker in `scripts/check-scripts.sh` if that is
      the project pattern for drift checkers.

### P1R-033 — Strengthen C-side API contract evidence

- [ ] Add native C tests proving API enum and constant mirrors compile under
      warning-as-error settings.
- [ ] Add tests for setup route policy constants if not already included.
- [ ] Add tests for route manifest constants.
- [ ] Add tests for limit constants used in API responses.
- [ ] Document which API examples are TypeScript-only and why, if no C serializer
      exists yet.
- [ ] Do not claim C parser/serializer parity for API examples unless a C parser
      or serializer actually consumes them.

### P1R-034 — Keep unknown-field rejection complete

- [ ] Ensure every v2 API request guard rejects unknown fields.
- [ ] Ensure every v2 API response guard rejects unknown top-level fields.
- [ ] Add nested unknown-field tests for settings, status, diagnostics, blob, and
      send objects where nested objects are allowed.
- [ ] Ensure parser-coordinate fields in error envelopes are all-present or
      all-absent.
- [ ] Ensure sparse arrays are rejected after P1R-030.

## 5. Limits repair

### P1R-040 — Verify central limits remain synchronized

- [ ] Run `python3 scripts/check-v2-limits.py`.
- [ ] Confirm `contracts/v2/limits.json` remains the source of truth.
- [ ] Confirm `webapp/src/v2/limits.ts` matches the JSON contract.
- [ ] Confirm `firmware/components/app_contracts_v2/include/app_limits_v2.h`
      matches the JSON contract.
- [ ] Confirm temporary legacy aliases in `macro_limits.h` still match where they
      are intentionally retained before Phase 2.

### P1R-041 — Repair production `/api/v1/limits` shape or mark it legacy

- [ ] Inspect the active production `GET /api/v1/limits` implementation.
- [ ] If the route is claimed as v2, change its serializer to emit the exact v2
      limits object.
- [ ] Remove `ok/data` wrapping from any response claimed as v2.
- [ ] Remove v1-only keys such as `macrosPerPackage`, `packages`, and
      `importBytes` from any response claimed as v2.
- [ ] Add host tests for the production limits serializer.
- [ ] Add TypeScript contract tests proving the response matches
      `isLimitsResponse`.
- [ ] If the production route remains intentionally v1 until Phase 2, document
      that explicitly and do not mark the Phase 1 production-route exit gate
      closed.

### P1R-042 — Add complete boundary evidence for limits

- [ ] Add or verify boundary tests for package name byte limit.
- [ ] Add or verify boundary tests for macro name byte limit.
- [ ] Add or verify boundary tests for macro source byte limit.
- [ ] Add or verify boundary tests for compiled action limit or document why it
      is bounded by source size first.
- [ ] Add or verify boundary tests for key press time.
- [ ] Add or verify boundary tests for inter-key delay.
- [ ] Add or verify boundary tests for delay directive time.
- [ ] Add or verify boundary tests for estimated macro duration.
- [ ] Add or verify boundary tests for executor absolute deadline constants.
- [ ] Add or verify boundary tests for JSON request body size.
- [ ] Add or verify boundary tests for repository blob maximum size.
- [ ] Add or verify boundary tests for active session count.
- [ ] Add or verify boundary tests for session idle lifetime.
- [ ] Add or verify boundary tests for session absolute lifetime.
- [ ] Add or verify boundary tests for serial confirmation timeout.
- [ ] Add or verify boundary tests for administrator password minimum and maximum.

## 6. Macro-language repair

### P1R-050 — Correct action-limit test overclaim

- [ ] Update the misleading TypeScript test name that says a 4097-byte source
      proves the compiled-action limit.
- [ ] Split source-size, action-count, and duration-limit tests into separately
      named cases.
- [ ] If action count cannot exceed 4096 before source byte limit under schema v1,
      document that explicitly in the test and implementation report.
- [ ] Ensure no test label claims stronger coverage than the assertion proves.

### P1R-051 — Preserve cross-language macro conformance

- [ ] Run TypeScript macro conformance tests.
- [ ] Run native C macro conformance tests.
- [ ] Confirm both consume `contracts/v2/macro-conformance.json`.
- [ ] Confirm expected actions match for all valid corpus cases.
- [ ] Confirm error code, byte offset, line, column, and message class match for
      all invalid corpus cases.
- [ ] Confirm lowercase directive rejection remains covered.
- [ ] Confirm duplicate modifier rejection remains covered.
- [ ] Confirm standalone `{A}` and `{1}` named-directive rejection remains covered
      in dedicated tests or corpus cases.

### P1R-052 — Document production parser boundary

- [ ] Inspect production execution and validation paths.
- [ ] Record whether production still uses old `macro_compile` paths.
- [ ] Do not claim production execution uses `macro_compile_v2` unless that is
      implemented and tested.
- [ ] If production parser integration is intentionally Phase 2 or later, record
      that boundary in the repair report.

## 7. Device-settings repair

### P1R-060 — Verify settings schema drift checks

- [ ] Run `python3 scripts/check-v2-settings-schema.py`.
- [ ] Confirm `contracts/v2/device-settings.json` matches
      `device_settings_v2.h` offsets.
- [ ] Confirm field lengths match exactly.
- [ ] Confirm defaults match exactly.
- [ ] Confirm enum values match exactly.
- [ ] Confirm record length remains 344 bytes.

### P1R-061 — Strengthen device-settings tests

- [ ] Verify tests reject wrong length.
- [ ] Verify tests reject wrong magic.
- [ ] Verify tests reject wrong record version.
- [ ] Verify tests reject wrong record length field.
- [ ] Verify tests reject unsupported credential version or password algorithm
      version.
- [ ] Verify tests reject invalid enum values.
- [ ] Verify tests reject invalid boolean bytes.
- [ ] Verify tests reject nonzero reserved bytes.
- [ ] Verify tests reject invalid UUIDs.
- [ ] Verify tests reject invalid UTF-8.
- [ ] Verify tests reject dirty bytes after a string terminator.
- [ ] Verify tests reject invalid provisioned/unprovisioned credential invariants.
- [ ] Verify tests reject invalid station invariants.
- [ ] Verify reset-settings preserves credentials, AP credentials, provisioning
      state, credential version, and next blob counter.
- [ ] Verify reset-settings restores v2 UI defaults and clears station fields.

### P1R-062 — Measure and freeze PBKDF2 iteration count on ESP32-S3R8

- [ ] Add or identify a hardware measurement harness for PBKDF2-HMAC-SHA-256.
- [ ] Run the harness on the reference ESP32-S3R8 hardware.
- [ ] Select an iteration count that produces 250-500 ms derivation time on the
      reference hardware.
- [ ] Commit the frozen iteration count into the v2 settings/auth contract.
- [ ] Add tests proving the frozen value is encoded/decoded and not zero for
      provisioned records.
- [ ] Record board model, port, firmware build ID, host OS, measured timings,
      chosen value, and command log in the implementation report.
- [ ] Do not close Phase 1 repair until this hardware evidence is recorded, unless
      the product owner explicitly accepts a documented deferral.

### P1R-063 — Document production NVS boundary

- [ ] Inspect whether production provisioning currently uses the v2 settings
      record.
- [ ] If production still uses legacy `provisioning_settings_t`, document that
      production integration is not complete.
- [ ] Do not claim production NVS migration complete unless production actually
      reads/writes the v2 record and tests prove it.

## 8. Setup-route policy repair

### P1R-070 — Verify setup policy synchronization

- [ ] Run `python3 scripts/check-v2-setup-route-policy.py`.
- [ ] Confirm reviewed JSON policy contains exactly unprovisioned
      `GET /api/v1/setup` and `POST /api/v1/setup`.
- [ ] Confirm provisioned `GET /api/v1/setup` status is 404.
- [ ] Confirm provisioned `POST /api/v1/setup` status is 409.
- [ ] Confirm all other unprovisioned API routes remain unavailable by policy.
- [ ] Confirm TypeScript validator rejects changed route order.
- [ ] Confirm TypeScript validator rejects extra routes.
- [ ] Confirm TypeScript validator rejects authentication changes.
- [ ] Confirm TypeScript validator rejects content-type changes.
- [ ] Confirm TypeScript validator rejects status changes.
- [ ] Confirm C constants compile and native tests pass.

### P1R-071 — Document production setup boundary

- [ ] Inspect current production setup routes and React setup client paths.
- [ ] Record whether production still uses `/api/v1/setup-state`,
      `/api/v1/setup/credentials`, or `/api/v1/setup/restart`.
- [ ] If old paths remain, document them as legacy production behavior pending
      Phase 2 or an explicit production-contract repair.
- [ ] Do not claim production setup satisfies v2 unless the active route table and
      React client use the exact v2 setup contract.

## 9. Traceability repair

### P1R-080 — Retarget traceability generator to v2 docs

- [ ] Update `scripts/generate-spec-traceability.py` so it reads
      `docs/SPEC_V2.md`.
- [ ] Include `docs/UI_UX_SPEC_V2.md` if UI/UX normative statements are part of
      the generated traceability scope.
- [ ] Update generated document preamble so it says v2 traceability.
- [ ] Remove references that imply retired `docs/SPEC.md` is authoritative.
- [ ] Ensure the generator does not count its own comments/docstrings as coverage.

### P1R-081 — Regenerate and gate v2 traceability

- [ ] Regenerate `docs/SPEC_TEST_TRACEABILITY.md` or create a new clearly named
      v2 traceability document.
- [ ] Confirm generated totals are nonzero.
- [ ] Confirm unmapped v2 requirements are visible and not hidden.
- [ ] Add or update `scripts/check-docs.sh` so traceability freshness is checked.
- [ ] Run the docs gate.
- [ ] Record traceability totals in the repair implementation report.

## 10. Focused and full validation

### P1R-090 — Run focused Phase 1 repair gates

- [ ] Run `python3 scripts/check-v2-limits.py`.
- [ ] Run `python3 scripts/check-v2-settings-schema.py`.
- [ ] Run `python3 scripts/check-v2-setup-route-policy.py`.
- [ ] Run any new route-manifest drift checker.
- [ ] Run `./scripts/check-v2-contracts.sh`.
- [ ] Run focused repository Vitest files.
- [ ] Run focused API contract/request Vitest files.
- [ ] Run focused macro conformance Vitest files.
- [ ] Run native v2 CTest suite.

### P1R-091 — Run authoritative local gate

- [ ] Run `./scripts/check-all.sh` from a clean checkout.
- [ ] Record complete pass/fail summary.
- [ ] If it fails, fix the root cause without suppressing the gate.
- [ ] Rerun until it passes.

### P1R-092 — Verify GitHub Actions on master

- [ ] Push/commit all changes directly to `master` only.
- [ ] Verify Browser Tests pass on the final `master` SHA.
- [ ] Verify Host Tests pass on the final `master` SHA.
- [ ] Verify Device Test Build passes on the final `master` SHA.
- [ ] Verify Quality passes on the final `master` SHA.
- [ ] Record workflow names, run IDs, conclusions, and final SHA in the repair
      implementation report.

### P1R-093 — Final repository hygiene check

- [ ] Confirm no temporary workflow files were added.
- [ ] Confirm no feature branches or PRs were created for this repair.
- [ ] Confirm no generated files are stale.
- [ ] Confirm docs and code agree on Phase 1 status.
- [ ] Confirm Phase 2 work has not begun.
- [ ] Confirm every checked task in this file has evidence.

## 11. Final acceptance statement

Phase 1 repair may be declared complete only after the implementation report says
all of the following in plain language:

- [ ] The final `master` SHA is recorded.
- [ ] All P1R tasks are complete or explicitly left unchecked.
- [ ] No unchecked task is claimed complete.
- [ ] The v2 contract layer is complete and verified.
- [ ] Production v1 architecture deletion has not been claimed as Phase 1 unless
      the phase plan was explicitly changed.
- [ ] Phase 2 may begin only after product-owner review of the final Phase 1
      repair evidence.
