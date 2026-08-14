# HTTP API reference

This is a human-readable index of the current v2 HTTP API. The normative,
byte-exact contract lives in `docs/SPEC_V2.md` §13 (request/response shapes,
status codes, and edge cases for every route) and is enumerated
machine-readably in `contracts/v2/api/routes.json` and
`contracts/v2/api/examples.json`, which `webapp/src/v2/apiRouteManifest.ts`
and `tests/v2_contracts/` validate against directly. When this file and
`docs/SPEC_V2.md` disagree, `docs/SPEC_V2.md` is authoritative — file an
issue against this page rather than trusting it.

A historical description of the retired v1 firmware-owned package/set/macro/
execution/backup API — deleted by the v1→v2 rebuild
(`docs/implementation-v2/V2_MIGRATION_MAP.md` §2.14, §8) — is kept at the
bottom of this file under "Archived: retired v1 API" for readers following
old links. None of it exists in current firmware; do not implement or test
against it.

## Current v2 API

All routes are under `/api/v1` and are JSON unless noted. Except for the
first-run setup and login routes, current v2 routes require the valid RAM-only
session described by SPEC_V2 §12.2. CORS is disabled, the session cookie is
`HttpOnly` and `SameSite=Strict`, and the development appliance profile uses no
separate CSRF token and performs no `Host`/`Origin` check. A product distributed
to third parties must revisit DNS-rebinding protection before release.
The maximum JSON request body is 8192 bytes; blob uploads accept only
`application/gzip` up to the blob size limit. There is no revision field,
`If-Match`, or checksum round trip anywhere in the v2 API — unlike the
retired v1 API, last successful write wins and repository snapshots are
immutable blobs (SPEC_V2 §13.1).

Success responses are the payload directly — there is no `{ok, data}`
wrapper. `GET /api/v1/blob/{blob_id}` is the one exception: it returns raw
`application/gzip` bytes, not JSON. Every JSON error response uses:

```json
{
  "error": {
    "code": "invalid_field",
    "message": "Device name exceeds 32 UTF-8 bytes.",
    "field": "deviceName"
  }
}
```

Macro-parse errors additionally carry `byteOffset`, `line`, and `column`
(SPEC_V2 §13.2).

### Route table

Grouped by domain; exact methods and paths from `contracts/v2/api/routes.json`.

**Setup** (SPEC_V2 §13.4)

| Method | Route |
| --- | --- |
| GET | `/api/v1/setup` |
| POST | `/api/v1/setup` |

**Auth and session** (SPEC_V2 §13.5)

| Method | Route |
| --- | --- |
| POST | `/api/v1/auth/login` |
| POST | `/api/v1/auth/logout` |
| GET | `/api/v1/auth/session` |

**Status and limits** (SPEC_V2 §13.6, §13.7)

| Method | Route |
| --- | --- |
| GET | `/api/v1/status` |
| GET | `/api/v1/limits` |

**Blob storage** (SPEC_V2 §13.8) — the repository blob store; the webapp owns
package/macro modeling client-side and stores/loads a whole compressed
repository as one opaque blob per snapshot

| Method | Route |
| --- | --- |
| GET | `/api/v1/blob` |
| POST | `/api/v1/blob` |
| GET | `/api/v1/blob/{blob_id}` |
| DELETE | `/api/v1/blob/{blob_id}` |

**Send** (SPEC_V2 §13.10) — submitting a compiled macro for USB HID execution

| Method | Route |
| --- | --- |
| POST | `/api/v1/send` |
| GET | `/api/v1/send` |
| DELETE | `/api/v1/send` |

**Settings** (SPEC_V2 §13.9)

| Method | Route |
| --- | ---- |
| GET | `/api/v1/settings` |
| PUT | `/api/v1/settings` |
| POST | `/api/v1/settings/change-password` |

**Device actions** (SPEC_V2 §13.12) — all report `connectionWillClose: true`;
reset-settings and factory-reset additionally require a typed `confirmation`
phrase in the request body, and factory-reset also requires `adminPassword`

| Method | Route |
| --- | --- |
| POST | `/api/v1/device/restart` |
| POST | `/api/v1/device/reset-settings` |
| POST | `/api/v1/device/factory-reset` |

**Diagnostics** (SPEC_V2 §13.13)

| Method | Route |
| --- | --- |
| GET | `/api/v1/diagnostics` |

