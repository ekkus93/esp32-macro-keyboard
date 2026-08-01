# Unit Test Expansion 1 — Handoff Status

**Date:** 2026-07-24  
**Repository:** `ekkus93/esp32-macro-keyboard`  
**Branch:** `master`  
**Overall status:** **In progress — do not treat the TODO as complete**

## 1. Purpose of this document

This document is a checkpoint for resuming work on `docs/UNIT_TESTS1_TODO.md` later. It records:

- what was already validated before the latest changes;
- what was subsequently implemented directly on `master`;
- what has not been validated;
- known defects and requirement mismatches;
- hardware-dependent work that cannot be claimed complete without physical evidence;
- the recommended sequence for continuing safely.

This document is a handoff record, not proof that the current `master` head is green.

## 2. Repository-change policy for future work

The following rules must be observed when work resumes:

1. Do **not** create a branch unless Phillip explicitly asks for one.
2. Do **not** connect Gmail, email alerts, or email-based notifications.
3. Do **not** claim physical ESP32-S3 execution, USB enumeration, typing, Wi-Fi behavior, button behavior, browser-to-device integration, or power-interruption behavior without reviewed evidence.
4. Do **not** mark TODO items complete merely because implementation code exists. The corresponding tests and CI jobs must pass.
5. Do **not** hide deterministic failures with retries, ignored exit codes, `continue-on-error`, `|| true`, warning suppressions, sanitizer suppressions, coverage exclusions, or silent fallbacks.
6. Do **not** invent production APIs solely to satisfy filenames in a TODO. Reconcile the TODO against the production architecture first.

## 3. Last previously validated milestone

Before the latest direct-to-`master` work, the following milestone was validated through the earlier pull-request CI flow:

- native host-test infrastructure and deterministic fakes;
- macro executor tests;
- authentication/session tests;
- HTTP security and server-adapter tests;
- storage atomic-write, transaction, quarantine, and fault-injection tests;
- application startup and rollback tests;
- USB keyboard state tests;
- device-control tests;
- Wi-Fi AP tests;
- frontend API, routing, authentication, execution, and error-banner tests;
- strict TypeScript, ESLint, Stylelint, Prettier, and Vitest validation;
- ESP32-S3 Unity test firmware compilation with ESP-IDF v5.5.5;
- no artifacts retained on ordinary pull-request runs.

The frontend milestone was squash-merged to `master` as:

```text
b37111fbd53e7e2c5b06c062b0f0c9d802395dd4
```

The storage fault-injection milestone was present immediately before it as:

```text
ceb10e60aed6886cf2b708425e9add1a33f81228
```

These earlier milestones do **not** prove that the later sanitizer, coverage, storage-set expansion, or parser/model expansion commits are green.

## 4. Changes subsequently committed directly to `master`

The following commits were made directly on `master` while attempting to finish the remaining software-only TODO items:

| Commit | Description |
| --- | --- |
| `9d8d5e12fdd0ef99c3f307f86b60c14f391a5fe7` | Expand and reconcile macro parser boundary tests |
| `dad0e8b0f6949d12747e4887a2c38f5831c0255b` | Expand and reconcile macro model boundary tests |
| `3a6cfd88f6cfa13d54f6a9bedc8c71a1a7f2cb8b` | Add a comprehensive macro-set repository test suite |
| `09790ccf831102f5266b3385a1e8463aa18b7d01` | Route the storage repository host target to the new set suite |
| `cbadab3e169c2a605f68a8339f2211193bbdd50f` | Add host sanitizer and coverage CMake modes |
| `f71ac61d58a90842df2c2e380bfce247bdd6df91` | Extend the host test runner with sanitizer and coverage modes |
| `d77701db895c4501b7c05db3489b791f069e8983` | Add native coverage report generation and gates |
| `96f9e59cb374dde7b7fa409690c8deefbb849f54` | Configure first-party frontend coverage |
| `78c082c7594475c14eead1e4d702428dbf4409a0` | Add the frontend coverage package command |
| `821bd1ca462c09a65f14cb49f9295a5683ea5d32` | Add a deterministic frontend coverage runner script |
| `f61dd306f2ee0928eb4db8514c470ca80db29fd3` | Add tagged test-asset validation |
| `e159bd328709dec56ea218a49a7c16a5721e1315` | Expand GitHub Actions with sanitizer and coverage jobs |

These commits represent implementation attempts. They must not be interpreted as successful validation until the exact resulting head passes all required checks.

## 5. Work implemented after the last validated milestone

### 5.1 Parser boundary expansion

`tests/host/test_macro_parser.c` was expanded to cover substantially more of Task 13.1, including areas such as:

