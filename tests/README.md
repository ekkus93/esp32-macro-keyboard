# Test Suites

The repository has three test layers:

- `tests/host/` builds strict native C tests through CTest, covering the macro
  parser and model, macro executor, authentication and sessions, HTTP security and
  the server adapter, application startup and rollback, USB keyboard state, device
  controls, Wi-Fi AP state, and storage (atomic writes, parent-directory
  durability, repository I/O, transaction recovery, quarantine, and set CRUD with
  deterministic fault injection). It also runs under AddressSanitizer/UBSan and
  produces gated native coverage.
- `webapp/` builds a Vitest suite for the API client and application workflows.
- `firmware/test_app/` builds an ESP-IDF Unity application for execution on a
  physical ESP32-S3.

## Validation states

Capabilities are labelled by how far they have been validated, from weakest to
strongest: **host-tested** (native suite passes), **sanitizer-tested** (clean under
ASan/UBSan), **coverage-gated** (meets the native coverage thresholds),
**frontend-tested** (Vitest and the frontend lint/type stack pass),
**device-build-tested** (compiles for ESP32-S3 with ESP-IDF v5.5.5), **device-executed**
(run on hardware with reviewed serial output), and **HIL-verified** (the full
`docs/HIL_TEST_PLAN.md`). Nothing in this repository is claimed device-executed or
HIL-verified. Per-capability status is tracked in `docs/UNIT_TESTS1_PROGRESS.md`.

Run host tests from the repository root:

```bash
./scripts/run-tests.sh              # normal
./scripts/run-tests.sh --sanitizers # ASan + UBSan
./scripts/run-tests.sh --coverage   # gcov instrumentation
```

Run the frontend suite:

```bash
npm --prefix webapp test
```

Build device tests after activating ESP-IDF v5.5.5:

```bash
./scripts/build-device-tests.sh
```

USB host behavior, actual keyboard input, Wi-Fi/browser integration, physical
buttons, and power-loss recovery remain hardware-in-the-loop work described in
`docs/HIL_TEST_PLAN.md`. They are not represented as passing tests.