That is the complete v2 route surface — 21 routes. There is no package,
macro, set, plural-execution, backup, or restore route family, and no
`/api/v1/diagnostics/storage` sub-route.

### Hardened mutation and recovery semantics

Several mutating routes have outcomes where treating every non-success response as
"nothing changed" would be unsafe. The authoritative wire contract remains
`docs/SPEC_V2.md`; the following is the current operational interpretation.

- **Password change:** `204` means the new password is durably stored, active in
  RAM, and all previous sessions were invalidated. `409 auth_state_incomplete`
  means the password **did change** and the new password is authoritative, but
  all old sessions could not be invalidated. A pre-commit concurrent password
  change is instead `503 conflict` and makes no credential change.
- **Factory reset:** `202` is returned only after reset ownership is durably
  established. Once accepted, later cleanup failure remains owned by the durable
  reset journal and resumes on reboot; while that journal is pending, status and
  diagnostics return `503 reset_recovery_required` rather than ordinary ready
  state.
- **Reset settings:** after the noncredential settings reset is durable, normal
  API authority is blocked until reboot. If reboot ownership cannot be
  established, the route returns `409 reset_settings_incomplete` instead of
  implying that no settings changed.
- **Confirmation-required send:** an accepted send starts in
  `awaiting_confirmation`; a settings-read failure rejects the send rather than
  defaulting confirmation off. Cancellation remains valid while confirmation is
  pending.
- **Execution recovery:** failure to establish current send state is represented
  as unavailable/unknown, never as confirmed no-send. Retry is status-only and
  must not issue another send POST; cancellation remains separately available.
- **Blob add:** `503 commit_uncertain` means the final blob may already exist
  because canonical rename succeeded but the final durability acknowledgement
  failed. The client must refresh/list/load and reconcile exact bytes before any
  new POST; it must not automatically retry the create.

See `docs/CURRENT_V2_HARDENED_BEHAVIOR.md` for the current cross-subsystem
behavior summary and proof boundaries.

### Static files

The static handler serves only normalized paths below `/web`, rejects
traversal and encoded or backslash paths, negotiates pre-generated gzip
variants, streams in bounded chunks, and never maps into the userdata
(`/data`) mount (`firmware/components/web_server/web_static_path.c`,
`web_server_static.c`). Blob access exists only through the JSON-structured
`/api/v1/blob*` routes above, not the static file handler.

---

## Archived: retired v1 API (historical reference only)

> **Retired.** Everything below documents the pre-rebuild, firmware-owned
> package/set/macro/execution/backup API. None of these routes exist in
> current firmware, which owns no package/macro/set repository or revision
> model at all (`docs/implementation-v2/V2_MIGRATION_MAP.md` §2.14, §8). Kept
> only so old links and history resolve to something explanatory; do not
> implement or test against it. (A few routes documented here under
> "Session and settings" — session validation, settings read/update,
> password change, restart, reset-settings, factory-reset — were **not**
> retired; they survived into the current v2 API unchanged in method and
> path, and are listed in the "Current v2 API" section above instead of
> repeated here.)

**Validation status (historical, describes the routes below):** every route
below was implemented and covered by host tests against the real
request-parsing and JSON-encoding code, at the time this file was the live
API reference. It predates the v1→v2 rebuild and was never re-verified
against v2 firmware.

The v1 envelope wrapped every response:

Success:

```json
{"ok":true,"data":{}}
```

Failure:

```json
{"ok":false,"error":{"code":"conflict","message":"..."}}
```

Package-download routes were the exception: they returned the raw validated
package JSON with its exact content length rather than wrapping it in the
success envelope.

### Sets

| Method | Route | Purpose |
| --- | --- | --- |
| GET, POST | `/api/v1/sets` | List or create sets |
| PUT | `/api/v1/sets/order` | Replace the complete set order |
| GET, PUT, DELETE | `/api/v1/sets/{setId}` | Read, revise, or delete one set |
| POST | `/api/v1/sets/{setId}/duplicate` | Atomically duplicate metadata, macros, and order |
| POST | `/api/v1/sets/{setId}/select` | Select the active set |
| GET | `/api/v1/sets/{setId}/export` | Export one deterministic macro-set package |
| POST | `/api/v1/sets/import` | Transactionally replace the selected set |
| POST | `/api/v1/sets/import-new` | Import a package as a brand-new set with a fresh identity |

