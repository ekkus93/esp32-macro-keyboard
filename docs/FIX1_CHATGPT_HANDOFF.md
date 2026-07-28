# FIX1 Handoff — Resume at Phase 16

## Current state

- Work is direct on `master`; do not create PR branches unless the operator changes that decision.
- FIX1 Phases 1–15 are complete.
- Phase 15 implementation commit: `b1ad2b8bb56d999dd88df8ace328c9990b41100c`.
- The authoritative gate passed after Phase 15: `./scripts/check-all.sh`, storage ASan/UBSan, and
  native coverage.
- No first-party warning suppression or fail-open fallback was introduced.

## Next phase

Resume at `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
**Phase 16 — Complete the HTTP API**.

The storage surface available to Phase 16 includes:

- set CRUD and transaction-owned delete;
- set-local and global macro CRUD, duplicate, order, and bounded reference-conflict details;
- procedure CRUD and order with strict macro reference validation;
- progress read/update/reset with explicit current/stale status;
- redacted encrypted-store settings read/update and active-set selection;
- corrupt object/order quarantine and atomic recovery validators.

## Phase 16 priorities

1. Centralized request policy for Content-Type, body limits, Host, Origin, cookie, CSRF, session,
   request ID, and physical confirmation.
2. Strict path UUID parsing and unknown-field rejection.
3. Set, macro, procedure, progress, settings, and active-set routes over the completed repositories.
4. Server-owned execution submission: load persisted macro by identity and exact revision, compile
   it server-side, transfer plan ownership, and return `202` only after executor acceptance.
5. Route-level failure/status mapping and comprehensive security/error tests.

## Deferred hardware and later-phase work

- Controls health aggregation: Phase 19.
- Execution identity/timestamps/current-action diagnostics: Phases 16/19.
- Import/restore lock serialization: Phase 18.
- eFuse/HMAC NVS physical validation, USB, SoftAP/browser, power interruption, and controls timing:
  Phase 20 on real ESP32-S3 hardware.

## Required rules

- Run the smallest relevant host suite during development and the full `./scripts/check-all.sh`
  before completion.
- Preserve fail-closed behavior and separate primary from cleanup errors.
- Do not use `NOLINT`, `eslint-disable`, `-Wno-*`, `|| true`, or other first-party suppression.
- Update the TODO and progress evidence with every completed phase.
