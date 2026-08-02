# Implementation status

**Updated:** 2026-08-01

This file distinguishes implemented software from host-tested, device-tested,
and release-ready behavior. It is a snapshot; the authoritative,
continuously-maintained record is
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
(the "FIX1 doc") - consult it, not this file, for anything more current than
the date above.

## Completed software phases

FIX1 Phases 1-19 and 21-22 are implemented on `master`, and every non-hardware
checkbox in the FIX1 document is now closed.

Phase 20 (hardware and integration validation) is partly done. §20.1's clean
production build ran on a real attached ESP32-S3 (QFN56 rev v0.2, 8MB PSRAM).
Six of §20.2's nine items now have real Linux evidence, and §20.3's network
boundary is verified on device. What remains needs physical manipulation
(cable disconnect, host suspend, power interruption, button gestures) or a
ChromeOS host and a browser - see "Validation assigned to manual owner
testing" below.

Phase 23 (final regression/acceptance) cannot close while any Phase 20
checkbox is open; see the FIX1 doc's §24 completion rule.

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
- **Device-executed**: the firmware has run on a physical ESP32-S3 and its
  behaviour was observed. This is now routine: the device is provisioned,
  serves its HTTP API over Wi-Fi, and enumerates as a USB HID keyboard. The
  on-device Unity test menu (`firmware/test_app`) has still not been run on
  hardware - it remains only device-build-tested.
- **HIL-verified** (hardware-in-the-loop): a specific, recorded pass/fail
  result for a real user-facing scenario on real hardware. Several now exist,
  reproducible via `tests/hardware/`: USB enumeration as `303a:4001`;
  printable text, chords, and release-all verified from raw HID reports;
  delay and rapid-typing cancellation verified to stop mid-execution and reach
  the `cancelled` terminal state; and the network boundary verified to reject
  unauthenticated, cross-origin, forged-CSRF, and forged-session requests and
  to rate-limit brute-force logins. See FIX1 §20.2 and §20.3 for the recorded
  evidence.

  Not HIL-verified: anything needing physical cable or host manipulation
  (disconnect/reconnect, suspend/resume, disconnect-during-execution, power
  interruption), the physical control gestures of §20.5, ChromeOS, and browser
  integration against the device's own SoftAP.

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
- Phase 23: blocked on the above; cannot close while any FIX1 checkbox is
  open.

Not release-blocking but required before shipping to third parties: the
unauthenticated UART serial console must be excluded from the shipped image
(`docs/SPEC.md` §16.5, tracked in `docs/RELEASE_NOTES.md`).

No hardware result or release-readiness claim is implied by the passing
software gate alone.