The active set was recorded in the set index, not in settings. It was
reported as `activeSetId` in the settings response for convenience, but was
read-only there. Set duplication required a new UUID, name, and the source
expected revision; the new set and all copied set-owned objects began at
revision 1. Set export returned the raw, validated package with its exact
byte length.

Set replacement used `POST /api/v1/sets/import` with an exact wrapper:

```json
{
  "targetSetId": "11111111-1111-4111-8111-111111111111",
  "expectedRevision": 3,
  "package": {
    "schema_version": 1,
    "package_type": "set",
    "sets": [],
    "macros": []
  }
}
```

The package had to contain exactly one set whose ID matched `targetSetId`.
The current set revision had to match `expectedRevision`, and the
replacement revision came from the validated package. The server wrote a
durable transaction manifest, staged and validated the complete replacement
tree, atomically activated it, updated the set index, and recovered or
rolled forward interrupted activation on startup. Physical confirmation was
required before the request reached the mutation handler.

`POST /api/v1/sets/import-new` accepted the same raw package body (no
`targetSetId`/`expectedRevision` wrapper) and assigned every set and macro a
fresh identity with every revision reset to 1, rather than replacing an
existing set. It was non-destructive, so unlike set replacement it did not
require physical confirmation.

### Macros

Every macro belonged to exactly one set, so all macro routes were under
`/api/v1/sets/{setId}/macros`. There was no `/api/v1/global/macros`.

| Method | Suffix | Purpose |
| --- | --- | --- |
| GET, POST | collection | List or create |
| GET, PUT, DELETE | `/{macroId}` | Read, revise, or delete |
| POST | `/{macroId}/validate` | Compile without execution and return exact action count and duration |
| POST | `/{macroId}/duplicate` | Duplicate with a new UUID and name |
| POST | `/reorder` | Replace the complete macro order |

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

The three fields above were the complete request; any extra field was
rejected. Clients never submitted macro source — the server loaded the
persisted macro, checked its revision, and compiled that stored source. This
whole resource family was replaced in v2 by the single opaque `POST /api/v1/send`
route (SPEC_V2 §13.10), which takes compiled macro source directly from the
client-owned repository rather than loading a server-stored macro by ID.

### Storage, backup, and recovery

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/v1/diagnostics/storage` | Redacted mount health |
| POST | `/api/v1/diagnostics/storage/check` | Run the bounded storage check boundary |
| GET | `/api/v1/backup` | Download a deterministic full logical-repository backup |
| POST | `/api/v1/restore` | Restore a complete backup all-or-nothing |

`GET /api/v1/backup` took one repository lock and returned a raw package with
this exact top-level shape:

```json
{
  "schema_version": 1,
  "package_type": "backup",
  "sets": [],
  "macros": []
}
```

The package contained every set and its macros in order. It was
deterministic, bounded by `APP_IMPORT_PACKAGE_MAX_BYTES`, and revalidated
before response. It excluded administrator credentials, AP credentials,
sessions, CSRF material, setup secrets, provisioning state, encryption keys,
schema markers, and transaction evidence by construction.

`POST /api/v1/restore` accepted the raw backup package as its request body.
It required an authenticated administrator session, a matching CSRF token,
accepted same-origin transport policy, and physical device confirmation. The
server fully validated the package before mutation, wrote a durable
`PREPARED` manifest, materialized and validated a complete staged
repository, and then replaced only `set-index.json` and `sets/`. Every
durable phase was idempotent and resolved to either the complete old logical
repository or the complete restored logical repository; contradictory
evidence failed closed and was preserved.

Successful restore returned the ordinary success envelope with:

```json
{"restored":true,"reloadRequired":true}
```

This whole family — server-side backup/restore of a firmware-owned
repository — has no v2 equivalent by design: v2 snapshots are opaque
client-compressed blobs the webapp reads and writes directly through
`/api/v1/blob*` (SPEC_V2 §10); there is no server-side backup/restore
transaction manager to replace.

### Status rules (v1)

Mutable object updates and deletes used expected revisions. Stale revisions
and reference conflicts returned 409. Storage exhaustion returned 507.
Malformed paths and transport policy failures used 400/401/403/413/415 as
appropriate; semantically invalid resource JSON or macro source used 422.
None of this optimistic-concurrency machinery exists in v2 (SPEC_V2 §13.1).
