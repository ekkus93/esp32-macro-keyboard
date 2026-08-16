# H10-102 — on-device Unity validation — 2026-08-16

## Status

**PASS — 12 Tests, 0 Failures, 0 Ignored.** Every registered case ran on the
reference board; this is a real execution result, not a build-only check.

## Exact conditions

| | |
| --- | --- |
| Candidate SHA | `fd0ddf76cba91a438270d61a51e85df0e4a18418` (clean tree) |
| Test image | `firmware/test_app` built by `./scripts/build-device-tests.sh` |
| Board | ESP32-S3R8, MAC `9c:13:9e:a8:77:38` |
| Console | `/dev/ttyACM0` (CH340 bridge) |
| Driver | `unity_run_menu()`, driven non-interactively by sending `*` |

## Result

```text
./main/test_uuid.c:7:hardware RNG generates distinct UUID v4 values:PASS
./main/test_uuid.c:24:UUID parser rejects malformed and non-v4 values:PASS
./main/test_macro_parser.c:22:macro compiler builds a complete immutable plan:PASS
./main/test_macro_parser.c:44:macro compiler rejects malformed input without a partial plan:PASS
./main/test_macro_parser.c:54:macro compiler accepts the maximum directive delay:PASS
./main/test_limits.c:4:firmware hard limits match the version 0.1 contract:PASS
./main/test_macro_executor.c:13:executor initializes idle and rejects unavailable USB:PASS
./main/test_executor_health.c:6:executor shutdown latch fails closed until worker stop is confirmed:PASS
./main/test_executor_health.c:30:executor cleanup health keeps the first release failure visible:PASS
./main/test_auth.c:30:authentication adapters create and validate secrets:PASS
./main/test_auth.c:84:PBKDF2 candidate timings are reported:PASS
./main/test_usb_state.c:6:USB keyboard initializes without emitting a key:PASS

12 Tests 0 Failures 0 Ignored
```

## Device coverage for new low-level hardening behaviour

Two of the twelve cases are exactly that, added by `f2ca986`
("test: add device executor hardening coverage"):

- **executor shutdown latch fails closed until worker stop is confirmed** — the
  H3-030b fail-safe latch, asserted on real silicon: submissions are refused
  once shutdown begins, the fault latches only on an unconfirmed stop, and the
  latch reopens only after a confirmed stop.
- **executor cleanup health keeps the first release failure visible** — the
  release-failure provenance that F-025/H7-070 exist to preserve.

So H10-102's final item is satisfied by coverage already on the device rather
than by adding more for its own sake.

## Incidental confirmation of the PBKDF2 cost baseline

The suite reports an isolated on-device KDF benchmark:

```text
PBKDF2_BENCH iterations=2500 samples=10 median_us=198472 p90_us=198473 worst_us=198474
PBKDF2_BENCH iterations=3500 samples=10 median_us=277838 p90_us=277881 worst_us=320873
PBKDF2_BENCH iterations=4500 samples=10 median_us=357201 p90_us=357205 worst_us=357205
PBKDF2_BENCH iterations=5500 samples=10 median_us=436569 p90_us=436571 worst_us=487245
PBKDF2_BENCH iterations=6500 samples=10 median_us=515933 p90_us=515935 worst_us=574240
```

At the shipped 5,500 iterations the median is 436,569 µs = **436.6 ms**, which
matches V2-041's recorded isolated figure of 436.6 ms exactly. That is an
independent confirmation of H2-024's "no cost regression" finding, measured at
the KDF rather than across an HTTP round trip.

## Restoring production afterwards

The test image was flashed over production, so production was rebuilt from the
same clean candidate SHA and reflashed, and its identity verified from the boot
log: running `ELF file SHA256: 746aa5cf29d7f28a…` equal to the flash manifest's
`appElfSha256`, `gitDirty: false`. The device remains provisioned and on the
LAN. **No test image remains flashed**, as H12-122 requires.

The test app writes its own single-app partition table and places its image at
`0x10000`, which in the production layout spans `otadata`, `phy_init` and
`nvs_keys`. That is safe here because this build keys NVS encryption from an
eFuse HMAC key (`CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC=y`,
`CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID=0`) rather than from the `nvs_keys`
partition, so no key material lives in flash to be lost. This was checked before
flashing, not discovered afterwards.
