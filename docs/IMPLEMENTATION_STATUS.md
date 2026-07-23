# Implementation status

**Updated:** 2026-07-23

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

- Strict release-mode host build and tests for the macro model/parser.
- Printable ASCII mapping coverage and malformed-directive cases.
- Ten thousand deterministic parser fuzz inputs with the no-partial-plan check.
- Strict first-party syntax/warning pass for ESP-IDF-facing C files using local
  API stubs and the configured warning set.
- Shell `bash -n`, JSON parsing, partition bounds, and standalone strict
  TypeScript checking with local type stubs.

The ESP-IDF stub pass validates first-party C syntax and warnings only. It is not
a substitute for a real ESP-IDF build or link.

## Release-blocking work still open

- ESP-IDF component resolution and committed `dependencies.lock`.
- Committed npm lockfile and clean `npm ci`/ESLint/Stylelint/Prettier/Vite run.
- Persistent encrypted NVS provisioning and production credential reset.
- Set, macro, procedure, progress, ordering, quarantine, and repository CRUD.
- Operation-specific transaction roll-forward/rollback and fault injection.
- Execution submission that loads and compiles server-owned persisted macros.
- Full CRUD, import/export, backup/restore, settings, and diagnostics APIs.
- Full frontend data workflows and accessibility/end-to-end tests.
- Real ESP-IDF `v5.5.5` build, USB enumeration, SoftAP/browser integration, and
  hardware-in-the-loop evidence.
- Size, heap, task-stack, filesystem, and cancellation-latency measurements.

No open item may be represented as complete in release documentation.
