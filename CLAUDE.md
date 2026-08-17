# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project layout

Monorepo, no root build file. Work happens in these areas:

- `firmware/` — ESP-IDF C11 firmware for ESP32-S3 (production app; `firmware/test_app/` is a separate on-device Unity test app)
- `webapp/` — React 19 / TypeScript / Vite / Tailwind frontend, served locally by the device
- `tests/host/` — native C host tests (CMake/CTest) with fakes for every hardware backend
- `scripts/` — authoritative bash entry points for build/test/lint/coverage — prefer these over ad-hoc commands
- `docs/` — `SPEC_V2.md` (authoritative spec; `SPEC.md` is a retired v1 stub), `TODO_V2.md` (implementation sequence; `TODO.md` is a retired v1 stub), `DEVELOPMENT.md`; the v2 machine-readable contracts (routes, examples, repository schema) live in `contracts/v2/`, not `docs/`

## Toolchain (exact versions — enforced)

- **ESP-IDF must be tag `v5.5.5`**, target `esp32s3`. Before any firmware/device command, source the environment: `. "$HOME/esp/esp-idf-v5.5.5/export.sh"` (sets `IDF_PATH`). `scripts/verify-toolchain.sh` fails on any other version.
- **Node must be exactly `v24.18.0`** (`.nvmrc`, `engine-strict=true`).
- npm uses `save-exact=true` — **never add caret/tilde version ranges**; pin exact versions.
- On first dependency resolution, commit `package-lock.json`; don't run `npm ci` without a committed lockfile or fabricate one (see `docs/DEVELOPMENT.md`).

### Host lint/test tooling (pin to CI versions)

The lint/test scripts assume the exact versions CI installs (see `.github/workflows/{quality,host-tests}.yml`). Mismatched versions produce spurious format/lint diffs. On Ubuntu 24.04 ("noble"):

