# HTTP API reference

All operational API paths are same-origin and use the `/api/v1` prefix. Responses
are JSON. Read routes require a valid RAM-only session. Mutating routes additionally
require the matching CSRF token and accepted `Host` and `Origin` headers.

## Envelope

Success:

```json
{"ok":true,"data":{}}
```

Failure:

```json
{"ok":false,"error":{"code":"conflict","message":"..."}}
```

A supplied valid `X-Request-ID` is echoed. Otherwise the server generates one.
Unknown JSON fields, trailing data, invalid UUIDs, encoded path separators,
backslashes, traversal segments, unsupported media types, and oversized bodies
are rejected before handler mutation.

## Resource routes

### Session and settings

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/v1/auth/session` | Validate the current session |
| GET, PUT | `/api/v1/settings` | Read or update redacted non-secret settings |
| POST | `/api/v1/settings/change-password` | Change the administrator password |
| POST | `/api/v1/device/reset-settings` | Restore secure settings defaults |
| POST | `/api/v1/device/restart` | Respond, then restart |
| POST | `/api/v1/device/factory-reset` | Factory reset and restart |

Password change, settings reset, restart, and factory reset require physical
confirmation. Settings responses never contain password records, AP credentials,
session tokens, CSRF tokens, setup secrets, or encryption material.

### Sets

| Method | Route | Purpose |
| --- | --- | --- |
| GET, POST | `/api/v1/sets` | List or create sets |
| PUT | `/api/v1/sets/order` | Replace the complete set order |
| GET, PUT, DELETE | `/api/v1/sets/{setId}` | Read, revise, or delete one set |
| POST | `/api/v1/sets/{setId}/duplicate` | Atomically duplicate metadata, macros, procedures, and order without progress |
| POST | `/api/v1/sets/{setId}/select` | Select the active set |
| GET | `/api/v1/sets/{setId}/export` | Phase 18 package boundary |
| POST | `/api/v1/sets/import` | Phase 18 package boundary |

Set duplication requires a new UUID, name, and the source expected revision. The
new set and all copied set-owned objects begin at revision 1. Progress is not
copied. Export and import currently return explicit `503 Service Unavailable`;
they cannot report false success before the Phase 18 package service exists.

### Macros

Set-owned routes are under `/api/v1/sets/{setId}/macros`; shared routes are under
`/api/v1/global/macros`.

| Method | Suffix | Purpose |
| --- | --- | --- |
| GET, POST | collection | List or create |
| GET, PUT, DELETE | `/{macroId}` | Read, revise, or delete |
| POST | `/{macroId}/validate` | Compile without execution and return exact action count and duration |
| POST | `/{macroId}/duplicate` | Duplicate with a new UUID and name |
| POST | `/reorder` | Replace the complete macro order |

Referenced macros cannot be deleted. A conflict response includes a bounded list
of referencing procedure IDs.

### Procedures and progress

| Method | Route shape | Purpose |
| --- | --- | --- |
| GET, POST | `/sets/{setId}/procedures` | List or create procedures |
| GET, PUT, DELETE | `/sets/{setId}/procedures/{procedureId}` | Read, revise, or delete |
| POST | `/sets/{setId}/procedures/reorder` | Replace procedure order |
| GET, PUT, DELETE | `/sets/{setId}/procedures/{procedureId}/progress` | Read, replace, or reset progress |
| POST | `.../progress/complete` | Complete a step |
| POST | `.../progress/skip` | Skip a step with explicit JSON confirmation |

Stale progress remains visible after a procedure revision changes and must be
reset against the current procedure revision.

### Execution

| Method | Route | Purpose |
| --- | --- | --- |
| POST | `/api/v1/executions` | Load a persisted macro by ID and revision, compile, and transfer ownership |
| GET | `/api/v1/executions/current` | Poll the server-owned current execution |
| POST | `/api/v1/executions/current/cancel` | Cancel the current execution |
| POST | `/api/v1/executions/{executionId}/cancel` | Cancel only when the execution ID matches |

Execution submission never accepts macro source. `202 Accepted` is returned only
after the executor owns the validated plan. Physical confirmation is required
when the persisted setting enables it. Cancellation maps no current execution to
404, terminal or repeat cancellation to 409, internal failure to 500, unavailable
executor to 503, and accepted cancellation to 202.

### Storage and recovery boundaries

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/v1/diagnostics/storage` | Redacted mount and quarantine health |
| POST | `/api/v1/diagnostics/storage/check` | Run the bounded storage check boundary |
| GET | `/api/v1/diagnostics/quarantine` | List redacted quarantine records |
| GET | `/api/v1/backup` | Phase 18 package boundary |
| POST | `/api/v1/restore` | Phase 18 transactional-restore boundary |

Full diagnostics aggregation remains Phase 19. Backup and restore return explicit
503 responses until Phase 18 supplies package validation, secret exclusion, and
transactional activation.

## Status rules

Mutable object updates and deletes use expected revisions. Stale revisions and
reference conflicts return 409. Storage exhaustion returns 507. Malformed paths
and transport policy failures use 400/401/403/413/415 as appropriate; semantically
invalid resource JSON or macro source uses 422.

## Static files

The static handler serves only normalized paths below `/web`, rejects traversal
and encoded or backslash paths, negotiates pre-generated gzip variants, streams
in bounded chunks, and never maps into `/data`.
