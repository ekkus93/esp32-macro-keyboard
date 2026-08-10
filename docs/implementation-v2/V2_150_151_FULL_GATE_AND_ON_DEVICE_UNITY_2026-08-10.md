# V2-150/V2-151 — Full local quality gate and on-device Unity validation

**Date:** 2026-08-10
**Task:** V2-150 and V2-151 (Phase 15 — Full validation and release evidence)
**Repo commit at time of gate run:** `de47eee5f7544e6c4c2d686ac4cbb07abc08736b`
(clean checkout)
**Board:** ESP32-S3 (QFN56, chip revision v0.2), 8MB embedded PSRAM, MAC `9c:13:9e:a8:77:38`
**Toolchain:** ESP-IDF v5.5.5, Node v24.18.0, Python 3.10.10, gcovr 8.6

## 1. A real bug found while running the gate, fixed first

`webapp/tests/v2-snapshot-client.test.ts`'s oversized-snapshot test
(`saveWorkingCopyAsSnapshot refuses an oversized snapshot before ever calling
fetch`) timed out under `vitest run --coverage` (default 5000ms), while
passing cleanly moments earlier in the plain `vitest run` step of the same
gate run. Root cause: the fixture deliberately builds ~1.6 MB of
high-entropy, gzip-incompressible filler (so the compressed upload genuinely
still exceeds the 128 KB blob limit — see the fixture's own comment), and
gzip-compressing that much incompressible data is real synchronous CPU work
that V8's coverage instrumentation slows down enough to blow the timeout. Not
a logic bug. Fixed with a per-test timeout override (`5000ms → 20000ms`, the
standard vitest mechanism) — no assertion logic touched. Verified the fixed
test passes under coverage in isolation before re-running the full gate.
Committed separately (`de47eee`) before the evidence run below.

## 2. V2-150 — full local quality gate

### `./scripts/check-all.sh`

**Result: `EXIT=0`.** Ran end-to-end (`real 7m37s`) with the documented
`clang-format` PATH-shadowing workaround already in `CLAUDE.md` (esp-clang's
`clang-format` otherwise shadows the apt one after sourcing `export.sh`).

| Stage | Result |
| --- | --- |
| Stack usage policy | passed: 588 first-party frames analyzed, largest 1536 bytes, 0 allowlisted |
| Release budget checks | app binary 999,392/2,097,152 B (47.7%); webfs 434,176/891,289 B (48.7%); userdata headroom 516,096/262,144 B min; static RAM 126,359/256,320 B (49.3%) — all within threshold |
| Webapp `format:check` | Prettier — all matched files clean |
| Webapp `typecheck` | `tsc -b` — clean |
| Webapp `lint` | ESLint `--max-warnings=0` — clean |
| Webapp `stylelint` | `--max-warnings=0` — clean |
| Webapp `test` (Vitest) | **494/494 passed** (46 files) |
| Webapp `test:coverage` | 494/494 passed; coverage 86.75% statements / 83.47% branches / 90.97% functions / 86.82% lines — all above the 60% policy gate |
| Webapp `build` | production build + webfs image, clean |
| Webapp `test:browser` (real Chrome) | 7/7 scenario groups passed: Macros/Quick Send, Snapshots/import-export, Settings/Diagnostics, axe-core accessibility, USB-unavailable, startup workflows (6 sub-scenarios), macro-editing/package-management |
| v2 contract checks | limits, device-settings, setup-route policy, API route manifest (21 routes), auth policy — all synchronized |
| `check-v2-contracts.sh` | clean |
| `tests/scripts/test-v2-035-hardware.py` | 7/7 regression tests passed (the harness fixed 2026-08-10) |
| Firmware build (`firmware/`, GCC) | clean |
| Firmware clang-tidy (esp-clang, `WarningsAsErrors: '*'`) | zero first-party findings (`firmware/` and `firmware/test_app/`) |
| Shell/workflow lint (shfmt, shellcheck, actionlint) | clean |
| Docs lint (markdownlint, yamllint, schema checks, spec traceability) | clean (pre-existing YAML-workflow style warnings only, unrelated to this repo's own code) |
| Host C tests (`run-tests.sh`, no label) | **56/56 passed** |

### `./scripts/run-tests.sh --sanitizers` (ASan + UBSan)

**56/56 passed**, `EXIT=0`. No sanitizer findings.

### `./scripts/generate-native-coverage.sh` (native coverage policy gate)

**`EXIT=0`** (`gcovr --fail-under-line 90 --fail-under-branch 80` on the
policy-file subset).

| Scope | Lines | Functions | Branches |
| --- | --- | --- | --- |
| Policy files (gated, ≥90% line / ≥80% branch required) | 96.2% (2432/2528) | 100.0% (212/212) | 82.7% (1792/2167) |
| All instrumented files (informational, not gated at this threshold) | 83.6% (6839/8176) | 93.3% (657/704) | 71.0% (4580/6455) |

## 3. V2-151 — on-device Unity validation

Built `firmware/test_app` at commit `de47eee` (`bash
scripts/build-device-tests.sh`), flashed via native USB in bootloader mode,
booted normally, and ran the complete Unity menu (`*`) over the UART console
— not a substitute build-only claim.

```text
./main/test_uuid.c:7:hardware RNG generates distinct UUID v4 values:PASS
./main/test_uuid.c:24:UUID parser rejects malformed and non-v4 values:PASS
./main/test_macro_parser.c:22:macro compiler builds a complete immutable plan:PASS
./main/test_macro_parser.c:44:macro compiler rejects malformed input without a partial plan:PASS
./main/test_macro_parser.c:54:macro compiler accepts the maximum directive delay:PASS
./main/test_limits.c:4:firmware hard limits match the version 0.1 contract:PASS
./main/test_macro_executor.c:13:executor initializes idle and rejects unavailable USB:PASS
./main/test_auth.c:30:authentication adapters create and validate secrets:PASS
./main/test_auth.c:84:PBKDF2 candidate timings are reported:PASS
./main/test_usb_state.c:6:USB keyboard initializes without emitting a key:PASS
10 Tests 0 Failures 0 Ignored
OK
```

**10/10 tests passed, 0 failures, 0 ignored** — every `TEST_CASE` in
`firmware/test_app/main/` (confirmed against source: 6 files, 10
`TEST_CASE`s, exact match).

The PBKDF2 benchmark test reproduced timings consistent with the frozen
iteration count's original calibration
(`docs/implementation-v2/V2_041_PBKDF2_BENCHMARK_2026-08-08.md`):

```text
PBKDF2_BENCH iterations=2500 samples=10 median_us=198471 p90_us=198474 worst_us=198474
PBKDF2_BENCH iterations=3500 samples=10 median_us=277839 p90_us=277867 worst_us=332338
PBKDF2_BENCH iterations=4500 samples=10 median_us=357201 p90_us=357204 worst_us=357205
PBKDF2_BENCH iterations=5500 samples=10 median_us=436565 p90_us=436569 worst_us=489152
PBKDF2_BENCH iterations=6500 samples=10 median_us=515932 p90_us=515936 worst_us=576162
```

Periodic `task_wdt` (IDLE0) triggers occurred throughout — the same known,
already-documented behavior from the 2026-08-08 PBKDF2 session (the Unity
console's blocking read loop not yielding while idle at the prompt). No
crash, reset, or test failure resulted; not a new finding.

## 4. What this report does not claim

- V2-150/V2-151 only. Phase 15's other tracks (V2-152 multi-OS HID matrix,
  V2-153 storage/power matrix, V2-154 network/auth matrix, V2-155 Android UI
  matrix, V2-156 final acceptance audit) are separate, not covered here.
- The board is left running `firmware/test_app` (the Unity test image), not
  the production app, at the end of this session — reflash the production
  build before any further production-API testing.

## 5. Commands run

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
nvm use 24.18.0
# PATH workaround for the documented esp-clang/clang-format shadowing gotcha:
ln -sf /usr/bin/clang-format <override-dir>/clang-format
export PATH="<override-dir>:$PATH"

./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh

bash scripts/build-device-tests.sh
idf.py -B firmware/test_app/build -p /dev/ttyACM1 flash   # native USB, bootloader mode
# console interaction over the UART bridge (/dev/ttyACM0): boot, then send `*`
```
