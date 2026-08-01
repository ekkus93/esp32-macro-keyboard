# ESP32 Macro Keyboard code-review fixes progress

**Source TODO:**
`docs/ESP32_MACRO_KEYBOARD_CODE_REVIEW_FIXES_TODO_2026-07-28.md`

**Implementation branch:** `agent/code-review-fixes-readable`

**Review:** draft PR #18

**Commit-history note (added 2026-08-01):** PR #18 was squash-merged into
`master` as commit `e8ee89cc9160155117d4b8e795c676ae475bed8c` ("Harden
procedure, storage, execution, and release gates (#18)", 2026-07-29). The
per-section "Primary commit(s)" hashes below were the feature branch's
individual commits at review time; none of them (except `dbbaefd`, `master`'s
tip immediately before the squash) are reachable in this repository anymore -
squash-merging discards the source branch's intermediate commits once the
branch itself is deleted. They are left as originally written for the
historical record, but a reader trying to resolve one today should use
`e8ee89c` instead - `git show --stat e8ee89c` covers every file this document
describes as fixed.

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

### Phase 17.7 execution confirmation and submission

- Standalone macro Send and procedure Send/Resend open a strict preview route;
  navigation never starts an execution.
- The page loads and revalidates persisted set-local or global macro data,
  server-owned metrics, exact procedure context, active settings, USB state,
  and executor state.
- Explicit Send fails closed on active-set, revision, source, validation,
  procedure-step, USB, or executor drift.
- Physical confirmation is shown as an explicit device-button wait, with a
  bounded 25-second request timeout around the firmware's 20-second window.
- Submission uses exact nested `sourceContext`, retains the accepted execution
  ID, and refuses to display or cancel a different current execution.
- Route, timeout, preview, global fallback, stale-data, submission, and identity
  regression tests are committed.

Final implementation commit before documentation sync: `9fb7130` (see the
commit-history note above - not reachable today; squashed into `e8ee89c`).

### Phase 17.9 management surfaces

- Set create, edit, duplicate, exact-order reorder, and guarded delete use
  current revisions and strict response validation.
- Settings, storage health, quarantine evidence, restart, settings reset, and
  factory reset are live and fail closed.
- Import, transactional replace, export, backup, and restore show disabled
  Phase 18 boundaries with explicit reasons; no unsupported request is sent.
- Every enabled management control performs a real request or navigation.

### Phase 17.10 accessibility and browser coverage

- Accessible dialogs trap focus, close with Escape, and restore focus.
- Reordering has Move first, up, down, and last controls.
- Offline and reconnect states are announced and reconnect reloads live data.
- A dedicated read-only Chrome workflow checks keyboard activation, status
  text, 44 by 44 CSS-pixel targets, reordering, reconnect, and end-to-end
  execution against a deterministic same-origin API fixture.
- Chrome absence is a hard failure; there is no jsdom substitution for the
  browser acceptance gate.

Final implementation commit before documentation sync: `ac12c50` (see the
commit-history note above - not reachable today; squashed into `e8ee89c`).

## Intentionally deferred scope

### Phase 18 executable import validation

The schema documentation is corrected, but no claim is made that package import
is implemented. Phase 18 must reject strings that fit schema code-point limits
but exceed firmware UTF-8 byte limits before any persistent mutation, and must
prove no partial set, macro, procedure, or progress state remains.

### Release budgets and hardware validation

Firmware-slot, webfs, RAM, heap, task-stack, and hardware-in-the-loop gates
remain open. This fix pass does not manufacture completion evidence for them.

## Validation status

- Host Tests run `30455828432`: passed, including strict frontend gates,
  frontend and native coverage, host tests, and ASan/UBSan.
- Browser Tests run `30455823494`: passed the real Chrome production-bundle
  workflow, including keyboard, focus, touch-target, reconnect, reorder, and
  complete execution checks.
- The documentation-only synchronization commit is revalidated by the
  read-only Quality, Host Tests, Browser Tests, and ESP32-S3 build workflows
  before Phase 17.9/17.10 is reported as final.