- null and empty input contracts;
- exact and above-limit source/action/duration behavior;
- default and custom timing values;
- invalid timing options;
- newline, carriage-return, tab, brace, directive, and location behavior;
- named key and modifier combinations;
- invalid UTF-8/non-ASCII behavior;
- output-plan reuse and cleanup behavior.

This implementation still requires an exact-head test run and requirement-by-requirement review before Task 13.1 can be marked complete.

### 5.2 Macro model boundary expansion

`tests/host/test_macro_model.c` was expanded to cover more of Task 13.2, including:

- text-length and embedded-NUL boundaries;
- revision boundaries;
- null and repeated cleanup;
- partially initialized macro/procedure ownership cleanup;
- preservation or clearing of fields according to the current production implementation.

The current test-memory tracker does not automatically instrument all production `malloc`, `calloc`, `realloc`, and `free` calls. Therefore, “leak tracking is clean across every required suite” is not yet established merely by expanding these tests.

### 5.3 Macro-set repository suite

A new file was added:

```text
tests/host/test_storage_sets.c
```

It exercises the **implemented macro-set repository**, including cases such as:

- create/read/list/update/delete;
- stable index ordering;
- stale revision conflicts;
- duplicate IDs;
- the macro-set count limit;
- revision overflow;
- malformed object quarantine and evidence preservation;
- corrupt/duplicate index quarantine;
- invalid or oversized object handling;
- nested-directory deletion cleanup;
- interrupted create/delete recovery;
- unknown transaction preservation;
- missing initialized-index handling.

The host CMake target named `storage_repository_tests` was changed to build this new set suite.

This does not satisfy separate macro, procedure, and progress repository tests because those production repositories do not currently exist.

### 5.4 Host sanitizer and coverage modes

The host build/test flow was extended with separate build modes and directories for:

- normal tests;
- AddressSanitizer plus UndefinedBehaviorSanitizer;
- native coverage.

The intent is to prevent normal, sanitizer, and coverage builds from contaminating one another.

Relevant additions or modifications include:

```text
tests/host/cmake/host_test_mode.cmake
scripts/run-tests.sh
scripts/generate-native-coverage.sh
```

The native coverage script attempts to:

- run the host suite with coverage instrumentation;
- include first-party production code;
- exclude tests, fakes, generated output, ESP-IDF, and third-party code;
- produce line and branch summaries;
- enforce a 90% line and 80% branch threshold for selected pure policy modules;
- reject coverage-ignore markers.

These gates are not yet proven to pass on the current exact head.

### 5.5 Frontend coverage mode

The frontend configuration and scripts were extended with:

```text
npm --prefix webapp run test:coverage
scripts/generate-frontend-coverage.sh
```

The intended reports are:

```text
webapp/coverage/coverage-summary.json
webapp/coverage/index.html
webapp/coverage/lcov.info
```

The frontend coverage configuration is intended to restrict coverage to first-party frontend source files.

A known dependency-integrity issue remains and is described in Section 7.1.

### 5.6 Expanded GitHub Actions workflow

`.github/workflows/host-tests.yml` was expanded into separate jobs for:

- normal host tests;
- host ASan/UBSan;
- native coverage;
- frontend tests;
- frontend coverage.

Artifact upload remains conditional on tag refs. A packaging validator was added:

```text
scripts/validate-tagged-test-assets.sh
```

The existence of these jobs and scripts does not prove that they pass or that tagged uploads contain the correct files.

## 6. Current TODO status by major area

| Area | Current status | Notes |
| --- | --- | --- |
| Existing host suites | Previously validated | Earlier PR CI passed before latest direct commits |
| Frontend functional tests/lint | Previously validated | Earlier PR CI passed |
| ESP32-S3 test firmware build | Previously validated | Build only; not physical execution |
| Parser/model expansion | Implemented but unverified | Requires exact-head run and checklist reconciliation |
| Macro-set repository expansion | Implemented but unverified | Tests only the repository that currently exists |
| Macro/procedure/progress repositories | Not implemented | Production repository APIs do not exist |
| ASan/UBSan | Configured but unverified | Exact-head sanitizer job must pass |
| Native coverage | Configured but unverified | Reports and thresholds must pass |
| Frontend coverage | Configured with a known dependency defect | Lockfile/reproducibility must be fixed |
| Tagged artifact validation | Scripted but unverified | Requires a real tagged workflow run |
| Documentation reconciliation | Incomplete | TODO and progress files are stale |
| Physical Unity execution | Not done | Requires ESP32-S3 and serial evidence |
| HIL validation | Not done | Requires the separate HIL plan and hardware evidence |

## 7. Known defects and unresolved issues

