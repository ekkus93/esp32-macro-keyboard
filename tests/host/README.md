# Host Tests

The host suite builds first-party C code with the same strict warning policy used by the
firmware. Assertions do not depend on `NDEBUG` and remain active in release builds.

Run every host test from the repository root:

```bash
./scripts/run-tests.sh
```

Run one CTest label:

```bash
./scripts/run-tests.sh parser
./scripts/run-tests.sh storage
./scripts/run-tests.sh executor
```

Supported labels are `support`, `parser`, `storage`, `executor`, `auth`, `web`, `startup`,
`usb`, `controls`, `wifi`, and `model`. A known label with no implemented tests returns
CTest's normal no-tests result; an unknown label is rejected before configuring the build.

## Build modes

The runner selects an isolated build directory per mode so normal, sanitizer, and coverage
builds never contaminate one another:

```bash
./scripts/run-tests.sh              # normal build (tests/host/build)
./scripts/run-tests.sh --sanitizers # AddressSanitizer + UndefinedBehaviorSanitizer
./scripts/run-tests.sh --coverage   # gcov instrumentation (tests/host/build-coverage)
```

Native line/branch coverage with the enforced pure-policy gate (line >= 90, branch >= 80) is
produced by:

```bash
bash ./scripts/generate-native-coverage.sh
```

Sanitizer findings are real defects and are never suppressed; a first-party leak fails the run.

## Test infrastructure

- `support/` contains release-safe assertions, temporary-directory helpers, and explicit
  allocation tracking for test-owned memory.
- `fakes/` contains deterministic clocks, random data, framework adapters, bounded call
  logs, strict expected-call checking, and named/Nth-call failure injection.
- Fake reset functions must be called before every test case.
- Tests must assert both the returned error and the resulting ownership/state after any
  injected failure.

The host suite does not replace ESP32-S3 Unity tests or hardware-in-the-loop validation.
