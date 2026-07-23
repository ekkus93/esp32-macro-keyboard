# Project Documentation

This directory contains the authoritative design, implementation plan, operational
references, security review, recovery policy, and implementation evidence for the
ESP32 Macro Keyboard project.

## Authoritative documents

- [`SPEC.md`](SPEC.md) defines required behavior, architecture, data models, APIs,
  safety invariants, and acceptance criteria.
- [`TODO.md`](TODO.md) defines the implementation order and completion gates.
- [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md) records implemented,
  validated, and still-open work without treating unverified hardware results as
  complete.

## Reference documents

- [`API.md`](API.md) documents implemented routes and explicitly lists missing API
  groups.
- [`DEVELOPMENT.md`](DEVELOPMENT.md) contains the pinned ESP-IDF and frontend setup.
- [`MACRO_LANGUAGE.md`](MACRO_LANGUAGE.md) defines the version 0.1 macro grammar.
- [`RECOVERY.md`](RECOVERY.md) defines persistence and corruption-recovery rules.
- [`SECURITY_REVIEW.md`](SECURITY_REVIEW.md) tracks enforced controls and blocking
  findings.
- [`HIL_TEST_PLAN.md`](HIL_TEST_PLAN.md) contains hardware-in-the-loop procedures.
- [`RELEASE_NOTES.md`](RELEASE_NOTES.md) records the unreleased 0.1.0 state.

The `mockups/` directory currently contains only the planned naming and workflow
guidance. Individual SVG and PNG mockups have not been committed.

Documentation must remain synchronized with the code. Implemented, host-tested,
device-build-tested, and physically verified are distinct states and must not be
conflated.
