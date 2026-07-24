# Unit Test Expansion 1 — Progress

Status: **In progress**

Validated milestones: **Storage fault injection, ESP32-S3 device build, and frontend test slice**

This file records implementation progress against `docs/UNIT_TESTS1_TODO.md`. A passing host or
frontend suite or successful firmware build does not imply physical device execution or
hardware-in-the-loop verification.

## 2026-07-24 — Local baseline verification and defect fixes

Commits `69adee9..c7a768f` on `master`. The full software-only gate was run locally on a pinned
toolchain (clang-format 18, cmakelang 0.6.13, shfmt 3.11.0, shellcheck 0.9.0, yamllint 1.38.0,
markdownlint-cli2 0.23.1, Node 24.18.0, gcovr 8.6) matching the CI pins. Results:

| Check | Result |
|-------|--------|
| Native host suite (`run-tests.sh`) | pass, 17/17 |
| ASan + UBSan (`run-tests.sh --sanitizers`) | pass, 17/17, no leaks |
| Native coverage pure-policy gate | pass, 95.1% line / 86.1% branch (thresholds 90/80) |
| Frontend coverage (lockfile-only) | pass, all three reports produced |
| `check-format.sh` | pass |
| `check-scripts.sh` | pass |
| `check-docs.sh` | pass |
| `check-webapp.sh` | pass |

Defects found and fixed while reaching this baseline:

- Macro parser leaked an uppercase key's implicit SHIFT into chord modifiers, so `{CTRL+A}`
  compiled to Ctrl+Shift+A. Corrected per SPEC 10.4.
- `fake_fs_close_dir` leaked the real `DIR` when simulating a close failure, aborting three
  storage suites under LeakSanitizer.
- `generate-native-coverage.sh` used a gcovr flag incompatible with the pinned gcovr 8.6, so
  the coverage run never completed.
- Frontend coverage was made lockfile-reproducible (§7.1): `@vitest/coverage-v8@3.2.4` added to
  `devDependencies`; dynamic install removed.
- Repo-wide formatter and linter debt cleared: clang-format/cmake-format on firmware, shfmt and
  shellcheck on scripts, markdownlint on docs, and a tightened `verify-no-remote-assets.sh`.

Not verified here and still gated: exact-head GitHub Actions CI (no ESP-IDF locally, cannot
trigger Actions), `check-firmware.sh` build + clang-tidy, tagged-artifact packaging (needs an
authorized tag), and all physical/HIL work.

## Implemented and validated in pull-request CI

### Host-test infrastructure

- Shared assertions, allocation tracking, temporary-directory support, and deterministic fakes
  for clock, randomness, FreeRTOS, USB, GPIO, Wi-Fi, HTTP, and filesystem operations.
- Strict first-party warning policy for native host targets.
- CTest labels and focused `./scripts/run-tests.sh <label>` execution with visible failure for
  unknown labels.

### Native host suites

- Macro parser and macro model.
- Macro executor.
- Authentication and session policy.
- HTTP security helpers and server adapter behavior.
- Application startup sequencing and rollback.
- USB keyboard state.
- Device controls.
- Wi-Fi AP state.
- Storage atomic writes, parent-directory durability, repository I/O, transaction recovery,
  quarantine, and the integrated storage repository suite.

### Storage hardening

- Injectable filesystem and repository-index operations for transaction recovery.
- Deterministic transaction recovery and compound failure injection.
- Injectable quarantine filesystem and UUID operations.
- Quarantine evidence, metadata, collision, cleanup, malformed-record, and limit tests.
- Explicit parent-directory durability barriers for atomic replacement on host filesystems.
- Deterministic parent-sync failure tests covering create, replacement, rollback, and
  compensation paths.
- ESP32 LittleFS durability policy documented in the production filesystem adapter without
  pretending unsupported POSIX directory `fsync` is available through ESP-IDF VFS.

### Frontend test milestone

- Deterministic fetch, location, timer, DOM, and React rendering support.
- API-client tests for same-origin requests, headers, mutation CSRF, response envelopes,
  malformed responses, network errors, abort timeout, and timer cleanup.
- Application tests for all implemented routes, login and CSRF handling, execution polling,
  completed/cancelled/failed transitions, cancellation, visible failures, and cleanup.
- Error-banner tests that verify untrusted text is rendered as text rather than markup.
- Strict TypeScript, ESLint, Stylelint, Prettier, and Vitest validation.
- A committed npm lockfile and reproducible `npm ci` execution in pull-request CI.

### ESP32-S3 device-build validation

- Unity sources for executor, authentication, and USB-state tests are registered.
- esp_tinyusb 2.2.1 configuration and HID descriptor integration compile successfully.
- Device-test firmware compiles with ESP-IDF v5.5.5 for ESP32-S3.
- Device-test source formatting passes with zero findings.

### CI behavior

- The complete configured native host suite passes.
- The frontend typecheck, lint, formatting, and Vitest stack passes.
- The configured ESP32-S3 device-test build passes.
- Pull-request jobs preserve complete failure diagnostics in job logs.
- Normal pull-request runs retain no workflow artifacts; artifact upload remains restricted to
  tagged runs.

## Milestone scope

Completed slices remain intentionally bounded. Remaining work should continue in smaller branches
rather than extending one pull request into a repository-wide mega-PR.

## Still open

1. Split object-repository suites for macros, procedures, and progress where implementations
   exist, and reconcile their required CRUD and corruption cases.
2. Reconcile every parser/model boundary requirement against the existing suites and add any
   missing cases.
3. Add AddressSanitizer and UndefinedBehaviorSanitizer builds.
4. Add native and frontend line/branch coverage and establish gates only after the required
   suites exist.
5. Expand CI to execute sanitizer and coverage jobs and validate tagged artifact packaging.
6. Update the remaining repository documentation after those jobs are green.
7. Execute the Unity suites on a physical ESP32-S3 and complete the separate HIL plan.

## Validation boundary

No physical ESP32-S3 execution, USB enumeration, real typing, radio behavior, button behavior,
browser-to-device integration, or power-interruption behavior is claimed here. Those require
reviewed serial output and the procedures in `docs/HIL_TEST_PLAN.md`.
