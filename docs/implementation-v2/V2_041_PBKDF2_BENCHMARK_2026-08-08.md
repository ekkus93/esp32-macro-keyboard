# V2-041 — PBKDF2 iteration count, measured on physical ESP32-S3R8

**Date:** 2026-08-08
**Task:** V2-041 (Phase 4 — Authentication, provisioning, and device settings)
**Baseline commit before this work:** `9b60b15a04a13516c8e48787feecd3ddafea84ca`
**Board:** ESP32-S3 (QFN56, chip revision v0.2), 8MB embedded PSRAM, MAC `9c:13:9e:a8:77:38`
**ESP-IDF:** v5.5.5, target `esp32s3`
**Host OS:** Ubuntu 22.04.5 LTS
**Ports used:** native USB (`303a:4001` running / `303a:1001` bootloader) for flashing;
UART bridge (CH340, `1a86:55d3`) at `/dev/ttyACM0` for the interactive Unity console

This report covers V2-041 only: the frozen PBKDF2-HMAC-SHA-256 iteration count. It
also documents two defects discovered and fixed along the way, both required before
any on-device Unity evidence — including V2-041 — could be collected at all.

## 1. Blocker found first: on-device Unity tests were not registering

Before any benchmark could run, the on-device Unity test menu showed **zero**
tests — for every test file, not just `test_auth.c`. This is apparently the first
time `firmware/test_app` has ever been executed on physical hardware in the V2
rebuild, so nothing had caught it before.

Root cause, confirmed by inspecting the linked ELF and the ninja link command:
`firmware/test_app/main/CMakeLists.txt` did not pass `WHOLE_ARCHIVE` to
`idf_component_register()`. Without it, `main` links as an ordinary static archive,
and the linker only pulls in archive members that something else already
references. Every `TEST_CASE` macro expands to a `static void
__attribute__((constructor))` function whose only caller is the constructor
mechanism itself (via the `.ctors`/`__init_array_start`..`__init_array_end` range
Xtensa GCC uses) — nothing in the normal call graph references
`test_reg_helper_*` or `unity_testcase_register`, so with no `WHOLE_ARCHIVE`, the
linker silently dropped every one of the 10 `TEST_CASE` constructor functions
before the constructor array was ever assembled. `s_unity_tests_first` stayed
permanently `NULL`.

Confirmed via the actual link inputs: no `--whole-archive` appears anywhere in the
generated `link.txt`/`build.ninja` for `test_app`; ESP-IDF's own reference Unity
test app (`$IDF_PATH/components/unity/test_apps/main/CMakeLists.txt`) explicitly
passes `WHOLE_ARCHIVE` for exactly this reason. Before the fix: `nm` on the linked
ELF showed 0 `test_reg_helper_*` symbols and no `unity_testcase_register`; `.init_array`
spanned only 3 unrelated pointers. After adding `WHOLE_ARCHIVE`: 10
`test_reg_helper_*` symbols present, `unity_testcase_register` present,
`__init_array` spans 13 pointers, and the on-device menu enumerates all 10 tests
correctly.

**Fix:** `firmware/test_app/main/CMakeLists.txt` — added `WHOLE_ARCHIVE` to the
`idf_component_register()` call.

This defect blocks not only V2-041 but the entire on-device Unity execution
requirement (V2-151, Phase 15). It is fixed now; no further action needed for that
requirement's build-level prerequisite, but V2-151 itself (running the complete `*`
menu and recording every result) is still separate, not-yet-completed work.

## 2. Benchmark test redesigned to bypass the production security floor

With registration fixed, the benchmark test failed immediately: `auth_password_verify()`
enforces `record->iterations >= AUTH_PBKDF2_ITERATIONS` as a production anti-downgrade
floor (`firmware/components/auth/auth_core_password.c`) — correct behavior for a real
login attempt, but it rejects every benchmark candidate below the *current* (at the
time, placeholder `120000`) floor before any derivation happens, making it impossible
to measure candidates below that value through this API.