### 7.1 Frontend coverage dependency is not locked

`webapp/package.json` contains:

```json
"test:coverage": "vitest run --coverage"
```

However, `@vitest/coverage-v8` was not added to the committed `devDependencies` and corresponding `package-lock.json` dependency graph.

Instead, `scripts/generate-frontend-coverage.sh` currently runs:

```bash
npm --prefix webapp ci --ignore-scripts --no-audit --no-fund
npm --prefix webapp install \
  --no-save \
  --package-lock=false \
  --ignore-scripts \
  --no-audit \
  --no-fund \
  "@vitest/coverage-v8@3.2.4"
```

This is not fully lockfile-reproducible. Transitive dependency resolution can change independently of `package-lock.json`.

Required correction:

1. Add exact `@vitest/coverage-v8` version `3.2.4` to `webapp/devDependencies`.
2. Regenerate and commit `webapp/package-lock.json` using the pinned Node/npm environment.
3. Remove the dynamic `npm install --no-save --package-lock=false` step.
4. Use only `npm ci` followed by `npm run test:coverage`.

### 7.2 Exact-head CI is not established

There is no authoritative recorded result in this handoff proving that commit
`e159bd328709dec56ea218a49a7c16a5721e1315` passes all of these jobs:

- Host Tests;
- Host ASan and UBSan;
- Native Coverage;
- Frontend Tests;
- Frontend Coverage;
- ESP32-S3 Device Test Build.

The current TODO must remain `In progress` until the exact head is green or later fixes are green.

### 7.3 Task 7.3 does not match the current production implementation

Task 7.3 requests:

```text
tests/host/test_storage_macros.c
tests/host/test_storage_procedures.c
tests/host/test_storage_progress.c
```

The current production storage repository implements macro-set CRUD, index handling, staging, transactions, and quarantine. It does not expose complete CRUD repositories for macros, procedures, and progress.

Do not create test-only fake repositories or unsupported production APIs merely to check these filenames off.

When resuming, make an explicit architectural decision:

- **Option A:** Treat Task 7.3 as conditional and leave it open until those repositories are implemented by the main product TODO.
- **Option B:** Implement the real macro/procedure/progress repositories as a separate product feature with its own specification, tests, transaction behavior, and API integration.

Option B is substantially larger than a unit-test-only task.

### 7.4 Tagged artifact behavior is unproven

The workflow contains tag-only upload steps and a local package-content validator. A real tagged run is still required to verify:

- expected binaries are present;
- logs are complete;
- coverage reports are present;
- metadata identifies the tag and commit;
- no npm dependencies, caches, credentials, tokens, or unrelated build trees are included;
- ordinary branch/pull-request runs retain no artifacts.

Do not create a tag solely for testing without explicit authorization from Phillip.

### 7.5 Documentation is stale

At this checkpoint:

- `docs/UNIT_TESTS1_TODO.md` still reports `In progress` and leaves many correct items unchecked;
- `docs/UNIT_TESTS1_PROGRESS.md` still lists parser reconciliation, sanitizers, coverage, CI expansion, packaging, and documentation as open even though implementation attempts were committed;
- required README documentation for `[device][executor]`, `[device][auth]`, and `[device][usb]` has not been fully reconciled;
- documentation has not been updated to distinguish exact-head host-tested, sanitizer-tested, coverage-gated, device-build-tested, device-executed, and HIL-verified states.

Documentation should be updated only after the corresponding validations pass.

### 7.6 Physical-device work remains unavailable here

The following cannot be marked complete without a connected ESP32-S3 and reviewed logs/evidence:

- executing the Unity suite on the device;
- USB enumeration against a real host;
- actual keyboard typing and release-all behavior;
- Wi-Fi AP and client behavior;
- physical button and LED behavior;
- browser-to-device workflows;
- reset and power-interruption recovery;
- the full `docs/HARDWARE_TEST_PLAN.md`.

Firmware compilation is not a substitute for physical execution.

## 8. Recommended resume sequence

Follow this order when work resumes.

### Step 1 — Confirm repository state

Inspect `master` and record the exact head SHA. Do not assume it is still
`e159bd328709dec56ea218a49a7c16a5721e1315`.

Check for any user-made commits after this handoff before changing files.

### Step 2 — Repair frontend coverage reproducibility

Add the exact frontend coverage provider to `package.json` and regenerate the lockfile.

Expected dependency:

```json
"@vitest/coverage-v8": "3.2.4"
```

Then simplify `scripts/generate-frontend-coverage.sh` to use the lockfile-only flow.

### Step 3 — Run normal host tests first

```bash
./scripts/run-tests.sh
```

Fix deterministic compiler or test failures before moving to sanitizers.

