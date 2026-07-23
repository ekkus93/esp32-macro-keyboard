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

## Test infrastructure

- `support/` contains release-safe assertions, temporary-directory helpers, and explicit
  allocation tracking for test-owned memory.
- `fakes/` contains deterministic clocks, random data, framework adapters, bounded call
  logs, strict expected-call checking, and named/Nth-call failure injection.
- Fake reset functions must be called before every test case.
- Tests must assert both the returned error and the resulting ownership/state after any
  injected failure.

The host suite does not replace ESP32-S3 Unity tests or hardware-in-the-loop validation.