**Fix:** `firmware/test_app/main/test_auth.c`'s benchmark test now calls
`mbedtls_pkcs5_pbkdf2_hmac_ext()` directly — the same primitive
`auth.c`'s `adapter_derive()` calls — instead of going through
`auth_password_verify()`. This measures the real KDF cost with no policy layer
involved, which is what the benchmark is actually for. Production behavior
(`auth_password_verify()`'s floor enforcement) is untouched.

## 3. Candidates recalibrated from a first real measurement

The original candidate list (`60000, 90000, 120000, 150000`) assumed
desktop/server-class PBKDF2 throughput. The very first real measurement showed how
far off that was:

```text
PBKDF2_BENCH iterations=60000 samples=10 median_us=4820277 p90_us=4826002 worst_us=4826015
```

4.82 seconds median for 60000 iterations — roughly **80.3 µs/iteration** on this
board, about 15-20x slower per round than the original candidates assumed. None of
the original four candidates would land anywhere near the 250–500ms target.

**Fix:** replaced the candidate list with `2500, 3500, 4500, 5500, 6500`, bracketing
the target range based on the measured per-iteration cost.

## 4. Final benchmark results (candidates: 2500 / 3500 / 4500 / 5500 / 6500, 10 samples each)

Raw output, verbatim:

```text
PBKDF2_BENCH iterations=2500 samples=10 median_us=198470 p90_us=198473 worst_us=198691
PBKDF2_BENCH iterations=3500 samples=10 median_us=277838 p90_us=277840 worst_us=331824
PBKDF2_BENCH iterations=4500 samples=10 median_us=357201 p90_us=357202 worst_us=357205
PBKDF2_BENCH iterations=5500 samples=10 median_us=436566 p90_us=436572 worst_us=496797
PBKDF2_BENCH iterations=6500 samples=10 median_us=515932 p90_us=515936 worst_us=578066
```

| Iterations | Median | p90 | Worst |
| ---: | ---: | ---: | ---: |
| 2,500 | 198.5 ms | 198.5 ms | 198.7 ms |
| 3,500 | 277.8 ms | 277.8 ms | 331.8 ms |
| 4,500 | 357.2 ms | 357.2 ms | 357.2 ms |
| 5,500 | 436.6 ms | 436.6 ms | 496.8 ms |
| 6,500 | 515.9 ms | 515.9 ms | 578.1 ms |

2,500 falls below the 250ms floor. 6,500's median already exceeds the 500ms
ceiling. 3,500 / 4,500 / 5,500 all land inside the target window on both median and
worst-case.

## 5. Selected count: 5,500 iterations

Chosen as the highest measured candidate whose worst-case sample (496.8ms) still
clears the 500ms ceiling — maximizing brute-force resistance within the target
window. Confirmed with the device owner before freezing.

**Frozen in:** `firmware/components/auth/include/auth.h`,
`AUTH_PBKDF2_ITERATIONS` changed from the `120000U` placeholder to `5500U`. Every
other reference in the tree (production `provisioning_core.c`, host tests) uses
the symbol, not a literal, and was confirmed via `grep` before the change — the
value change is a single point of truth.

Three host tests hardcoded literals tied to the *old* placeholder value and needed
updating to match (all now reference `AUTH_PBKDF2_ITERATIONS` instead of a magic
number, so they stay correct if the constant ever changes again):

- `tests/host/auth_existing_tests.inc:13` — asserted the create path's default
  iteration count equals a literal `120000U`; now asserts equality with
  `AUTH_PBKDF2_ITERATIONS`.
- `tests/host/auth_additional_password_tests.inc:83` and `:128` — both
  constructed a record with `iterations` one below the *old* floor
  (`119999U`) to test boundary rejection; both now use
  `AUTH_PBKDF2_ITERATIONS - 1U`.

Not changed (confirmed unrelated, left as-is): `tests/host/test_device_settings_core.c:82`
(an arbitrary settings-fixture value, not tied to the auth floor);
`tests/host/auth_additional_password_tests.inc:61` and
`tests/host/auth_test_fixture.inc:88` (a self-consistent known-answer-vector pair,
deliberately independent of whatever the current production default is — the same
pattern as the device test's own fixed `TEST_VECTOR_ITERATIONS`).

## 6. Watchdog observation (not a blocker, recorded for the record)

The FreeRTOS task watchdog trips roughly every 5 seconds whenever `IDLE0` (CPU0) is
starved — observed both while the device sits idle at the Unity console prompt and
during long derive() calls. It never caused a crash, reset, or test failure in any
run this session, including all five benchmark candidates and the full `*` test
suite. It is unrelated to the iteration-count decision: even the slowest sample
observed (6,500 iterations, 578ms worst-case) is nowhere near the 5-second watchdog
window. Worth a separate look at some point (likely the Unity console's blocking
read loop not yielding), but out of scope here.

## 7. Commands run

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
bash ./scripts/build-device-tests.sh
cd firmware/test_app
idf.py -B build -p /dev/ttyACM1 flash   # native USB, bootloader mode (303a:1001)
# console interaction (Enter, then `[benchmark]`) driven over the UART bridge,
# /dev/ttyACM0, at 115200 baud, via a small pyserial script rather than an
# interactive monitor session
```

Host-side verification after freezing the constant:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/run-tests.sh          # 38/38 suites passed
./scripts/check-firmware.sh     # firmware build + clang-tidy, clean
./scripts/check-all.sh          # full authoritative gate
```

## 8. What this report does not claim

- This is software+hardware evidence for V2-041 specifically (the iteration-count
  decision). It is **not** a claim that V2-040, V2-151, or any other Phase 4/15 item
  is complete.
- The on-device Unity registration fix (§1) unblocks V2-151's build-level
  prerequisite, but V2-151 itself (execute the complete `*` menu and record every
  result) has not been done and is not claimed here.
- No unchecked TODO item is marked complete by this report alone; `docs/TODO_V2.md`
  should only be updated once this evidence is reviewed and any remaining Phase 4
  exit-gate requirements are satisfied.
