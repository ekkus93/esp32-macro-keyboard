# ESP32 Macro Keyboard — Code Review Fixes TODO

**Document status:** implementation TODO for issues found during the 2026-07-28
code review of `master`

**Source review target:** commit `9343141b63970a76f6c1b881051a324c3def2fba`

**Primary objective:** fix the concrete correctness, safety, specification,
dependency, and release-gate issues found in the review without broadening scope
into unrelated product work.

**Status note (added 2026-08-01):** This document is a historical snapshot,
not a live task list - the checkboxes below are left unchecked exactly as
originally written. Sections 1-7 (every concrete fix) were implemented the
next day, 2026-07-29, per
`docs/ESP32_MACRO_KEYBOARD_CODE_REVIEW_FIXES_PROGRESS_2026-07-29.md`, and
squash-merged into `master` as commit
`e8ee89cc9160155117d4b8e795c676ae475bed8c` ("Harden procedure, storage,
execution, and release gates (#18)"). Section 8 (doc sync) was addressed by
that same PROGRESS document and by this repository's later, more thorough
FIX1 documentation sweep (see
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
§22). Section 9 (final regression checklist) was satisfied - see the PROGRESS
document's "Validation status". A reader who opens only this file should not
conclude any of these issues are still open; see the per-section notes below
for exactly where each was fixed.

## 0. Rules for this fix pass

- Read `docs/TODO.md` and the relevant implementation files before changing code.
- Keep this pass focused on the issues in this document.
- Do not add silent fallback behavior.
- Do not relax warnings, linting, tests, static analysis, or CI policy to pass.
- Do not use `NOLINT`, `eslint-disable`, `@ts-ignore`, `@ts-nocheck`, ignored
  exit codes, broad `|| true`, or hidden stderr.
- Do not change the execution, procedure, or storage API contracts without also
  updating tests and documentation.
- Do not run `npm audit fix --force` as a shortcut. Upgrade intentionally.
- Preserve the Phase 17.6 boundary: procedure workflow may navigate to execution
  confirmation, but it must not auto-submit execution.
- Add regression tests before marking a bug fixed.
- The final gate for this TODO is a clean `./scripts/check-all.sh` from a clean
  checkout, plus any targeted audit or documentation checks added below.

## 1. Fix backend procedure progress authority

> **Fixed** 2026-07-29, commit `e8ee89c` - see PROGRESS doc's "Procedure
> progress authority".

### Problem

The frontend only exposes completion and skip controls for the current procedure
step, but the backend accepts any existing `stepId` in the procedure. A direct
API client can complete or skip a previous or future step out of order because
`load_progress_action_context()` checks only that the submitted step exists in
the procedure, and `apply_progress_action()` advances progress from the submitted
step's index.

This is a backend authority bug. The server must enforce the procedure progress
state; it cannot trust the UI.

### Files to inspect

- `firmware/components/web_server/web_api_procedures.c`
- `firmware/components/storage/storage_repository_progress.c`
- `tests/host/test_web_api_repository_handlers.c`
- `tests/host/test_storage_progress.c`
- `webapp/src/features/procedures/procedureState.ts`
- `webapp/src/features/procedures/ProcedureWorkflowPage.tsx`

### Tasks

- [ ] Add server-side current-step enforcement for progress actions.
  - [ ] In `load_progress_action_context()`, after loading `out_current`, reject
        the request unless `out_action->step_id` equals
        `out_current->progress.current_step_id`.
  - [ ] Return a deterministic error for non-current mutation.
        Prefer `APP_ERROR_CONFLICT` because the submitted action conflicts with
        server-owned progress state.
  - [ ] Ensure the rejection happens before `apply_progress_action()` mutates a
        replacement progress object.
  - [ ] Preserve current behavior for stale progress: stale progress actions must
        still return conflict and must not be reconciled silently.
  - [ ] Preserve current behavior for unknown step IDs: unknown steps should still
        return invalid argument, not conflict.

Suggested implementation shape:

```c
if (result == APP_ERROR_NONE &&
    !app_uuid_equal(&out_current->progress.current_step_id,
                    &out_action->step_id)) {
    result = APP_ERROR_CONFLICT;
}
```

- [ ] Add host API regression tests.
  - [ ] Seed a procedure with at least three steps.
  - [ ] Start progress at step one.
  - [ ] Attempt to complete step two directly.
  - [ ] Assert HTTP status is `409`.
  - [ ] Assert stored progress still points at step one.
  - [ ] Assert `completed_step_ids` remains empty.
  - [ ] Attempt to skip step two directly with `confirmed: true`.
  - [ ] Assert HTTP status is `409`.
  - [ ] Assert stored progress still points at step one.
  - [ ] Assert `skipped_step_ids` remains empty.

- [ ] Add storage-level or lower-level regression coverage if useful.
  - [ ] Keep storage repository behavior focused on data validity.
  - [ ] Keep action ordering policy in the API layer unless there is already a
        repository helper responsible for progress transitions.

- [ ] Add a test for the positive path after the fix.
  - [ ] Complete the actual current step.
  - [ ] Assert progress advances exactly one step.
  - [ ] Complete or skip the new current step.
  - [ ] Assert the second mutation succeeds only when targeting the new current
        step.

- [ ] Add a test for final-step behavior.
  - [ ] Complete the last step.
  - [ ] Assert progress does not wrap around or advance to an invalid ID.
  - [ ] Assert subsequent attempts to complete an already completed non-current
        step are rejected.

### Acceptance criteria

- [ ] A direct API request cannot complete a non-current procedure step.
- [ ] A direct API request cannot skip a non-current procedure step.
- [ ] A direct API request cannot use stale progress to mutate state.
- [ ] Current-step completion still works.
- [ ] Current-step skip with explicit confirmation still works.
- [ ] No frontend-only assumption is required for procedure ordering safety.
- [ ] `./scripts/check-all.sh` passes.

## 2. Fix `storage_atomic.c` descriptor leak on temp/backup collision paths

> **Fixed** 2026-07-29, commit `e8ee89c` - see PROGRESS doc's "Atomic-write
> descriptor cleanup".

### Problem

`create_temporary_file()` opens a temporary file and then checks whether the
backup path already exists. If the backup path exists, or if `stat(backup)` fails
with an error other than `ENOENT`, the code unlinks the temporary path but does
not close the open descriptor first.

This is a rare path, but it is a real resource leak under collision or fault
injection.

### Files to inspect

- `firmware/components/storage/storage_atomic.c`
- `firmware/components/storage/storage_fs_ops.h`
- `tests/host/test_storage_atomic.c`
- Any existing storage fault-injection helpers under `tests/host/`

### Tasks

- [ ] Fix the open-descriptor cleanup path.
  - [ ] In the branch where `stat_path(backup)` succeeds, close the temporary file
        descriptor before unlinking the temporary path.
  - [ ] In the branch where `stat_path(backup)` fails with an error other than
        `ENOENT`, close the descriptor before unlinking the temporary path.
  - [ ] Preserve the original error when close succeeds.
  - [ ] If close fails and no stronger cleanup error exists, report the close
        error instead of success.
  - [ ] Do not ignore close errors.

Suggested helper shape:

```c
static app_error_code_t close_descriptor(const storage_fs_ops_t *operations,
                                         int descriptor,
                                         app_error_code_t original) {
    if (operations->close_file(operations->context, descriptor) == 0) {
        return original;
    }
    const app_error_code_t close_error = map_error_number(errno);
    return original == APP_ERROR_NONE ? close_error : original;
}
```

Use the helper only if it preserves the existing error-priority policy. If the
current storage tests imply a different priority, follow the tests and document
the priority in the helper name or comments.

- [ ] Add fault-injection regression tests.
  - [ ] Force `open_file()` for the temp path to succeed.
  - [ ] Force `stat_path()` for the generated backup path to report that the
        backup exists.
  - [ ] Assert `close_file()` is called exactly once for the temporary descriptor.
  - [ ] Assert `unlink_path()` is called for the temporary path.
  - [ ] Assert no descriptor remains marked open in the fake filesystem.
  - [ ] Force `stat_path()` for the backup path to fail with an error other than
        `ENOENT`.
  - [ ] Assert `close_file()` is still called.
  - [ ] Assert the returned error is deterministic and not success.
  - [ ] Add a close-failure variant if the fake ops layer supports it.

- [ ] Confirm no regression in normal atomic writes.
  - [ ] New file write succeeds.
  - [ ] Existing file replacement succeeds.
  - [ ] Short write still fails.
  - [ ] Failed flush/close still fails.
  - [ ] Failed rename still preserves old or new committed state according to the
        existing atomic-write contract.

### Acceptance criteria

- [ ] Every descriptor opened in `create_temporary_file()` is closed on every exit
      path after successful open.
- [ ] Cleanup errors are not silently ignored.
- [ ] Fault-injection tests prove no descriptor leak on backup-collision paths.
- [ ] Existing atomic-write success and rollback tests still pass.
- [ ] `./scripts/check-all.sh` passes.

## 3. Resolve execution submission API contract mismatch

> **Fixed** 2026-07-29, commit `e8ee89c` - see PROGRESS doc's "Execution
> submission contract" and "Phase 17.7 execution confirmation and
> submission".

### Problem

`docs/TODO.md` specifies this shape for `POST /api/v1/executions`:

```json
{
  "setId": "uuid",
  "macroId": "uuid",
  "macroRevision": 3,
  "sourceContext": {
    "procedureId": "uuid",
    "stepId": "uuid"
  }
}
```

The implemented parser and frontend route currently use flat procedure fields,
with optional top-level `procedureId` and `stepId`.

This must be reconciled before Phase 17.7 because confirmation and submission
will depend on this contract.

### Files to inspect

- `docs/TODO.md`
- `docs/SPEC.md`
- `docs/API.md` or the current API reference file if present
- `firmware/components/web_server/web_api_json.c`
- `firmware/components/web_server/web_api_execution.c`
- `firmware/components/web_server/web_execution_submit.c`
- `firmware/components/web_server/include/web_execution_submit.h`
- `tests/host/test_web_api_json.c`
- `tests/host/test_web_api_execution.c`
- `webapp/src/api/client.ts`
- `webapp/src/types/models.ts`
- `webapp/src/routing.ts`

### Decision task

Choose one contract and make all code/docs/tests match it.

Preferred option for consistency with `docs/TODO.md`:

- [ ] Use nested `sourceContext` in JSON request bodies.
- [ ] Keep URL/query route fields independent of the submit body if needed.
- [ ] Treat missing `sourceContext` as allowed for standalone macro execution.
- [ ] Treat partial `sourceContext` as invalid.
- [ ] Treat extra fields inside `sourceContext` as invalid.
- [ ] Treat extra top-level `procedureId` or `stepId` fields as invalid.

Alternative option:

- [ ] Keep the flat contract only if `docs/SPEC.md`, `docs/TODO.md`, frontend
      types, backend parser tests, and API docs are all updated in the same
      commit.
- [ ] Clearly document why the flat contract is preferable on the constrained
      device.

### Backend tasks if nested `sourceContext` is selected

- [ ] Update the execution-submit parser.
  - [ ] Require exact top-level keys for standalone execution:
        `setId`, `macroId`, `macroRevision`.
  - [ ] Permit one additional top-level key, `sourceContext`, for procedure
        execution.
  - [ ] Parse `sourceContext.procedureId` into `procedure_id`.
  - [ ] Parse `sourceContext.stepId` into `step_id`.
  - [ ] Set `has_procedure_context` only when the whole object is present and
        valid.
  - [ ] Reject `sourceContext: null`.
  - [ ] Reject `sourceContext` with only one ID.
  - [ ] Reject extra fields at all levels.

- [ ] Keep persisted-source execution safety unchanged.
  - [ ] Continue loading macro source server-side.
  - [ ] Continue validating macro revision before compile.
  - [ ] Continue validating procedure step context before submission.
  - [ ] Continue returning `202` only after the executor owns a valid plan.

- [ ] Add backend parser tests.
  - [ ] Valid standalone request succeeds.
  - [ ] Valid nested procedure request succeeds.
  - [ ] Flat `procedureId` and `stepId` are rejected if nested contract is chosen.
  - [ ] Missing `procedureId` inside `sourceContext` is rejected.
  - [ ] Missing `stepId` inside `sourceContext` is rejected.
  - [ ] Unknown fields are rejected.
  - [ ] Malformed UUIDs are rejected.
  - [ ] Revision zero is rejected.

### Frontend tasks for Phase 17.7 readiness

- [ ] Add a typed execution-submit request model.
- [ ] Add runtime validation for the accepted execution response.
- [ ] Ensure confirmation page constructs the chosen request shape exactly.
- [ ] Ensure procedure context is included only when sending from a procedure
      macro step.
- [ ] Ensure standalone macro execution does not include an empty or partial
      `sourceContext`.

### Documentation tasks

- [ ] Update `docs/TODO.md` only if the selected contract differs from current
      text.
- [ ] Update API documentation with the exact JSON shape.
- [ ] Include at least one standalone macro execution example.
- [ ] Include at least one procedure-context execution example.
- [ ] State explicitly that clients never submit macro source for execution.

### Acceptance criteria

- [ ] Backend parser, execution submitter, frontend request builder, and docs all
      agree on one request shape.
- [ ] Invalid partial procedure context is rejected.
- [ ] Server-side persisted-source loading remains enforced.
- [ ] `202 Accepted` is returned only after the executor owns a valid compiled
      plan.
- [ ] `./scripts/check-all.sh` passes.

## 4. Triage and fix npm audit findings intentionally

> **Fixed** 2026-07-29, commit `e8ee89c` - see PROGRESS doc's "Dependency
> audit".

### Problem

The latest validation logs reported `18 vulnerabilities`, including `16 high`
and `2 critical`. The current package versions are pinned, which is good, but
release cannot ignore critical/high findings without an explicit triage record.

### Files to inspect

- `webapp/package.json`
- `webapp/package-lock.json`
- `.nvmrc`
- `scripts/check-webapp.sh`
- CI logs for the latest `npm ci`

### Tasks

- [ ] Generate a deterministic audit report.
  - [ ] Run `npm audit --json` in `webapp/`.
  - [ ] Save a redacted or summarized audit report under `docs/review-source/` or
        another committed docs path if it is useful for handoff.
  - [ ] Do not commit noisy machine-specific paths.

- [ ] Classify each finding.
  - [ ] Runtime dependency or dev/build-only dependency.
  - [ ] Direct dependency or transitive dependency.
  - [ ] Exploitable in the deployed ESP32 static webapp or only during local/CI
        build.
  - [ ] Patch, minor, or major upgrade required.
  - [ ] Whether a safe override/resolution is possible.

- [ ] Upgrade safely.
  - [ ] Prefer direct dependency upgrades that keep Vite, React, Tailwind,
        TypeScript, Vitest, ESLint, Stylelint, and Prettier compatible.
  - [ ] Use package-manager-supported overrides only when the dependency graph
        requires it and tests prove compatibility.
  - [ ] Do not use `npm audit fix --force` unless the resulting diff is reviewed
        manually and all breaking changes are handled deliberately.
  - [ ] Keep `package-lock.json` committed.

- [ ] Add a permanent audit policy.
  - [ ] Decide whether CI should fail on high/critical vulnerabilities.
  - [ ] If yes, add a script such as `npm run audit:ci` and call it from
        `scripts/check-webapp.sh` or a dedicated release gate.
  - [ ] If no, document the reason and required manual release review.
  - [ ] Avoid network-dependent audit checks in the normal offline firmware build
        path if that would make local embedded development unreliable.

- [ ] Add documentation.
  - [ ] Record the final audit count.
  - [ ] Record any accepted dev-only findings with rationale and expiration date.
  - [ ] Record exact upgraded packages.

### Acceptance criteria

- [ ] No untriaged critical or high npm audit finding remains.
- [ ] Any accepted finding has a documented scope and rationale.
- [ ] Frontend tests, coverage, lint, stylelint, format, and build pass.
- [ ] No remote asset dependency is introduced.
- [ ] `./scripts/check-all.sh` passes.

## 5. Make the permanent web quality gate match the validated feature gates

> **Fixed** 2026-07-29, commits `dbbaefd` and `e8ee89c` - see PROGRESS doc's
> "Frontend and CI quality gates".

### Problem

The Phase 17.6 one-shot validation ran frontend format checking and coverage, but
`check-webapp.sh` currently runs typecheck, lint, stylelint, tests, build, and
no-remote-assets verification without directly running `format:check` or
`test:coverage`.

`check-format.sh` may cover Prettier globally, but the permanent web gate should
be explicit and easy to audit.

### Files to inspect

- `scripts/check-webapp.sh`
- `scripts/check-format.sh`
- `webapp/package.json`
- `.github/workflows/quality.yml`
- `docs/TODO.md`

### Tasks

- [ ] Decide the permanent location for frontend format and coverage checks.
  - [ ] Option A: add `npm run format:check` and `npm run test:coverage` directly
        to `scripts/check-webapp.sh`.
  - [ ] Option B: keep format in `check-format.sh`, but add comments and docs that
        explain the split.
  - [ ] Prefer Option A unless runtime becomes excessive.

- [ ] Add coverage to the permanent gate.
  - [ ] Confirm `vitest` coverage thresholds are configured.
  - [ ] Ensure the coverage command fails if thresholds are not met.
  - [ ] Ensure the coverage command does not depend on a browser or network.

- [ ] Preserve no-warning policy.
  - [ ] Keep `eslint --max-warnings=0`.
  - [ ] Keep `stylelint --max-warnings=0`.
  - [ ] Ensure Prettier differences fail.

- [ ] Update docs if needed.
  - [ ] Document which local script developers should run before committing.
  - [ ] Keep `docs/TODO.md` quality-gate text accurate.

### Acceptance criteria

- [ ] The permanent local gate runs every web check relied on by the one-shot
      feature gates or clearly documents why a check is elsewhere.
- [ ] A frontend formatting difference fails locally.
- [ ] A coverage regression fails locally.
- [ ] `./scripts/check-all.sh` passes.

## 6. Harden release CI reproducibility

> **Fixed** 2026-07-29, commits `dbbaefd` and `e8ee89c` - see PROGRESS doc's
> "Frontend and CI quality gates" (`docs/CI_REPRODUCIBILITY.md`).

### Problem

The main `Quality` workflow uses `ubuntu-latest`, while the validated one-shot
feature workflow used `ubuntu-24.04`. The project progress file also marks
release budgets and immutable CI as not started. This is acceptable for ongoing
feature work, but not release-ready.

### Files to inspect

- `.github/workflows/quality.yml`
- `scripts/install-esp-idf.sh`
- `scripts/check-all.sh`
- `docs/TODO.md`
- Any release or progress document under `docs/`

### Tasks

- [ ] Pin the CI runner image.
  - [ ] Replace `ubuntu-latest` with a specific supported image, preferably
        `ubuntu-24.04` if it matches the feature validation runners.
  - [ ] Confirm all apt packages used by the workflow exist on that image.

- [ ] Document CI immutability expectations.
  - [ ] Record the selected runner image.
  - [ ] Record pinned ESP-IDF, Node, Python package, Go tool, and apt package
        assumptions.
  - [ ] Keep the documentation concise and actionable.

- [ ] Consider safe dependency caches.
  - [ ] Use caches only if keys include lockfiles and tool versions.
  - [ ] Do not cache mutable build output in a way that can hide failures.
  - [ ] Do not make CI success depend on a warm cache.

- [ ] Keep release budget work separate unless small.
  - [ ] Do not fake size-budget completion.
  - [ ] If adding budget checks now, record firmware slot size, webfs size, and
        headroom from the actual build.
  - [ ] Otherwise leave budget work explicitly open in release prep docs.

### Acceptance criteria

- [ ] Main CI uses a pinned runner image.
- [ ] CI still calls `./scripts/check-all.sh`.
- [ ] CI does not rely on hidden generated state.
- [ ] CI documentation matches implementation.
- [ ] `./scripts/check-all.sh` passes locally or in the pinned CI environment.

## 7. Align package schemas with byte-oriented string limits

> **Fixed** 2026-07-29, commit `e8ee89c` - see PROGRESS doc's "Package schema
> accuracy". (Import enforcement itself was explicitly deferred to Phase 18 -
> see PROGRESS doc's "Intentionally deferred scope" - and was later
> implemented; see FIX1 TODO §18.1-§18.6.)

### Problem

The JSON Schema package file uses `maxLength`, which counts Unicode characters,
while firmware and frontend validation enforce UTF-8 byte limits. Multi-byte
strings can pass schema validation while failing firmware import validation.

This becomes important for Phase 18 import/export and backup/restore work.

### Files to inspect

- `docs/schemas/macro-set-package.schema.json`
- Any other schemas under `docs/schemas/`
- `firmware/components/macro_model/include/macro_limits.h`
- `webapp/src/types/limits.ts`
- `webapp/src/api/guards.ts`
- Future Phase 18 import validators if already started

### Tasks

- [ ] Decide the schema strategy.
  - [ ] Option A: keep `maxLength` as a human-readable approximate limit and add
        explicit documentation that byte limits are enforced by import code.
  - [ ] Option B: add schema comments such as `$comment` documenting the UTF-8
        byte limit for every bounded string.
  - [ ] Option C: restrict importable strings to ASCII where the product spec
        allows it, so `maxLength` equals byte length.
  - [ ] Do not rely on JSON Schema alone for byte limits unless a custom validator
        is added.

- [ ] Add import-validator tests for multi-byte strings.
  - [ ] Create a package with a string that is under `maxLength` in characters but
        over the firmware byte limit.
  - [ ] Assert import validation rejects it before mutation.
  - [ ] Assert no partial set, macro, procedure, or progress data is written.

- [ ] Update schema documentation.
  - [ ] For each bounded string, document whether the limit is bytes, characters,
        or both.
  - [ ] Ensure docs do not imply schema-only validation is sufficient when it is
        not.

- [ ] Keep frontend and firmware constants synchronized.
  - [ ] Verify `webapp/src/types/limits.ts` matches firmware limits.
  - [ ] Add or update tests that compare frontend limits against API or generated
        values if available.

### Acceptance criteria

- [ ] Multi-byte string limits are enforced by executable import validation.
- [ ] Schema documentation does not misrepresent byte limits.
- [ ] Import validation rejects over-byte-limit strings before mutation.
- [ ] No partial import state is left behind after byte-limit rejection.
- [ ] `./scripts/check-all.sh` passes.

## 8. Synchronize docs with actual implemented and deferred scope

> **Addressed** 2026-07-29 by the PROGRESS doc itself and commit `e8ee89c`,
> and superseded by this repository's later, more thorough FIX1
> documentation sweep (`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
> §22), which has its own open items tracked there rather than here.

### Problem

Some implementation status is correctly tracked in progress documents, but
`docs/TODO.md` still contains broad tasks whose subparts are implemented only in
later progress slices or explicitly deferred. The new fixes above will touch API
contracts and quality gates, so the docs need to stay precise.

### Files to inspect

- `docs/TODO.md`
- `docs/SPEC.md`
- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md`
- Any API reference under `docs/`
- This file

### Tasks

- [ ] Update `docs/TODO.md` only for facts changed by this fix pass.
  - [ ] Do not mark product phases complete unless their completion gates are
        actually satisfied.
  - [ ] Mark the procedure-progress backend authority fix as done only after tests
        pass.
  - [ ] Mark atomic-write descriptor cleanup as done only after fault-injection
        coverage passes.
  - [ ] Keep Phase 18 import/export/backup/restore open unless implemented.
  - [ ] Keep hardware-in-the-loop validation open unless performed on hardware.

- [ ] Update API documentation.
  - [ ] Document the chosen execution submission request shape.
  - [ ] Document progress action conflict behavior for non-current step IDs.
  - [ ] Document that stale progress requires reload/reset and is never silently
        reconciled.

- [ ] Update review/progress docs.
  - [ ] Add a short summary of completed fixes.
  - [ ] Include commit evidence after implementation.
  - [ ] Do not reference generated files unless they are committed at the exact
        path named.

### Acceptance criteria

- [ ] Documentation matches implemented behavior.
- [ ] Documentation clearly separates completed fixes from future phases.
- [ ] No missing referenced handoff/review files are introduced.
- [ ] `./scripts/check-docs.sh` passes.
- [ ] `./scripts/check-all.sh` passes.

## 9. Add final regression checklist for this fix pass

> **Satisfied** 2026-07-29 - see PROGRESS doc's "Validation status" (Host
> Tests run `30455828432`, Browser Tests run `30455823494`, both passed).

### Required targeted checks

- [ ] Procedure API host tests pass.
- [ ] Storage atomic fault-injection tests pass.
- [ ] Execution JSON parser tests pass.
- [ ] Frontend execution request-builder tests pass if Phase 17.7 code is touched.
- [ ] Webapp typecheck passes.
- [ ] Webapp lint passes with zero warnings.
- [ ] Webapp stylelint passes with zero warnings.
- [ ] Webapp Prettier check passes.
- [ ] Webapp unit tests pass.
- [ ] Webapp coverage passes if coverage is added to the permanent gate.
- [ ] Firmware build passes.
- [ ] Host tests pass.
- [ ] Static analysis passes.
- [ ] Docs checks pass.

### Required full gate

Run from a clean checkout with the expected toolchain activated:

```bash
./scripts/check-all.sh
```

### Optional release checks

Run these if dependency or CI-release work is included in the same branch:

```bash
cd webapp
npm audit --json
npm run test:coverage
```

### Completion criteria

- [ ] Every issue from the review is either fixed, intentionally deferred with a
      clear phase owner, or documented with a risk rationale.
- [ ] Every code fix has at least one regression test.
- [ ] No new silent fallback path is introduced.
- [ ] No new source-level warning suppression is introduced.
- [ ] No unrelated product feature is bundled into this fix pass.
- [ ] The final commit message clearly describes the fix scope.

## 10. Suggested implementation order

1. Fix backend procedure current-step enforcement and tests.
2. Fix `storage_atomic.c` descriptor cleanup and tests.
3. Reconcile execution submission contract before Phase 17.7 implementation.
4. Update docs for the API-contract decision and fixed backend semantics.
5. Make permanent web gate coverage/format behavior explicit.
6. Pin the main CI runner image.
7. Triage npm audit findings and apply safe upgrades.
8. Add schema byte-limit documentation/tests as Phase 18 prep.
9. Run targeted tests.
10. Run the full `./scripts/check-all.sh` gate.

## 11. Out of scope for this TODO unless explicitly pulled in

- Full Phase 17.7 confirmation/submission UI implementation.
- Full Phase 18 import/export/backup/restore implementation.
- Full diagnostics aggregation.
- Hardware-in-the-loop validation.
- Release size budgets.
- Factory reset or physical provisioning changes.
- New macro/procedure editor features unrelated to the issues above.
