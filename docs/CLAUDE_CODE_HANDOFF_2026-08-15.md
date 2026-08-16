# Development handoff — esp32-macro-keyboard, 2026-08-15

**Repository:** `ekkus93/esp32-macro-keyboard`
**Handoff commit:** `3f6b97b` (`master`, pushed to `origin/master`, working tree
clean)
**Purpose:** orient the next session quickly. This is a status snapshot, **not**
a specification — if it ever conflicts with `docs/SPEC_V2.md`,
`docs/UI_UX_SPEC_V2.md` or `docs/TODO_V2.md`, those win. Read the project root
`CLAUDE.md` first; this assumes it.

Earlier handoff documents are superseded — do not start from them:

- `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` — predates the v1→v2 retirement.
- `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` — superseded in
  its own text.
- `docs/CLAUDE_CODE_HANDOFF_2026-08-10.md` — accurate when written; 31 commits
  stale relative to this one, and its "hardware-required" section is now largely
  discharged (see §3).

---

## 1. What happened this session

31 commits, `a68f6dc..3f6b97b`. Three bodies of work, all finished:

1. **Large-file refactor** — `docs/REFACTOR_LARGE_FILES_TODO_2026-08-15.md`,
   tasks T0–T10, 45/45 checkboxes.
2. **Post-v2 hardening Round 2** — closed entirely, all 8 phases, closing SHA
   `3eee651`.
3. **V2-035 physical hardware evidence** — all seven required scenarios
   collected on the board and committed
   (`docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-15.json`, `3af03bc`).

### 1.1 Large-file refactor

Every tracked source file is now under 800 lines, including the new `.inc`
fragments. Largest remaining is `tests/host/test_provisioning.c` at 774, which
was never in scope.

| Task | File | Before | Largest after |
| --- | --- | ---: | ---: |
| T0 | `check-format.sh` `.inc` coverage | — | — |
| T1 | `SnapshotsPage.tsx` | 836 | 598 |
| T2 | `MacrosPage.tsx` | 966 | 619 |
| T3 | `SettingsPage.tsx` | 1048 | 499 |
| T4 | `v2-app-v2.test.tsx` | 956 | 320 |
| T5 | `v2-macros-page.test.tsx` | 1098 | 502 |
| T6 | `v2-snapshots-page.test.tsx` | 1502 | 526 |
| T7 | `test_web_settings.c` | 1220 | 601 |
| T8 | `test_web_server_administration_route.c` | 1229 | 388 |
| T9 | `run-v2-035-hardware.py` | 1081 | 585 |
| T10 | `run-browser-tests.mjs` | 2401 | 336 |

Every split was verified byte-exact by diffing the extracted ranges against the
pre-edit file; the only permitted differences were added `export` keywords and
trailing blank lines. Test counts never changed.

**T0 was a prerequisite that paid off immediately.** `check-format.sh` globbed
only `*.c`/`*.h`, so the 26 `.inc` fragments holding the large host suites' test
bodies were never format-checked — and two had already drifted. T0 added `*.inc`
plus a regression test proving the glob can't silently regress. It then rejected
all four fragments T7 created, for trailing blanks it would not have looked at
the day before.

### 1.2 Round 2 hardening — closed

Every remaining phase turned out to be **already implemented**, with only the
"run the gate" step open, because the environments that wrote those fixes had
Node 22 against this repo's pinned 24.18.0, or built in a sandbox rather than
the repository, and correctly declined to claim a pass they hadn't run.

So this was a verification pass, and each fix's regression test was **reverted
and re-run to prove it isn't vacuous**:

| Finding | Revert applied | Result |
| --- | --- | --- |
| F-018 send-tracker leak | dropped unmount cleanup | 2 failed |
| F-019 settings data loss | resync keyed back on `[settings]` | 1 failed |
| F-020 unbounded retry | unconditional `schedulePoll()` | 2 failed |
| F-024 health races | neutralised mutual exclusion | 3 runs of 3 failed |
| F-023 route divergence | removed a route; duplicated a case | both rejected |

F-018 failing **two** paths rather than three is the correct result — the
`initialSend` effect already had a cleanup before the fix, exactly as the
finding states. A third failure would have meant the test asserted something
other than the defect.

