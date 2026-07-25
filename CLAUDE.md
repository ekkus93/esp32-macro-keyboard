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
- pip: `python3 -m pip install --user cmakelang==0.6.13 yamllint==1.38.0 gcovr==8.6` (`cmakelang` provides `cmake-format`/`cmake-lint`)
- go: `go install mvdan.cc/sh/v3/cmd/shfmt@v3.11.0`
- `markdownlint-cli2` 0.23.1 comes from `npm --prefix webapp ci` (it's a pinned devDependency); `check-docs.sh` runs the local `webapp/node_modules/.bin` copy, not a global one.

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

## Hard rules

- **No failure-hiding**: no `|| true`, no redirecting errors away, no warning suppression, no first-party lint/analyzer exclusions. Every first-party warning is a defect (`scripts/README.md`). CI runs clang-tidy with `WarningsAsErrors: '*'` and ESLint/stylelint with `--max-warnings=0`.
- **Production web assets must be fully local** — no remote `//` URLs in `webapp/dist`; `verify-no-remote-assets.sh` enforces this.

## Code style (differs from defaults)

- C: `.clang-format` (LLVM base, IndentWidth 4, ColumnLimit 100, no short funcs/ifs on one line, right pointer alignment). Host tests compile with `-Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2`.
- Host tests use a **custom assert harness** (`tests/host/support/test_assert.*`), not Unity. Unity is only for the on-device `firmware/test_app/`.
- Frontend ESLint is `strictTypeChecked` + `stylisticTypeChecked`, with `no-floating-promises` and `consistent-type-imports` as errors.
- Shell: `shfmt` + `shellcheck` (bash). CMake: `cmake-format`/`cmake-lint`.

## Hardware flashing

Use the board's **native USB port** (D+/D-), not the USB-UART port: `cd firmware/test_app && idf.py -B build -p /dev/ttyACM0 flash monitor` (exit monitor with `Ctrl+]`). `sdkconfig` is gitignored; only `sdkconfig.defaults` is tracked.
