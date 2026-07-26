# FIX1 Spec/TODO Review — Authoritative Responses

**Status:** Answered and approved for implementation  
**Applies to:**

- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`

These responses are specific operator decisions for FIX1. They clarify ambiguous
implementation choices without relaxing the fail-closed, no-false-success, no-silent-
fallback, ownership, durability, and error-causality requirements.

Where a response below materially changes or narrows wording in the FIX1 specification
or TODO, update those documents in the same phase before marking the affected task
complete. Record this response file and the synchronization commit in the FIX1 progress
document.

---

## 1. clang-tidy fail-closed vs. third-party diagnostics

**Question:** `run-clang-tidy` exits nonzero because of third-party ESP-IDF/FreeRTOS and
TinyUSB findings. Should `misc-header-include-cycle` be disabled, or should the script
continue filtering findings by first-party location while conditionally accepting a
nonzero analyzer exit?

**Answer:** Preserve fail-closed analyzer execution. Do **not** retain `|| true`, do not
ignore the process status, and do not convert a nonzero analyzer exit into success by
post-processing diagnostic locations.

The required design is:

1. Select only first-party translation units from `compile_commands.json`.
2. Configure clang-tidy so known third-party headers are excluded before diagnostics
   are emitted.
3. Run clang-tidy and preserve its actual exit status.
4. Treat any remaining nonzero exit as a gate failure.
5. Independently verify that at least one first-party translation unit was analyzed and
   that the report was produced and parsed successfully.
6. Print the complete analyzer report on every failure.

For `misc-header-include-cycle`, do **not** globally disable first-party include-cycle
checking merely because ESP-IDF contains a cycle. Use the narrowest supported mechanism:

- Prefer the check-specific `misc-header-include-cycle.IgnoredFilesList` option with
  anchored patterns that match only ESP-IDF, FreeRTOS, and managed-component roots.
- Use `ExcludeHeaderFilterRegex` or the equivalent command-line option for other
  diagnostics located in third-party headers when the installed tool supports it.
- Verify supported options with the actual pinned analyzer using `--dump-config` and
  `--help`; do not assume an option exists merely because a newer LLVM release has it.

If the pinned analyzer cannot exclude the third-party include-cycle finding narrowly,
use two explicit passes:

1. the normal fail-closed clang-tidy pass with `misc-header-include-cycle` disabled; and
2. a dedicated first-party include-cycle gate that examines only repository-owned
   headers.

The dedicated pass may use the clang-tidy check if it can be restricted correctly, or a
small deterministic script that builds a graph from project-local quoted includes and
fails on a first-party cycle. It must have direct regression tests. Do not omit
first-party include-cycle checking entirely.

The gate must distinguish configuration from result handling:

- A documented third-party exclusion is permitted.
- Ignoring a nonzero result after execution is not permitted.
- A compiler error, analyzer crash, missing executable, malformed compilation database,
  zero selected translation units, unsupported configuration option, or report parser
  failure must fail the gate.

Add regression tests proving all of the following:

- a first-party warning fails;
- a first-party include cycle fails;
- a third-party-only include cycle is excluded cleanly;
- an analyzer crash or arbitrary nonzero exit fails even with no warning-shaped output;
- zero selected first-party translation units fails;
- a clean run returns success.

Do not use location-based grep as an override for a nonzero process status. Grep or a
structured report parser may be an additional assertion only after analyzer execution
has succeeded.

---

## 2. The three currently disabled clang-tidy checks

**Question:** May these checks remain disabled with documentation, or must FIX1
restructure the implementation to enable them?

- `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`
- `readability-non-const-parameter`
- `concurrency-mt-unsafe`

**Answer:** The exact three existing exclusions may remain for the pinned toolchain as
**explicit, reviewed, repository-wide static-analysis exceptions**. They are not hidden
suppressions, and Claude Code should not rewrite correct code to call unavailable,
deprecated, or signature-incompatible APIs merely to satisfy these checks.

This approval is narrow. It does not authorize additional disabled checks.

Create or update a committed exception register, preferably:

```text
docs/STATIC_ANALYSIS_EXCEPTIONS.md
```

For each exception, record:

- exact check name;
- exact toolchain/version;
- representative finding;
- why the suggested remediation is invalid or unavailable;
- affected source/interface;
- compensating controls;
- condition that requires re-evaluation;
- date and FIX1 decision reference.

Add a policy check under `scripts/` and its tests under `tests/scripts/` that parses
`.clang-tidy` and fails when:

- a fourth disabled check appears without an explicit approved update;
- one of the approved check names is misspelled or replaced by a broader wildcard;
- `WarningsAsErrors` is weakened;
- first-party `NOLINT`, compiler diagnostic pragmas, or equivalent local suppressions
  appear.

Use the following compensating controls.

### 2.1 DeprecatedOrUnsafeBufferHandling

Keep the check disabled because the demanded C11 Annex K functions are unavailable in
the ESP-IDF/newlib target environment. Compensate with:

- existing strict size and bounds validation;
- checked formatting and copy lengths;
- compiler warnings as errors;
- ASan/UBSan host tests;
- deterministic short-read, short-write, overflow, and malformed-input tests;
- prohibition of unbounded `strcpy`, `strcat`, `sprintf`, and equivalent calls.

### 2.2 readability-non-const-parameter

Keep the check disabled because the TinyUSB callback signature is externally mandated
and changing the pointer qualification would create an incompatible callback type.
Compensate by:

- keeping the mandated callback adapter small;
- treating callback input as read-only unless the external contract explicitly requires
  writing;
- copying into an internal const-qualified representation before passing data deeper
  into first-party code where appropriate;
- adding a compile-time callback-signature test or normal firmware build coverage;
- re-enabling the check temporarily during toolchain upgrades to verify no unrelated
  first-party findings have appeared.

### 2.3 concurrency-mt-unsafe

Keep the check disabled for `readdir()` because `readdir_r()` is deprecated and is not an
acceptable replacement. Compensate by:

- serializing all repository mutation and recovery operations as required by FIX1;
- never sharing one `DIR *` between concurrent operations;
- keeping directory iteration inside the repository/storage ownership boundary;
- testing lock ownership at every directory iteration seam;
- adding ThreadSanitizer or real-thread smoke coverage later when the host toolchain can
  support it reliably, without making that optional test a substitute for deterministic
  lock tests.

This response is an explicit clarification to FIX1 §3.4: documented, exact,
non-actionable toolchain exceptions with compensating controls are permitted. Hidden,
per-finding, convenience, or warning-count-reduction suppressions remain prohibited.

---

## 3. Execution timeout representation

**Question:** Add a distinct `EXECUTION_TIMED_OUT` state end-to-end, or keep
`EXECUTION_FAILED` plus `APP_ERROR_TIMEOUT` and derive the label in the frontend?

**Answer:** Add a distinct terminal state end-to-end.

Required representation:

```c
typedef enum {
    EXECUTION_IDLE = 0,
    EXECUTION_RUNNING,
    EXECUTION_COMPLETED,
    EXECUTION_CANCELLED,
    EXECUTION_FAILED,
    EXECUTION_TIMED_OUT
} execution_state_t;
```

The API string must be:

```text
timed_out
```

Retain `APP_ERROR_TIMEOUT` as the primary error code. The state answers what happened to
the execution; the error code answers why it reached that state. Do not make the
frontend infer a terminal state from an error string.

Update all of the following together:

- firmware enum and state-to-string conversion;
- executor terminal-state selection;
- execution status JSON;
- TypeScript `ExecutionStatus` type and runtime validator;
- result-page label and unsafe-state presentation;
- firmware and frontend tests;
- API and macro-execution documentation.

Required user-visible labels are distinct:

- `Completed`;
- `Cancelled`;
- `Failed`;
- `Timed out`;
- a separate unsafe key-release warning whenever `releaseError` is non-success.

A release failure after an otherwise completed action sequence must not be presented as a
safe successful completion.

---

## 4. Branch vs. master

**Question:** Continue FIX1 directly on `master`, or create a dedicated implementation
branch?

**Answer:** Continue directly on `master`.

The operator has explicitly selected direct `master` work for this project. Do not create
a FIX1 branch unless the operator later changes that instruction.

Rules for direct work:

- do not reset `master` to the review baseline;
- treat `992f2a018aff97e5b167c98d6a0d469d6a7c84ff` as the reviewed ancestor, not the
  current head;
- record the actual `master` SHA immediately before the first implementation change in
  the FIX1 progress file;
- commit in coherent phase-sized increments;
- do not force-push or rewrite history;
- do not combine unrelated changes in one commit;
- run the phase gate before claiming that phase complete;
- record each implementation commit and its validation evidence in the progress file.

The commit containing these completed responses is part of the documented starting
state. Claude Code must resolve and record the exact current SHA rather than copying an
older SHA from the question text.

---

## 5. Scope and sequencing of the engagement

**Question:** Implement all host-testable phases while marking hardware work blocked,
implement only an early slice and reassess, or use another scope?

**Answer:** Implement the complete host-testable/software FIX1 program in the required
order. Start with Phase 2 because it determines whether later green results are
trustworthy, then continue through subsequent phases without stopping merely for a
routine reassessment.

The required working policy is:

1. establish the progress file and actual baseline;
2. complete Phase 2 quality-gate hardening first;
3. continue through ownership, cleanup, storage, concurrency, authentication,
   provisioning software, repositories, APIs, frontend, import/export, diagnostics,
   and release automation in TODO order;
4. run and record each phase gate;
5. leave genuinely physical/HIL acceptance items unchecked until observed on hardware.

Do not mark an item complete merely because its code compiles or a host fake passes.

The following can be environment-blocked when the required device or provisioning
fixture is unavailable:

- irreversible or device-specific eFuse provisioning evidence;
- physical flash-encryption/HMAC-key provisioning confirmation;
- Linux and ChromeOS USB enumeration and typing matrix;
- real SoftAP/browser integration on the ESP32-S3;
- real power-interruption testing;
- measured physical cancellation latency;
- observed host-side release-all behavior;
- physical reset-gesture validation.

Environment-blocked items must remain open and must be listed in the progress document
with:

- exact prerequisite hardware;
- exact command or operator procedure;
- expected evidence;
- pass/fail criteria;
- any safety precautions.

Do not classify the software portion of secure provisioning as hardware-blocked. Claude
Code can and should implement:

- encrypted-NVS configuration and partition changes;
- provisioning state machine;
- readback validation;
- setup/reset APIs;
- production-build rejection of development bypasses;
- host-testable policy and serialization logic;
- a hardware validation runbook.

Proceed until the remaining unchecked work genuinely requires external hardware,
operator action, secret provisioning material, or a physical measurement. Do not hide
those remaining items behind a completed FIX1 label.

---

## 6. Concurrency-test mechanism

**Question:** Use deterministic fake scheduling/an injected lock seam, or real host
threads?

**Answer:** Use deterministic fake scheduling and an injected lock seam as the required
primary proof. Real host threads are optional supplementary coverage, not the primary
completion gate.

The production repository must still use the appropriate ESP-IDF synchronization
primitive. The test seam must exercise the same lock boundaries and mutation logic.

Required deterministic scenario for an update using expected revision `N`:

1. Caller A acquires the repository mutation lock.
2. Caller A reads revision `N`.
3. A test hook pauses Caller A before the durable write.
4. Caller B attempts the same public mutation with expected revision `N`.
5. Caller B must not enter the read-check-write critical section while A owns it.
6. Caller A writes and commits revision `N + 1` and releases the lock.
7. Caller B is retried after release.
8. Caller B reads revision `N + 1` and returns `APP_ERROR_CONFLICT` for stale expected
   revision `N`.
9. Exactly one durable update exists and exactly one caller reports success.

Also test:

- lock acquisition failure;
- unlock failure and resulting health/error state;
- no recursive acquisition by `_locked` helpers;
- create/create collision;
- update/update collision;
- delete/update collision;
- reorder/object-update collision;
- recovery versus normal mutation;
- import/restore versus normal mutation.

The fake lock may return a deterministic would-block/failure result instead of actually
blocking a single host thread. The important proof is that the second operation cannot
cross the critical-section boundary and that its later retry observes the new revision.

A small pthread smoke test may be added if it is stable under the pinned host toolchain,
but it must not replace the deterministic tests and must not introduce flaky timing-based
assertions.

---

## 7. Correcting tests that encode unsafe behavior

**Question:** Should existing passing tests that assert unsafe or false-success behavior
be rewritten?

**Answer:** Yes. Rewrite them. They are defect-preservation tests, not compatibility
requirements.

This explicitly includes:

- authentication verification tests as the API changes from `bool` to
  `app_error_code_t` plus `out_matches`;
- the frontend cancellation result changing from `Macro finished` to `Cancelled`;
- timeout tests changing to the distinct `timed_out` state;
- startup tests that currently accept partial resource ownership or false-idle state;
- HTTP lifecycle tests that currently fail to expose a retained partial server;
- storage tests that accept unreconciled atomic artifacts;
- cleanup tests that preserve only one error when primary and cleanup errors coexist;
- controls tests that treat an LED-only fatal state as adequate error visibility.

For each corrected test:

1. document the unsafe old assertion in the commit message or progress evidence;
2. replace it with the FIX1-required behavior;
3. add a direct regression test for the original defect;
4. ensure the test fails against the old implementation and passes only after the fix;
5. do not keep the old assertion as an alternate accepted path.

A reduction in passing tests during an intermediate commit is not acceptable on
`master`; change the production code and its corrected tests in the same coherent commit
or phase so the committed tree remains internally consistent.

---

## 8. FIX1 precedence over `docs/SPEC.md`

**Question:** Is FIX1 authoritative wherever it diverges from `docs/SPEC.md`?

**Answer:** Follow the exact precedence already stated in FIX1 §2:

1. `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`;
2. `docs/SPEC.md`;
3. `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`;
4. `docs/TODO.md`;
5. implementation-status and review documents;
6. existing code behavior and existing tests.

Therefore:

- the FIX1 **specification** overrides `docs/SPEC.md` on a direct conflict;
- `docs/SPEC.md` overrides the FIX1 TODO when the FIX1 specification does not resolve
  the conflict;
- the FIX1 TODO is an implementation plan, not an independent authority above the
  original specification;
- existing tests never override either specification.

This response file records specific operator decisions for the eight questions above.
For Q1, Q2, and Q3, Claude Code must synchronize the FIX1 specification/TODO wording as
needed before claiming the affected phase complete. Do not leave contradictory active
documents and rely on this response file as a hidden exception.

For the settings/UI-preference example, persist only behavior required by the higher-
precedence specifications. Do not invent persistence for ephemeral UI state. Persist
security, device, active-set policy, and other settings explicitly required by FIX1 or
`docs/SPEC.md`; keep transient dialog, navigation, draft, and presentation state in RAM
unless a specification explicitly requires persistence.

---

## Implementation authorization

Claude Code is authorized to proceed directly on `master` using these decisions.

Before the first implementation phase is marked complete:

- create the FIX1 progress document;
- record the actual starting SHA;
- synchronize any materially affected FIX1 wording;
- complete the fail-closed quality-gate work;
- run and record the Phase 2 gate.

Continue through all host-testable phases in order. Leave hardware-dependent items open
with exact validation instructions and evidence requirements.