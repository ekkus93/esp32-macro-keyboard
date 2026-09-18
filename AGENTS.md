# AGENTS.md

Compact operational guide for future coding sessions. `CLAUDE.md` remains the
authoritative repo policy; use this file to find the right command, scope,
constraint, and gotcha quickly.

## Orientation

Monorepo with no root build file. Work happens in:

- `firmware/`: ESP-IDF C11 firmware for `esp32s3`; `firmware/test_app/` is a separate on-device Unity test app.
- `webapp/`: React 19 / TypeScript / Vite / Tailwind frontend served locally by the device.
- `tests/host/`: native C host tests for first-party firmware logic, using fakes for hardware backends.
- `scripts/`: authoritative bash/python entry points. Prefer these over ad-hoc commands.
- `contracts/v2/`: machine-readable v2 API/contract assets.
- `docs/`: `SPEC_V2.md`, `UI_UX_SPEC_V2.md`, `TODO_V2.md`, `DEVELOPMENT.md`, and the post-v2 hardening trackers are live. Most other docs are retired v1 or round-specific historical artifacts.

## Toolchain pins

- ESP-IDF must be tag `v5.5.5`, target `esp32s3`.
- Before firmware/device/full-gate commands: `. "$HOME/esp/esp-idf-v5.5.5/export.sh"`.
- `scripts/verify-toolchain.sh` fails on any other ESP-IDF version or wrong Node.js version.
- Node must be exactly `24.18.0` (`.nvmrc`, `webapp/package.json` engines).
- `.env.example`:
  - `IDF_VERSION=v5.5.5`
  - `IDF_TARGET=esp32s3`
  - `IDF_PATH=${HOME}/esp/esp-idf-v5.5.5`
  - `NODE_VERSION=24.18.0`
- npm policy in `webapp/.npmrc`:
  - `engine-strict=true`
  - `save-exact=true`
  - `fund=false`
  - `audit=true`
- Never add caret/tilde version ranges; pin exact versions.
- Commit `webapp/package-lock.json` on first dependency resolution. Do not run `npm ci` without a committed lockfile or fabricate one.

Host lint/test tools must match CI on Ubuntu 24.04:

```bash
sudo apt-get install --yes clang-format clang-tidy shellcheck libcjson-dev jq
python3 -m pip install --user cmakelang==0.6.13 yamllint==1.38.0 gcovr==8.6 littlefs-python==0.15.0
go install mvdan.cc/sh/v3/cmd/shfmt@v3.11.0 github.com/rhysd/actionlint/cmd/actionlint@v1.7.12
```

Put `$(go env GOPATH)/bin` (usually `$HOME/go/bin`) on `PATH` before `check-all.sh` or `check-scripts.sh`; if missing, `check-scripts.sh` can fail with bare exit `127`.

## Commands

Run scripts from the repo root.

- Full quality gate: `./scripts/check-all.sh`
- Host tests: `./scripts/run-tests.sh [--normal|--sanitizers|--coverage] [label]`
  - Valid labels: `support parser storage executor auth web startup usb controls wifi model`
  - Only one mode and one label may be selected.
  - Build dirs: `tests/host/build`, `tests/host/build-sanitizers`, `tests/host/build-coverage`.
- Frontend checks: `./scripts/check-webapp.sh`
- Firmware build + clang-tidy: `./scripts/check-firmware.sh`
  - Equivalent manual build: `cd firmware && idf.py set-target esp32s3 && idf.py build`
- Format check (no auto-fix): `./scripts/check-format.sh`
- Auto-fix frontend only: `npm --prefix webapp run format:write`
- Native coverage gate: `./scripts/generate-native-coverage.sh` (line ≥90 / branch ≥80 on policy files)
- Check-script self-tests: `./scripts/check-scripts.sh`
- Docs lint: `./scripts/check-docs.sh`
- v2 contract tests: `./scripts/check-v2-contracts.sh`

`check-all.sh` is slow and verbose. Capture output and check the exit code:

```bash
./scripts/check-all.sh > /tmp/log 2>&1; echo "EXIT=$?"
```

`check-all.sh` order:

