# Phase 1 Blocker — First-Run Setup-State Read Route

**Status:** Specification clarification required before route policy is frozen  
**Detected:** 2026-08-03

## Conflict

`docs/SPEC_V2.md` §12.3 says:

> An unprovisioned device exposes only setup state and setup submission, plus the
> static assets required for that UI.

The §13.3 route table defines `POST /api/v1/setup` but does not define a read
operation for setup state. It also does not state that `GET /api/v1/status` is
available without authentication while the device is unprovisioned.

Therefore the implementation cannot determine from the authoritative
specification whether first-run React should use:

1. `GET /api/v1/setup` plus `POST /api/v1/setup`;
2. unauthenticated `GET /api/v1/status` only while unprovisioned;
3. a different explicit setup-state route.

This choice affects the unauthenticated HTTP attack surface and must not be
invented inside a handler or route table.

## Work deliberately not completed

- No machine-readable route-access policy is being treated as authoritative.
- No new unauthenticated GET route has been implemented.
- No current v1 route has been adopted as a v2 requirement.
- Phase 1 route-contract completion remains open.

A prematurely created `contracts/v2/api/routes.json` was removed in the next
forward commit after the ambiguity was found.

## Recommended resolution

Use `GET /api/v1/setup` while unprovisioned. Return only:

```json
{
  "provisioned": false,
  "deviceName": "ESP32 Macro Keyboard"
}
```

After provisioning, both `GET` and `POST /api/v1/setup` should return `404` or
`409` according to the final contract, and normal session/status routes should
apply. This keeps the public pre-authentication surface minimal and avoids
exposing the full status object before login.

The recommendation is not yet a requirement and is not implemented until the
product owner approves it or the authoritative specification is amended.
