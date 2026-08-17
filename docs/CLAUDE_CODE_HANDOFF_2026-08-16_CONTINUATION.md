# Development handoff — esp32-macro-keyboard, 2026-08-16 (continuation)

**Written for a move to a different computer.** §2 lists everything that is
machine-local and will not come across with the repository — read it before
running anything hardware-related.

**Repository:** `ekkus93/esp32-macro-keyboard`
**Handoff commit:** `0a8f1f2` (`master`, pushed, working tree clean)
**Purpose:** orient the next session. A status snapshot, **not** a
specification — if it conflicts with `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`
or `docs/TODO_V2.md`, those win. Read the project root `CLAUDE.md` first.

Supersedes `docs/CLAUDE_CODE_HANDOFF_2026-08-16.md`, written mid-session on the
same day; that file is accurate up to `28359e8` but predates roughly half this
session's work. Two files share today's date, so prefer **this one** — the
"newest handoff by date" rule in `CLAUDE.md` cannot break the tie on its own.

---

## 0. Read this first

**The release evidence is stale, and that is the single most important fact
here.** H12-120/121/122 were last run end-to-end at `9de20b6`. Since then
**five commits changed shipped code** — 29 files, +608/−1778:

| Commit | What |
| --- | --- |
| `f089d5b` | two P0 webapp fixes (sign-in, ID minting) |
| `20c805f` | v1 provisioning store and parser stack deleted |
| `ab34d1b` | `stack` console command |
| `4fd39f3` | UI redesign |
| `0a8f1f2` | empty state and storage summary |

So `docs/implementation-v2/H12_FINAL_CANDIDATE_ACCEPTANCE_2026-08-16.md` no
longer describes the shipped image. **Before any release claim, re-run the H12
sequence on the new candidate**: clean clone → `check-all.sh` →
`run-tests.sh --sanitizers` → `generate-native-coverage.sh` →
`scripts/run-h12-122-hardware.py`. This has already happened three times today
(`07c40a4b` → `6666e79` → `28359e8` → `9de20b6`); each time a source change
invalidated it, the whole sequence was re-run rather than carried forward. Keep
that discipline.

---

## 1. Where things stand

- `master` = `0a8f1f2`, clean, pushed. `./scripts/check-all.sh` exits 0:
  **63/63** host tests, **543** frontend tests across 56 files, all Real Chrome
  workflows and axe-core scans.
- **Post-v2 hardening Round 1: 13 open** — 12 are §0.1 standing rules that are
  never ticked, plus the product-owner review. **Round 2: closed** (0 open).
- **`docs/TODO_V2.md`: 47 open / 551 done.** 11 of the 47 are standing rules,
  leaving ~36 substantive. See §4.
- The device on the bench runs the current tree and is provisioned.

### What this session did (65 commits, `a68f6dc..0a8f1f2`)

1. **Closed post-v2 hardening Round 1** from 137 open checkboxes to 1 — phases
   H1, H2, H3, H4, H5, H10, H12, plus H12-123 and most of the final gate.
2. **Found and fixed four production defects that only real hardware exposed**
   (§3). Two of them made the product unusable on a phone.
3. **V2-155 Android UI workflow matrix** — all twelve workflows on a real LG G6.
4. **V2-156 final acceptance audit** — all 108 normative requirements in both
   specs walked and linked.
5. **V2-140 firmware dead-code audit and removal** — 2,731 deletions.
6. **UI redesign** (§5).

---

## 2. What does NOT transfer to another machine

Everything in this section is local to the old bench. None of it is in the
repository, and most of it is wrong by default on a new machine.

**Bench credentials are not in git.** They live in
`~/.config/esp32-macro-keyboard/hil/` (dir `700`, files `600`):
`admin_password.txt`, `ap_passphrase.txt`, `ap_ssid.txt`, `device_ip.txt`,
`setup_code.txt`, `wifi_ssid`, `wifi_password`, plus collected evidence JSON.
Either copy that directory across by hand, or factory-reset the device and
re-provision, which regenerates all of them. Deliberately never committed.

**Device address is DHCP.** It was `192.168.88.108` on the `kensington2`
network. Do not hardcode it; read `device_ip.txt`, or re-derive it from the
serial console (`wifi-connect`). Several harnesses default to that literal —
`tests/hardware/android_browser.mjs` (`HIL_DEVICE_ORIGIN`) and
`tests/hardware/run-v2-desktop-matrix.mjs` (`--origin`) both take an override.

**Serial port numbering is plug-order dependent and was actively misleading.**
On the old bench `/dev/ttyACM0` was the CH340 console — the opposite of the
"typically" column in `CLAUDE.md`'s port table — and `/dev/ttyACM1` was an
unrelated Samsung phone. Always identify by USB vendor ID before using a port:

