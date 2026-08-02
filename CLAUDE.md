# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project layout

Monorepo, no root build file. Work happens in these areas:

- `firmware/` — ESP-IDF C11 firmware for ESP32-S3 (production app; `firmware/test_app/` is a separate on-device Unity test app)
- `webapp/` — React 19 / TypeScript / Vite / Tailwind frontend, served locally by the device
- `tests/host/` — native C host tests (CMake/CTest) with fakes for every hardware backend
- `scripts/` — authoritative bash entry points for build/test/lint/coverage — prefer these over ad-hoc commands
- `docs/` — `SPEC.md`, `TODO.md` (mandatory implementation sequence), `DEVELOPMENT.md`, JSON schemas

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

### Where the tests are, and the fast loops

The two suites are in different places and neither sits beside the code it tests.

| Suite | Location | Run just this |
| --- | --- | --- |
| Host C (52 `test_*.c` + 20 `.inc` fragments) | `tests/host/` | `./scripts/run-tests.sh [label]` |
| Frontend vitest (17 files) | `webapp/tests/` — **not** under `webapp/src/` | `npm --prefix webapp run test` |
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

## Hard rules

- **No failure-hiding**: no `|| true`, no redirecting errors away, no warning suppression, no first-party lint/analyzer exclusions. Every first-party warning is a defect (`scripts/README.md`). CI runs clang-tidy with `WarningsAsErrors: '*'` and ESLint/stylelint with `--max-warnings=0`. Approved exceptions are tracked in `docs/STATIC_ANALYSIS_EXCEPTIONS.md` — don't add a new suppression without registering it there.
- **Production web assets must be fully local** — no remote `//` URLs in `webapp/dist`; `verify-no-remote-assets.sh` enforces this.

## Code style (differs from defaults)

- C: `.clang-format` (LLVM base, IndentWidth 4, ColumnLimit 100, no short funcs/ifs on one line, right pointer alignment). Host tests compile with `-Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2`.
- Host tests use a **custom assert harness** (`tests/host/support/test_assert.*`), not Unity. Unity is only for the on-device `firmware/test_app/`.
- Frontend ESLint is `strictTypeChecked` + `stylisticTypeChecked`, with `no-floating-promises` and `consistent-type-imports` as errors.
- Shell: `shfmt` + `shellcheck` (bash). CMake: `cmake-format`/`cmake-lint`.

## Active development constraints

See `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md` for full context. Currently in force:

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
`sdkconfig` sets `CONFIG_ESP_CONSOLE_UART_DEFAULT=y` with
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
