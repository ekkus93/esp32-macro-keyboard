# HTTP API reference

All API paths are same-origin and use the `/api/v1` prefix. Responses are JSON.
Mutating routes require a valid RAM-only session, a matching CSRF token, and
accepted `Host` and `Origin` headers.

## Envelope

Success:

```json
{"ok": true, "data": {}}
```

Failure:

```json
{"ok": false, "error": {"code": "conflict", "message": "..."}}
```

## Implemented routes

| Method | Route | Purpose |
|---|---|---|
| GET | `/api/v1/status` | Redacted subsystem status and versions |
| GET | `/api/v1/limits` | Authoritative hard limits |
| POST | `/api/v1/auth/login` | Verify password and create a RAM-only session |
| POST | `/api/v1/auth/logout` | Invalidate the current session |
| GET | `/api/v1/executions/current` | Poll the current execution summary |
| POST | `/api/v1/executions/current/cancel` | Request cancellation of the current execution |

The cancellation route is the documented memory-conscious consolidation of the
execution-ID cancellation route from `docs/SPEC.md`. It still requires session,
CSRF, Host, and Origin validation.

## Required but not yet implemented

| Route group | Missing behavior |
|---|---|
| Setup/session | Persistent first-run provisioning and session introspection |
| Sets | List, create, read, update, delete, duplicate, select, import, export |
| Macros | Set/global CRUD, ordering, and validation summaries |
| Procedures | CRUD, ordering, progress, skip, and reset |
| Executions | Server-side macro lookup, compile, submit, and physical confirmation |
| Settings/device | Settings updates, password changes, restart, and reset |
| Recovery | Diagnostics, backup, and all-or-nothing restore |

Unimplemented routes are not registered. They therefore cannot accidentally
succeed without persistence, revision checks, or authorization.

## Static files

The static handler serves only normalized paths below `/web`, rejects traversal
and encoded or backslash paths, negotiates pre-generated gzip variants, streams
in bounded chunks, and never maps into `/data`.

## Status rules

Mutable object operations use an expected revision and return `409 Conflict` for
a stale revision. `202 Accepted` for execution means only that the executor owns
a validated plan; it is not completion. The current firmware does not yet expose
the execution-submission route.