### Step 4 — Run sanitizer tests

```bash
./scripts/run-tests.sh --sanitizers
```

Treat every ASan/UBSan finding as a real defect. Do not suppress first-party findings.

### Step 5 — Generate native coverage

Install the pinned `gcovr` version expected by CI, then run:

```bash
./scripts/generate-native-coverage.sh
```

Confirm:

- line and branch reports are produced;
- only intended first-party production sources are included;
- selected policy-module gates pass;
- no ignore directives are present.

### Step 6 — Run the frontend stack

```bash
npm --prefix webapp ci --ignore-scripts --no-audit --no-fund
npm --prefix webapp run typecheck
npm --prefix webapp run lint
npm --prefix webapp run stylelint
npm --prefix webapp run format:check
npm --prefix webapp test
npm --prefix webapp run test:coverage
```

Fix lockfile, formatting, test, and threshold failures directly. Do not weaken the checks.

### Step 7 — Validate exact-head CI

Confirm all relevant jobs pass for the same commit SHA. Record the workflow run and job results in `docs/UNIT_TESTS1_PROGRESS.md`.

Do not rely on older green PR runs after modifying the tests or workflow.

### Step 8 — Reconcile the authoritative TODO

Review every unchecked acceptance item in `docs/UNIT_TESTS1_TODO.md` against actual evidence.

Only check an item when its exact requirement is satisfied. For conditional Task 7.3, document explicitly that the object repositories do not yet exist and leave the item open unless the production feature is implemented.

### Step 9 — Update documentation

After green validation, update at least:

```text
tests/README.md
tests/host/README.md
firmware/test_app/README.md
.github/workflows/README.md
README.md
docs/IMPLEMENTATION_STATUS.md
docs/UNIT_TESTS1_PROGRESS.md
docs/UNIT_TESTS1_TODO.md
```

Clearly label each capability as one of:

- implemented only;
- host-tested;
- sanitizer-tested;
- coverage-gated;
- ESP32-S3 build-tested;
- physically device-executed;
- HIL-verified.

### Step 10 — Tagged-run validation only with authorization

After normal CI is green, request explicit authorization before creating a validation tag. Inspect the resulting artifact ZIP contents before checking tagged packaging complete.

### Step 11 — Physical execution and HIL

Run the device Unity suite and HIL plan only when hardware, serial access, and evidence capture are available. Commit or otherwise preserve the complete output required by the project process.

## 9. Files most relevant when resuming

### Authoritative status and requirements

```text
docs/UNIT_TESTS1_TODO.md
docs/UNIT_TESTS1_PROGRESS.md
docs/SPEC.md
docs/HARDWARE_TEST_PLAN.md
```

### Latest test additions

```text
tests/host/test_macro_parser.c
tests/host/test_macro_model.c
tests/host/test_storage_sets.c
tests/host/CMakeLists.txt
tests/host/cmake/host_test_mode.cmake
```

### Test and coverage runners

```text
scripts/run-tests.sh
scripts/generate-native-coverage.sh
scripts/generate-frontend-coverage.sh
scripts/validate-tagged-test-assets.sh
```

### Frontend dependency and coverage configuration

```text
webapp/package.json
webapp/package-lock.json
webapp/vite.config.ts
```

### CI

```text
.github/workflows/host-tests.yml
.github/workflows/device-tests-build.yml
```

## 10. Definition of completion for the software-only portion

The software-only portion of `UNIT_TESTS1_TODO.md` can be considered complete only when:

1. the normal native host suite passes on the exact final commit;
2. ASan and UBSan pass without suppressions;
3. native line/branch coverage reports are correct and required gates pass;
4. frontend typecheck, lint, formatting, tests, and coverage pass from the committed lockfile;
5. parser/model requirements are reconciled one by one;
6. the implemented macro-set repository suite passes;
7. Task 7.3 is explicitly resolved as conditional or implemented as a real product feature;
8. applicable fakes fail on unexpected calls;
9. leak/ownership cleanup is demonstrated for required suites;
10. Unity tags and test limitations are documented;
11. exact-head CI is green;
12. an authorized tagged run proves packaging behavior;
13. authoritative TODO/progress/status documentation matches the evidence.

Even after that, physical ESP32-S3 execution and HIL remain separate open work until real evidence is reviewed.

## 11. Final checkpoint statement

At this checkpoint, substantial implementation work exists on `master`, but the repository is **not known to be fully green**, `docs/UNIT_TESTS1_TODO.md` is **not complete**, and physical/HIL work is **not done**.

Resume by fixing the frontend coverage dependency/lockfile issue and validating the exact current head before adding more features or updating acceptance claims.
