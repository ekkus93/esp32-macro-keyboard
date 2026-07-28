# Implementation status

**Updated:** 2026-07-28

This file distinguishes implemented software from hardware-validated and release-ready behavior.

## Completed software phases

- FIX1 Phases 1–15 are implemented on `master`.
- Phase 13 cooperative controls shutdown is complete; redacted controls-health aggregation remains
  part of Phase 19.
- Phase 14 encrypted persistent provisioning is software-complete; physical eFuse/HMAC-backed NVS
  confidentiality and reboot behavior remain Phase 20 hardware evidence.
- Phase 15 storage object repositories are complete at `b1ad2b8bb56d999dd88df8ace328c9990b41100c`: set, macro, procedure,
  progress, ordering, reference validation, stale-progress visibility, non-secret settings,
  active-set consistency, atomic object writes, quarantine, and set transaction recovery.

## Validation at the Phase 15 gate

The authoritative CI-pinned toolchain passed:

- `./scripts/check-all.sh`;
- storage host tests and ASan/UBSan;
- native coverage;
- frontend checks;
- production firmware and device-test builds;
- fail-closed clang-tidy with zero first-party findings;
- formatting, scripts, documentation, partition, and production-configuration policy gates.

Macro and procedure repository tests are now registered CTest targets rather than dormant source
files, and the firmware storage component now compiles the shared order implementation.

## Release-blocking work still open

- Phase 16: complete HTTP resource APIs and server-owned persisted-macro execution submission.
- Phase 17: replace remaining frontend mock/incomplete workflows and add accessibility/E2E tests.
- Phase 18: import, export, transactional replace, backup, and restore.
- Phase 19: redacted diagnostics and complete subsystem-health aggregation.
- Phase 20: USB, SoftAP/browser, encrypted-NVS reboot, power-loss, controls, and other real-hardware
  evidence.
- Phase 21: release size/resource budgets and immutable pinned GitHub Actions.
- Phases 22–23: final documentation synchronization and acceptance.

No hardware result or release-readiness claim is implied by the passing software gate.
