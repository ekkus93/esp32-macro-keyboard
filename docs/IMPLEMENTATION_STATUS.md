# Implementation status

**Updated:** 2026-07-24

This file records implementation evidence without claiming build or hardware
results that have not been observed.

## Implemented and committed foundations

- Repository structure and idempotent bootstrap script.
- Exact ESP-IDF `v5.5.5`, ESP32-S3 target, and Node.js version declarations.
- ESP-IDF project skeleton, OTA-ready partition table, and exact component
  version constraints.
- First-party lint, formatting, host-test, partition, firmware, frontend, and
  documentation check entry points.
- Shared limits, strict UUID validation/generation, stable errors, and bounded
  C/TypeScript domain models.
- Macro parser/compiler with printable US-ASCII key mapping, named keys, chords,
  delays, exact error locations, duration limits, and no partial plans.
- Separate non-formatting LittleFS mounts, typed paths, verified atomic writes,
  and conservative transaction-manifest handling.
- USB HID descriptors/state/report foundation and one-owner executor with busy
  rejection, cancellation, watchdog, and terminal release-all accounting.
- Debounced controls, Kconfig GPIO settings, LED-state task, protected SoftAP,
  PBKDF2 password records, bounded throttling, RAM sessions, and CSRF tokens.
- Bounded authenticated HTTP foundation with login/logout, status, limits,
  execution polling/cancellation, safe static files, gzip negotiation, and
  staged startup rollback.
- React/TypeScript/Tailwind/Vite mobile-first shell covering all required screen
  states, with real login, CSRF, polling, cancellation, and visible failures.
- Versioned import/export/diagnostic JSON schemas and implementation/security
  evidence documents.

## Validation completed in this environment

Verified locally on the current `master` head using the CI-pinned toolchain
(clang-format 18, cmakelang 0.6.13, shfmt 3.11.0, shellcheck 0.9.0, yamllint 1.38.0,
markdownlint-cli2 0.23.1, Node 24.18.0, gcovr 8.6). These are local results; exact-head
GitHub Actions confirmation is a separate step (see `docs/UNIT_TESTS1_PROGRESS.md`).

- **Host-tested:** the full native CTest suite passes (17 suites) — macro parser and
  model, macro executor, authentication and sessions, HTTP security and server adapter,
  application startup and rollback, USB keyboard state, device controls, Wi-Fi AP state,
  and storage (atomic writes, parent-directory durability, repository I/O, transaction
  recovery, quarantine, and set CRUD with deterministic fault injection).
- **Sanitizer-tested:** the full suite passes under AddressSanitizer and
  UndefinedBehaviorSanitizer with no leaks.
- **Coverage-gated:** native coverage runs over first-party code only and the pure-policy
  gate passes (line 95.1% >= 90, branch 86.1% >= 80).
- **Frontend-tested:** `check-webapp.sh` passes end to end — typecheck, ESLint and
  Stylelint at zero warnings, Prettier, Vitest, the production build, and the local-only
  asset check — from the committed `webapp/package-lock.json` via `npm ci`. Frontend
  coverage is lockfile-reproducible.
- The full first-party lint gate passes: `check-format.sh`, `check-scripts.sh`, and
  `check-docs.sh`.

Not validated in this local environment (no SDK): a real ESP-IDF `v5.5.5` firmware
build or link and `check-firmware.sh` clang-tidy — both now run green in CI via the
`Quality` workflow (which runs `check-all.sh` on every push and pull request).
Tagged-artifact packaging and any physical/HIL result remain unverified.

## Release-blocking work still open

- ESP-IDF component resolution and committed `dependencies.lock`.
- Persistent encrypted NVS provisioning and production credential reset.
- Macro, procedure, and progress object-repository CRUD. The set repository, its
  ordering index, and quarantine are implemented and host-tested; the macro,
  procedure, and progress repositories do not yet exist (see Task 7.3 in
  `docs/UNIT_TESTS1_TODO.md`).
- Operation-specific transaction roll-forward/rollback for the object repositories
  above.
- Execution submission that loads and compiles server-owned persisted macros.
- Full CRUD, import/export, backup/restore, settings, and diagnostics APIs.
- Full frontend data workflows and accessibility/end-to-end tests.
- Real ESP-IDF `v5.5.5` build, USB enumeration, SoftAP/browser integration, and
  hardware-in-the-loop evidence.
- Size, heap, task-stack, filesystem, and cancellation-latency measurements.

No open item may be represented as complete in release documentation.
