# V2-035 — Physical storage evidence harness

**Status:** Ready for physical execution; V2-035 remains open  
**Task:** V2-035  
**Target:** ESP32-S3R8 running the production V2 firmware

## Purpose

V2-035 is a physical-hardware exit gate. Hosted CI can build the firmware and
validate storage behavior in host/image tests, but it cannot prove persistence
through real power loss, real LittleFS exhaustion, or failure behavior of the
board's actual flash.

`scripts/run-v2-035-hardware.py` collects those observations as a staged,
fail-closed evidence record. It never stores the administrator password or
session cookie. The password is read only from an environment variable.

The collector does not check a V2-035 TODO item merely because a command ran.
Each stage verifies byte identity with SHA-256, verifies the exact expected HTTP
status or diagnostic state, binds the record to the exact 40-character firmware
commit supplied at the first stage, and refuses to finalize unless all seven
physical scenarios have a passing observation.

The HTTP client uses the current V2 production routes: `/api/v1/auth/login`,
`/api/v1/blob`, `/api/v1/blob/{id}`, and `/api/v1/diagnostics`. Legacy
plural blob paths are rejected by the collector regression suite.

## Safety boundaries

- Run against a dedicated validation board or back up all important device data.
- The mount-failure stage deliberately replaces the entire `userdata`
  partition with an invalid 524,288-byte image.
- The procedure reads a byte-for-byte backup before corruption and verifies the
  restored partition against that backup afterward.
- Never commit the raw backup, corrupt, post-boot, or restored partition images.
  They can contain repository payloads or other device data.
- Flash encryption or secure boot can intentionally prevent raw partition
  replacement. Do not disable either protection merely to run this test.
- V2-035 remains incomplete unless the actual reference hardware can execute
  every required scenario.

## Prerequisites

1. Check out the exact clean commit being validated and source ESP-IDF v5.5.5.
2. Build the production firmware and generate `firmware/build/flash-manifest.json`
   with `scripts/generate-flash-manifest.sh`. The manifest must report
   `gitDirty: false` and `buildType: production`.
3. Flash the application image represented by that exact manifest.
4. Complete first-run provisioning and connect the test computer to the board.
5. Confirm `GET /api/v1/status` is reachable through the production network
   path.
6. Keep the working state outside the repository until finalization.

Example build provenance commands:

```bash
git status --short
idf.py -C firmware build
bash scripts/generate-flash-manifest.sh
python3 -m json.tool firmware/build/flash-manifest.json
```

The collector rejects dirty or development manifests, requires exactly ESP-IDF
v5.5.5, hashes the application image named by the manifest, reruns `esptool.py
image_info` to verify the full ELF SHA-256, and refuses to start if the board's
39-character diagnostics `buildId` differs from that verified ELF SHA prefix.
These checks happen before any V2-035 blob is created or deleted.

Example environment:

```bash
export DEVICE_URL='http://192.168.4.1'
export FLASH_MANIFEST="${PWD}/firmware/build/flash-manifest.json"
export V2_035_PASSWORD='the-current-device-password'
export V2_035_STATE='/tmp/esp32-macro-keyboard-v2-035-state.json'
```

Do not place the password directly in a shell command, evidence file, serial
log, or Git commit.

## Recovery after a failed mutating stage

The collector writes its state before the first mutation and journals every
collector-owned blob immediately after creation. If `start`, `fill-storage`, or
`finalize` fails or the host process is interrupted, do not delete IDs by hand.
Run:

```bash
python3 scripts/run-v2-035-hardware.py recover-cleanup \
  --state "${V2_035_STATE}"
```

Recovery verifies every baseline hash, refuses to touch unowned IDs, verifies
each surviving collector-owned blob before deletion, tolerates an owned blob
that was already deleted immediately before a host crash, restores the exact
pre-test blob set, and only then removes the local state file. After recovery,
restart V2-035 from Stage 1 with a newly generated state path.

## Stage 1 — Numeric ordering and deletion preservation

```bash
python3 scripts/run-v2-035-hardware.py start \
  --base-url "${DEVICE_URL}" \
  --flash-manifest "${FLASH_MANIFEST}" \
  --state "${V2_035_STATE}"
```

This stage:

- snapshots every pre-existing final blob and its SHA-256;
- creates three deterministic gzip blobs;
- requires strictly increasing created IDs;
- requires the list endpoint to return newest-first numeric order;
- loads every new blob and verifies byte identity;
- deletes the middle blob; and
- proves all pre-existing blobs and both remaining test blobs are unchanged.

Two small sentinel blobs remain for the power-cycle tests. Only blobs created by
the collector are ever deleted by the collector.

## Stage 2 — Real power-cycle persistence

Remove all power from the ESP32-S3. Do not use only a software reset. Restore
power, wait for the production network path to return, and run:

```bash
python3 scripts/run-v2-035-hardware.py verify-power-cycle \
  --state "${V2_035_STATE}"
```

The stage reloads every baseline and sentinel blob and requires all hashes to
match the pre-power-cycle values.

