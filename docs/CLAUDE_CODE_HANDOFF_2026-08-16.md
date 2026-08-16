# Development handoff — esp32-macro-keyboard, 2026-08-15/16

**Repository:** `ekkus93/esp32-macro-keyboard`
**Handoff commit:** `28080a5` (`master`, pushed to `origin/master`, working tree
clean)
**Final release candidate:** `28359e8` — the last commit that changed shipped
code. Everything after it is evidence. `git diff 28359e8 HEAD -- ':!docs'` is
empty.
**Purpose:** orient the next session quickly. This is a status snapshot, **not**
a specification — if it ever conflicts with `docs/SPEC_V2.md`,
`docs/UI_UX_SPEC_V2.md` or `docs/TODO_V2.md`, those win. Read the project root
`CLAUDE.md` first; this assumes it.

This session ran across two calendar days; the file is dated to its last day so
the "newest handoff by date" rule in `CLAUDE.md` picks it.

Earlier handoff documents are superseded — do not start from them:

- `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` — predates the v1→v2 retirement.
- `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` — superseded in
  its own text.
- `docs/CLAUDE_CODE_HANDOFF_2026-08-10.md` — accurate when written; its
  "hardware-required" section is now discharged.

---

## 0. The one-paragraph version

The **post-v2 hardening program** is finished except for your review. Round 2
closed earlier in the session; Round 1 went from 137 open checkboxes to **1**,
and that one is the product-owner sign-off, which is not the implementer's to
check. The final candidate `28359e8` passes the clean-checkout software gate and
full physical device acceptance. Along the way the hardware acceptance found two
real production defects that the 66-test host suite could not see, because the
host tests **asserted the broken behaviour**. That story is §2 and it is the
most important thing in this document.

**This is not the same as the product being done.** `docs/TODO_V2.md` still has
85 open checkboxes (~74 substantive), concentrated in validation matrices and
exit gates — Android UI workflows, USB HID across hosts, network/auth, plus the
v2 final acceptance audit and sign-off checklist. They were not audited this
session. See §5.2 before assuming any of it is close.

---

## 1. What happened this session

49 commits, `a68f6dc..28080a5`. Five bodies of work, all finished:

1. **Large-file refactor** — `docs/REFACTOR_LARGE_FILES_TODO_2026-08-15.md`,
   T0–T10, 45/45.
2. **Post-v2 hardening Round 2** — closed entirely, 8 phases, closing SHA
   `3eee651`.
3. **V2-035 physical hardware evidence** — all seven scenarios collected.
4. **Post-v2 hardening Round 1 (H0–H12)** — driven to closure; see §1.4.
5. **Two production defects found by hardware acceptance and fixed** — §2.

### 1.1 Large-file refactor

Every tracked source file is now under 800 lines, including the new `.inc`
fragments. Largest remaining is `tests/host/test_provisioning.c` at 774, never in
scope. Splits ranged from `SnapshotsPage.tsx` 836→598 to `run-browser-tests.mjs`
2401→336; each was verified byte-exact by diffing extracted ranges against the
pre-edit file, and test counts never changed.

**T0 was a prerequisite that paid off immediately.** `check-format.sh` globbed
only `*.c`/`*.h`, so the 26 `.inc` fragments holding the large host suites' test
bodies were never format-checked — and two had already drifted. T0 added `*.inc`
plus a regression test proving the glob can't silently regress. It then rejected
all four fragments T7 created.

### 1.2 Round 2 hardening — closed

Every remaining phase was **already implemented**, with only the "run the gate"
step open, because the environments that wrote those fixes had Node 22 against
this repo's pinned 24.18.0, and correctly declined to claim a pass they hadn't
run. So it was a verification pass, and each regression test was **reverted and
re-run to prove it isn't vacuous** (F-018 through F-024). F-018 failing *two*
paths rather than three was the correct result — the `initialSend` effect already
had a cleanup before the fix.

### 1.3 V2-035 hardware evidence

All seven scenarios recorded on the board. **Provenance is not uniform and the
evidence says so:** `power_cycle_persistence` used an esptool RTS→EN reset
(reported `power_on`), adequate because no write is in flight;
`interrupted_upload_no_partial_final` used a **real power cut** — both USB cables
pulled 57,344 bytes into a 131,072-byte upload — because losing the flash chip's
supply mid-write is the thing that stage exists to prove.

### 1.4 Round 1 hardening (H0–H12) — 137 open → 1

Phases closed this session, each with committed evidence under
`docs/implementation-v2/`:

