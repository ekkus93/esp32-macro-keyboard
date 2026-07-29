# ESP32 Macro Keyboard code-review fixes progress

**Source TODO:**
`docs/ESP32_MACRO_KEYBOARD_CODE_REVIEW_FIXES_TODO_2026-07-28.md`

**Implementation branch:** `agent/code-review-fixes-readable`

**Review:** draft PR #18

## Completed implementation

### Procedure progress authority

- The API rejects complete or skip actions unless `stepId` matches the
  server-owned current step.
- Existing but non-current steps return conflict before replacement progress is
  mutated.
- Unknown step IDs remain invalid input.
- Stale progress remains conflict-only and is never silently reconciled.
- Repository-backed API tests cover out-of-order complete and skip, successful
  advancement, final-step behavior, repeated non-current completion, and stale
  progress preservation.

Primary commits: `d977980`, `f35a6a5`.

### Atomic-write descriptor cleanup

- Temporary descriptors are closed before unlink on backup collision and backup
  stat-error paths.
- Original errors retain priority unless close is the only error.
- Cleanup errors remain visible.
- Fault-injection tests check call counts and `/proc/self/fd` balance for backup
  collision, backup stat failure, and close failure.

Primary commits: `94762ce`, `1660bff`.

### Execution submission contract

- The accepted JSON body contract is nested `sourceContext`.
- Standalone requests contain exactly `setId`, `macroId`, and `macroRevision`.
- Contextual requests add exactly a complete object containing `procedureId` and
  `stepId`.
- Null, partial, flat, malformed, duplicate, and extra context fields fail
  closed.
- The frontend has typed request and accepted-response models, an exact request
  function, and runtime response validation.
- API documentation contains standalone and procedure-context examples and
  states that clients never submit macro source.

Primary commits: `2efe24a`, `8bcc1fd`, `61cd08e`, `a0ef659`, `4e0b2af`.

### Frontend and CI quality gates

- The permanent web gate explicitly runs Prettier and Vitest coverage.
- Coverage has a committed 60% minimum for branches, functions, lines, and
  statements.
- Main Quality CI uses `ubuntu-24.04`, read-only repository permission,
  non-persisted checkout credentials, and lockfile-keyed npm caching.
- CI reproducibility assumptions are recorded in `docs/CI_REPRODUCIBILITY.md`.

Primary commits: `ef580ce`, `e2252aa`, `dbbaefd`, `b4453f6`.

### Dependency audit

- Reviewed frontend toolchain dependencies and the exact lockfile are committed.
- The reviewed graph has zero critical findings and five high package entries
  belonging to one ESLint development-tool advisory graph.
- Every affected installed node is dev-only and absent from shipped ESP32 static
  assets.
- CI accepts only the exact reviewed names and advisory source through
  2026-09-30. Critical, runtime, changed-name, changed-source, lower-severity,
  or expired findings fail closed.
- The policy has direct regression tests.

Primary commits: `d915796`, `1d7f14e`, `0abbaa3`, `f85f96c`, `dd14028`,
`b1a2b92`, `9d13b7c`.

### Package schema accuracy

- The package schema now states that JSON Schema `maxLength` counts Unicode code
  points rather than UTF-8 bytes.
- Every bounded string documents the matching firmware byte limit.
- Schema-only validation is explicitly insufficient for import.

Primary commit: `c4440a2`.

## Intentionally deferred scope

### Phase 17.7 confirmation UI

The typed execution request, serializer, and response guard are ready. The
actual confirmation screen is not implemented in this fix pass because the
source TODO explicitly excludes full Phase 17.7 UI implementation. That phase
must construct the same nested request and omit `sourceContext` for standalone
execution.

### Phase 18 executable import validation

The schema documentation is corrected, but no claim is made that package import
is implemented. Phase 18 must reject strings that fit schema code-point limits
but exceed firmware UTF-8 byte limits before any persistent mutation, and must
prove no partial set, macro, procedure, or progress state remains.

### Release budgets and hardware validation

Firmware-slot, webfs, RAM, heap, task-stack, and hardware-in-the-loop gates
remain open. This fix pass does not manufacture completion evidence for them.

## Validation status

Targeted Host Tests passed for the procedure authority and atomic-write fixes.
The final branch gate is the read-only Quality workflow running
`./scripts/check-all.sh` from the committed dependency graph. This section must
be updated with the final workflow conclusions before the TODO is closed.
