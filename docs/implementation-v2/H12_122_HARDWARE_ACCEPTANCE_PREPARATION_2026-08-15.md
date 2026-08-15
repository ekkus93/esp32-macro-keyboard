# H12-122 — Exact-SHA hardware acceptance preparation

- **Date:** 2026-08-15
- **Task:** H12-122 — Final hardware confirmation on exact release SHA
- **Starting master:** `457788759656c4d8c20feb9e32cae9c44d7ff96e`
- **Disposition:** preparation complete; physical-device acceptance remains open

## Release blocker found before touching hardware

The H12 hardware audit found that the current first-run/reprovision path could not satisfy the authoritative v2 setup contract. `docs/SPEC_V2.md` requires an eight-digit decimal setup code generated randomly on every unprovisioned boot and shown on the trusted serial console. Production startup already generated that random code and used it for the live setup session, but H9 had removed its value from the UART logging boundary. At the same time, `scripts/generate-setup-label.py` still generated an unrelated, stable 24-hex HMAC-derived `setup_code`.

Those two values could never agree. After factory reset a user or hardware harness could know the bootstrap AP credential from the manufacturing label but could not know the live one-time setup code accepted by `POST /api/v1/setup`. Treating the printed HMAC value as the code would therefore make final reprovision acceptance fail for the wrong reason.

## Product correction

The repair follows the authoritative v2 specification instead of preserving the stale H9 interpretation:

1. the manufacturing bootstrap derivation now owns only the stable device ID, setup SoftAP SSID, and HMAC-derived AP passphrase;
2. the label generator emits only those stable values and no `setup_code`;
3. the existing unbiased v2 random setup-session generator remains the sole setup-code authority;
4. startup passes that eight-digit code through a dedicated `show_setup_code` operation;
5. the production adapter exposes the code only with `serial_console_show_setup_code()` on trusted UART0, validates the exact eight-digit format, flushes the stream, and fails startup if disclosure cannot be completed;
6. the generic app-core log event no longer contains a setup-code field or setup-code event type; and
7. the credential-output policy permits only the exact UART statement in `serial_console.c`; the same output anywhere else, or any other credential output from that file, remains forbidden.

The setup code is still forbidden from HTTP setup state, diagnostics, persistence, repository/snapshot export, browser console output, ordinary ESP logging, and the manufacturing label.

## Hardware-evidence harness hardening

The existing retained hardware helpers had several stale or fail-open behaviors:

- `provision_device.py` called retired v1-shaped `/api/v1/setup-state`, `/api/v1/setup/credentials`, and `/api/v1/setup/restart` routes and expected a persisted `setup_code.txt`;
- `test_acceptance_reset.py` used removed revisioned settings fields and retired setup routes;
- `Device.logout()` swallowed every exception;
- confirmation and HTTP-concurrency helpers could print PASS despite failure to restore the owner's `requireSerialConfirmation` setting; and
- HID capture swallowed reader/close failures.

The H12 preparation replaces those evidence gaps with fail-closed behavior:

- `provision_device.py` captures the fresh UART code in memory only, uses exactly `GET /api/v1/setup` plus one `POST /api/v1/setup`, and proves the authenticated normal service after restart;
- `test_acceptance_reset.py` is explicitly retired so it cannot manufacture false v2 evidence;
- cleanup failures in logout/settings restoration/HID capture make the acceptance run fail;
- `test_h12_release_smoke.py` requires an explicit destructive flag, exact firmware SHA, and clean production flash manifest; verifies live diagnostics against the flashed image; then exercises login, ordinary send, blob snapshot save/load/delete, password change, restart, factory reset, reprovision, and final production-image continuity; and
- `test_send_confirmation.py` remains the separate native-USB proof for confirmation-required send, cancel, and the real 60-second timeout path.

## Local validation

From the network-isolated sandbox, after rebasing the two files that changed after the uploaded source archive:

- H12 hardware-harness contract regression: PASS;
- setup-label generator regression: PASS;
- credential-output regression: **21/21 PASS**;
- v2 authentication policy: PASS;
- specification traceability: current;
- Python compilation for all touched hardware helpers: PASS;
- shell syntax and changed-line whitespace checks: PASS;
- focused startup host tests: **6/6 PASS**;
- focused storage/provisioning host tests: **14/14 PASS**;
- complete normal host suite: **66/66 PASS**;
- native v2 contracts: **6/6 PASS**; and
- ASan+UBSan host suite with the previously documented sandbox-only cJSON development shim: **66/66 PASS**.

The sanitizer shim is diagnostic only; the repository-pinned authoritative environment must rerun the release gates on the new runtime candidate. The sandbox still cannot perform the ESP-IDF production build or physical-device run.

## Acceptance boundary

H12-122 remains open. This preparation does not claim that a production image has been built/flashed or that the hardware smoke passed. Because this repair changes production runtime behavior, the complete H12 software gate must be rerun on the resulting exact candidate before that SHA is used for final physical acceptance.