```bash
for n in $(ls /dev/ttyACM* | xargs -n1 basename); do
  d=$(readlink -f "/sys/class/tty/$n/device")
  while [ "$d" != "/" ] && [ ! -f "$d/idVendor" ]; do d=$(dirname "$d"); done
  [ -f "$d/idVendor" ] && echo "$n -> $(cat $d/idVendor):$(cat $d/idProduct)"
done
```

`1a86:55d3` is the CH340 console, `303a:1001` the ESP32 in download mode,
`303a:4001` the running HID app.

**The Android phone's adb serial is hardcoded as a default.**
`tests/hardware/android_browser.mjs` defaults to `LGH87250967ab9` (the LG G6);
override with `HIL_ANDROID_SERIAL`. Two Android devices were attached, so
always pass `-s <serial>` to `adb`.

**Toolchain must be reinstalled to exact versions** — ESP-IDF **v5.5.5** at
`~/esp/esp-idf-v5.5.5`, Node **24.18.0** via nvm, plus the go/apt/pip tools in
`CLAUDE.md`. `scripts/install-esp-idf.sh` handles ESP-IDF. Note the
format authority is esp-clang's `clang-format` 19, not apt's 18: run
`check-format.sh` only from a shell that has sourced `export.sh`, or you get
false failures.

**The hardware itself.** The ESP32-S3R8 and the LG G6 are physical. Every
hardware-gated item in §4 needs the board attached.

**One environmental quirk worth knowing:** the old machine continuously created
and destroyed Docker `veth` interfaces, which Chromium sees as network changes
and which aborted in-flight navigations with `ERR_NETWORK_CHANGED`. The desktop
harness retries for that reason. If the new machine is quiet, the retries simply
never fire.

---

## 3. The four defects real hardware found

All fixed, all with regression tests that fail when the fix is reverted. They
share one root cause worth internalising: **the tests did not run in the
product's deployment context**, so each bug was invisible to a green suite.

1. **Restart and factory reset never delivered their `202`** (`6666e79`).
   `httpd_resp_send()` only queues bytes with lwIP, so calling `esp_restart()`
   immediately after reset the chip before the response reached the wire.
2. **`POST /api/v1/setup` had the same defect** (`28359e8`) — in a handler whose
   comment said it was "mirroring" the code just deleted as wrong.
   *For both: three host assertions had `g_esp_restart_calls == 1`. The tests
   did not miss the bug, they **encoded** it, because the host fake captures
   responses in memory where lwIP is not involved.*
3. **Sign-in was impossible** (`f089d5b`). `isSessionStatus` demanded the
   session lifetimes equal the configured maxima exactly, but firmware sends the
   *remaining* lifetime, so a real login (`86399`) was rejected as
   `invalid_response`. Unit tests always returned the exact maxima.
4. **The first screen that mints an ID crashed** (`f089d5b`).
   `crypto.randomUUID` is secure-context-only and the device serves plain HTTP;
   measured on the phone, `isSecureContext === false`. Browser tests run on
   `localhost`, which *is* a secure context.

Full analysis:
`docs/implementation-v2/H12_122_RESTART_RESPONSE_FINDING_2026-08-16.md` and
`docs/implementation-v2/V2_155_ANDROID_UI_WORKFLOW_MATRIX_2026-08-16.md`.

---

## 4. What is left

### 4.1 Needs the product owner (12)