Two findings needed no new work, only recording: F-023's guard already existed
as `scripts/check-web-route-dispatch-sync.py` (added `46beba4` on 2026-08-12,
*two days after* the Round 2 spec, which is why the finding doesn't mention it),
and F-023's optional request-ID ordering half was already reconciled.

### 1.3 Hardware — the board is now a first-class part of the loop

The board was running firmware **35 commits stale**, discovered because a
console message didn't match the source tree. It was rebuilt from a clean HEAD
and reflashed:

- clean build: `gitCommit 897038f`, `gitDirty false`, `buildType production`
- all five images flashed with verified hashes; `nvs`/`userdata` untouched
- identity confirmed twice — boot-log ELF SHA and authenticated
  `/api/v1/diagnostics` `buildId`, both `189918701cff8b00…`

Then re-provisioned (NVS erased → setup mode → `setup-code` → `POST /api/v1/setup`),
put on the LAN in station mode, and taken through the full V2-035 collector.

All seven scenarios recorded: `numeric_ordering`, `delete_preservation`,
`power_cycle_persistence`, `interrupted_upload_no_partial_final`,
`reboot_temporary_cleanup`, `storage_full_507_preservation`,
`mount_failure_no_format`.

**Provenance is not uniform and the evidence says so.** `power_cycle_persistence`
used an esptool RTS→EN-pin reset, which the device reports as `power_on` and the
harness accepts; adequate there because no write is in flight.
`interrupted_upload_no_partial_final` used a **real power cut** — both USB
cables pulled 57,344 bytes into a 131,072-byte upload — because losing the flash
chip's supply mid-write is the thing that stage exists to prove.

---

## 2. Bench facts learned this session

These cost real time to establish. Don't re-derive them.

**Never join this host's Wi-Fi to the device SoftAP.** The machine has one
radio; associating with the AP severs this session, other concurrent Claude
sessions on the same box, and Phil's remote access. Use station mode — the
device is on the LAN and reachable over it.

**Resets are not interchangeable.** `esp_restart()` → `software` (harness
rejects). `esptool --after hard_reset` drives RTS into EN — a real hardware
reset reporting `power_on`, and fully scriptable. Only removing power drops the
flash chip's supply. Documented in `CLAUDE.md`.

**The bench USB hub cannot switch port power.** Genesys `05e3:0610` at `3-1`,
board on ports 2 and 3; `uhubctl` as root reports no compatible devices. There
is no software fix, so V2-035 Stage 3 will always need both cables pulled by
hand. `~/stage3.sh` exists for exactly that: the operator runs it, watches for
the `CUT POWER NOW` banner (~8 s in with `--chunk-delay 2`, ~56 s window), and
pulls both cables.

**`/dev/ttyACM0` is the CH340 bridge here**, not native USB — the opposite of
the "typically" column in `CLAUDE.md`'s port table. Native USB exposes no tty
while the HID app runs. Always confirm by vendor ID.

**The console REPL comes up before startup finishes.** A command sent at the
first `keyboard>` prompt can run while `app_core` is still wiring subsystems —
`setup-code` returns "not in setup mode" for that reason alone. Wait for
`Returned from app_main()`.

**The setup code rotates on reboot, and opening the serial port often reboots
the board.** Read it and use it within one serial session or you get
`401 incorrect setup code`.

**`POST /api/v1/setup` times out rather than returning 202** — the contract says
`connectionWillClose`/`restartRequired`. Verify by polling for the 200→404
transition, not by the POST's return value.

Bench credentials live in `~/.config/esp32-macro-keyboard/hil/` (dir 700, files
600) and never in the repo: `admin_password.txt`, `ap_passphrase.txt`,
`ap_ssid.txt`, `setup_code.txt`, `wifi_ssid`, `wifi_password`, `device_ip.txt`,
plus the completed evidence copy. Device name is `Bench ESP32-S3`;
`requireSerialConfirmation` is **false**, so sends don't block on a console
`confirm`.

---

## 3. Current state

- `master` = `3f6b97b`, clean, pushed.
- `./scripts/check-all.sh` → exit 0; `./scripts/generate-native-coverage.sh` →
  exit 0. 66/66 host tests, 6/6 v2 contract, 56 frontend files / 538 tests, 9/9
  Real Chrome browser workflows.
- Device: verified clean HEAD build, provisioned, on the LAN at
  `192.168.88.111`, left as found — zero blobs, `storage` and `usb` both `ready`.
- **Round 2 hardening: closed.** 0 unchecked tasks; the single unchecked
  sub-item is `R3-030a`, the deliberately unselected half of an either/or.
- **V2-035 hardware evidence: complete**, committed, validated.

---

## 4. What's left

**Round 1 hardening (`…HARDENING_TODO_2026-08-10.md`, phases H0–H12) is the main
body of remaining work: 137 unchecked checkboxes.** Many are acceptance criteria
rather than discrete tasks. **This session did not audit them**, so treat that
count as a measurement, not an estimate of effort. Round 1's H0-003 failure
matrix is still open, and `R8-084` records three rows it must cite when closed
(R2-021, R2-022, R3-031/H7-070).

`docs/TODO_V2.md` has 85 unchecked checkboxes; likewise unaudited here.

Known-open specifics carried forward:

- The firmware-side v1 dead-code audit still has not been done. `web_setup_core.c`
  and `web_setup_json.c` are confirmed unreachable from production but are
  **intentionally retained** with a `LEGACY / NOT SHIPPED` banner (`7292ba3`);
  that decision stands — see §5.
- `docs/SPEC_V2.md` §17 lists six `[unattributed]` v1 requirements awaiting a
  keep/delete ruling from Phil.

---

## 5. Process notes — including four mistakes worth not repeating

**A passing test proves nothing about whether it would catch the bug.** Every
fix verified this session was reverted and re-run first. Two of them would have
looked "verified" on a green suite alone.

**`str.replace` in an edit script fails silently.** It returns the input
unchanged when the pattern is absent, and a script that prints "done"
afterwards will lie to you. Three documentation edits this session reported
success while changing nothing; one was only caught by grepping for the text
afterwards. **Assert the anchor exists and is unique before replacing** — when
that assert was finally added it fired immediately, because the phrase occurred
three times.

**Don't grep a file when the answer might be in it.** R6-060 was re-decided and
its files deleted on the stated grounds that "no reason to retain exists". The
reason was a banner at the top of each file, missed because they were greped
rather than opened. Reverted in `a7f2209`. Compliance never lapsed, but
re-deciding a settled question is outside a verification pass and is the owner's
call.

**Verify a conclusion, not just its mechanism.** `CLAUDE.md` claimed apt
`clang-format` 18 was the format authority and esp-clang 19 produced false
positives. It is the exact opposite: `quality.yml` sources `export.sh` before
`check-all.sh`, so CI formats with 19 (measured: 0 dirty under 19, 2 under 18).
A `/init` pass had reported that section "verified" after checking the mechanism
it describes but not its conclusion — and a format hook was then pinned to the
wrong version on that basis (`a280deb`, fixed in `75db7c2`, doc in `4b78391`).

**Count tests with the test runner, not a grep.** `grep -c 'test('` undercounted
`v2-macros-page` by 3 because a `test.each` block expands to four cases. Use
vitest's own reported count as the before/after check.

**Don't claim hardware validation from a green gate.** T9 refactored the V2-035
collector and the gate passed, but the collector had not been run against a
board; that was stated explicitly at the time and only discharged later in this
session.

---

## 6. Quick command reference

```bash
# Full gate (source everything first — shell state does not persist between calls)
export PATH="$HOME/go/bin:$PATH"
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 24.18.0
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/check-all.sh            # only EXIT=0 is a pass
./scripts/generate-native-coverage.sh

# Device (station mode, no host Wi-Fi change — never join its AP)
curl -s -o /dev/null -w '%{http_code}\n' http://192.168.88.111/api/v1/diagnostics   # 401 = up

# Serial console: CH340 bridge, confirm by vendor ID first
lsusb | grep -E '303a|1a86'

# Hardware reset (real, reports power_on) — not a software restart
cd firmware/build && python -m esptool --chip esp32s3 --port /dev/ttyACM0 \
  --after hard_reset read_mac

# V2-035 collector: Stages 1,2,4,5 scriptable; Stage 3 needs both cables pulled
bash ~/stage3.sh                  # operator runs this one and watches the banner
```
