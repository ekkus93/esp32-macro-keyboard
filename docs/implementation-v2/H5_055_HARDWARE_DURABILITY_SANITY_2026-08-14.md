# H5-055 — Hardware durability sanity — 2026-08-14

## Status

H5-055 is **prepared but not physically closed**. The task requires evidence from a
real ESP32-S3R8 after the H5 storage semantic changes. This repository session
cannot access the reference board, its USB bootloader, its UART log, or its
`userdata` partition, so no hardware pass is inferred from source inspection.

The permanent H5-055 validation layer is:

- `scripts/run-v2-035-hardware.py` — the already-proven physical collector and
  evidence state machine;
- `scripts/run-h5-055-hardware.py` — the H5 entry point, which delegates to that
  collector but reconciles `503 commit_uncertain` by canonical list + exact-byte
  hash inspection and never automatically repeats the blob POST;
- `scripts/validate-h5-055-storage-evidence.py` — H5-specific fail-closed
  evidence validator;
- `tests/scripts/test-h5-055-hardware.py` — uncertain-commit collector wrapper
  regressions;
- `tests/scripts/test-h5-055-storage-evidence.py` — evidence-validator
  regressions;
- `scripts/check-scripts.sh` — runs both H5-055 regression suites on every full
  quality pass.

The historical August 10 V2-035 hardware evidence is useful precedent, but it
**cannot close H5-055** because its firmware SHA predates H5-051 through H5-054.
The new validator requires the exact firmware candidate SHA supplied on the
command line and rejects evidence from any other SHA.

## Why the existing physical collector is reused

The H5-055 checklist asks for three physical facts:

1. interrupted upload / real power-cycle behavior after the storage changes;
2. no formatting when mounting corrupted `userdata` fails;
3. byte identity and blob-list behavior remain correct.

The existing V2-035 collector already obtains stronger evidence for all three:

- creates deterministic gzip sentinels and verifies SHA-256 on every reload;
- proves numeric newest-first blob listing and delete preservation;
- requires a real `power_on` reset after physical power removal;
- sends a maximum-length blob in chunks and asks the operator to remove power
  after at least 16 KiB has entered staging;
- after reboot, requires the exact pre-interruption blob ID/hash set and zero
  `storage.temporaryFiles`;
- backs up the 524,288-byte `userdata` partition, writes a deterministic corrupt
  image, boots production firmware, reads the partition back, and proves the
  failed mount did not change one byte;
- restores the backup and proves the restored image matches the original;
- self-hashes the finalized evidence and cleans up every collector-created blob.

H5 changed one assumption the older collector legitimately made: before H5-053,
a non-201 blob-create response meant the create had not committed. That is no
longer true for `503 commit_uncertain`. The H5 wrapper replaces only the
collector's journaled-create seam. It sends one POST, and on `commit_uncertain`
it invokes the collector's existing pending-creation reconciliation logic. One
new exact-hash canonical blob is adopted; no match returns the uncertainty to
the stage without a repost; ambiguous or mismatched canonical state fails
closed while retaining the pending journal. Other non-201 failures still require
storage to be unchanged.

Reusing the mature collector plus this narrow H5 seam avoids a second hardware
implementation with different power-cut, partition-backup, or byte-identity
semantics. The extra storage-exhaustion stage exercised by V2-035 remains useful
additional coverage; H5-055 simply does not rely on it for closure.

## H5-specific validator

Run:

```text
python3 scripts/validate-h5-055-storage-evidence.py \
  --evidence docs/hardware-evidence/H5_055_V2_035_SOURCE_ESP32S3R8.json \
  --expected-firmware-commit "${H5_055_CANDIDATE}" \
  --output docs/hardware-evidence/H5_055_STORAGE_ESP32S3R8.json
```

The validator fails closed unless all of the following are true:

- source evidence is finalized V2-035 schema-3 evidence;
- source `evidenceSha256` matches the source contents;
- source firmware commit exactly equals `H5_055_CANDIDATE`;
- ESP-IDF is exactly v5.5.5 and hardware is ESP32-S3R8;
- collector-created blobs were cleaned up;
- power-cycle evidence reports `resetReason: power_on`, zero temporary files,
  and the exact baseline-plus-sentinel ID/hash set;
- the interrupted transfer ended after at least 16 KiB but before the declared
  content length;
- the post-interruption and reboot-cleanup ID/hash sets exactly equal the
  pre-interruption set;
- numeric created IDs are strictly increasing and the observed blob list is
  newest-first numeric order;
- delete-preservation hashes still equal the live baseline-plus-sentinel set;
- corrupt `userdata` SHA equals post-failed-boot SHA;
- backup SHA equals restored SHA and differs from the corrupt image;
- serial evidence contains an explicit mount failure and no known formatting
  success marker.

A mismatch in any one of those facts prevents H5-055 summary generation.

## Physical rerun procedure

Start from a clean checkout of the exact H5 candidate that will be evaluated.
Do not use a later working tree while retaining an older flash manifest.

