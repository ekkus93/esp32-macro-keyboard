# Release notes

**Note:** the paragraph below is a historical snapshot from early
implementation (pre-FIX1) and is stale in multiple places - dependency
lockfiles, resource repositories/APIs, import/restore transactions, hardware
size/RAM/stack release budgets, and pinned immutable CI are now all
implemented. See
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
for current, maintained status; physical hardware-in-the-loop validation
(USB matrix, SoftAP/browser integration, power interruption, physical
control latency) genuinely remains open there.

## 0.1.0 — unreleased

**Release blocker — unauthenticated serial console.** Builds include an
interactive UART0 command console (`wifi-connect`, `wifi-status`) that takes
no session, CSRF token, or physical confirmation. It is an intentional
development interface under the trust model in `docs/SPEC.md` §16.5, where
physical possession of the board is the authorization. It MUST be excluded
from any image shipped to third parties; FIX1 §21.3 tracks that work.

Implemented foundations include the strict project toolchain, bounded macro
model/parser, safe storage primitives, USB/executor state machines, controls,
protected SoftAP/authentication, bounded HTTP/static serving, and a mobile-first
web application shell.

The project is not ready for release. Dependency lockfiles, a real ESP-IDF build,
persistent secure provisioning, resource repositories and APIs, import/restore
transactions, full frontend workflows, hardware testing, and size budgets remain
release blockers. See `docs/IMPLEMENTATION_STATUS.md` and
`docs/SECURITY_REVIEW.md`.
