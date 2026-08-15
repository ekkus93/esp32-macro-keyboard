# H12-122 — Exact-SHA hardware acceptance preparation

- **Date:** 2026-08-15
- **Task:** H12-122 — Final hardware confirmation on exact release SHA
- **Starting master:** `457788759656c4d8c20feb9e32cae9c44d7ff96e`
- **Disposition:** preparation complete; physical-device acceptance remains open

## Release blocker found before touching hardware

The H12 hardware audit found that the current first-run/reprovision path could not satisfy the authoritative v2 setup contract. `docs/SPEC_V2.md` requires an eight-digit decimal setup code generated randomly on every unprovisioned boot and shown on the trusted serial console. Production startup already generated that random code and used it for the live setup session, but H9 had removed its value from the UART logging boundary. At the same time, `scripts/generate-setup-label.py` still generated an unrelated, stable 24-hex HMAC-derived `setup_code`.

Those two values could never agree. After factory reset a user or hardware harness could know the bootstrap AP credential from the manufacturing label but could not know the live one-time setup code accepted by `POST /api/v1/setup`. Treating the printed HMAC value as the code would therefore make final reprovision acceptance fail for the wrong reason.

## Product correction

The first H12 preparation commit removed the stale label-derived setup code and restored a trusted-UART path for the live random setup code. That interim implementation was subsequently tightened by the final H12-122 preparation: disclosure is now explicit via the physical `setup-code` console command rather than automatic startup output, and the code authority is cleared at successful setup.

The setup code remains forbidden from HTTP setup state, diagnostics, persistence, repository/snapshot export, browser console output, ordinary ESP logging, and the manufacturing label.

## Hardware-evidence harness hardening

The preparation also found stale/fail-open retained hardware helpers. The final H12 path supersedes them with fail-closed logout, HID-capture and cleanup handling, current one-shot setup semantics, exact release-manifest flashing, and one unified destructive acceptance runner. Legacy reset and interim H12 smoke entry points now fail explicitly and direct the operator to `scripts/run-h12-122-hardware.py`.

## Local validation

The original preparation recorded passing H12 harness, setup-label, credential-output, authentication-policy, traceability, host, native-contract, and sanitizer checks. The final preparation evidence is recorded separately in `H12_122_FINAL_HARDWARE_CONFIRMATION_2026-08-15.md` and supersedes this document for the exact commands and current acceptance boundary.

## Acceptance boundary

H12-122 remains open. Preparation does not claim that a production image has been built/flashed or that the hardware smoke passed. Because H12-122 preparation changes production runtime and release-provenance behavior, H12-120/H12-121 must be re-established on the resulting exact candidate before physical acceptance.
