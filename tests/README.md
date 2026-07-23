# Test Suites

The repository currently has two implemented test layers:

- `tests/host/` builds strict native C tests for the macro parser and storage set
  repository and runs them through CTest.
- `firmware/test_app/` builds an ESP-IDF Unity application for manual execution on
  a physical ESP32-S3.

Run host tests from the repository root:

```bash
./scripts/run-tests.sh
```

Build device tests after activating ESP-IDF v5.5.5:

```bash
./scripts/build-device-tests.sh
```

USB host behavior, actual keyboard input, Wi-Fi/browser integration, physical
buttons, fault injection, and power-loss recovery remain hardware-in-the-loop work
described in `docs/HIL_TEST_PLAN.md`. They are not represented as passing tests.
