# Finding — `POST /api/v1/device/restart` never delivers its 202 — 2026-08-16

## Status

**FIXED and verified on hardware, 2026-08-16.** Kept as the record of the defect,
its root cause and its proof. See "The fix" below.

## What the contract promises

`contracts/v2/api/routes.json`:

```json
{ "path": "/api/v1/device/restart", "authentication": "session",
  "response": { "contentType": "application/json", "successStatus": 202 } }
```

`contracts/v2/api/examples.json`:

```json
"restartAccepted": { "accepted": true, "connectionWillClose": true,
                     "reprovisioningRequired": false }
```

The firmware is written to honour that. `web_server_api.c`'s
`restart_after_response()` exists precisely so the response goes out *first*:

```c
/* All three device-control actions schedule a delayed restart internally.
 * This post-response fast path intentionally preserves the existing immediate
 * restart behavior for restart/factory-reset only; reset-settings relies on
 * its already-scheduled delayed restart so its accepted response can drain. */
```

## What the device actually does

Measured on the reference ESP32-S3R8 running firmware
`07c40a4b1a5b9c63c494f4f6c8482e14f8222d7e`, authenticated session, raw socket so
no client library could hide the result:

```text
bytes received in 6.5s: 0
socket error: ConnectionResetError
```

**Zero bytes.** The connection is reset and no response line, no headers and no
body ever arrive. Through `urllib` the same request simply times out — 45.1 s
against a 45 s timeout, reproducibly.

## Why it matters

A client cannot distinguish "restart accepted" from "the device died". They are
the same observation: a connection that closes with nothing on it.

`webapp/src/v2/deviceActionsClient.ts`'s `restartDevice()` calls
`v2PostJson(..., isActionAccepted)`, so it parses and type-guards a response
that never arrives. The V2-121 destructive-action flow is built on that
acknowledgement.

The same shape affects `POST /api/v1/device/factory-reset`, which also routes
through `restart_after_response()`. That was observed during H3-035 (a 60 s
client timeout) and attributed at the time to `connectionWillClose: true`. This
finding supersedes that reading: `connectionWillClose` describes the connection
closing *after* the accepted response, not instead of it.

`POST /api/v1/setup` behaves the same way, and is documented in this session's
notes as "times out rather than returning 202". That was recorded as expected
behaviour. It should be re-examined against the same contract.

## What is not yet known

- Whether the response is queued but lost when `esp_restart()` runs before
  lwIP flushes, or never written at all.
- Whether this ever worked. The H12-122 harness contains a
  `device restart returned 202` assertion, implying it did at some point, so a
  regression is plausible but unproven.
- Whether the reset-settings path — which the comment says deliberately relies
  on a *delayed* restart "so its accepted response can drain" — is unaffected.
  If it is unaffected, that asymmetry is the likely root cause and the fix is to
  give restart and factory-reset the same drain.

## Effect on the release gate

- **H12-120 and H12-121 are met** — see `H12_120_121_CLEAN_CHECKOUT_2026-08-16.md`.
- **H12-122 is blocked here.** Everything before this step passed: exact-SHA
  release flash with all five images hash-verified, on-device diagnostics
  matching the manifest ELF provenance, an active send typing the exact expected
  text and releasing all keys, the confirmation-required flow with a real UART
  `confirm`, cancel producing no key-down report, a byte-identical snapshot
  save/load round trip, and a password change that invalidated the active
  session. The run reached `restart_smoke()` and stopped there.
- **The Phase H12 exit gate and the final completion gate remain open.**

No H12 checkbox is being ticked from a partially-passing acceptance run.

## Root cause

`device_controls` already schedules the real restart for all three device
actions via an `esp_timer`, `DEVICE_CONTROLS_RESTART_DELAY_MS` = 500 ms:

```c
app_error_code_t device_controls_restart(void) {
    const device_controls_reset_ops_t operations = reset_operations();
    return device_controls_reset_engine_restart(&operations, DEVICE_CONTROLS_RESTART_DELAY_MS);
}
```

On top of that, two call sites restarted the chip *immediately* once the
response had been handed to the HTTP server — `api_handler()` for synchronous
routes and the async worker for confirmation-gated ones:

```c
if (should_restart) {
    esp_restart();
}
```

`httpd_resp_send()` only queues bytes with lwIP. The immediate `esp_restart()`
preempted the 500 ms timer and reset the chip before the TCP stack put the
segment on the wire, so the peer saw a reset and no response. Reset-settings was
unaffected precisely because it had no such fast path — which is what its own
code comment says, and why the asymmetry was the clue.

