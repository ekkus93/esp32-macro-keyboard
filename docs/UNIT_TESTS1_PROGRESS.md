# Unit Test Expansion 1 — Progress

Status: **In progress**

Validated milestone: **Storage fault-injection and ESP32-S3 device-build slice complete**

This file records implementation progress against `docs/UNIT_TESTS1_TODO.md`. A passing host
suite or successful firmware build does not imply physical device execution or
hardware-in-the-loop verification.

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

### Storage hardening completed by this slice

- Injectable filesystem and repository-index operations for transaction recovery.
- Deterministic transaction recovery and compound failure injection.
- Injectable quarantine filesystem and UUID operations.
- Quarantine evidence, metadata, collision, cleanup, malformed-record, and limit tests.
- Explicit parent-directory durability barriers for atomic replacement on host filesystems.
- Deterministic parent-sync failure tests covering create, replacement, rollback, and
  compensation paths.
- ESP32 LittleFS durability policy documented in the production filesystem adapter without
  pretending unsupported POSIX directory `fsync` is available through ESP-IDF VFS.

### ESP32-S3 device-build validation

- Unity sources for executor, authentication, and USB-state tests are registered.
- esp_tinyusb 2.2.1 configuration and HID descriptor integration compile successfully.
- Device-test firmware compiles with ESP-IDF v5.5.5 for ESP32-S3.
- Device-test source formatting passes with zero findings.

### CI behavior

- The complete configured native host suite passes.
- The configured ESP32-S3 device-test build passes.
- Pull-request jobs preserve complete failure diagnostics in job logs.
- Normal pull-request runs retain no workflow artifacts; artifact upload remains restricted to
  tagged runs.

## Milestone closure

This branch is intended to merge as the storage-hardening milestone after its documentation-only
closure commits pass the same host and device-build workflows. Remaining work should continue in
smaller branches rather than extending this pull request into a repository-wide mega-PR.

## Still open

1. Split object-repository suites for macros, procedures, and progress where implementations
   exist, and reconcile their required CRUD and corruption cases.
2. Add frontend test support plus API, routing, authentication, execution, and error-banner
   suites.
3. Reconcile every parser/model boundary requirement against the existing suites and add any
   missing cases.
4. Add AddressSanitizer and UndefinedBehaviorSanitizer builds.
5. Add native and frontend line/branch coverage and establish gates only after the required
   suites exist.
6. Expand CI to execute frontend, sanitizer, and coverage jobs and validate tagged artifact
   packaging.
7. Update the remaining repository documentation after those jobs are green.
8. Execute the Unity suites on a physical ESP32-S3 and complete the separate HIL plan.

## Validation boundary

No physical ESP32-S3 execution, USB enumeration, real typing, radio behavior, button behavior,
browser-to-device integration, or power-interruption behavior is claimed here. Those require
reviewed serial output and the procedures in `docs/HIL_TEST_PLAN.md`.
