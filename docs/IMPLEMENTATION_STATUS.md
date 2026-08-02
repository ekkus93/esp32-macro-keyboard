# Implementation status

**Updated:** 2026-08-01

This file distinguishes implemented software from host-tested, device-tested,
and release-ready behavior. It is a snapshot; the authoritative,
continuously-maintained record is
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
(the "FIX1 doc") - consult it, not this file, for anything more current than
the date above.

## Completed software phases

FIX1 Phases 1-19 and 21 are implemented and host-tested on `master`. Phase 20
(hardware and integration validation) is partially complete: §20.1's clean
production build was run on a real attached ESP32-S3 (QFN56 rev v0.2, 8MB
PSRAM), producing real binary-size, RAM, and heap/stack measurements; §20.2-
20.5 (USB host matrix, SoftAP/browser integration, power interruption,
physical controls) remain genuinely open - they need either physical device
possession this session did not have exclusive, uninterrupted access to
arrange, or (for SoftAP/browser items) a second network path, since this
development machine's only Wi-Fi radio is also its own connection to the
assistant session that would need to drive the test. Phase 22 (documentation
synchronization) is mostly complete; one item remains open (a full
implemented/host-tested/device-tested/release-ready distinction pass across
every file in `docs/`, not just the ones audited so far). Phase 23 (final
regression/acceptance) cannot close until Phase 20's remaining hardware items
and Phase 22's remaining item close - see the FIX1 doc's §24 completion rule.

## Validation levels in this codebase

- **Implemented**: the behavior exists in `firmware/` or `webapp/` source.
- **Host-tested**: covered by `tests/host/` (native C tests against fakes for
  every ESP-IDF/FreeRTOS/filesystem dependency) and/or `webapp/tests/`
  (Vitest + jsdom). This is the large majority of this codebase's test
  coverage today.
- **Browser-tested**: covered by the one real-Chrome DevTools-Protocol
  workflow (`webapp/tests/browser/run-browser-tests.mjs`), which drives the
  production frontend bundle against a deterministic same-origin HTTP
  fixture - not the real device.
- **Device-build-tested**: the firmware and device-test application compile
  for the real `esp32s3` target (CI does this on every push; §20.1 confirmed
  it again from a clean checkout on real hardware).
- **Device-executed**: the firmware has actually run on a physical ESP32-S3
  and its behavior was observed (serial console or HTTP). This has happened
  exactly once, narrowly: §20.1's heap/task-stack-high-water-mark
  measurement, via a temporary instrumentation patch reverted before commit.
  The on-device Unity test menu (`firmware/test_app`) has not been run on
  hardware this session - only device-build-tested.
- **HIL-verified** (hardware-in-the-loop): a specific, recorded pass/fail
  result for a real user-facing scenario on real hardware (USB enumeration
  against a real host OS, a real SoftAP client, a real power interruption
  mid-write, a real physical-button gesture). None of Phase 20's HIL items
  are currently recorded as passed - see `docs/HARDWARE_TEST_PLAN.md`, whose
  matrix is entirely "Not run".

No capability in this codebase is currently release-ready by itself; release
readiness requires every FIX1 checkbox closed (see that document's §24).

## Validation at the last full gate run

The authoritative CI-pinned toolchain passed (`./scripts/check-all.sh`,
verified locally 2026-08-01, and continuously in CI on every push per
`.github/workflows/quality.yml`):

- host tests (59 CTest suites, including storage crash-consistency,
  ASan/UBSan);
- native and frontend coverage gates;
- frontend checks (typecheck, lint, stylelint, unit tests, real-Chrome
  browser workflow, production build);
- firmware build and fail-closed clang-tidy with zero first-party findings;
- formatting, scripts, documentation, partition, production-configuration,
  and release-budget policy gates.

## Validation assigned to manual owner testing

ChromeOS USB-host validation (§20.2) and browser-against-SoftAP validation
(§20.3) are assigned to the repository owner to perform manually, decided
2026-08-01. No ChromeOS host is attached to the development machine and
driving a browser against the device's own access point is outside what the
automated harness in `tests/hardware/` can reach. The corresponding FIX1
checkboxes remain open: the assignment records who does the work, not that
it has been done.

## Release-blocking work still open

- Phase 20: USB host matrix, SoftAP/browser integration, encrypted-NVS
  reboot persistence, power-loss, and physical-controls real-hardware
  evidence (§20.2-§20.5).
- Phase 21.3: one narrow sub-item (debug-server-enabled detection) has
  nothing to enforce against yet - no debug-server feature exists in this
  codebase - so it stays an open checkbox rather than a vacuously-passing one.
- Phase 22: one item open - a full implemented/host-tested/device-tested
  distinction editorial pass across every file in `docs/`, not just the ones
  audited in this and the prior documentation sweep.
- Phase 23: blocked on the above; cannot close while any FIX1 checkbox is
  open.

No hardware result or release-readiness claim is implied by the passing
software gate alone.