| Phase | What closed it |
| --- | --- |
| H1 | Send-confirmation, real hardware evidence (`8931f4e`) |
| H2 | Password change on hardware (`ee35608`); KDF 436.6 ms median, matching V2-041 exactly |
| H3 | Factory-reset **interruption** on hardware (`cbd65fa`) |
| H4 | Active-send recovery on the pinned toolchain (`3d31278`, `fd0ddf7`) |
| H5 | Storage error provenance + durability sanity (`d857eeb`, `5652fdb`) |
| H10 | Device Unity 12/12 (`cb9b157`), hardware matrix (`af19ac2`), exit gate |
| H12 | Clean checkout, authoritative gate, final hardware acceptance |

**H3-035 took three attempts and the failures are instructive.** Interrupting a
factory reset mid-cleanup needs the EN pulse to land inside the handler's
critical window. Attempt 1 pulsed 150 ms after asking urllib to POST — that
landed during TCP setup (Wi-Fi RTT here is 33–77 ms), so the handler never ran.
Attempt 2 pre-established the socket so timing started when bytes left the host,
but opened the serial port *after* logging in — and opening that port pulses the
auto-reset circuit, destroying the RAM-only session, producing a 401 that looked
like a routing bug but was self-inflicted ordering. Attempt 3 (open port → let
the reset settle → authenticate → send → pulse) worked. The scratch harnesses
keep those explanations in their docstrings.

**H12-123 and the final gate.** H12-123 is recorded item-by-item. The final
completion gate's "all P0/P1 findings" item had no enumerable list — `P0`/`P1`
appears nowhere in either tracker or spec except in that checkbox — so it was
left open and raised. **Phil ruled 2026-08-16 that the H-phase tasks are the
P0/P1 list**, which supplies the definition; it was then audited (every H-phase
task closed by an actual code fix, not by documenting a finding) and ticked.

---

## 2. The two restart-response defects — read this one

`docs/implementation-v2/H12_122_RESTART_RESPONSE_FINDING_2026-08-16.md`.

H12-122 is the first gate that puts a real HTTP client in front of the device,
and it found the same defect twice:

- **`6666e79`** — `/api/v1/device/restart` and `/api/v1/device/factory-reset`
- **`28359e8`** — `POST /api/v1/setup`

Both sites assumed `httpd_resp_send()` *delivers* bytes. It only queues them
with lwIP. Calling `esp_restart()` immediately afterwards reset the chip before
the promised `202` reached the wire, so every client saw a socket timeout.

The clue for the first was an asymmetry the code documented itself: reset-settings
worked, and its comment explained it "relies on its already-scheduled delayed
restart so its accepted response can drain." It worked *because* it had no fast
path. So the fix was to delete the fast path, not add a sleep.

The second was in `setup_submit_handler()`, whose comment read *"Mirrors
web_server_api.c's api_handler()"* — it was faithfully mirroring the code just
deleted as wrong. But setup had **no** already-scheduled restart to fall back on,
so it needed one *added* via `device_controls_restart()`; simply deleting the
call would have resurrected the original 2026-08-10 bug where setup committed
settings and never left setup mode.

**Why the host suite could not catch either.** Three assertions in the admin
route tests and one in the setup route test asserted `g_esp_restart_calls == 1`.
They did not miss the bug — they **encoded** it. The host fake captures responses
in memory, where lwIP is not involved, so the fake cannot observe the loss. All
now assert `0` immediate restarts alongside the unchanged `1` *scheduled*
restart, and reintroducing either fast path fails a named test (verified both
times).

Two guards pushed back correctly during the second fix and both were honored
rather than worked around:

- `check-h9-production-audit.py` refused the change until the new fallback
  (schedule fails → immediate reboot, because settings are already committed and
  stranding the device in setup mode is worse) was **explicitly classified**. It
  was registered in the audit, not reworded to slip past the regex.
- `run-h12-122-hardware.py` refuses `--flash-port == --console`. Honored by
  putting the board in download mode so native USB provided a genuinely distinct
  port — see §3.

---

## 3. Bench facts

These cost real time to establish. Don't re-derive them.

**Never join this host's Wi-Fi to the device SoftAP.** One radio; associating
severs this session, other concurrent Claude sessions on the box, and Phil's
remote access. Use station mode — the device is on the LAN.

**Identify every tty by sysfs vendor ID, every time.** `/dev/ttyACM1` was an
unrelated **Samsung Android phone** (`04e8:6860`) during the final acceptance.
Numbering shifts with plug order and unrelated devices land in the same range;
an earlier plan would have pointed esptool at the phone.

```bash
for n in $(ls /dev/ttyACM* | xargs -n1 basename); do
  d=$(readlink -f "/sys/class/tty/$n/device")
  while [ "$d" != "/" ] && [ ! -f "$d/idVendor" ]; do d=$(dirname "$d"); done
  [ -f "$d/idVendor" ] && echo "$n -> $(cat $d/idVendor):$(cat $d/idProduct)"
done
```

