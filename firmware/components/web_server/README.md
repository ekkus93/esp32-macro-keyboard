# Web Server Component

This component owns the bounded ESP-IDF HTTP server, same-origin security checks,
JSON responses, and static frontend delivery.

Implemented API routes currently cover status, limits, login, logout, current
execution polling, and current-execution cancellation. Mutating routes require a
valid session, CSRF token, accepted `Host`, and accepted `Origin`.

Static paths are normalized, traversal and backslash paths are rejected, files are
streamed in bounded chunks, and pre-generated gzip variants are supported. Static
serving never maps into mutable user storage.

Set, macro, procedure, progress, execution-submission, settings, backup, restore,
and full diagnostics APIs remain unimplemented. HTTP handlers must never emit USB
reports directly; execution must pass through validated server-owned data and the
single-owner executor.