```bash
set -euo pipefail
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
nvm use 24.18.0

H5_055_CANDIDATE="$(git rev-parse HEAD)"
test "$(git status --porcelain)" = ""

cd firmware
idf.py build
bash ../scripts/generate-flash-manifest.sh
cd ..

MANIFEST_COMMIT="$(python3 - <<'PY'
import json
from pathlib import Path

manifest = json.loads(Path("firmware/build/flash-manifest.json").read_text())
assert manifest["gitDirty"] is False
assert manifest["buildType"] == "production"
print(manifest["gitCommit"])
PY
)"
test "${MANIFEST_COMMIT}" = "${H5_055_CANDIDATE}"

# Flash the production candidate while the board is in USB-Serial-JTAG
# bootloader mode. Adjust the port only to the actual bootloader device.
cd firmware
idf.py -p /dev/ttyACM1 flash
cd ..

export DEVICE_URL='http://DEVICE_IP'
export FLASH_MANIFEST="${PWD}/firmware/build/flash-manifest.json"
export H5_055_STATE="${PWD}/h5-055-v2-035-state.json"
export V2_035_PASSWORD='<disposable admin password>'

# Stage 1: create byte-identity/list sentinels. Use the H5 wrapper for every
# collector command so any real commit_uncertain response is reconciled
# without an automatic second POST.
python3 scripts/run-h5-055-hardware.py start \
  --base-url "${DEVICE_URL}" \
  --flash-manifest "${FLASH_MANIFEST}" \
  --state "${H5_055_STATE}" \
  --password-env V2_035_PASSWORD

# PHYSICAL ACTION: fully remove board power, restore it, and wait for Wi-Fi.
python3 scripts/run-h5-055-hardware.py verify-power-cycle \
  --state "${H5_055_STATE}" \
  --password-env V2_035_PASSWORD

# Stage 2: interrupted upload. The collector prints CUT POWER NOW after at
# least 16 KiB has been sent. Remove board power at that banner and restore it.
python3 scripts/run-h5-055-hardware.py arm-interrupted-upload \
  --state "${H5_055_STATE}" \
  --password-env V2_035_PASSWORD \
  --chunk-delay 3.0

python3 scripts/run-h5-055-hardware.py verify-interrupted-upload \
  --state "${H5_055_STATE}" \
  --password-env V2_035_PASSWORD

# V2-035's mature state machine includes its storage-full preservation stage
# before the mount-failure stage. H5-055 does not depend on this result, but
# running it keeps the proven collector path intact instead of mutating state
# manually.
python3 scripts/run-h5-055-hardware.py fill-storage \
  --state "${H5_055_STATE}" \
  --password-env V2_035_PASSWORD
```

### Destructive mount-failure stage

This stage intentionally corrupts `userdata`. The backup is mandatory. Do not
continue to the corrupt write if the backup read failed or is not exactly
524,288 bytes.

The production application normally claims native USB as HID, so the reference
board may need BOOT+RESET or a full unplug/replug to return to its
USB-Serial-JTAG bootloader identity before each `parttool.py` read/write.

```bash
set -euo pipefail
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
PORT='/dev/ttyACM1'
WORK="${PWD}/h5-055-mount"
mkdir -p "${WORK}"

parttool.py --port "${PORT}" read_partition \
  --partition-name=userdata --output "${WORK}/backup.bin"
test "$(stat -c %s "${WORK}/backup.bin")" -eq 524288
sha256sum "${WORK}/backup.bin"

python3 - <<PY
from pathlib import Path
Path("${WORK}/corrupt.bin").write_bytes(b"\x00" * 524288)
PY

parttool.py --port "${PORT}" write_partition \
  --partition-name=userdata --input "${WORK}/corrupt.bin"

# Boot the production image and capture the complete UART startup log to:
#   ${WORK}/mount-failure.log
# The log must show an explicit userdata/LittleFS mount failure.
# After the failed boot, return the board to bootloader mode before reading.

parttool.py --port "${PORT}" read_partition \
  --partition-name=userdata --output "${WORK}/post-boot.bin"
cmp "${WORK}/corrupt.bin" "${WORK}/post-boot.bin"

parttool.py --port "${PORT}" write_partition \
  --partition-name=userdata --input "${WORK}/backup.bin"
parttool.py --port "${PORT}" read_partition \
  --partition-name=userdata --output "${WORK}/restored.bin"
cmp "${WORK}/backup.bin" "${WORK}/restored.bin"

python3 scripts/run-h5-055-hardware.py record-mount-failure \
  --state "${H5_055_STATE}" \
  --backup-image "${WORK}/backup.bin" \
  --corrupt-image "${WORK}/corrupt.bin" \
  --post-boot-image "${WORK}/post-boot.bin" \
  --restored-image "${WORK}/restored.bin" \
  --serial-log "${WORK}/mount-failure.log"
```

### Finalize and bind H5-055

After the restored production board is reachable again:

```bash
mkdir -p docs/hardware-evidence

python3 scripts/run-h5-055-hardware.py finalize \
  --state "${H5_055_STATE}" \
  --password-env V2_035_PASSWORD \
  --output docs/hardware-evidence/H5_055_V2_035_SOURCE_ESP32S3R8.json

python3 scripts/run-h5-055-hardware.py validate \
  --evidence docs/hardware-evidence/H5_055_V2_035_SOURCE_ESP32S3R8.json

python3 scripts/validate-h5-055-storage-evidence.py \
  --evidence docs/hardware-evidence/H5_055_V2_035_SOURCE_ESP32S3R8.json \
  --expected-firmware-commit "${H5_055_CANDIDATE}" \
  --output docs/hardware-evidence/H5_055_STORAGE_ESP32S3R8.json
```

Commit both JSON files together with the exact candidate SHA and the captured
hardware report. Only after that evidence exists may the three H5-055 checkboxes
and the H5 phase exit gate be evaluated for closure.

## Software validation for this preparation

Both H5 regressions are permanently wired into:

```text
python3 tests/scripts/test-h5-055-hardware.py
python3 tests/scripts/test-h5-055-storage-evidence.py
```

through `./scripts/check-scripts.sh`, and therefore through:

```text
./scripts/check-all.sh
```

The exact repository gates still need to run on the final descendant containing
this preparation. No CI status is claimed here.
