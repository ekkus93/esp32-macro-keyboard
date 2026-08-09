# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project layout

Monorepo, no root build file. Work happens in these areas:

- `firmware/` — ESP-IDF C11 firmware for ESP32-S3 (production app; `firmware/test_app/` is a separate on-device Unity test app)
- `webapp/` — React 19 / TypeScript / Vite / Tailwind frontend, served locally by the device
- `tests/host/` — native C host tests (CMake/CTest) with fakes for every hardware backend
- `scripts/` — authoritative bash entry points for build/test/lint/coverage — prefer these over ad-hoc commands
- `docs/` — `SPEC_V2.md` (authoritative spec; `SPEC.md` is a retired v1 stub), `TODO_V2.md` (implementation sequence; `TODO.md` is a retired v1 stub), `DEVELOPMENT.md`, JSON schemas

## Toolchain (exact versions — enforced)

- **ESP-IDF must be tag `v5.5.5`**, target `esp32s3`. Before any firmware/device command, source the environment: `. "$HOME/esp/esp-idf-v5.5.5/export.sh"` (sets `IDF_PATH`). `scripts/verify-toolchain.sh` fails on any other version.
- **Node must be exactly `v24.18.0`** (`.nvmrc`, `engine-strict=true`).
- npm uses `save-exact=true` — **never add caret/tilde version ranges**; pin exact versions.
- On first dependency resolution, commit `package-lock.json`; don't run `npm ci` without a committed lockfile or fabricate one (see `docs/DEVELOPMENT.md`).

### Host lint/test tooling (pin to CI versions)

The lint/test scripts assume the exact versions CI installs (see `.github/workflows/{quality,host-tests}.yml`). Mismatched versions produce spurious format/lint diffs. On Ubuntu 24.04 ("noble"):

