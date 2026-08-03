# Phase 1 Decision — First-Run Setup-State Read Route

**Status:** Resolved by product-owner approval  
**Detected:** 2026-08-03  
**Approved:** 2026-08-03

## Original conflict

The earlier `docs/SPEC_V2.md` wording said that an unprovisioned device exposed
setup state and setup submission, but the route table defined only
`POST /api/v1/setup`. It did not identify the setup-state read route or authorize
unauthenticated access to the full status endpoint.

Because this choice changes the unauthenticated HTTP attack surface, the
implementation correctly stopped rather than inventing a route policy.

## Approved resolution

The product owner approved an explicit, minimal setup-state route:

```http
GET /api/v1/setup
```

It is available without authentication only while the device is unprovisioned
and returns `200` with exactly:

```json
{
  "provisioned": false,
  "deviceName": "ESP32 Macro Keyboard"
}
```

The response must not contain the setup code, credentials, credential-presence
hints, sessions, firmware/build details, diagnostics, network status, or
repository information.

While unprovisioned, the only API routes available are:

```text
GET   /api/v1/setup
POST  /api/v1/setup
```

The static assets required for the setup UI also remain available. Every other
`/api/v1` route is unavailable.

After provisioning:

- `GET /api/v1/setup` returns `404`;
- `POST /api/v1/setup` returns `409`;
- normal authentication and authenticated route policy applies.

## Authoritative updates

The approved decision is now recorded in:

- `docs/SPEC_V2.md` §§12.3, 13.3, 13.4, 18, and 19;
- `docs/TODO_V2.md` tasks V2-011, V2-040, V2-051, V2-057, V2-080,
  V2-154, and the associated exit gates.

The earlier draft `contracts/v2/api/routes.json` remains deleted. A new
machine-readable route-access policy may be introduced only when it exactly
matches the approved specification and is consumed by tests.

## Remaining Phase 1 work

This decision removes the specification blocker. Phase 1 still requires:

- the setup-state response to be added to the shared API examples and TypeScript
  and C contract models;
- strict response-guard tests;
- a route-policy fixture or equivalent contract test for unprovisioned and
  provisioned access;
- successful execution of the focused v2 contract gate and the authoritative
  clean-checkout gate.

No Phase 1 exit claim is made by this decision record alone.
