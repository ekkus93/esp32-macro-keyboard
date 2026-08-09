# Project Documentation

This directory contains the authoritative design, implementation plan, operational
references, security review, recovery policy, and implementation evidence for the
ESP32 Macro Keyboard project.

## Authoritative documents

- [`SPEC_V2.md`](SPEC_V2.md) and [`UI_UX_SPEC_V2.md`](UI_UX_SPEC_V2.md) define
  required behavior, architecture, data models, APIs, safety invariants, and
  acceptance criteria for the v2 rebuild.
- [`TODO_V2.md`](TODO_V2.md) defines the implementation order and completion
  gates.
- [`SPEC.md`](SPEC.md) and [`TODO.md`](TODO.md) are retired v1 compatibility
  pointers, kept only for historical investigation; see
  [`implementation-v2/V2_MIGRATION_MAP.md`](implementation-v2/V2_MIGRATION_MAP.md)
  for what changed and why.
- [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md) and
  [`UNIT_TESTS1_PROGRESS.md`](UNIT_TESTS1_PROGRESS.md) are retired v1-era
  snapshots (both predate the v1→v2 rebuild); current v2 status lives in
  `TODO_V2.md` and the reports under [`implementation-v2/`](implementation-v2/).

## Reference documents

- [`API.md`](API.md) documents implemented routes and explicitly lists missing API
  groups.
- [`DEVELOPMENT.md`](DEVELOPMENT.md) contains the pinned ESP-IDF and frontend setup.
- [`MACRO_LANGUAGE.md`](MACRO_LANGUAGE.md) defines the version 0.1 macro grammar.
- [`RECOVERY.md`](RECOVERY.md) defines persistence and corruption-recovery rules.
- [`SECURITY_REVIEW.md`](SECURITY_REVIEW.md) tracks enforced controls and blocking
  findings.
- [`HARDWARE_TEST_PLAN.md`](HARDWARE_TEST_PLAN.md) contains hardware-in-the-loop procedures.
- [`RELEASE_NOTES.md`](RELEASE_NOTES.md) records the unreleased 0.1.0 state.

The `mockups/` directory currently contains only the planned naming and workflow
guidance. Individual SVG and PNG mockups have not been committed.

Documentation must remain synchronized with the code. Implemented, host-tested,
device-build-tested, and physically verified are distinct states and must not be
conflated.