- apt (`clang-format` 18, `clang-tidy` 18, `shellcheck` 0.9.0, `libcjson-dev`, `jq`): `sudo apt-get install --yes clang-format clang-tidy shellcheck libcjson-dev jq`
- pip: `python3 -m pip install --user cmakelang==0.6.13 yamllint==1.38.0 gcovr==8.6 littlefs-python==0.15.0` (`cmakelang` provides `cmake-format`/`cmake-lint`; `littlefs-python` is required by `scripts/build-webfs-image.sh`, part of `check-all.sh`, and is pinned to the version `firmware/managed_components/joltwallet__littlefs/image-building-requirements.txt` uses)
- go: `go install mvdan.cc/sh/v3/cmd/shfmt@v3.11.0 github.com/rhysd/actionlint/cmd/actionlint@v1.7.12` (`actionlint` lints `.github/workflows/*.yml` as part of `check-scripts.sh`)
- `markdownlint-cli2` 0.23.2 comes from `npm --prefix webapp ci` (it's a pinned devDependency); `check-docs.sh` runs the local `webapp/node_modules/.bin` copy, not a global one.

`clang-format`/`shellcheck` are distro apt packages, so their versions track the runner's Ubuntu release; build on 24.04 to match CI.

`check-firmware.sh` runs **clang-tidy from the ESP-IDF clang toolchain** (esp-clang, LLVM 19), installed by `scripts/install-esp-idf.sh` and put on `PATH` by sourcing `export.sh` — not the apt `clang-tidy`. It must parse xtensa targets, which needs a clang-built compile database (`build-clang`); the apt clang-tidy and the GCC build database cannot (`clang: error: unsupported option '-mcpu='`).

## Commands

Run scripts from the repo root. All frontend commands go through `npm --prefix webapp` (or the wrapper scripts).

- Full quality gate: `./scripts/check-all.sh`
- Host tests: `./scripts/run-tests.sh` — `--sanitizers` (ASan+UBSan), `--coverage`, or a single label from: `support parser storage executor auth web startup usb controls wifi model`. One mode and one label max.
- Frontend checks: `./scripts/check-webapp.sh` (ci → typecheck → lint → stylelint → test → build → local-assets check)
- Firmware build + clang-tidy: `./scripts/check-firmware.sh` (or `cd firmware && idf.py set-target esp32s3 && idf.py build`)
- Format check (no auto-fix): `./scripts/check-format.sh`. Auto-fix frontend only: `npm --prefix webapp run format:write`
- Native coverage gate (line ≥90 / branch ≥80 on policy files): `./scripts/generate-native-coverage.sh`
- The `check-v2-*.py`/`check-v2-*.sh` script family (invoked from `check-all.sh`) enforces v1→v2 migration policy — API routes, auth policy, device-settings policy, limits, setup contract/route policy, phase-2 architecture. Treat these as first-party lint, not optional checks.

### Where the tests are, and the fast loops

The two suites are in different places and neither sits beside the code it tests.

| Suite | Location | Run just this |
| --- | --- | --- |
| Host C (47 `test_*.c` + 20 `.inc` fragments) | `tests/host/` | `./scripts/run-tests.sh [label]` |
| v2 contract tests (API routes, device settings, setup contract, macro tokens/conformance) | `tests/v2_contracts/` — separate from `tests/host/` | `./scripts/check-v2-contracts.sh` |
| Frontend vitest (35 files) | `webapp/tests/` — **not** under `webapp/src/` | `npm --prefix webapp run test` |
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
| `storage` | LittleFS mount (no auto-format on failure), bounded fs ops, atomic `<id>.gz.tmp`→`<id>.gz` blob commit, opaque byte-blob repository under `/data/repository/` |
| `web_server` | Bounded ESP-IDF HTTP server, same-origin checks, JSON responses, static frontend delivery |
| `support` | Cross-cutting: operation results, health reporting, CRC, clocks, random, bounded helpers |

The project is mid v1→v2 rebuild (see `docs/implementation-v2/V2_MIGRATION_MAP.md`):
firmware no longer owns package/repository semantics — it stores and serves
opaque blobs; the webapp owns package/macro modeling and talks to the firmware
through the fixed `/api/v1/*` contracts in `app_contracts_v2` and
`docs/schemas/*.schema.json`. Don't reintroduce package/revision/index logic
into firmware storage or `macro_model` without checking the migration map first.

### Webapp (`webapp/src/`)

- `v2/` — the current v2 contract layer: `apiContracts.ts`/`apiGuards.ts`/`apiRequestGuards.ts` (runtime type guards for every `/api/v1/*` payload), `apiTypes.ts`, `apiRouteManifest.ts`, `limits.ts` (mirrors firmware `app_limits_v2.h`), `macroCompiler.ts` (shares the parser conformance corpus with `macro_parser`), `repository.ts`/`repositoryValidation.ts` (client-owned package/macro modeling, since firmware doesn't do this anymore).
- `types/` — legacy v1 model types; being superseded by `v2/`.
- `api/` — the HTTP client (`client.ts`), route table, and error handling used by feature pages.
- `features/<domain>/` — legacy v1 route-level pages by domain (`auth`, `execution`, `macros`, `package`, `settings`); these are presentation scaffolds over representative data per `webapp/README.md` — a page rendering doesn't mean its persistence/API path is real, check the component before relying on one. `features/<domain>/v2/` subdirectories (`auth`, `macros`, `shell`, `snapshots`, `startup`) are the real, `v2/`-contract-wired implementations that `AppV2` actually renders — Settings and Diagnostics have no `v2/` implementation yet (Phase 12, unstarted) and render a placeholder.
- `routing.ts` — legacy v1 hash-based routing; `v2/routingV2.ts` is what `AppV2` actually uses.
- `App.tsx`/`components/` — the retired v1 shell, layout, and shared widgets (`AppShell`, `ConnectivityBanner`, `ErrorBanner`, `StatusBadge`, `AccessibleDialog`); not the mounted entry point. `main.tsx` boots `AppV2.tsx` instead.
- `pages/` — placeholder only (see its own `README.md`); route components currently live in `App.tsx`, not here.

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
`docs/implementation-v2/` for current v2-rebuild context
(`docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` predates the v1→v2 retirement).
Currently in force:

- The project is mid a v1→v2 rebuild (package/repository data-model rewrite,
  started 2026-08-03). `docs/SPEC.md` and `docs/TODO.md` are retired v1
  compatibility pointers. `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` are the
  authoritative synchronized requirements; `docs/TODO_V2.md` is the
  authoritative implementation sequence. Phase-by-phase status lives in
  `docs/implementation-v2/` (start at `V2_MIGRATION_MAP.md`).
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
- Work directly on `master`; don't create a branch or PR unless explicitly requested.
- Never force-push, reset `master`, or rewrite history — use normal forward commits.
- Don't mark a TODO checkbox complete without exact implementation and reproducible evidence (commit, commands, results).
- Don't claim physical hardware validation from compilation, host fakes, or CI device builds alone.
- Never commit or expose real credentials, tokens, keys, or flash dumps — use generated disposable credentials for security testing.

## Hardware: the two USB ports do different jobs

The board exposes two USB connectors and they are **not interchangeable**. Confirm
with `lsusb` and `ls -l /dev/ttyACM*` before assuming any path — the numbering
depends on plug order, and the vendor IDs are the reliable identifier.

| Port | Enumerates as | Typically | Use it for |
| --- | --- | --- | --- |
| **Native USB** (D+/D−, the ESP32-S3's own USB peripheral) | `303a:4001` running the app (TinyUSB HID), `303a:1001` (USB-Serial/JTAG) otherwise | `/dev/ttyACM0`, `hidraw*` | Flashing, `esptool`, **HID validation**, boot/log output |
| **USB-UART bridge** (a separate CH340/CP210x chip on UART0) | `1a86:55d3` (CH340) or `10c4:ea60` (CP210x) | `/dev/ttyACM1` or `/dev/ttyUSB0` | **The interactive serial console** |

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

`sdkconfig` is gitignored; only `sdkconfig.defaults` is tracked. Enabling
`CONFIG_APP_MANUFACTURING_PROVISIONING_LOG=y` there is how a device gets
re-provisioned after an NVS erase — it prints one-time credentials to the
console. **Revert it and reflash a production build afterwards**;
`check-production-config.sh` rejects it, and those credentials must be stored
outside the repository (`~/.config/esp32-macro-keyboard/hil/`, mode 600).