**Download mode needs no hands.** The CH340's DTR/RTS are wired to IO0/EN, so
esptool can drive the board into the ROM bootloader over the console port. With
`--after no_reset` it *stays* there, and native USB then enumerates as
`303a:1001` with its own tty — which is how the distinct flash port for H12-122
was obtained:

```bash
python3 -m esptool --port /dev/ttyACM0 --chip esp32s3 \
  --before default_reset --after no_reset read_mac      # leaves it in bootloader
```

Flashing over the CH340 alone also works end to end. Only **removing power**
(both cables) is still manual, and only V2-035 Stage 3 needs it — the bench hub
(Genesys `05e3:0610`) cannot switch port power; `~/stage3.sh` exists for that.

**`/dev/ttyACM0` is the CH340 bridge here**, not native USB — opposite the
"typically" column in `CLAUDE.md`'s port table. Native USB exposes no tty while
the HID app runs.

**Resets are not interchangeable.** `esp_restart()` → `software`.
`esptool --after hard_reset` drives RTS into EN — a real hardware reset reporting
`power_on`, fully scriptable. Only pulling power drops the flash supply.

**The console REPL comes up before startup finishes.** A command sent at the
first `keyboard>` prompt can run while `app_core` is still wiring subsystems —
`setup-code` returns "not in setup mode" for that reason alone. Wait for
`Returned from app_main()`.

**The setup code rotates on reboot, and opening the serial port often reboots the
board.** Read it and use it within one serial session.

**`POST /api/v1/setup` now returns its `202` — this was fixed in `28359e8`.**
The previous handoff recorded "times out rather than returning 202" as a device
quirk to work around. It was not a quirk, it was the defect in §2. Poll-for-the-
transition workarounds built on that assumption are no longer needed.

Bench credentials live in `~/.config/esp32-macro-keyboard/hil/` (dir 700, files
600) and never in the repo. Device name `Bench ESP32-S3`;
`requireSerialConfirmation` is toggled by the acceptance harness and left
**false**.

---

## 4. Current state

- `master` = `28080a5`, clean, pushed. Shipped code = `28359e8`.
- Software, from a clean clone of `28359e8`: `check-all.sh` exit 0 in 268 s;
  `run-tests.sh --sanitizers` 66/66; `generate-native-coverage.sh` exit 0 with
  policy coverage **95.6 %** lines / **82.7 %** branches / **100 %** functions,
  all above gate.
- Device: running that exact production build, provisioned, on the LAN at
  **`192.168.88.108`**, no test image flashed. (The IP changed from
  `192.168.88.111` — it was reprovisioned by the acceptance.)
- **Round 1 hardening: 384 done, 13 open** — 12 are §0.1 standing rules that are
  never ticked, plus the product-owner review.
- **Round 2 hardening: closed.** **V2-035 evidence: complete.**
- `docs/TODO_V2.md`: 513 done, 85 open — **not audited this session**; treat the
  count as a measurement, not an effort estimate.

---

## 5. What's left

**The post-v2 hardening program is finished; the v2 product ledger is not.**
Those are two different things and this section keeps them apart.

### 5.1 Hardening (Round 1 + Round 2) — one checkbox

**The product-owner review**, the last line of the final completion gate. Start
here if you are Phil:

- `docs/implementation-v2/H12_FINAL_CANDIDATE_ACCEPTANCE_2026-08-16.md` — clean
  checkout, gate, coverage margins, hardware acceptance, in one place.
- `docs/hardware-evidence/H12_122_FINAL_ACCEPTANCE_ESP32S3R8_2026-08-16.json` —
  the 43 named checks, verified secret-free.
- `docs/implementation-v2/H12_122_RESTART_RESPONSE_FINDING_2026-08-16.md` — §2
  above; the clearest case in this program of tests agreeing with broken code.

### 5.2 `docs/TODO_V2.md` — 85 open, and this is the real remaining work

**Not audited this session**, so the counts below are a measurement, not an
effort estimate. 11 of the 85 are §0.1 standing rules that are never ticked,
leaving ~74 substantive items. The concentrations:

| Open | Section |
| ---: | --- |
| 12 | V2-155 — Android UI workflow matrix |
| 11 | V2-156 — Final acceptance audit |
| 7 | Final sign-off checklist |
| 5 | Phase 15 exit gate |
| 4 | V2-151 — On-device Unity validation |
| 4 | V2-000 — Record the starting state |
| 3 each | V2-152 USB HID matrix, V2-130 responsive layout, Phase 13 exit gate |
| 2 each | V2-154 network/auth matrix, V2-140 dead v1 code, V2-133 accessibility, Phase 14 exit gate |

The pattern: the remaining v2 work is mostly **validation matrices and exit
gates**, several needing a phone or a second host — Android UI workflows, USB
HID across hosts, network/auth matrix — plus responsive-layout and accessibility
items. Some overlap evidence already collected under H10/H12 and may close by
citation rather than re-execution; that determination is itself unaudited. Audit
before planning.

