# V2-035 — Storage hardware evidence, collected on physical ESP32-S3R8

**Date:** 2026-08-10
**Task:** V2-035 (Phase 3 — Opaque blob storage), plus fixes to the collector
harness that had never been run against real hardware before.
**Board:** ESP32-S3 (QFN56, chip revision v0.2), 8MB embedded PSRAM, MAC `9c:13:9e:a8:77:38`
**Firmware commit:** `7f322c1129daa5002dc2c7f8d3b48cae4926d947` (clean,
`gitDirty:false`, `buildType:production`)
**Evidence:** `docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-10.json`
(all seven required scenarios, `phase:"complete"`, validated)

This continues directly from
`docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md`, using the
same real-Wi-Fi station-mode approach (device joins the existing network over
the trusted UART console's `wifi-connect` command, so the test host never
leaves its own network) instead of isolating onto the device's own setup AP.

## 1. The harness had never actually talked to a real device

`scripts/run-v2-035-hardware.py` was "prepared but never executed against
physical hardware" per `docs/TODO_V2.md`. Running it for the first time
immediately surfaced that it encoded stale, v1-era assumptions about the v2
API contract throughout — every one of these was found and fixed in
`scripts/run-v2-035-hardware.py` (test fixtures updated to match in
`tests/scripts/test-v2-035-hardware.py`, two new regression tests added, full
diff and rationale in that commit):

1. **`login()` sent `{"password": ...}`.** The real contract
   (`contracts/v2/api/examples.json`'s `loginRequest`) requires
   `{"adminPassword": ...}`. Every real login attempt failed `400`.
2. **`parse_success()` expected a v1-style `{"ok":true,"data":...}` envelope.**
   `web_api_response.c` documents that v2 success responses are the flat
   object itself. Every response-parsing call site (login, diagnostics,
   list_blobs, create_blob) was affected — this alone would have failed the
   entire harness at the very first authenticated call.
3. **`parse_diagnostics()` read `diagnostics.blobScan.{temporaryFileCount,
   temporaryFiles}`.** The real shape has no `blobScan` object at all —
   it's `diagnostics.storage.temporaryFiles`, with the count derived from the
   list length, not a separate field.
4. **`verify_firmware_provenance()` required exact equality between the
   live `diagnostics.buildId` and the flash manifest's 39-character
   `diagnosticsBuildId`.** The board only reports
   `CONFIG_APP_RETRIEVE_LEN_ELF_SHA` hex characters (9, on this project's
   current sdkconfig) — a genuine but much shorter prefix of the same ELF
   SHA. Fixed to check the prefix relationship instead of exact equality.

Each was confirmed against the real device's actual responses (not just
inferred from source) before being fixed. After the fix, the harness's own
regression suite (`tests/scripts/test-v2-035-hardware.py`) and a live dry-run
against the device (login, diagnostics, blob listing, manifest/provenance
check) all passed before any of the seven physical scenarios were attempted,
to avoid burning real power-cycle steps on a harness that might still have
had further contract drift.

## 2. Physical execution — all seven scenarios

