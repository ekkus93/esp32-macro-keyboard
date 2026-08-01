# Release notes

**Note:** the paragraph below is a historical snapshot from early
implementation (pre-FIX1) and is stale in multiple places - dependency
lockfiles, resource repositories/APIs, and import/restore transactions are
now all implemented. See
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
for current, maintained status; hardware validation and release-budget
enforcement genuinely remain open there.

## 0.1.0 — unreleased

Implemented foundations include the strict project toolchain, bounded macro
model/parser, safe storage primitives, USB/executor state machines, controls,
protected SoftAP/authentication, bounded HTTP/static serving, and a mobile-first
web application shell.

The project is not ready for release. Dependency lockfiles, a real ESP-IDF build,
persistent secure provisioning, resource repositories and APIs, import/restore
transactions, full frontend workflows, hardware testing, and size budgets remain
release blockers. See `docs/IMPLEMENTATION_STATUS.md` and
`docs/SECURITY_REVIEW.md`.