- apt (`clang-format` 18, `clang-tidy` 18, `shellcheck` 0.9.0, `libcjson-dev`, `jq`): `sudo apt-get install --yes clang-format clang-tidy shellcheck libcjson-dev jq`
- pip: `python3 -m pip install --user cmakelang==0.6.13 yamllint==1.38.0 gcovr==8.6 littlefs-python==0.15.0` (`cmakelang` provides `cmake-format`/`cmake-lint`; `littlefs-python` is required by `scripts/build-webfs-image.sh`, part of `check-all.sh`, and is pinned to the version `firmware/managed_components/joltwallet__littlefs/image-building-requirements.txt` uses)
- go: `go install mvdan.cc/sh/v3/cmd/shfmt@v3.11.0 github.com/rhysd/actionlint/cmd/actionlint@v1.7.12` (`actionlint` lints `.github/workflows/*.yml` as part of `check-scripts.sh`) — put `$(go env GOPATH)/bin` (typically `$HOME/go/bin`) on `PATH` before running `check-all.sh`/`check-scripts.sh`; if it's missing, `check-scripts.sh` fails with a bare **exit 127** that doesn't name `actionlint` as the cause.
- `markdownlint-cli2` 0.23.2 comes from `npm --prefix webapp ci` (it's a pinned devDependency); `check-docs.sh` runs the local `webapp/node_modules/.bin` copy, not a global one.

`shellcheck` is a distro apt package, so its version tracks the runner's Ubuntu release; build on 24.04 to match CI. The apt `clang-format` is **not** what formats this repo — see below.

`check-firmware.sh` runs **clang-tidy from the ESP-IDF clang toolchain** (esp-clang, LLVM 19), installed by `scripts/install-esp-idf.sh` and put on `PATH` by sourcing `export.sh` — not the apt `clang-tidy`. It must parse xtensa targets, which needs a clang-built compile database (`build-clang`); the apt clang-tidy and the GCC build database cannot (`clang: error: unsupported option '-mcpu='`).

**The format authority is esp-clang's `clang-format` (LLVM 19.1.2), not apt's 18.1.3.** `check-format.sh` does a bare `PATH` lookup, and `.github/workflows/quality.yml` sources `export.sh` immediately before `./scripts/check-all.sh` — so esp-clang wins in CI, and the apt `clang-format` installed alongside it is shadowed for this purpose. Measured at `a68f6dc` across the 314 first-party `.c`/`.h` files: **0 dirty under esp-clang 19, 2 dirty under apt 18**. The tree is 19-formatted.

Consequence: run `check-format.sh` from a shell that has sourced `export.sh`. A plain apt-only shell reports false failures on files CI accepts (currently `tests/host/fakes/esp_http_server_stub/esp_http_server.h` and `tests/host/fakes/freertos_stub/freertos/FreeRTOS.h`). `.claude/hooks/format-on-edit.sh` resolves esp-clang explicitly for the same reason.

## Commands

Run scripts from the repo root. All frontend commands go through `npm --prefix webapp` (or the wrapper scripts).

- Full quality gate: `./scripts/check-all.sh`
- Host tests: `./scripts/run-tests.sh` — `--sanitizers` (ASan+UBSan), `--coverage`, or a single label from: `support parser storage executor auth web startup usb controls wifi model`. One mode and one label max.
- Frontend checks: `./scripts/check-webapp.sh` (ci → typecheck → lint → stylelint → test → build → local-assets check)
- Firmware build + clang-tidy: `./scripts/check-firmware.sh` (or `cd firmware && idf.py set-target esp32s3 && idf.py build`)
- Format check (no auto-fix): `./scripts/check-format.sh`. Auto-fix frontend only: `npm --prefix webapp run format:write`
- Native coverage gate (line ≥90 / branch ≥80 on policy files): `./scripts/generate-native-coverage.sh`
- The `check-v2-*.py`/`check-v2-*.sh` family enforces v1→v2 migration policy — API routes, auth policy, device-settings policy, limits, setup contract/route policy, phase-2 architecture. `check-h2/h3/h9-architecture.py` and `check-h9-production-audit.py` do the same for the post-v2 hardening phases. Both are first-party lint, not optional; `check-all.sh` runs some directly and reaches the rest through `check-scripts.sh`.
- Stack-usage ratchet (`scripts/check-stack-usage.sh`, part of `check-all.sh`, allowlist `scripts/stack-usage-allowlist.txt`): fails on an unlisted frame over 4096 bytes, a listed frame that grew, or a listed frame that no longer exists. Fix growth by heap-allocating the large local — **never bump the recorded number**; delete allowlist entries for frames that no longer exist.
- `check-all.sh` is slow and prints a lot, including thousands of suppressed third-party clang-tidy warnings on a clean run — that volume is normal, not a failure. Capture output and check the exit code: `./scripts/check-all.sh > /tmp/log 2>&1; echo "EXIT=$?"`.

### Where the tests are, and the fast loops

The suites are spread across six trees and none sits beside the code it tests.

| Suite | Location | Run just this |
| --- | --- | --- |
| Host C (69 `test_*.c` + 26 `.inc` fragments) | `tests/host/` | `./scripts/run-tests.sh [label]` |
| Check-script self-tests (26 files) | `tests/scripts/` — tests *of* the gate scripts | `./scripts/check-scripts.sh` |
| v2 contract tests (API routes, device settings, setup contract, macro tokens/conformance) | `tests/v2_contracts/` — separate from `tests/host/` | `./scripts/check-v2-contracts.sh` |
| Frontend vitest (49 files) | `webapp/tests/` — **not** under `webapp/src/` | `npm --prefix webapp run test` |
| Browser (Playwright) | `webapp/tests/browser/` | `npm --prefix webapp run test:browser` |
| On-device Unity | `firmware/test_app/` | flash it; see the port table above |
| Hardware-in-the-loop (Python) | `tests/hardware/` | needs the board attached |

`check-webapp.sh` runs the whole chain (`ci → typecheck → lint → stylelint →
test → build → local-assets`), which is right before committing and slow while
iterating — use `npm --prefix webapp run test` for the inner loop and the script
before you commit.

Several C suites keep their bodies in `.inc` fragments that one `test_*.c`
includes (auth, executor, web security, web-server adapter), so grepping only
`test_*.c` for a test will miss them.

## Architecture

### Firmware (`firmware/components/`)

`main/app_main.c` hands startup to `app_core`, which wires every subsystem in an
explicit, fail-visible order (NVS/settings → userdata blob store → AP/station →
USB → executor → auth → static server → diagnostics) and does rollback on
failure rather than partial-booting. Components, each first-party and lint-scoped:

| Component | Owns |
| --- | --- |
| `app_core` | Startup ordering, dependency wiring, fatal-state coordination |
| `app_contracts_v2` | The v2 wire-format contracts shared with the webapp — device settings record layout (`device_settings_v2.c`) and the `/api/v1/setup` request/response shape (`setup_contract_v2.c`) |
| `macro_model` | Bounded data types, canonical UUIDs (`app_uuid`), stable app error codes (`app_error`) — v1's package/repository object model was deleted here; only execution-time macro/action types remain |
| `macro_parser` | The v0.1 macro language: US-ASCII/chord/directive parsing into action plans, complete-before-execute, exact source-location errors |
| `macro_executor` | Single-owner FreeRTOS execution engine: one active send, cancellation, progress, confirmation-wait (`EXECUTION_AWAITING_CONFIRMATION` + `macro_executor_confirm()`), unconditional release-all |
| `usb_keyboard` | TinyUSB HID integration, readiness gating, bounded report transmission |
| `auth` | PBKDF2-HMAC-SHA-256 passwords, RAM-only sessions, login rate limiting, constant-time comparison |
| `provisioning` | First-run state, NVS-backed settings record, `/api/v1/setup` workflow |
| `wifi_ap` | Protected SoftAP + optional station mode; no open-AP fallback ever |
| `serial_console` | The trusted UART0 dev console (`confirm`, `cancel`, `wifi-connect`, …) — see the hardware table below |
| `device_controls` | Status indication, physical confirmation signal, restart/reset-settings/factory-reset |
| `device_settings` | Device-level settings storage separate from Wi-Fi/auth provisioning |
| `factory_reset_state` | H3 durable factory-reset journal in its own NVS namespace (`none`/`pending`), one bounded transaction per call |
| `factory_reset_recovery` | Completes a committed factory reset at boot before ordinary startup; clears the journal only after every cleanup stage succeeds |
| `storage` | LittleFS mount (no auto-format on failure), bounded fs ops, atomic `<id>.gz.tmp`→`<id>.gz` blob commit, opaque byte-blob repository under `/data/repository/` |
| `web_server` | Bounded ESP-IDF HTTP server, session/request-policy enforcement, JSON responses, static frontend delivery |
| `support` | Cross-cutting: operation results, health reporting, CRC, clocks, random, bounded helpers |

The v1→v2 implementation has largely landed and the project is in final
hardening/release closure (see the post-v2 hardening tracker and
`docs/implementation-v2/V2_MIGRATION_MAP.md`). Firmware no longer owns
package/repository semantics — it stores and serves opaque blobs; the webapp
owns package/macro modeling and talks to the firmware through the fixed
`/api/v1/*` contracts in `app_contracts_v2` and `docs/schemas/*.schema.json`.
Do not reintroduce package/revision/index logic into firmware storage or
`macro_model`; some legacy migration cleanup and hardware acceptance items remain
explicitly open until their evidence gates close.

### Webapp (`webapp/src/`)

`main.tsx` boots `AppV2.tsx` — the only application tree in the codebase. The v1
shell, hash router, HTTP client, model types and route-level pages were deleted
in V2-140; nothing imports them (verified: zero references).

- `v2/` — the current v2 contract layer: `apiContracts.ts`/`apiGuards.ts`/`apiRequestGuards.ts` (runtime type guards for every `/api/v1/*` payload), `apiTypes.ts`, `apiRouteManifest.ts`, `limits.ts` (mirrors firmware `app_limits_v2.h`), `macroCompiler.ts` (shares the parser conformance corpus with `macro_parser`), `repository.ts`/`repositoryValidation.ts` (client-owned package/macro modeling, since firmware doesn't do this anymore).
- `types/limits.ts` — the one surviving file in `types/`; still used across `v2/` alongside `v2/limits.ts`.
- `features/<domain>/v2/` subdirectories (`auth`, `macros`, `settings`, `shell`, `snapshots`, `startup`) are the real, `v2/`-contract-wired implementations that `AppV2` renders — including Settings and Diagnostics (shipped Phase 12, V2-120–V2-122), with the destructive device actions (restart, reset-settings, factory-reset) and their reconnect handling.
- `components/` — shared widgets still used by `v2/`: `ErrorBanner`, `StatusBadge`.
- `pages/` — placeholder only (see its own `README.md`); route components live under `features/<domain>/v2/`, composed by `AppV2.tsx`.

### Host tests (`tests/host/`)

First-party firmware code is written against small backend interfaces
(filesystem, GPIO, HTTP, USB, Wi-Fi, FreeRTOS, clock, random), each with a fake
in `tests/host/fakes/` — that's what makes `./scripts/run-tests.sh` exercise real
firmware logic natively without hardware or QEMU. `tests/host/support/` holds the
custom assert harness plus test-only utilities (temp dirs, secret-sentinel
scanning, memory tracking). Large suites (auth, executor, web-server adapter, web
security) split their bodies across `.inc` fragments included by one `test_*.c` —
search those, not just `test_*.c`, when looking for a specific test.

## Hard rules

- **No failure-hiding**: no `|| true`, no redirecting errors away, no warning suppression, no first-party lint/analyzer exclusions. Every first-party warning is a defect (`scripts/README.md`). CI runs clang-tidy with `WarningsAsErrors: '*'` and ESLint/stylelint with `--max-warnings=0`. Approved exceptions are tracked in `docs/STATIC_ANALYSIS_EXCEPTIONS.md` — don't add a new suppression without registering it there.
- **Production web assets must be fully local** — no remote `//` URLs in `webapp/dist`; `verify-no-remote-assets.sh` enforces this.
- **Never embed a raw NUL byte in a markdown/doc file** — write the literal `\u0000` escape text instead; a raw NUL corrupts git's file-type detection (bit this repo twice — see commits `c33322f`/`2fd5e5d`).

## Code style (differs from defaults)

- C: `.clang-format` (LLVM base, IndentWidth 4, ColumnLimit 100, no short funcs/ifs on one line, right pointer alignment). Host tests compile with `-Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations -Wstrict-prototypes`.
- Host tests use a **custom assert harness** (`tests/host/support/test_assert.*`), not Unity. Unity is only for the on-device `firmware/test_app/`.
- Frontend ESLint is `strictTypeChecked` + `stylisticTypeChecked`, with `no-floating-promises` and `consistent-type-imports` as errors.
- Shell: `shfmt` + `shellcheck` (bash). CMake: `cmake-format`/`cmake-lint`.

## Active development constraints

See `docs/implementation-v2/V2_MIGRATION_MAP.md` and the phase docs in
`docs/implementation-v2/` for current v2-rebuild context. For current state,
blockers, and next steps, read the newest `docs/CLAUDE_CODE_HANDOFF_*.md` by
date — check the directory listing rather than assuming a filename, since a
fresh one is written each session and older ones are superseded (they say so
at the top when they are).
Currently in force:

- The v1→v2 rebuild began 2026-08-03 and its main implementation has landed;
  current work is final correctness hardening, validation, and release closure.
  `docs/SPEC.md` and `docs/TODO.md` are retired v1 compatibility pointers.
  `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` are the authoritative
  synchronized requirements; `docs/TODO_V2.md` remains the implementation
  ledger. Two post-v2 hardening trackers run in parallel, both dated 2026-08-10:
  `…_HARDENING_TODO_2026-08-10.md` (Round 1, phases H0–H12) and
  `…_HARDENING_TODO_ROUND2_2026-08-10.md` (Round 2, phases R1–R8, 10 items still
  open). Each has its own governing spec — check which round a task belongs to
  before citing a requirement. Phase evidence lives in `docs/implementation-v2/`.
- **`docs/SPEC.md` is frozen. Never modify it — not a section, not a sentence, not a
  typo — without explicit per-change permission.** Propose; do not apply. Set
  2026-08-02, after an acceptance criterion turned out to have been invented by the
  assistant and then cited back as a requirement. The rule is about normative
  content, not one filename: putting requirements into this file, into a new `docs/`
  file that later gets cited as spec, or writing "SPEC requires X" in a code comment
  when it says no such thing, is the same violation. On a genuine spec/code
  conflict, fix the code and report what the spec asked for that could not be
  satisfied. The same freeze applies to any replacement spec once it exists.
  The current replacement is `docs/SPEC_V2.md`, with `docs/UI_UX_SPEC_V2.md` as
  its synchronized companion per `docs/TODO_V2.md` §0 — the freeze applies to
  both.
- `docs/` holds 58 markdown files; most are retired v1 or round-specific
  historical artifacts, not requirements. Only `SPEC_V2.md`, `UI_UX_SPEC_V2.md`,
  `TODO_V2.md` and the two hardening trackers are live. Never implement from a
  `FIX*`/`PHASE_*`/`PROPOSAL_*`/`*_OUTSTANDING_*` document without reading its
  status header — the retired ones say so at the top.
- Work directly on `master`; don't create a branch or PR unless explicitly requested.
- Never force-push, reset `master`, or rewrite history — use normal forward commits.
- Don't mark a TODO checkbox complete without exact implementation and reproducible evidence (commit, commands, results).
- Don't claim physical hardware validation from compilation, host fakes, or CI device builds alone.
- Never commit or expose real credentials, tokens, keys, or flash dumps — use generated disposable credentials for security testing.
- Git worktrees (e.g. for parallel subagents) clone from `origin`, not the local branch tip — push `master` before launching worktree-based parallel work, or the worktree builds on a stale base.
- The v1→v2 dead-code audit (V2-140) covered the webapp; the firmware-side half was never done — `firmware/components/` may still contain unidentified v1-only paths.

## Hardware: the two USB ports do different jobs

The board exposes two USB connectors and they are **not interchangeable**. Confirm
with `lsusb` and `ls -l /dev/ttyACM*` before assuming any path — the numbering
depends on plug order, and the vendor IDs are the reliable identifier.

| Port | Enumerates as | Typically | Use it for |
| --- | --- | --- | --- |
| **Native USB** (D+/D−, the ESP32-S3's own USB peripheral) | `303a:4001` running the app (TinyUSB HID), `303a:1001` (USB-Serial/JTAG) otherwise | `/dev/ttyACM0`, `hidraw*` | **HID validation**, boot/log output |
| **USB-UART bridge** (a separate CH340/CP210x chip on UART0) | `1a86:55d3` (CH340) or `10c4:ea60` (CP210x) | `/dev/ttyACM1` or `/dev/ttyUSB0` | **The interactive serial console, and flashing** — see below |

**Prefer the UART bridge for flashing (`idf.py -p <bridge port> flash` /
`esptool --port <bridge port> ...`) — it needs no physical button press at
all.** Verified 2026-08-17: this chip's DTR/RTS lines are wired to EN/GPIO0
through the standard auto-reset transistor circuit, so `esptool` toggles the
board into the ROM bootloader and back entirely in software, the same as most
ESP32 dev boards.

Native USB has no such circuit — the peripheral is claimed entirely by
TinyUSB HID while the app runs, so there is no control channel for `esptool`
to request a reset through. Flashing over native USB needs a **manual**
BOOT+RESET (hold BOOT, tap RESET, release BOOT) to enter the bootloader, and
after flashing, a full **physical unplug/replug** of the cable to actually
boot the app again — a soft reset issued over the native-USB JTAG-serial
channel reliably leaves the board stuck showing `303a:1001` with the GPIO0
strap effectively still latched (reproduced twice, `idf.py flash` exit 0
both times). Only fall back to native USB when the UART bridge is
unavailable.

**The interactive console is on the UART bridge, not on native USB.**
`sdkconfig` packages `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` with
`CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG=y`, so `esp_console` reads stdin
from **UART0** while USB-Serial-JTAG is the *secondary* console — it mirrors log
output but **accepts no input**. Typing `wifi-connect`, `confirm`, or `cancel`
into `/dev/ttyACM0` produces no reply and no error; the bytes are simply
discarded. Send console commands to the UART bridge.

Reading `/dev/ttyACM0` with pyserial also stalls unless the port was just opened:
the USB-Serial-JTAG peripheral gates its TX on the CDC handshake that
`serial.Serial(...)` performs on open, and that same handshake resets the chip.

```bash
lsusb | grep -E '303a|1a86|10c4'          # identify before choosing a path
cd firmware/test_app && idf.py -B build -p /dev/ttyACM0 flash monitor   # exit: Ctrl+]
```

### Resets are not interchangeable when collecting hardware evidence

Three kinds, and `scripts/run-v2-035-hardware.py` distinguishes them:

- `esp_restart()` (the `/api/v1/restart` route) → `resetReason: software`.
- `esptool --after hard_reset` drives RTS into the **EN pin** — a real hardware
  reset. The device reports `resetReason: power_on`, so it satisfies any gate
  requiring `ESP_RST_POWERON`, including the collector's Stage 2.
- Removing power (**both** USB cables — each one powers the board).

Only the third drops the flash chip's supply, so only it can prove a partial
write leaves nothing behind. That is the collector's Stage 3 and it is the one
step no software can perform; the bench hub has no per-port power switching.
Stages 1, 2, 4 and 5 need no physical action. Verified end to end 2026-08-15 —
`docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-15.json`.

`sdkconfig` is gitignored; only `sdkconfig.defaults` is tracked. Enabling
`CONFIG_APP_MANUFACTURING_PROVISIONING_LOG=y` there is how a device gets
re-provisioned after an NVS erase — it prints one-time credentials to the
console. **Revert it and reflash a production build afterwards**;
`check-production-config.sh` rejects it, and those credentials must be stored
outside the repository (`~/.config/esp32-macro-keyboard/hil/`, mode 600).