1. `./scripts/verify-toolchain.sh`
2. `./scripts/check-format.sh`
3. `./scripts/check-static-analysis-policy.sh`
4. `./scripts/check-partitions.sh`
5. `python3 ./scripts/check-v2-034-capacity.py`
6. `python3 ./scripts/check-v2-device-settings-policy.py`
7. `bash ./scripts/check-production-config.sh`
8. `python3 ./scripts/check-no-wall-clock.py`
9. `bash ./scripts/check-credential-logging.sh`
10. `bash ./scripts/check-mount-policy.sh`
11. `bash ./scripts/check-layer-boundaries.sh`
12. `bash ./scripts/check-removed-features.sh`
13. `python3 ./scripts/check-v2-phase2-architecture.py`
14. `python3 ./scripts/check-v2-snapshot-send-policy.py`
15. `bash ./scripts/check-usb-identity.sh`
16. `bash ./scripts/check-frontend-persisted-state.sh`
17. `bash ./scripts/check-setup-route-isolation.sh`
18. `python3 ./scripts/check-web-route-dispatch-sync.py`
19. `python3 ./scripts/check-v2-auth-policy.py`
20. `bash ./scripts/check-v2-contracts.sh --native-only`
21. `./scripts/check-firmware.sh`
22. `bash ./scripts/check-stack-usage.sh`
23. `bash ./scripts/build-webfs-image.sh`
24. `bash ./scripts/generate-flash-manifest.sh`
25. `bash ./scripts/check-release-budgets.sh`
26. `./scripts/check-webapp.sh`
27. `./scripts/check-scripts.sh`
28. `./scripts/check-docs.sh`
29. `./scripts/run-tests.sh`

## Webapp commands

Use `npm --prefix webapp run <script>`.

- `dev`: `vite`
- `build`: `tsc -b && vite build`
- `typecheck`: `tsc -b --pretty false`
- `lint`: `eslint . --max-warnings=0`
- `stylelint`: `stylelint 'src/**/*.css' --max-warnings=0`
- `format:check`: `prettier --check .`
- `format:write`: `prettier --write .`
- `test`: `vitest run`
- `test:coverage`: `vitest run --coverage`
- `test:browser`: builds, then runs Playwright browser/recovery/storage tests.
- `test:visual`: builds, then runs visual regression tests.
- `check:no-orphan-classes`: builds, then checks for orphan classes.
- `check:status-badge-shapes`: builds, then checks StatusBadge shapes.
- `audit:ci`: `python3 ../scripts/check-npm-audit.py`

`check-webapp.sh` chain:

1. `npm ci`
2. `npm run format:check`
3. `npm run typecheck`
4. `npm run lint`
5. `npm run stylelint`
6. `npm run test`
7. `npm run test:coverage`
8. `npm run build`
9. `npm run test:browser`
10. `npm run test:visual`
11. `npm run check:no-orphan-classes`
12. `npm run check:status-badge-shapes`
13. `./scripts/verify-no-remote-assets.sh webapp/dist`

For an inner loop use `npm --prefix webapp run test`; run `./scripts/check-webapp.sh` before committing.

Key pinned webapp dependencies:

- `react@19.1.1`, `react-dom@19.1.1`, `tailwindcss@4.1.11`, `@tailwindcss/vite@4.1.11`
- `vite@7.3.6`, `typescript@5.8.3`, `vitest@4.1.10`, `@vitest/coverage-v8@4.1.10`
- `eslint@9.39.4`, `@eslint/js@9.39.4`, `typescript-eslint@8.65.0`
- `playwright@1.62.1`, `@axe-core/playwright@4.12.1`
- `prettier@3.6.2`, `prettier-plugin-tailwindcss@0.7.2`
- `stylelint@16.22.0`, `stylelint-config-standard@38.0.0`
- `jsdom@26.1.0`, `markdownlint-cli2@0.23.2`
- `@types/node@24.13.3`, `@types/react@19.1.9`, `@types/react-dom@19.1.7`
- `@vitejs/plugin-react@4.6.0`, `globals@16.3.0`
- `eslint-plugin-react-hooks@5.2.0`, `eslint-plugin-react-refresh@0.4.20`

## Testing locations

| Suite | Location | Run just this |
| --- | --- | --- |
| Host C | `tests/host/` | `./scripts/run-tests.sh [label]` |
| Check-script self-tests | `tests/scripts/` | `./scripts/check-scripts.sh` |
| v2 contract tests | `tests/v2_contracts/` | `./scripts/check-v2-contracts.sh` |
| Frontend unit tests | `webapp/tests/` (not under `webapp/src/`) | `npm --prefix webapp run test` |
| Browser tests | `webapp/tests/browser/` | `npm --prefix webapp run test:browser` |
| On-device Unity | `firmware/test_app/` | flash the test app |
| Hardware-in-the-loop | `tests/hardware/` | needs the board attached |

