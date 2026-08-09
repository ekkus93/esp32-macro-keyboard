# Hardware-in-the-loop test plan

No result in this document is marked passed until observed and recorded.

## USB host matrix

| Host | Enumeration | Reconnect | Suspend/resume | Text | Chords | Cancel | Disconnect mid-run |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Linux | Not run | Not run | Not run | Not run | Not run | Not run | Not run |
| ChromeOS | Not run | Not run | Not run | Not run | Not run | Not run | Not run |
| Windows | Not run | Not run | Not run | Not run | Not run | Not run | Not run |

Use only harmless text in a disposable editor during validation.

## Chromebook workflow dry run

Create representative packages for HP Chromebook 11 G6 EE and one other model
or a generic test package. Verify explicit package selection, macro order,
optional physical send confirmation, cancellation, and no automatic next
execution (`docs/SPEC_V2.md` §6.3-§6.4, §7.12). Version 0.2 has no procedures,
workflow steps, checkpoints, or progress-tracking concept - `docs/SPEC_V2.md`
§4 lists that as an explicit non-goal.

## Persistence and fault tests

Record power interruption during a blob-add commit (before and after the
`.tmp`→`.gz` rename, `docs/SPEC_V2.md` §10.3), credential change, firmware
slot switch, full userdata, corrupt object, and mount failure. Verify old or
new committed state, never partial active state. Unlike repository blobs and
device settings, current send/execution state has no persistence or
reboot-recovery contract in `docs/SPEC_V2.md` §13.10 or §18.3 and is not
expected to survive a reboot.

## Physical controls

Measure cancel latency during a 10-second delay and rapid typing. There is no
physical reset gesture to test against: reset settings and factory reset are
network requests gated by a typed confirmation phrase (and, for factory
reset, the administrator password), not a button press
(`firmware/components/device_controls/README.md`, `docs/SPEC_V2.md` §13.12).