## Stage 3 — Interrupted upload and reboot cleanup

Start the deliberately slow, maximum-length upload:

```bash
python3 scripts/run-v2-035-hardware.py arm-interrupted-upload \
  --state "${V2_035_STATE}"
```

Wait for this exact banner:

```text
=== CUT POWER NOW: remove ESP32-S3 power, then restore it ===
```

Immediately remove power. The collector must observe the connection loss after
at least 16,384 bytes were transmitted. Restore power, wait for the board, and
run:

```bash
python3 scripts/run-v2-035-hardware.py verify-interrupted-upload \
  --state "${V2_035_STATE}"
```

The verification requires:

- no new final blob ID;
- every previously committed final blob remains byte-identical;
- `GET /api/v1/diagnostics` reports `blobScan.temporaryFileCount == 0`;
- `blobScan.temporaryFiles` is empty; and
- diagnostics report a physical `power-on` reset with the same build ID.

Closing the client connection normally is not equivalent to this test because
the production handler can then execute normal abort cleanup. The required
observation is loss of board power while the temporary file is active.

## Stage 4 — Real storage exhaustion and HTTP 507

```bash
python3 scripts/run-v2-035-hardware.py fill-storage \
  --state "${V2_035_STATE}"
```

The collector builds a valid deterministic gzip body of exactly 131,072 bytes
and uploads copies until a within-limit request returns HTTP 507. It then:

- reloads every baseline, sentinel, and successfully committed fill blob;
- verifies every SHA-256;
- removes only fill blobs created by this stage; and
- verifies the pre-stage blob set again.

Any status other than 201 before exhaustion or 507 at exhaustion fails the
stage.

## Stage 5 — Mount failure without formatting

Set the serial port and enter the production firmware directory:

```bash
export PORT='/dev/ttyACM0'
cd firmware
. "${IDF_PATH}/export.sh"
mkdir -p /tmp/v2-035-mount
```

Read the real partition by name rather than relying on a hard-coded offset:

```bash
parttool.py --port "${PORT}" read_partition \
  --partition-name=userdata \
  --output /tmp/v2-035-mount/backup.bin
```

Create a deterministic invalid image with the exact production partition size:

```bash
python3 - <<'PY'
from pathlib import Path
Path('/tmp/v2-035-mount/corrupt.bin').write_bytes(b'\x00' * 524288)
PY
```

Write it, reboot, and capture the production serial failure. Keep the complete
serial output:

```bash
parttool.py --port "${PORT}" write_partition \
  --partition-name=userdata \
  --input /tmp/v2-035-mount/corrupt.bin

esptool.py --port "${PORT}" run
set +e
timeout --signal=INT 30s idf.py -p "${PORT}" monitor \
  | tee /tmp/v2-035-mount/mount-failure.log
set -e
```

Read the partition after the failed boot, restore the backup, reboot, and read
it once more:

```bash
parttool.py --port "${PORT}" read_partition \
  --partition-name=userdata \
  --output /tmp/v2-035-mount/post-boot.bin

parttool.py --port "${PORT}" write_partition \
  --partition-name=userdata \
  --input /tmp/v2-035-mount/backup.bin

esptool.py --port "${PORT}" run
sleep 10

parttool.py --port "${PORT}" read_partition \
  --partition-name=userdata \
  --output /tmp/v2-035-mount/restored.bin
```

Return to the repository root and record the observation:

```bash
cd ..
python3 scripts/run-v2-035-hardware.py record-mount-failure \
  --state "${V2_035_STATE}" \
  --backup-image /tmp/v2-035-mount/backup.bin \
  --corrupt-image /tmp/v2-035-mount/corrupt.bin \
  --post-boot-image /tmp/v2-035-mount/post-boot.bin \
  --restored-image /tmp/v2-035-mount/restored.bin \
  --serial-log /tmp/v2-035-mount/mount-failure.log
```

This stage fails unless:

- every image is exactly 524,288 bytes;
- the post-boot image is byte-identical to the corrupt image;
- the restored image is byte-identical to the original backup;
- the corrupt image differs from the backup;
- the serial log explicitly reports a mount failure; and
- the serial log contains no explicit formatting-success marker.

The equality of the corrupt and post-boot partition hashes is the primary proof
that the failed mount did not silently format or otherwise rewrite userdata.

## Stage 6 — Finalize and validate evidence

After the restored production device is reachable again:

```bash
mkdir -p docs/hardware-evidence
python3 scripts/run-v2-035-hardware.py finalize \
  --state "${V2_035_STATE}" \
  --output docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-06.json

python3 scripts/run-v2-035-hardware.py validate \
  --evidence docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-06.json
```

Finalization reloads the remaining sentinels, deletes only those collector-owned
blobs, verifies the original baseline again, and emits the sanitized evidence
file. Raw partition images and the complete serial log remain outside Git.

## Completion rule

Only after the finalized evidence JSON is reviewed and committed may the seven
V2-035 checklist items and the hardware-dependent Phase 3 exit gate be marked
complete. Hosted CI success alone is not physical evidence.
