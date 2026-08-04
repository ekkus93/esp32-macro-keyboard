# ESP32 Macro Keyboard v2 — Phase 1 Completion Repair Spec

**Document status:** Implementation control spec for repairing Phase 1 before Phase 2  
**Created:** 2026-08-04  
**Target branch:** `master` only  
**Reviewed baseline:** `26920fe1917895f0ebd0ab4287285a09dcedde3e`  
**Primary sources:** `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, `docs/TODO_V2.md`, and code review of `master`

## 1. Purpose

This file defines the repair scope required before the project may move from
Phase 1 to Phase 2.

The current Phase 1 work added useful v2 contract infrastructure, but it must not
be treated as complete until the defects found during review are fixed, tested,
and recorded. The purpose of this repair is to make Phase 1 truthful, internally
consistent, and enforceable.

Phase 2 must not begin until this repair spec and its companion TODO are fully
closed on `master`.

## 2. Non-negotiable workflow constraints

- Work directly on `master`.
- Do not create a feature branch.
- Do not open a pull request.
- Do not add temporary workflow files.
- Do not use CI visibility workarounds that change the repository process.
- Do not claim Phase 1 completion from local-only evidence.
- Do not start Phase 2 package/macro repository deletion work until the Phase 1
  repair TODO is complete.
- Do not weaken any existing gate to make the repair pass.
- Do not use `|| true`, ignored exit codes, broad analyzer suppressions, or
  warning-silencing comments to hide failures.
- Every fix must include tests or deterministic checks in the same commit series.

## 3. Current Phase 1 state

Phase 1 currently contains a useful contract layer:

- strict React repository validator and canonical repository fixture;
- v2 limits JSON with TypeScript and C mirrors;
- TypeScript API request and response models/guards;
- C parser-neutral API model header;
- shared macro conformance corpus consumed by TypeScript and native C tests;
- fixed-length v2 device-settings codec and native tests;
- setup-route policy JSON, TypeScript validator, C constants, and drift checker;
- focused `scripts/check-v2-contracts.sh` gate;
- CI evidence on the reviewed repair head and final master validation SHA.

However, the code review found that Phase 1 is not finished. The existing
checkpoint correctly avoided claiming completion. This repair converts that
status into an actionable closure plan.

## 4. Phase-boundary problem that must be fixed

`docs/TODO_V2.md` currently contains a Phase 1 exit gate requiring that no
production route or repository serializer still depends on a v1 shape. At the
same time, Phase 2 is explicitly defined as the phase that removes the retired
firmware-owned package/macro repository architecture.

That creates a boundary conflict. The repository still contains active production
v1 package, macro, selection, validation, revision, and plural-execution routes.
Deleting all of that is Phase 2-scale work. The project must not pretend this is
already done in Phase 1.

The repair must resolve the boundary truthfully by doing both of the following:

1. Close all Phase 1 contract defects listed in this spec.
2. Update Phase 1 documentation so the exit gate cannot overclaim Phase 2
   deletion work.

The allowed wording is not "Phase 1 complete except for Phase 2." The allowed
state is:

> Phase 1 contract infrastructure is complete and verified. Production v1
> package/macro repository deletion remains blocked behind the Phase 2 entry
> gate and is not claimed by Phase 1.

If the product owner later decides that production v1 route deletion must become
part of Phase 1 instead of Phase 2, that decision must be made explicitly by
updating the phase plan before implementation starts. It must not happen through
silent scope creep.

## 5. Repair areas

### 5.1 Repository contract repair

The repository contract is functionally close, but it still carries confusing
`V1` names in the v2 implementation. This creates a future maintenance hazard:
v2 code, tests, and docs should not call the v2 repository schema `V1`.

Required repairs:

- Rename exported TypeScript repository contract names from `RepositoryV1`,
  `RepositoryPackageV1`, `RepositoryMacroV1`, `validateRepositoryV1`, and
  `serializeRepositoryV1` to v2-neutral names.
- Keep backward aliases only if an in-repository caller still needs them during
  the same commit; remove aliases before the repair is complete.
- Add a checked-in repository fixture corpus for valid, boundary, malformed,
  duplicate-ID, prototype-bearing, sparse-array, non-finite-number, unknown-root,
  unknown-package, unknown-macro, and `activePackageId` cases.
- Keep the canonical compact JSON fixture.
- Ensure repository fixtures are consumed by Vitest, not merely documented.
- Preserve exact-field, dense-array, plain-object, UTF-8 byte, UUID, uniqueness,
  timing, macro-source, and canonical serialization behavior.

### 5.2 API contract repair

The TypeScript API guards are useful, but the API contract is not yet complete
enough to close Phase 1.

Required repairs:

- Add a reviewed route manifest for every intended `/api/v1` v2 route.
- For every route, define method, path, authentication class, request content
  type, response content type, request body shape or `none`, maximum body size,
  success status, and primary error statuses.
- Extend deterministic checks so the route manifest cannot drift from TypeScript
  constants and C constants.
- Add C-side contract tests for API route metadata and examples where practical.
  A parser-neutral C header is acceptable only if tests prove it still matches
  reviewed JSON contracts.
- Fix the TypeScript sparse-array validation bug in `apiGuards.ts` by replacing
  `Array.prototype.every()`-based density checks with explicit index ownership
  checks.
- Add tests that sparse arrays are rejected for every API response/request field
  where an array is allowed.
- Keep unknown-field rejection for every JSON request and every response example.
- Ensure the standard v2 error envelope is exactly `{ "error": { ... } }` in all
  v2 contract tests.

Production may still contain v1 routes until Phase 2, but Phase 1 documentation
must not claim those routes are v2-compliant unless tests prove they are.

### 5.3 Centralized limits repair

The shared limits file and mirrors are good, but production `/api/v1/limits` is
still old-shaped and still exposes v1 package/import concepts.

Required repairs:

- Ensure the reviewed v2 limits JSON remains the single source of truth.
- Ensure TypeScript and C mirrors are generated or drift-checked from that JSON.
- Replace production `GET /api/v1/limits` serialization with the exact v2 limits
  object if the route remains active before Phase 2.
- Remove old `ok/data` wrapping from limits responses that are claimed as v2.
- Remove v1-only keys such as `macrosPerPackage`, `packages`, and `importBytes`
  from any limits response claimed as v2.
- Add host tests for the production limits serializer and TypeScript tests for
  the API guard.
- Add boundary tests for every numeric and byte-count limit listed in Phase 1, or
  explicitly record why a limit is contract-only and not runtime-testable yet.

### 5.4 Macro-language conformance repair

The shared macro corpus is strong, but one test overclaims action-limit coverage,
and production still uses the old parser path for old execution flows.

Required repairs:

- Rename or rewrite the TypeScript test that claims a 4097-byte source proves the
  compiled-action limit. That case proves the source-size limit first.
- Add explicit documentation and test names distinguishing source-size limit,
  action-count limit, and estimated-duration limit.
- If an independent action-count limit cannot be exceeded before source-size
  limit under schema version 1, document that interaction in the tests and in the
  implementation report.
- Keep the shared JSON corpus consumed by both C and TypeScript.
- Preserve exact error code, byte offset, line, column, and message-class checks.
- Keep lowercase directive and standalone `{A}`/`{1}` rejection coverage.
- Do not switch production execution to `macro_compile_v2` as a hidden Phase 1
  change unless the TODO explicitly adds tests and evidence for that integration.
  Full production send integration belongs to the production-contract phase that
  follows Phase 1 repair.

### 5.5 Device-settings contract repair

The v2 device-settings codec is well designed, but Phase 1 cannot be closed
without making the remaining evidence and integration boundary explicit.

Required repairs:

- Keep the fixed-length little-endian record contract and explicit offsets.
- Keep wrong-length, wrong-version, wrong-magic, invalid-enum, invalid-boolean,
  invalid-reserved-byte, invalid-UUID, invalid-UTF-8, string-tail, credential,
  station, and reset-settings tests.
- Add or verify drift checks between `contracts/v2/device-settings.json` and the
  C header offsets/lengths/defaults.
- Explicitly record that PBKDF2 iteration measurement requires ESP32-S3R8
  hardware and cannot be replaced by host tests.
- Add a hardware evidence task for measuring and freezing the PBKDF2 iteration
  count before Phase 1 is accepted as complete.
- Keep reset-settings semantics: preserve provisioning state, AP credentials,
  administrator verifier metadata, credential version, and next blob counter;
  clear station configuration and UI preferences; restore v2 UI defaults.
- Do not claim production NVS migration is complete unless production code is
  actually using the v2 record and tests prove it.

### 5.6 Setup-route policy repair

The setup-route policy contract exists and is good, but production setup routes
still use old paths and shapes.

Required repairs:

- Keep reviewed setup policy JSON with exactly unprovisioned `GET /api/v1/setup`
  and `POST /api/v1/setup`.
- Keep C constants and TypeScript validator synchronized with the reviewed JSON.
- Add tests proving route reordering, extra route entries, authentication changes,
  status changes, body-limit changes, and content-type changes fail.
- If production setup remains old-shaped before Phase 2, document it as legacy
  production behavior and do not claim it satisfies the v2 setup contract.
- If production setup is changed during Phase 1 repair, it must be changed to the
  exact v2 contract with host tests, browser/client tests, and evidence.

### 5.7 Traceability repair

The current traceability generator is pointed at retired `docs/SPEC.md` and the
current traceability document reports zero normative statements. That makes it
useless for v2 acceptance.

Required repairs:

- Update the generator to read `docs/SPEC_V2.md` and, where applicable,
  `docs/UI_UX_SPEC_V2.md`.
- Rename generated document text so it clearly says it is v2 traceability.
- Ensure generated totals are nonzero for v2 normative statements.
- Ensure `scripts/check-docs.sh` or another authoritative gate verifies the
  generated traceability document is current.
- Do not let the generator cite itself as coverage.
- Keep the distinction between referenced, gate-enforced, and unmapped.
- Record any unmapped Phase 1 contract requirements as TODO items, not as
  completed coverage.

### 5.8 Evidence and CI repair

Required repairs:

- Add a new Phase 1 repair implementation report under `docs/implementation-v2/`.
- The report must include exact commit SHA, files changed, tests run, CI run IDs,
  remaining limitations, and an explicit statement that no Phase 2 deletion work
  is being claimed.
- Run the focused contract gate locally or in CI.
- Run the full authoritative gate.
- Verify all required GitHub Actions on `master` after the final commit.
- Record hardware evidence separately from host evidence.
- If hardware measurement cannot be completed in the same work session, the
  repair must remain incomplete.

## 6. Required final Phase 1 repair exit state

The repair is complete only when all of the following are true:

1. `master` contains all repair commits.
2. No unmerged branch or PR is involved.
3. Repository contract names no longer mislabel the v2 schema as v1.
4. Repository fixture corpus is checked in and consumed by tests.
5. API sparse arrays are rejected everywhere arrays are allowed.
6. API route metadata is represented in reviewed JSON and checked against C and
   TypeScript mirrors.
7. Production `/api/v1/limits`, if active and claimed as v2, emits the exact v2
   object.
8. Macro source-size, action-count, and duration-limit tests are named and scoped
   accurately.
9. Device-settings schema drift checks and hardware PBKDF2 evidence are recorded.
10. Setup-route policy remains synchronized across JSON, TypeScript, and C.
11. v2 traceability is generated from v2 docs and reports nonzero statements.
12. Phase 1 documentation no longer overclaims Phase 2 deletion work.
13. `./scripts/check-v2-contracts.sh` passes.
14. `./scripts/check-all.sh` passes.
15. Browser Tests, Host Tests, Device Test Build, and Quality pass on the final
    `master` SHA.

## 7. Out of scope until Phase 2

Unless the phase plan is explicitly changed, this repair does not perform the
full Phase 2 deletion of:

- firmware-owned package repository state;
- firmware-owned macro repository state;
- package/macro CRUD routes;
- repository import/export/restore/replace routes;
- optimistic concurrency and ETag/revision architecture;
- plural execution resources;
- production React package/macro CRUD API usage.

Those remain Phase 2 work. The Phase 1 repair must make that boundary explicit
and impossible to misread.

## 8. Hand-off note for Claude Code

Claude Code should start from `master`, read this spec and the companion TODO,
and implement the tasks directly on `master`. It should not create a branch or
PR. It should keep commits small enough to review, but it should not advance to
Phase 2 until every repair item is checked off with evidence.