Ran the full six-stage procedure from
`docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md`
against the live device at its real station-mode IP
(`http://100.64.64.61`, joined to the same Wi-Fi network as the test host,
not the device's isolated setup AP):

- **Stage 1 (numeric ordering / delete preservation):** three blobs created
  with strictly increasing IDs, newest-first listing confirmed, all
  byte-identical round-trips, middle blob deleted, survivors verified
  unchanged.
- **Stage 2 (real power-cycle persistence):** full USB power removal and
  restoration (not a software reset); all baseline/sentinel blobs reloaded
  byte-identical; `resetReason` confirmed `power_on`.
- **Stage 3 (interrupted upload / reboot cleanup):** a maximum-length upload
  was armed and power was cut mid-transfer, after 98,304 bytes had been sent
  (well past the required 16,384-byte minimum); after reboot, no partial
  final blob exists and `diagnostics.storage.temporaryFiles` is empty.
- **Stage 4 (storage exhaustion / `507`):** maximum-size uploads repeated
  until a within-limit request returned `507`; every previously committed
  blob remained byte-identical; all fill blobs cleaned up afterward.
- **Stage 5 (mount failure without formatting):** the real `userdata`
  partition was backed up via `parttool.py`
  (`sha256:4027f6ce…`), overwritten with a deterministic 524,288-byte
  all-zero image (`sha256:07854d2f…`), and the board rebooted onto it. Real
  firmware output:

  ```text
  E (1025) esp_littlefs: .../lfs.c:1383:error: Corrupted dir pair at {0x0, 0x1}
  E (1035) esp_littlefs: mount failed,  (-84)
  E (1035) esp_littlefs: Failed to initialize LittleFS
  E (1045) app_core: stage failed: storage_mount (storage_unavailable)
  E (1055) app_main: startup failed: storage_unavailable
  ```

  The post-boot partition read back byte-identical to the corrupt image
  (`sha256:07854d2f…`, unchanged from what was written — proving the failed
  mount did not silently format or otherwise rewrite `userdata`), and no
  formatting-success marker appears anywhere in the serial log. The backup
  was then restored and re-read, coming back byte-identical to the original
  (`sha256:4027f6ce…`).
- **Stage 6 (finalize):** all seven scenarios validated together; the
  finalized evidence file's self-hash (`evidenceSha256`) matches; test-created
  blobs cleaned up; the original baseline blob set (empty, this board had no
  prior blobs) confirmed unchanged.

## 3. A real operational finding along the way, not a defect

Getting to a bootloader-flashable state repeatedly required physically
cycling the board (BOOT+RESET, or a full USB unplug/replug) between every
flash and every `parttool.py` read/write — once the production app is
running, it claims native USB as a TinyUSB HID keyboard (`303a:4001`), which
has no software-triggerable path back to the USB-Serial-JTAG bootloader
identity (`303a:1001`) that `esptool`/`parttool` need. This is expected board
behavior (`CLAUDE.md`'s hardware table already documents the two identities),
not a bug — noted here only because it made this stage far more
manual-intervention-heavy than the harness doc's step list alone suggests.

## 4. What this report does not claim

- This closes V2-035 and the Phase 3 exit gate's hardware-evidence
  requirement specifically. It does not claim Phase 4/6/15's remaining
  hardware-gated items, and does not touch the still-open AP-passphrase
  disclosure gap documented in
  `docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md` §1
  (irrelevant here since this session reached the device via station mode,
  not the setup AP).
- The harness bugs found and fixed here are specific to
  `scripts/run-v2-035-hardware.py`; they don't imply anything about the
  correctness of the production firmware's own V2-035 behavior, which is
  exactly what this evidence run independently verified.

## 5. Commands run

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
nvm use 24.18.0
cd firmware && idf.py build
bash ../scripts/generate-flash-manifest.sh   # gitCommit 7f322c1, gitDirty:false, buildType:production
idf.py -p /dev/ttyACM1 flash                 # native USB, bootloader mode

# harness fixes verified first:
python3 tests/scripts/test-v2-035-hardware.py
# (plus a live dry-run: login, diagnostics, list_blobs, manifest/provenance)

export DEVICE_URL='http://100.64.64.61'      # station-mode IP, shared LAN
export FLASH_MANIFEST="${PWD}/firmware/build/flash-manifest.json"
export V2_035_STATE='<scratch path>/v2-035-state.json'
export V2_035_PASSWORD='<disposable admin password>'

python3 scripts/run-v2-035-hardware.py start --base-url "${DEVICE_URL}" \
  --flash-manifest "${FLASH_MANIFEST}" --state "${V2_035_STATE}" \
  --password-env V2_035_PASSWORD
# physical power cycle, then:
python3 scripts/run-v2-035-hardware.py verify-power-cycle --state "${V2_035_STATE}" \
  --password-env V2_035_PASSWORD
python3 scripts/run-v2-035-hardware.py arm-interrupted-upload --state "${V2_035_STATE}" \
  --password-env V2_035_PASSWORD --chunk-delay 3.0
# physical power cycle at the CUT POWER NOW banner, then:
python3 scripts/run-v2-035-hardware.py verify-interrupted-upload --state "${V2_035_STATE}" \
  --password-env V2_035_PASSWORD
python3 scripts/run-v2-035-hardware.py fill-storage --state "${V2_035_STATE}" \
  --password-env V2_035_PASSWORD

# mount-failure stage (native USB bootloader mode):
export PORT='/dev/ttyACM1'
parttool.py --port "${PORT}" read_partition --partition-name=userdata --output backup.bin
python3 -c "from pathlib import Path; Path('corrupt.bin').write_bytes(b'\x00' * 524288)"
parttool.py --port "${PORT}" write_partition --partition-name=userdata --input corrupt.bin
esptool.py --port "${PORT}" run                 # capture serial log on the UART bridge
parttool.py --port "${PORT}" read_partition --partition-name=userdata --output post-boot.bin
parttool.py --port "${PORT}" write_partition --partition-name=userdata --input backup.bin
parttool.py --port "${PORT}" read_partition --partition-name=userdata --output restored.bin
python3 scripts/run-v2-035-hardware.py record-mount-failure --state "${V2_035_STATE}" \
  --backup-image backup.bin --corrupt-image corrupt.bin \
  --post-boot-image post-boot.bin --restored-image restored.bin \
  --serial-log mount-failure.log

mkdir -p docs/hardware-evidence
python3 scripts/run-v2-035-hardware.py finalize --state "${V2_035_STATE}" \
  --password-env V2_035_PASSWORD \
  --output docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-10.json
python3 scripts/run-v2-035-hardware.py validate \
  --evidence docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-10.json
```
