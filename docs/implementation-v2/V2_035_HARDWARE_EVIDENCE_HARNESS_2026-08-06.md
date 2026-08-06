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
status or diagnostic state, and refuses to finalize unless all seven physical
scenarios have a passing observation.

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

1. Flash the production firmware from the exact commit being validated.
2. Complete first-run provisioning and connect the test computer to the board.
3. Confirm `GET /api/v1/status` is reachable through the production network
   path.
4. Use ESP-IDF v5.5.5 for the destructive partition stage.
5. Keep the working state outside the repository until finalization.

Example environment:

```bash
export DEVICE_URL='http://192.168.4.1'
export V2_035_PASSWORD='the-current-device-password'
export V2_035_STATE='/tmp/esp32-macro-keyboard-v2-035-state.json'
```

Do not place the password directly in a shell command, evidence file, serial
log, or Git commit.

## Stage 1 — Numeric ordering and deletion preservation

```bash
python3 scripts/run-v2-035-hardware.py start \
  --base-url "${DEVICE_URL}" \
  --state "${V2_035_STATE}"
```

This stage:

- snapshots every pre-existing final blob and its SHA-256;
- creates three deterministic gzip blobs;
- requires strictly increasing created IDs;
- requires the list endpoint to return numeric order;
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
- diagnostics report `temporaryFiles == 0`; and
- diagnostics do not report `scanFailed == true`.

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