Phase 15's exit gate (5) and the final sign-off checklist (7) in `TODO_V2.md`,
plus the last line of the hardening tracker's final completion gate. Several are
genuinely owner judgements ("Product owner performs the final v0.2 acceptance
review"); the rest are downstream of §0's re-run.

### 4.2 Blocked on hardware not on the old bench (6)

- **V2-152 ChromeOS and Windows** — no test machines. Honestly recorded as *not
  performed*, never inferred from the Linux run.
- **V2-152 send timeout on hardware** — needs a *ruling*, not a rig: the
  compiler caps estimated duration at 300 s while the executor deadline is
  310 s, so no valid macro can reliably cross it.
- **V2-132 foldable** — the desktop/tablet matrix covers desktops and
  tablet-dimension coarse-pointer viewports, but the checkbox names foldables
  and §0.1 forbids ticking a compound item while any named behaviour is
  unverified.
- **V2-130 display cutouts** — needs a notched device.
- **Phase 13 manual screen-reader pass** — needs a human with a screen reader.
  Focus order is already verified by a real-browser keyboard test (47/47
  elements, DOM order, no positive `tabindex`), which is *not* the same thing.

### 4.3 Long-running but doable (1)

**V2-154 idle and absolute session expiry** — 24-hour and 7-day wall-clock waits
on the board. The device deliberately has no wall clock (SPEC_V2 §5.4), so there
is nothing to fast-forward.

### 4.4 Ordinary work, no special hardware (~17)

`V2-000` baseline (4) and Phase 0 gate (1) — possibly moot this late, worth a
ruling. `V2-013` conformance corpus, `V2-050` malformed paths/methods, `V2-056`
diagnostics fields, `V2-057` contract/security matrix, `V2-122` diagnostics UI,
`V2-131` web app manifest, `V2-143` mockups, and the Phase 1/5/14 exit gates.

**`V2-056` needs a decision, not code.** Its open item asks for `stack` data in
`GET /api/v1/diagnostics`, but SPEC_V2 §13.13 fixes that schema, has no `stack`
field, and says contract definitions "may add fields only through an explicit
specification update". The spec is frozen. The two stack accessors the V2-140
audit found unwired are therefore exposed on the **trusted UART console** as a
`stack` command instead (`ab34d1b`), which needs no contract change. If you want
it on the API, that is a spec change only the owner can authorise.

---

## 5. The UI redesign

Direction **"PBT and petrol"**, grounded in the device's own materials: three
surface tones (desk → panel → keycap), legend ink, one petrol-teal actuation
colour, amber reserved solely for the status lamp. Signature is **key travel** —
every control has a bottom edge that collapses on press. System fonts only,
because `verify-no-remote-assets.sh` fails the build on any webfont.

Constraints that are load-bearing and must not be "tidied away":

- **Status badges are deliberately not uppercased**, unlike key legends.
  `text-transform` rewrites `innerText`, which silently broke a real-browser
  assertion matching `"Unsaved changes"`. Legends label a key; status text is
  prose.
- **Each badge carries a distinct lamp *shape*** as well as a colour, so
  UI_UX_SPEC_V2 §5's "colour is never the only indicator" holds without hue.
- **Contrast was computed, not eyeballed.** axe-core caught `.status-neutral` at
  4.47:1; checking `--legend-soft` against all three grounds then found the same
  failure on the shell (4.34:1) that axe had not rendered. Both are now ≥5.2:1.

Four bugs surfaced while restyling, three pre-existing: Tailwind's preflight
resets headings to `inherit` (so every `h1`/`h2`/`h3` rendered at body size);
`input { width: 100% }` applied to radios, which had no override; snapshot
actions were forced to `width: 100%`, giving four full-width stacked buttons with
Delete as loud as Load; and `flex: 1 1 16rem` grew *vertically* once the card
flipped to a column, opening a tall void.

---

## 6. Process notes that earned their place

**A test can assert the bug.** Four times this session a test encoded the
defective behaviour rather than missing it. When a fake cannot observe the real
failure mode, a green suite is evidence about the fake.

**`git checkout` to undo a temporary edit discards real work in that file.**
This happened twice — once losing an uncommitted fix, once reverting a
migration. Copy the file to scratch *before* using checkout to undo an injected
change.

**Assert the anchor before `str.replace`.** It returns the input unchanged when
the pattern is absent and a script that prints "done" will lie to you. Every
doc-editing script here asserts the anchor exists and is unique.

**Check exit codes directly, not through a pipe.** A guard that printed errors
while `head` swallowed its status looked like it exited 0.

**Quote heredocs.** An unquoted `<<EOF` executes backticks; this once ran
`npm ci` against the real repo.

**Verify the conclusion, not just the mechanism.** `CLAUDE.md` once claimed apt
`clang-format` 18 was the format authority; it is the opposite, and a hook was
pinned to the wrong version on that basis.

**When a first-party guard blocks you, satisfy it rather than reword around it.**
The H9 audit's fallback regex could have been evaded by rephrasing a comment; the
fallback was classified in the audit instead.

**Suspect the harness first.** Several "device defects" were the test rig: a HID
capture reader that could never observe its stop flag, a session check that
caught exceptions from a client which *returns* status, and a console startup
race hit four times.

---

## 7. Quick command reference

```bash
# Environment first — shell state does not persist between tool calls
export PATH="$HOME/go/bin:$PATH"
export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use 24.18.0
. "$HOME/esp/esp-idf-v5.5.5/export.sh"

./scripts/check-all.sh            # only EXIT=0 is a pass
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh

# Device reachability (read the address, do not assume it)
curl -s -o /dev/null -w '%{http_code}\n' \
  "http://$(cat ~/.config/esp32-macro-keyboard/hil/device_ip.txt)/api/v1/diagnostics"   # 401 = up

# Download mode without touching the board: CH340 DTR/RTS drive IO0/EN
python3 -m esptool --port <ch340> --chip esp32s3 \
  --before default_reset --after no_reset read_mac

# Flash only the web assets (webfs partition)
npm --prefix webapp run build && ./scripts/build-webfs-image.sh
python3 -m esptool --chip esp32s3 --port <esp32> --baud 921600 \
  write_flash 0x520000 firmware/build/webfs.bin
# then pulse EN over the console, or the board stays in download mode

# Browser matrices
node tests/hardware/run-v2-desktop-matrix.mjs --origin http://<ip>
HIL_ANDROID_SERIAL=<serial> HIL_DEVICE_ORIGIN=http://<ip> node …   # android_browser.mjs
```

**Never join this host's Wi-Fi to the device SoftAP** — on a single-radio
machine it severs the session, any other Claude Code sessions, and remote
access. Use station mode; the device is reachable on the LAN.
