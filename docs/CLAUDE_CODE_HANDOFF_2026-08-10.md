# Development handoff — esp32-macro-keyboard, 2026-08-10

**Repository:** `ekkus93/esp32-macro-keyboard`
**Handoff commit:** `bdbf698` (`master`, pushed to `origin/master`, working
tree clean)
**Purpose:** continue development on a different machine. This is a
handoff/status snapshot, not a specification — it is **not** authoritative
and must never be treated as such if it conflicts with `docs/SPEC_V2.md`,
`docs/UI_UX_SPEC_V2.md`, or `docs/TODO_V2.md`. Those three are the ground
truth; this file just orients you quickly on top of them.

Two earlier handoff documents exist and are both stale/superseded — don't
use them as a starting point:

- `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` — predates the v1→v2 retirement.
- `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` — already
  marked superseded in its own text; ~80 commits stale relative to this one.

---

## 1. What this project is

An ESP32-S3 macro keyboard: firmware runs USB HID typing macros stored as
opaque compressed blobs; a React web app (served locally by the device)
owns all macro/package modeling and talks to the firmware over a fixed
`/api/v1/*` JSON contract. Full architecture is in the project root
`CLAUDE.md` — read that first if you haven't. This document assumes it.

The project is **mid a v1→v2 rebuild** (started 2026-08-03, documented in
`docs/implementation-v2/V2_MIGRATION_MAP.md`). `docs/SPEC.md`/`docs/TODO.md`
are retired v1 stubs. `docs/SPEC_V2.md` (+ `docs/UI_UX_SPEC_V2.md`) and
`docs/TODO_V2.md` are the live, authoritative documents. **Both spec files
are frozen** — changes need Phil's explicit per-change permission; propose,
never apply.

---

## 2. Environment setup on a new machine

Exact versions are enforced, not suggestions:

```bash
# ESP-IDF — must be exactly tag v5.5.5, target esp32s3
./scripts/install-esp-idf.sh
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/verify-toolchain.sh

# Node — must be exactly v24.18.0 (.nvmrc, engine-strict=true)
nvm install 24.18.0 && nvm use 24.18.0

# Host lint/test tooling pinned to CI versions (Ubuntu 24.04 "noble"):
sudo apt-get install --yes clang-format clang-tidy shellcheck libcjson-dev jq
python3 -m pip install --user cmakelang==0.6.13 yamllint==1.38.0 gcovr==8.6 littlefs-python==0.15.0
go install mvdan.cc/sh/v3/cmd/shfmt@v3.11.0 github.com/rhysd/actionlint/cmd/actionlint@v1.7.12
```

Then `npm --prefix webapp ci` (lockfile is committed — never fabricate one),
and `npx playwright install --with-deps chromium` for the browser test
suite (see §6).

### A real gotcha discovered this session: `clang-format` version shadowing

Sourcing ESP-IDF's `export.sh` puts **esp-clang's own `clang-format`**
(LLVM 19.1.2) ahead of apt's pinned `clang-format` (18.1.3, the
CI-matching version) on `PATH`. The two versions disagree on
brace-initializer spacing, so running `./scripts/check-all.sh` after
sourcing `export.sh` can report false-positive formatting violations on
correctly-formatted files. `check-firmware.sh`'s clang-tidy step
genuinely needs esp-clang's toolchain (`run-clang-tidy`/`clang-tidy`), but
`check-format.sh` needs apt's `clang-format`. If you hit spurious
`-Wclang-format-violations` errors after sourcing `export.sh`, verify with
apt's binary directly (`/usr/bin/clang-format --dry-run --Werror <file>`)
before touching any file — the fix is a `PATH` override (a one-file
directory with a `clang-format` symlink to `/usr/bin/clang-format`,
prepended to `PATH` after sourcing `export.sh`), not reformatting correct
code. CI (`.github/workflows/quality.yml`) runs the same sequence and is
green, so this is a local `PATH`-ordering artifact, not a real defect —
but confirm that reasoning yourself on the new machine rather than
assuming it still applies.

### Another gotcha: shell state does not persist between separate tool-call invocations

