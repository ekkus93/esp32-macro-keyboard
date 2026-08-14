# Web Server Component

This component owns the bounded ESP-IDF HTTP server, request-policy enforcement,
JSON and blob responses, and static frontend delivery.

The current v2 surface includes first-run setup; login/logout/session; status,
limits, and diagnostics; opaque blob list/create/load/delete; send create/poll/
cancel; settings and password change; and restart/reset-settings/factory-reset.
Authenticated routes require the RAM-only session cookie. Administrative routes
that require physical confirmation are gated by the shared request policy before
the handler runs.

The development appliance profile follows SPEC_V2 §12.2: CORS is disabled, the
session cookie is `HttpOnly` and `SameSite=Strict`, there is no separate CSRF
token, and there is no `Host`/`Origin` check. A product distributed to third
parties must revisit DNS-rebinding protection before release.

Static paths are normalized, traversal and backslash paths are rejected, files are
streamed in bounded chunks, and pre-generated gzip variants are supported. Static
serving never maps into mutable user storage. HTTP handlers never emit USB reports
directly; execution passes through validated server-owned data and the single-owner
executor.
