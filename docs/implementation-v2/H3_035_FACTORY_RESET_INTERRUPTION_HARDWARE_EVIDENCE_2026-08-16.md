# H3-035 — factory-reset interruption hardware evidence — 2026-08-16

## Status

**PASS.** All four H3-035 items and the Phase H3 exit gate are evidenced on the
reference board.

## Exact conditions

| | |
| --- | --- |
| Firmware Git SHA (flashed) | `897038f2c3dce3bda142c6ec1968339b3d738dbd` |
| Firmware build id | `189918701cff8b00df3775cad4c8b773d9e2d5e` |
| Board | ESP32-S3R8, MAC `9c:13:9e:a8:77:38` |
| Host | Linux 7.0.0-28-generic x86_64 |
| Device | 192.168.88.111, station mode |
| Interrupt mechanism | RTS→EN pulse on an already-open serial port |

## 1. Normal factory reset and reprovisioning

`POST /api/v1/device/factory-reset` with a valid session, the administrator
password and the `FACTORY RESET` phrase. The device wiped provisioning,
credentials and blobs, dropped off the LAN, and booted into
`device is unprovisioned; starting protected V2 setup-only service` with no
recovery line — correct for an *uninterrupted* reset, because the durable
journal was cleared before the restart.

Reprovisioned over the UART console (`wifi-connect`, `setup-code`) and
`POST /api/v1/setup`. A blob seeded before the reset (id `7`) was absent
afterwards, with `usedBytes: 0`.

## 2 and 3. Interruption during cleanup, and boot resuming the reset

`device_controls_reset_engine_factory_reset()` runs synchronously inside the
HTTP handler:

```text
mark_factory_reset_pending -> erase_all_settings -> invalidate_all_sessions
-> delete_all_blobs -> cleanup_temporary_files -> schedule_restart
-> clear_factory_reset_pending
```

The window in which the durable journal reads PENDING therefore sits between the
request being sent and its response arriving, which makes an external reset
timeable rather than a blind race.

Three blobs (`9`, `10`, `11`) were seeded first so `delete_all_blobs` had
real work to do, widening that window. The socket was pre-established so only
handler execution sat inside the timing window, and EN was pulsed **530 ms**
after the request bytes left the host.

Boot output:

```text
I (6082) app_core: joined configured Wi-Fi network, IP address: 192.168.88.111
I (6092) app_core: stage complete: wifi
I (6092) app_core: stage complete: http
I (6092) main_task: Returned from app_main()
I (942) main_task: Calling app_main()
I (2042) app_core: stage complete: nvs
W (2182) app_core: factory reset recovery completed; continuing into unprovisioned setup
I (2182) app_core: stage complete: settings_init
I (2182) app_core: stage complete: settings_read
I (2222) app_core: stage complete: storage_mount
W (2222) app_core: device is unprovisioned; starting protected V2 setup-only service
I (2222) app_core: stage complete: authentication
I (2232) app_core: stage complete: setup_bootstrap
I (2232) app_core: stage complete: setup_code
I (2232) app_core: stage complete: setup_code_console
I (2252) app_core: run setup-code on the physical UART console to reveal the one-time setup code
I (2612) app_core: stage complete: wifi
I (2632) app_core: stage complete: http
I (2632) main_task: Returned from app_main()
```

`factory reset recovery completed` appears at 2182 ms — immediately after the
`nvs` stage and **before** `settings_init`, `settings_read` and
`storage_mount` — proving recovery ran ahead of ordinary startup rather than
the device presenting an ambiguous normal/setup state.

## 4. Reprovision and prove old blobs are absent

After reprovisioning the resumed reset's device, the blob list was empty and
`usedBytes` was 0; seeded blobs `9`, `10`, `11` were all gone. The resumed
reset finished the cleanup it had been interrupted during.

## Calibration record

The interrupt delay was bisected, and the misses are worth recording because
each rules something out:

| Delay (actual) | Outcome |
| --- | --- |
| 111 ms | reset never started — pulse landed during TCP setup |
| 301 ms | reset never started |
| 450 ms | reset never started |
| **530 ms** | **interrupted mid-cleanup; boot resumed the reset** |
| 551 ms | reset already complete before the pulse |
| 650 ms | reset already complete before the pulse |

The entire mark→clear sequence occupies roughly a 100 ms band. Seeding blobs
before the attempt is what made the vulnerable sub-window wide enough to hit
reliably.

## Two harness mistakes that produced misleading results

1. **Pulsing EN a fixed delay after asking urllib to POST** measured from the
   wrong instant. Wi-Fi RTT here is 33–77 ms, so connection setup consumed the
   window and the handler never ran. Fixed by pre-establishing the socket and
   timing from when the bytes leave.
2. **Opening the serial port after authenticating.** Opening it can pulse the
   auto-reset circuit, and sessions are RAM-only, so the session was destroyed
   before the request was sent. The device answered `401` and no reset
   occurred — which looked like a routing or payload fault but was self-inflicted
   ordering. Fixed by opening the port, letting it settle, and only then logging
   in.

Neither was a firmware defect. Both would have produced a false conclusion about
the device if taken at face value.