If you're driving this from Claude Code (or similar), `export`/`nvm
use`/sourcing `export.sh` must all happen in the **same** shell
invocation as the command that needs them — each tool call gets a fresh
shell.

### `scripts/check-all.sh` is the full authoritative gate

Run it via the `/lint-n-test` skill if available, or directly:

```bash
export PATH="$HOME/go/bin:$PATH"
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
nvm use 24.18.0
./scripts/check-all.sh
```

Only `EXIT=0` is a pass. It runs format/static-analysis/production-config
checks, the firmware build + clang-tidy, stack-usage ratchet, webfs image
build, the full webapp chain (typecheck/lint/stylelint/Vitest/build/real-
Chrome browser suite), shell/workflow lint, docs lint, and host C tests —
in that order, fail-fast.

---

## 3. Current state (as of `bdbf698`)

`docs/TODO_V2.md`: **467 checked / 130 unchecked** checkboxes across 15
phases + a final sign-off checklist. This count was earned through
extensive independent re-verification this session (not self-reported by
implementers) — every checkbox was checked against real code, tests
re-run, and status corrected in both directions (unchecking false
completions, not just adding new checks).

### Fully or near-fully closed

- **Phases 0–2, 7, 11**: essentially complete. Phase 0's baseline-recording
  items are honestly left open — verified unrecoverable via the real
  GitHub Actions API history, not fabricated.
- **Phase 12** (Settings/Diagnostics/destructive-ops UI): fully built. One
  item left open by design — the diagnostics TODO text names a `stack`
  field that doesn't exist in `SPEC_V2.md` §13.13's fixed schema or the
  checked-in example corpus; flagged for you rather than invented.
- **Phase 13** (portrait/responsive/accessibility): built out this
  session — landscape-block enforcement, touch-target fixes, dialog focus
  trap/restore, keyboard-only reordering (Move first/up/down/last),
  reduced-motion CSS policy, automated `@axe-core/playwright` scanning (3
  of ~10 screens). Real remaining gaps: no web app manifest exists yet
  (so `orientation: "portrait-primary"` has no host to be added to — a
  prerequisite gap, not invented around), the 320px-exact viewport claim
  is unverified (real check tests 360px), and manual keyboard/screen-reader
  passes are un-doable without a human.
- **Phase 14, V2-140** (delete dead v1 code): the **webapp half** is done
  — `App.tsx` and the entire retired v1 feature tree deleted (577→448
  Vitest tests, confirmed unreachable via a full import-graph audit before
  deletion). The **firmware half was never audited** — explicitly out of
  that track's scope, explicitly left open in the TODO.

### The browser test harness: migrated to real Playwright this session

`webapp/tests/browser/run-browser-tests.mjs` used to be a hand-rolled
Chrome DevTools Protocol client (`class Cdp`, raw WebSocket). It's now
real `playwright` (pinned exact, `1.62.1`), driving real Chromium. Seven
scenario groups run in one script invocation (not `@playwright/test` —
see `docs/implementation-v2/V2_BROWSER_HARNESS_PLAYWRIGHT_MIGRATION_2026-08-09.md`
for why that specific decision was made): Macros/Quick Send,
Snapshots/import-export, Settings/Diagnostics, axe-core accessibility,
USB-unavailable, startup workflows, macro-editing/package-management.
`npm --prefix webapp run test:browser` runs it; `check-webapp.sh` includes
it.

### Two spec/example discrepancies found and fixed (with Phil's explicit approval, 2026-08-10)

`docs/SPEC_V2.md` and `contracts/v2/api/examples.json` had two internal
inconsistencies, found by extending example-corpus diff testing
(`cJSON_Compare()` against live handler output) to more routes:

1. `sendAccepted`/`sendStatus.estimatedDurationMs` read `214` for a
   documented request; the real, independently-verified formula in both
   the firmware and webapp macro compilers computes `207`. Now `207`
   everywhere.
2. `settingsUpdated.restartRequired`/`reconnectRequired` read `false`/
   `false` for an example that changes access-point credentials — directly
   contradicting the very next sentence in the same spec section
   ("Changing access-point credentials sets both flags to `true`"). Now
   `true`/`true` everywhere, matching the code and the prose.

Full writeup: `docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md`.
If you find another spec/code discrepancy, follow the same pattern:
verify against source independently, report to Phil, do not silently
"fix" the spec or quietly match code to an unverified doc number.

---

## 4. What's left — and the ESP32-S3 is required for almost all of it

**As of this handoff, the ESP32-S3 reference board is disconnected** (Phil
took it offline mid-session to switch machines). Nearly everything
remaining in `docs/TODO_V2.md` needs it:

### Software-only gaps still open (don't need hardware)

- **Phase 14, V2-140's firmware half**: audit `firmware/` for obsolete
  build registrations/dead files from the v1→v2 rebuild. Never done.
- **Phase 14 exit gate**: a full repository-wide (not just webapp-scoped)
  sweep for stray v1 references — `docs/API.md`'s archived-v1 section is
  fine (intentional), but the sweep itself hasn't run past the webapp.
- **Phase 13 residual items**: manifest-dependent progressive enhancement
  (blocked on deciding whether to add a web app manifest at all — a
  bigger decision than one checkbox, flag it to Phil rather than
  inventing one), 320px-exact viewport verification, logical-focus-order
  manual/screen-reader confirmation, tablet/foldable/desktop
  landscape-block false-positive verification on real devices (though the
  media-query logic is verified sound by construction).
- **V2-057's FreeRTOS async-worker path**: `web_server_async.c`'s real
  queue/task/semaphore worker code (not its synchronous fallback branch,
  which does have coverage now) is untestable at the host level without a
  much larger, genuinely risky FreeRTOS host fake. Investigated twice this
  session and deliberately left undone both times — a good candidate to
  revisit if you have a block of uninterrupted time, but don't force it.
- **`login` route**: still has no live `httpd_req_t`-level test anywhere
  (needs a real or faked socket for its IP-rate-limiting call) — a
  distinct, larger gap than routine example-diffing.

### Hardware-required — this is most of what's left

- **Phase 3 (V2-035)**: storage hardware evidence — power-cycle byte
  identity, interrupted-upload recovery, full-partition `507` behavior,
  mount-failure-without-formatting. `docs/hardware-evidence/` does not
  exist yet as a directory — will need creating.
- **Phase 4 (V2-041)**: PBKDF2 hardware benchmark and iteration-count
  selection, on real hardware.
- **Phase 6 (V2-063/V2-064)**: real-device cancellation latency
  measurement; USB HID identity/report matrix (`303a:4001` descriptor
  strings, captured HID reports, chord/modifier/release-all/disconnect-
  reconnect verification).
- **Phase 15 (V2-150 through V2-156)**: essentially untouched.
  On-device Unity validation, USB HID hardware matrix, storage/power-
  failure matrix, network/auth matrix, Android UI workflow matrix
  (real phone), and the final acceptance audit. This is the largest
  remaining block of work and needs the board plus, for the Android
  matrix, an actual phone.
- **Final sign-off checklist**: gated on all of the above plus Phil's own
  product-owner acceptance review — not something to close from code.

### Recommended order when the board is back

1. Reconnect and confirm port identity before touching anything —
   **the two USB ports are not interchangeable** (see `CLAUDE.md`'s
   hardware table: native USB `303a:4001`/`303a:1001` vs. the UART bridge
   `1a86:55d3`/`10c4:ea60` for the serial console). `lsusb` first.
2. Phase 3 storage evidence and Phase 4 PBKDF2 benchmark are the smallest,
   most self-contained hardware tasks — good to knock out first.
3. Phase 6 USB HID matrix next (needs `firmware/test_app` flashed, HID
   report capture tooling).
4. Phase 15 last — it's the big one, and several of its items (Android
   matrix, final acceptance audit) explicitly need Phil in the loop, not
   just the board.

---

## 5. Process notes worth carrying forward

- **Verify, don't trust self-reports.** Every implementer's claim this
  session (test counts, "found X bug," "left Y honestly unchecked") was
  independently re-checked against real code and re-run commands before
  being accepted. This caught several real issues (a Prettier defect, a
  misplaced edit, a stale test-normalization workaround that would have
  silently masked a fixed discrepancy). Keep doing this.
- **`docs/TODO_V2.md` checkbox discipline**: never check a box without
  exact implementation + reproducible evidence (file, test name, command,
  result) cited inline. Leaving something honestly unchecked is always
  correct over fabricating evidence — this project was explicitly
  burned by invented acceptance criteria before (see `CLAUDE.md`'s spec-
  freeze rationale).
- **Never touch `docs/SPEC.md`/`docs/SPEC_V2.md`/`docs/UI_UX_SPEC_V2.md`**
  without Phil's explicit per-change permission. Propose the exact diff,
  wait for a yes, then apply — as happened with the two example-value
  fixes in §3 above.
- **Never embed a raw NUL byte in a markdown/doc file** — write the
  literal `\u0000` escape text instead. This has now bitten the repo at
  least three times (`c33322f`/`2fd5e5d`, plus twice more this session) —
  every single time while writing prose *about* this exact bug, including
  the sentence you are reading right now on its first draft. If you're
  about to describe this bug in a markdown file, stop and type the escape
  text, don't type the literal character.
- **If using worktree-isolated subagents**: worktree provisioning appears
  to clone from `origin`, not the local repo's current branch tip. Push
  `master` to `origin` before launching parallel worktree agents, or they
  may silently build on a stale base. This caused three consecutive
  failed migration attempts earlier in this session before the root cause
  was found.
- **Commit discipline**: work directly on `master` (branches only for
  worktree-isolated parallel agents, merged back with `--no-ff`); never
  force-push or rewrite history; no `Co-Authored-By:` trailer in commit
  messages (a global hook rejects it).

---

## 6. Quick command reference

| Task | Command |
| --- | --- |
| Full quality gate | `./scripts/check-all.sh` |
| Host tests only | `./scripts/run-tests.sh [label]` (labels: `support parser storage executor auth web startup usb controls wifi model`) |
| Host tests w/ sanitizers | `./scripts/run-tests.sh --sanitizers` |
| Native coverage gate | `./scripts/generate-native-coverage.sh` |
| Frontend full chain | `./scripts/check-webapp.sh` |
| Frontend fast loop | `npm --prefix webapp run test` |
| Browser suite alone | `npm --prefix webapp run test:browser` (needs `npx playwright install chromium` once) |
| Firmware build + clang-tidy | `./scripts/check-firmware.sh` |
| v2 contract tests | `./scripts/check-v2-contracts.sh` |
| Format check (no fix) | `./scripts/check-format.sh` |
| Format auto-fix (frontend) | `npm --prefix webapp run format:write` |
| Regenerate spec traceability | `python3 scripts/generate-spec-traceability.py` (never hand-edit `docs/SPEC_V2_TEST_TRACEABILITY.md`) |

Full command/architecture reference: root `CLAUDE.md`. Implementation
sequence and per-item evidence: `docs/TODO_V2.md`. Phase-by-phase written
history: `docs/implementation-v2/` (start at `V2_MIGRATION_MAP.md`).
