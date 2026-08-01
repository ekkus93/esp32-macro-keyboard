# HTTP API reference

**Validation status:** every route below is implemented and covered by host
tests against the real request-parsing and JSON-encoding code (fakes replace
only ESP-IDF/FreeRTOS/filesystem calls, per `tests/host/`'s design). One
execution submit→poll→cancel workflow is additionally covered by a real
Chrome browser test (`webapp/tests/browser/run-browser-tests.mjs`) against a
deterministic same-origin fixture, not the real device. No route has been
exercised against real device firmware serving real HTTP over real Wi-Fi to a
real browser client - that is FIX1 §20.3's still-open SoftAP/browser
integration matrix.

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

Package-download routes are the exception: they return the raw validated package
JSON with its exact content length rather than wrapping it in the success envelope.

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

`GET /api/v1/auth/session` returns `authenticated: true` plus the current CSRF token so a
same-origin page reload can restore the RAM-only frontend token. It never returns the
HttpOnly session token, and all API responses use `Cache-Control: no-store`.

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
| GET | `/api/v1/sets/{setId}/export` | Export one deterministic macro-set package |
| POST | `/api/v1/sets/import` | Transactionally replace the selected set |
| POST | `/api/v1/sets/import-new` | Import a package as a brand-new set with a fresh identity |

Set duplication requires a new UUID, name, and the source expected revision. The
new set and all copied set-owned objects begin at revision 1. Progress is not
copied. Set export returns the raw, validated Phase 18 package with its exact byte
length.

Set replacement uses `POST /api/v1/sets/import` with an exact wrapper:

```json
{
  "targetSetId": "11111111-1111-4111-8111-111111111111",
  "expectedRevision": 3,
  "package": {
    "schema_version": 1,
    "package_type": "set",
    "sets": [],
    "macros": [],
    "global_macros": [],
    "procedures": [],
    "progress": []
  }
}
```

The package must contain exactly one set whose ID matches `targetSetId`. The
current set revision must match `expectedRevision`, and the replacement revision
comes from the validated package. Referenced global macros are dependencies: they
must already exist with identical canonical content and are never modified by set
replacement. The server writes a durable transaction manifest, stages and validates
the complete replacement tree, atomically activates it, updates the set index, and
recovers or rolls forward interrupted activation on startup. Physical confirmation
is required before the request reaches the mutation handler.

`POST /api/v1/sets/import-new` accepts the same raw package body (no
`targetSetId`/`expectedRevision` wrapper) and assigns every set, macro,
procedure, and progress object a fresh identity with every revision reset to
1, rather than replacing an existing set. It is non-destructive, so unlike set
replacement it does not require physical confirmation. It reuses the same
package-parsing/validation pipeline as replacement, recovers deterministically
via a dedicated durable manifest type, and returns the committed set's
identity and revision.

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
| POST | `.../progress/complete` | Complete the current step |
| POST | `.../progress/skip` | Skip the current step with explicit JSON confirmation |

The server owns procedure ordering. Complete and skip requests must target the
stored `current_step_id`; an existing but non-current step returns `409 Conflict`
and leaves progress unchanged. An unknown step ID is invalid input rather than a
conflict. Stale progress remains visible after a procedure revision changes, but
stale progress actions return `409 Conflict` and are never silently reconciled.
The client must reload and explicitly reset progress against the current
procedure revision.

### Execution

| Method | Route | Purpose |
| --- | --- | --- |
| POST | `/api/v1/executions` | Load a persisted macro by ID and revision, compile, and transfer ownership |
| GET | `/api/v1/executions/current` | Poll the server-owned current execution |
| POST | `/api/v1/executions/current/cancel` | Cancel the current execution |
| POST | `/api/v1/executions/{executionId}/cancel` | Cancel only when the execution ID matches |

Standalone execution request:

```json
{
  "setId": "11111111-1111-4111-8111-111111111111",
  "macroId": "22222222-2222-4222-8222-222222222222",
  "macroRevision": 7
}
```

Procedure-context execution request:

```json
{
  "setId": "11111111-1111-4111-8111-111111111111",
  "macroId": "22222222-2222-4222-8222-222222222222",
  "macroRevision": 7,
  "sourceContext": {
    "procedureId": "33333333-3333-4333-8333-333333333333",
    "stepId": "44444444-4444-4444-8444-444444444444"
  }
}
```

`sourceContext` is optional, but when present it must contain exactly both IDs.
`null`, partial context, extra context fields, and flat top-level `procedureId` or
`stepId` fields are rejected. Clients never submit macro source. The server loads
the persisted macro, checks its revision and optional procedure-step context,
and compiles that stored source. `202 Accepted` is returned only after the
executor owns the validated plan. Physical confirmation is required when the
persisted setting enables it. Cancellation maps no current execution to 404,
terminal or repeat cancellation to 409, internal failure to 500, unavailable
executor to 503, and accepted cancellation to 202.

### Storage, backup, and recovery

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/v1/diagnostics/storage` | Redacted mount and quarantine health |
| POST | `/api/v1/diagnostics/storage/check` | Run the bounded storage check boundary |
| GET | `/api/v1/diagnostics/quarantine` | List redacted quarantine records |
| GET | `/api/v1/diagnostics` | Redacted build identity, heap, stack marks, storage capacity, quarantine count, current execution state, and per-subsystem health |
| GET | `/api/v1/backup` | Download a deterministic full logical-repository backup |
| POST | `/api/v1/restore` | Restore a complete backup all-or-nothing |

`GET /api/v1/backup` takes one repository lock and returns a raw package with this
exact top-level shape:

```json
{
  "schema_version": 1,
  "package_type": "backup",
  "sets": [],
  "macros": [],
  "global_macros": [],
  "procedures": [],
  "progress": []
}
```

The package contains every set, set-local macro, global macro, procedure, order,
and optional current progress. It is deterministic, bounded by
`APP_IMPORT_PACKAGE_MAX_BYTES`, and revalidated before response. It excludes
administrator credentials, AP credentials, sessions, CSRF material, setup
secrets, provisioning state, encryption keys, schema markers, quarantine, and
transaction evidence by construction.

`POST /api/v1/restore` accepts the raw backup package as its request body. It
requires an authenticated administrator session, a matching CSRF token, accepted
same-origin transport policy, and physical device confirmation. The server fully
validates the package before mutation, writes a durable `PREPARED` manifest,
materializes and validates a complete staged repository, and then replaces only
`set-index.json`, `sets/`, and `global/`. The restore transaction recovers at
startup before ordinary set transactions. Every durable phase is idempotent and
resolves to either the complete old logical repository or the complete restored
logical repository; contradictory evidence fails closed and is preserved.

Successful restore returns the ordinary success envelope with:

```json
{"restored":true,"reloadRequired":true}
```

The web application validates the backup locally, requires the exact typed phrase
`RESTORE FULL BACKUP`, waits visibly for physical confirmation, and reloads after
success so no stale in-memory repository state survives.

`GET /api/v1/diagnostics` excludes all secret material and raw macro source by
construction (its response type has no field capable of holding either);
`/api/v1/diagnostics/storage` and `/api/v1/diagnostics/quarantine` remain the
narrower, pre-existing storage- and quarantine-specific views alongside it.

## Status rules

Mutable object updates and deletes use expected revisions. Stale revisions and
reference conflicts return 409. Storage exhaustion returns 507. Malformed paths
and transport policy failures use 400/401/403/413/415 as appropriate; semantically
invalid resource JSON or macro source uses 422.

## Static files

The static handler serves only normalized paths below `/web`, rejects traversal
and encoded or backslash paths, negotiates pre-generated gzip variants, streams
in bounded chunks, and never maps into `/data`.