Large C suites use `.inc` fragments included by one `test_*.c`. Search the fragments too when looking for a specific test.

## Architecture

### Firmware

`main/app_main.c` hands startup to `app_core`, which wires subsystems in an explicit fail-visible order and rolls back on failure rather than partially booting.

| Component | Owns |
| --- | --- |
| `app_core` | Startup ordering, dependency wiring, fatal-state coordination |
| `app_contracts_v2` | Shared v2 wire-format contracts with the webapp |
| `macro_model` | Bounded data types, canonical UUIDs, stable app error codes |
| `macro_parser` | v0.1 macro language parsing into action plans |
| `macro_executor` | Single-owner FreeRTOS execution engine with cancellation, progress, confirmation-wait, release-all |
| `usb_keyboard` | TinyUSB HID integration, readiness gating, bounded reports |
| `auth` | PBKDF2-HMAC-SHA-256 passwords, RAM-only sessions, rate limiting, constant-time compare |
| `provisioning` | First-run state, NVS-backed settings, `/api/v1/setup` workflow |
| `wifi_ap` | Protected SoftAP + optional station mode; no open-AP fallback |
| `serial_console` | Trusted UART0 dev console |
| `device_controls` | Status indication, confirmation signal, restart/reset/factory-reset |
| `device_settings` | Device-level settings storage separate from Wi-Fi/auth provisioning |
| `factory_reset_state` | Durable factory-reset journal in its own NVS namespace |
| `factory_reset_recovery` | Completes committed factory reset at boot before ordinary startup |
| `storage` | LittleFS mount, bounded fs ops, atomic blob commit, opaque byte-blob repository |
| `web_server` | Bounded ESP-IDF HTTP server, session/request policy, JSON, static frontend delivery |
| `support` | Cross-cutting helpers: results, health, CRC, clocks, random, bounded utilities |

Firmware stores and serves opaque blobs. Do not reintroduce package/revision/index semantics into firmware storage or `macro_model`; the webapp owns package/macro modeling.

### Webapp

`webapp/src/main.tsx` boots `AppV2.tsx`, the only application tree. The v1 shell, hash router, HTTP client, model types, and route-level pages were deleted in V2-140.

- `webapp/src/v2/`: runtime contract guards, route manifest, limits, macro compiler, repository/validation.
- `webapp/src/types/limits.ts`: one surviving file in `types/`, still used across `v2/`.
- `webapp/src/features/<domain>/v2/`: real feature implementations rendered by `AppV2.tsx`.
- `webapp/src/pages/`: placeholder only; route components live under `features/<domain>/v2/`.

### Host tests

First-party firmware code is written against small backend interfaces with fakes in `tests/host/fakes/`. `tests/host/support/` holds the custom assert harness and test-only utilities.

## Hard rules

- No failure-hiding: no `|| true`, no error redirection that hides failures, no warning suppression, no first-party lint/analyzer exclusions.
- CI runs clang-tidy with `WarningsAsErrors: '*'` and ESLint/stylelint with `--max-warnings=0`.
- Approved exceptions are tracked in `docs/STATIC_ANALYSIS_EXCEPTIONS.md`; do not add new suppressions without registering them.
- Production web assets must be fully local; `verify-no-remote-assets.sh` enforces this.
- Never embed a raw NUL byte in a markdown/doc file; write the literal `\u0000` escape text instead.
- Work directly on `master`; do not create branches or PRs unless explicitly requested.
- Never force-push, reset `master`, or rewrite history; use normal forward commits.
- Do not mark TODO checkboxes complete without exact implementation and reproducible evidence.
- Do not claim physical hardware validation from compilation, host fakes, or CI device builds alone.
- Never commit or expose real credentials, tokens, keys, or flash dumps; use generated disposable credentials for security testing.
- `docs/SPEC.md` is frozen. `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` are the current live specs and are also frozen for normative content. Propose changes; do not apply them without explicit per-change permission.
- Git worktrees for parallel subagents clone from `origin`, not the local branch tip; push `master` before launching worktree-based parallel work.

## Code style

- C: `.clang-format` (LLVM base, IndentWidth 4, ColumnLimit 100, no short funcs/ifs on one line, right pointer alignment).
- Host tests compile with strict warnings: `-Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations -Wstrict-prototypes`.
- Host tests use the custom assert harness in `tests/host/support/test_assert.*`, not Unity. Unity is only for on-device `firmware/test_app/`.
- Frontend ESLint uses `strictTypeChecked` + `stylisticTypeChecked`, with `no-floating-promises` and `consistent-type-imports` as errors.
- Shell: `shfmt` + `shellcheck` (bash). CMake: `cmake-format`/`cmake-lint`.