## The fix

Delete the fast path. The scheduled restart is now the only restart, so the
accepted response drains first. That deletes code rather than adding a timing
hack, and makes all three device actions behave identically.

Removed: `restart_after_response()`, the `out_should_restart` plumbing through
`web_api_handle_call()` / `web_api_handle_call_with_body()` and
`web_server_internal.h`, both `esp_restart()` call sites, and the now-unused
`esp_system.h` includes.

## Verification

**On hardware**, same board, same request that previously returned nothing:

```text
HTTP/1.1 202 Accepted
Content-Type: application/json
Content-Length: 75
Cache-Control: no-store
X-Request-ID: 4d33e6a2-c278-4f16-bb77-4c0a1dbfb970

{"accepted":true,"connectionWillClose":true,"reprovisioningRequired":false}
```

Byte-for-byte the contract's `restartAccepted` example. The device still
restarts: diagnostics afterwards report `resetReason: software` with an uptime
of 48 s, so the scheduled restart fired after the response drained.

**Regression coverage.** Three host assertions previously encoded the defect —
one in `admin_route_session_tests.inc` for restart and two in
`admin_route_reset_tests.inc` for factory reset — asserting
`g_esp_restart_calls == 1`. They could not observe the real failure, because the
host fake captures the response in memory where lwIP is not involved. They now
assert `0` immediate restarts alongside the unchanged `1` *scheduled* restart,
with a comment naming what a non-zero count would mean. Reintroducing the fast
path fails `web_server_administration_route`, verified.

`./scripts/check-all.sh` exit 0 with 66/66 host tests on the fixed tree.

## Scope note

`POST /api/v1/device/factory-reset` shared the same fast path and is fixed by
the same change. `POST /api/v1/setup` is a separate handler
(`setup_submit_handler`), was never routed through `restart_after_response()`,
and was unchanged by the fix above; its own timeout-instead-of-202 behaviour was
left explicitly unexamined here — see the second instance below.

## Second instance — `POST /api/v1/setup` (found 2026-08-16, fixed `28359e8`)

The scope note was right to flag it. With the restart defect fixed, the H12-122
acceptance ran 34 further checks and then failed at reprovisioning: after the
factory reset entered setup mode, `POST /api/v1/setup` never answered and the
client timed out in `provision_device.request_json`.

Same defect, different route. `setup_submit_handler()` ended with:

```c
if (send_result == ESP_OK && accepted.restart_required) {
    esp_restart();
}
```

and its comment said it was *"Mirrors web_server_api.c's api_handler()"* — it
was faithfully mirroring the code deleted as wrong in `6666e79`. The reset again
preempted the lwIP flush, so the 202 never reached the wire.

The fix routes the reboot through `device_controls_restart()`, the same
`DEVICE_CONTROLS_RESTART_DELAY_MS` timer every other device action uses. Unlike
the admin routes there was no already-scheduled restart to fall back on, so this
route needed the scheduled restart *added* rather than the fast path merely
removed — the original 2026-08-10 bug that comment records (setup committing
settings but never leaving setup mode) must not come back.

Because settings are already committed when the reboot is requested, a failure
to schedule one must not strand the device in setup mode: scheduling failure
falls back to an immediate reboot, losing the response but leaving setup mode.
That trade is classified in `scripts/check-h9-production-audit.py`, which
correctly refused the change until it was registered.

`test_web_server_setup_route.c` asserted `g_esp_restart_calls == 1` — the same
bug-encoding assertion as the admin routes. It now requires a scheduled restart
and **no** immediate one, plus a new case covering the fallback branch.
Reintroducing `esp_restart()` fails `web_server_setup_route` at
`test_web_server_setup_route.c:296`, verified.

Verified on hardware: the H12-122 acceptance at `28359e8` reprovisions
successfully (`setup accepted; waiting for automatic restart`, device back at
`192.168.88.108`) and passes end to end.

## Why the pattern survived two routes

Both sites were written to the same wrong model — that `httpd_resp_send()`
delivers bytes, so restarting immediately after it is safe. Both had host tests
that *asserted* the immediate restart, because the host fake captures responses
in memory where lwIP is not involved and therefore cannot observe the loss. The
tests did not miss the bug; they encoded it. Only hardware could distinguish the
two behaviours, which is why both instances surfaced in H12-122 and not in the
66-test host suite.
