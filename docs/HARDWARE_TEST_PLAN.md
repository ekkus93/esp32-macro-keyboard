# Hardware-in-the-loop test plan

No result in this document is marked passed until observed and recorded.

## USB host matrix

| Host | Enumeration | Reconnect | Suspend/resume | Text | Chords | Cancel | Disconnect mid-run |
|---|---|---|---|---|---|---|---|
| Linux | Not run | Not run | Not run | Not run | Not run | Not run | Not run |
| ChromeOS | Not run | Not run | Not run | Not run | Not run | Not run | Not run |
| Windows | Not run | Not run | Not run | Not run | Not run | Not run | Not run |

Use only harmless text in a disposable editor during validation.

## Chromebook workflow dry run

Create representative sets for HP Chromebook 11 G6 EE and one other model or a
generic test set. Verify explicit set selection, procedure order, manual
checkpoints, send confirmation, resend, skip, reset, cancellation, and no
automatic next execution.

## Persistence and fault tests

Record power interruption after each transaction phase, credential change,
firmware slot switch, full userdata, corrupt object, mount failure, and progress
updates. Verify old or new committed state, never partial active state.

## Physical controls

Measure cancel latency during a 10-second delay and rapid typing. Verify setup
reset and factory reset gestures cannot be triggered by normal short presses.
