# H12-120 / H12-121 / H12-122 — final candidate acceptance — 2026-08-16

Supersedes `H12_120_121_CLEAN_CHECKOUT_2026-08-16.md` (candidate
`07c40a4b1a5b9c63c494f4f6c8482e14f8222d7e`), which was overtaken when H12-122
found two production defects requiring source changes.

| Field | Value |
| --- | --- |
| Final candidate SHA | `28359e884d4fdbc3748853ce880a421ee0644a01` |
| Board | ESP32-S3R8 (QFN56 rev v0.2, 8 MB octal PSRAM), MAC `9c:13:9e:a8:77:38` |
| Host | `Linux-7.0.0-28-generic-x86_64-with-glibc2.39` |
| Toolchain | ESP-IDF v5.5.5, Node 24.18.0 |
| Result | **H12-120, H12-121, H12-122 all met** |

## Why the candidate moved twice

`07c40a4b` → `6666e79` → `28359e8`. H12-122 is the first gate that puts a real
HTTP client in front of the device, and it found two instances of one defect
that no host test could see — an immediate `esp_restart()` preempting the lwIP
flush, so a promised 202 never reached the wire. Both are recorded in
`H12_122_RESTART_RESPONSE_FINDING_2026-08-16.md`:

- `6666e79` — `/api/v1/device/restart` and `/api/v1/device/factory-reset`
- `28359e8` — `POST /api/v1/setup`

Each source change reopened H12-120/H12-121, so both were re-run from scratch on
the replacement SHA rather than carried forward. The evidence below is entirely
from `28359e8`; nothing is inherited from the superseded candidates.

## H12-120 — clean checkout

Fresh `git clone` of `28359e8`:

- **0** modified or untracked files in the clone
- **0** build outputs present before building — no `firmware/build`, no
  `webapp/node_modules`
- dependencies installed only through the documented `npm --prefix webapp ci`
  against the committed lockfile

No generated or untracked artifact from the working repository was required.

## H12-121 — authoritative gate

All three commands run from that clean checkout, all **exit 0**:

| Command | Result |
| --- | --- |
| `./scripts/check-all.sh` | exit 0 in **268 s**, 66/66 host tests |
| `./scripts/run-tests.sh --sanitizers` | exit 0, **66/66** in a separate `build-sanitizers` tree |
| `./scripts/generate-native-coverage.sh` | exit 0 |

Policy coverage against the line ≥90 / branch ≥80 gate:

| Metric | Measured | Margin |
| --- | --- | --- |
| Lines | **95.6 %** (2942/3077) | +5.6 |
| Branches | **82.7 %** (2112/2555) | +2.7 |
| Functions | **100 %** (264/264) | — |

No first-party warning was ignored, suppressed, or downgraded. The H9
production-audit guard rejected the setup fix until its fallback was explicitly
classified; it was classified rather than reworded around, and the guard passes.

## H12-122 — final hardware confirmation

Command, run from the clean checkout so `validate_source_checkout` binds the
manifest to the same tree:

```text
python3 scripts/run-h12-122-hardware.py \
  --flash-manifest firmware/build/flash-manifest.json \
  --firmware-sha 28359e884d4fdbc3748853ce880a421ee0644a01 \
  --flash-port /dev/ttyACM2 --console /dev/ttyACM0 \
  --output docs/hardware-evidence/H12_122_FINAL_ACCEPTANCE_ESP32S3R8_2026-08-16.json
```

**Result: PASS**, exit 0 — evidence
`docs/hardware-evidence/H12_122_FINAL_ACCEPTANCE_ESP32S3R8_2026-08-16.json`.
The harness flashes from the manifest itself, so the flash and the observed
provenance are one fail-closed sequence. Covered on the board:

- login; version/commit diagnostics matching the manifest ELF at every stage
- active send; confirmation-required send (nothing typed before `confirm` on the
  trusted UART, exact expected text after, all keys released)
- cancel during an active delayed send — **no** key-down report, all keys released
- snapshot save/load, byte-identical gzip round trip
- password change: 204 with an empty body, active session invalidated, old
  password rejected, new password accepted
- **restart: 202 received**, then `resetReason: software` with an uptime
  discontinuity — the defect fixed in `6666e79`
- factory reset: 202, reprovisioning required, unprovisioned setup mode observed
  on the UART
- **reprovision through `POST /api/v1/setup`** — the defect fixed in `28359e8`
- after reprovision: pre-reset administrator password rejected, blob list
  available and correctly shaped, the H12 snapshot erased by the reset
- the same production build remained flashed through restart, factory reset and
  reprovision

### Port identification

`--flash-port` and `--console` must be distinct devices, and the harness
enforces it. Bench numbering had shifted since the previous run: `/dev/ttyACM1`
was a **Samsung Android phone**, not the board. The mapping was resolved from
sysfs vendor IDs rather than assumed —

| Node | VID:PID | Device |
| --- | --- | --- |
| `/dev/ttyACM0` | `1a86:55d3` | CH340 UART bridge — the console |
| `/dev/ttyACM1` | `04e8:6860` | unrelated Samsung device |
| `/dev/ttyACM2` | `303a:1001` | ESP32-S3 USB-Serial/JTAG in download mode |

The board was put into download mode without physical intervention by running
esptool over the CH340 with `--after no_reset`; its DTR/RTS are wired to
IO0/EN, so the ROM bootloader enumerates on native USB as `303a:1001` and
provides the distinct flash port the guard requires.

### Sign-off state

No test image remains flashed: the device is running the production build from
`28359e8`, confirmed by on-device diagnostics matching the manifest ELF after
the final reprovision.