### 5.3 Known-open specifics carried forward, none blocking

- **The firmware-side v1 dead-code audit still has not been done.**
  `web_setup_core.c`/`web_setup_json.c` are confirmed unreachable from production
  and **intentionally retained** with a `LEGACY / NOT SHIPPED` banner (`7292ba3`);
  that decision stands. Confirmed again this session: `web_setup_core_init` and
  `web_setup_core_restart` have zero non-test callers, so the setup fix was
  deliberately *not* routed through them.
- `docs/SPEC_V2.md` §17 lists six `[unattributed]` v1 requirements awaiting a
  keep/delete ruling from Phil.
- H10-103 honest limits, recorded not claimed: USB disconnect/reconnect is
  covered only as re-enumeration across resets (a cable pull mid-send needs a
  hand on the connector), and **ChromeOS and Windows were not performed**.

---

## 6. Process notes — mistakes worth not repeating

**A passing test proves nothing about whether it would catch the bug.** Every fix
this session was reverted and re-run first. This is also how §2's tests were
shown to have encoded the defect rather than missed it.

**A test can assert the bug.** When a fake cannot observe the real failure mode,
a green suite is evidence about the fake, not the system. Ask what the fake
*cannot* see before trusting it.

**`git checkout` to undo a temporary edit will discard your real work in that
file.** While revert-checking the restart fix, `git checkout web_server_api.c`
silently threw away the uncommitted fix living in the same file. Caught by
grepping for the fix rather than assuming, and re-applied. Snapshot the file
first (`cp` to scratch) when using checkout to undo an injected change.

**`str.replace` in an edit script fails silently.** It returns the input
unchanged when the pattern is absent, and a script printing "done" afterwards
will lie to you. **Assert the anchor exists and is unique before replacing** —
every doc-editing script in this session does.

**Quote your heredocs.** An unquoted `<<EOF` executes backticks and `$(...)` in
the body. This happened twice; the second time it ran `npm ci` against the real
repo (aborted on the engine check; lockfile and `node_modules` verified intact).
Use `<<'PY'` / `<<'EOF'` unless interpolation is genuinely wanted.

**Don't grep a file when the answer might be in it.** R6-060 files were deleted
on the stated grounds that "no reason to retain exists" — the reason was a banner
at the top of each file, missed because they were greped rather than opened.
Reverted in `a7f2209`.

**Verify a conclusion, not just its mechanism.** `CLAUDE.md` claimed apt
`clang-format` 18 was the format authority; it is the exact opposite (CI sources
`export.sh`, so 19 wins — measured 0 dirty under 19, 2 under 18). A `/init` pass
had reported that section "verified" after checking the mechanism it describes
but not its conclusion, and a hook was pinned to the wrong version on that basis
(`a280deb` → `75db7c2`, `4b78391`).

**Count tests with the test runner, not a grep.** `grep -c 'test('` undercounted
by 3 because a `test.each` block expands to four cases.

**When a first-party guard blocks you, satisfy it — don't reword around it.** The
H9 audit's fallback regex could have been evaded by rephrasing a comment. The
fallback was classified in the audit instead, which is the documented act.

**Harness bugs masquerade as firmware bugs.** Several "device defects" this
session were the test harness: a HID capture reader that could never observe its
stop flag, a `session_still_valid()` that caught exceptions from a client which
*returns* status (a false accusation against firmware), a console startup race
hit four times, and a wrong request body shape. Suspect the harness first when
the device otherwise looks healthy.

---

## 7. Quick command reference

```bash
# Full gate (source everything first — shell state does not persist between calls)
export PATH="$HOME/go/bin:$PATH"
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 24.18.0
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/check-all.sh            # only EXIT=0 is a pass
./scripts/generate-native-coverage.sh

# Device (station mode, no host Wi-Fi change — never join its AP)
curl -s -o /dev/null -w '%{http_code}\n' http://192.168.88.108/api/v1/diagnostics   # 401 = up

# Always identify ports by vendor ID before using them (see §3 for the sysfs loop)
lsusb | grep -E '303a|1a86'

# Put the board in download mode without touching it, then flash/accept
python3 -m esptool --port /dev/ttyACM0 --chip esp32s3 \
  --before default_reset --after no_reset read_mac

# Final hardware acceptance (from a clean clone of the candidate SHA)
python3 scripts/run-h12-122-hardware.py \
  --flash-manifest firmware/build/flash-manifest.json \
  --firmware-sha <40-char-sha> \
  --flash-port /dev/ttyACM2 --console /dev/ttyACM0 \
  --output docs/hardware-evidence/<new-path>.json

# V2-035 collector: Stages 1,2,4,5 scriptable; Stage 3 needs both cables pulled
bash ~/stage3.sh                  # operator runs this one and watches the banner
```