## Hardware: two USB ports do different jobs

The two USB connectors are not interchangeable. Confirm with `lsusb` and `ls -l /dev/ttyACM*` before assuming a path; vendor IDs are reliable, device numbers depend on plug order.

| Port | Enumerates as | Typical device | Use it for |
| --- | --- | --- | --- |
| Native USB | `303a:4001` running app, `303a:1001` otherwise | `/dev/ttyACM0`, `hidraw*` | HID validation, boot/log output |
| USB-UART bridge | `1a86:55d3` (CH340) or `10c4:ea60` (CP210x) | `/dev/ttyACM1` or `/dev/ttyUSB0` | Interactive serial console and flashing |

Prefer the UART bridge for flashing:

```bash
idf.py -p <bridge port> flash
# or
esptool --port <bridge port> ...
```

The UART bridge DTR/RTS lines are wired to EN/GPIO0, so `esptool` can enter the ROM bootloader in software. Native USB lacks that circuit; flashing over native USB requires manual BOOT+RESET entry and a physical unplug/replug afterward.

The interactive console is on the UART bridge, not native USB. `esp_console` reads stdin from UART0; USB-Serial/JTAG mirrors log output but accepts no input.

```bash
lsusb | grep -E '303a|1a86|10c4'
cd firmware/test_app && idf.py -B build -p /dev/ttyACM0 flash monitor   # exit: Ctrl+]
```

Resets are not interchangeable for hardware evidence:

- `esp_restart()` / `/api/v1/restart` → `resetReason: software`
- `esptool --after hard_reset` → real hardware reset via EN pin, reports `resetReason: power_on`
- Removing power from both USB cables → only reset that drops flash supply and can prove partial writes leave nothing behind

`sdkconfig` is gitignored; only `sdkconfig.defaults` is tracked. `CONFIG_APP_MANUFACTURING_PROVISIONING_LOG=y` prints one-time provisioning credentials to the console. Revert it and reflash a production build afterward; `check-production-config.sh` rejects it. Store such credentials outside the repository, e.g. `~/.config/esp32-macro-keyboard/hil/` mode `600`.

## CI notes

- `quality.yml`: full gate on push to `master`, tags, PRs to `master`, and manual dispatch.
- `host-tests.yml`: host tests on push to `master`, tags, PRs to `master`, and manual dispatch; uploads failing test log artifacts.
- `browser-tests.yml`: builds the webapp and runs Playwright browser tests.
- `device-tests-build.yml`: builds the on-device Unity test app and lints `firmware/test_app/main` top-level C files.
- `r1-010-targeted-verification.yml` and `h5-055-hardware-start.yml` are targeted/hardware workflows.
- `publish-ci-status.yml` publishes CI status.

## Gotchas

- `check-firmware.sh` uses clang-tidy from the ESP-IDF esp-clang toolchain, not apt clang-tidy. It needs a clang-built compile database (`build-clang`); apt clang-tidy and the GCC build database fail on xtensa `-mcpu=` options.
- The format authority is esp-clang `clang-format` (LLVM 19.1.2), not apt `clang-format` 18. Run `check-format.sh` from a shell that has sourced `export.sh`; a plain apt-only shell can report false failures on files CI accepts.
- `shellcheck` is a distro apt package and tracks the Ubuntu release; build on 24.04 to match CI.
- `markdownlint-cli2` 0.23.2 comes from `npm --prefix webapp ci`; `check-docs.sh` uses the local `webapp/node_modules/.bin` copy, not a global install.
- `littlefs-python==0.15.0` is required by `scripts/build-webfs-image.sh`.
- `cmakelang==0.6.13` provides `cmake-format`/`cmake-lint`.
- `gcovr==8.6` is required for native coverage.
- Stack-usage ratchet: `scripts/check-stack-usage.sh` fails on unlisted frames over 4096 bytes, listed frames that grew, or missing allowlist entries. Fix growth by heap-allocating the large local; never bump the recorded number; delete allowlist entries for frames that no longer exist.
- `firmware/components/` may still contain unidentified v1-only paths; the v1→v2 dead-code audit covered the webapp but not the firmware side.
- `docs/` contains many retired historical artifacts. Do not implement from `FIX*`, `PHASE_*`, `PROPOSAL_*`, or `*_OUTSTANDING_*` documents without reading the status header.
