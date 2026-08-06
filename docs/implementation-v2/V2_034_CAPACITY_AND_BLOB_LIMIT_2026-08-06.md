# V2-034 — Capacity and candidate blob limit

**Status:** Complete  
**Task:** V2-034  
**Authoritative implementation commit:** `5b138fe21c49b94938713a2433ca67b606f4f41e`

## Scope

V2-034 validates the candidate repository-blob limit against the real userdata
LittleFS geometry, permanently gates the capacity and HTTP admission contracts,
and records exact-SHA CI evidence. It does not claim the physical-board storage
and power-loss evidence reserved for V2-035.

## Final accepted limit

The accepted maximum repository blob request body is:

```text
131,072 bytes (128 KiB)
```

The value remains defined by the authoritative firmware constant:

```c
#define APP_V2_BLOB_MAX_BYTES UINT32_C(131072)
```

`GET /api/v1/limits` derives `blobMaxBytes` from that same constant. The value was
not reduced because the real LittleFS image proof leaves a positive and substantial
margin after representing two maximum-size final blobs and one maximum-size
interrupted temporary upload.

## Real LittleFS capacity evidence

The permanent gate uses `littlefs-python==0.15.0`, a 4,096-byte block size, and
the production userdata partition from `firmware/partitions.csv`.

| Measurement | Bytes |
| --- | ---: |
| Userdata partition | 524,288 |
| Maximum blob | 131,072 |
| Two final blobs plus one temporary blob | 393,216 |
| LittleFS used bytes | 421,888 |
| Directory and filesystem overhead | 28,672 |
| Remaining bytes | 102,400 |

The generated image contains exactly:

```text
/repository/00000000000000000001.gz
/repository/00000000000000000002.gz
/repository/00000000000000000003.gz.tmp
```

The gate writes 131,072 bytes to each file, reads each file back, verifies its
size and SHA-256 byte identity, verifies the generated image is exactly 524,288
bytes, and fails if the observed used/overhead/remaining geometry drifts from the
reviewed values. It also requires at least one full LittleFS block of remaining
capacity; the observed margin is 25 blocks.

The initial real-image probe passed in workflow run `31083618598`, job
`92557853441`. The same proof is now permanent in
`scripts/check-v2-034-capacity.py` and runs through `scripts/check-all.sh`.

## Capacity reporting contracts

The permanent gate verifies the existing exact v2 schema split:

- the status response reports userdata `totalBytes`, `usedBytes`, and
  `remainingBytes`;
- `GET /api/v1/limits` reports `blobMaxBytes` from
  `APP_V2_BLOB_MAX_BYTES`; and
- the blob-list response remains exactly responsible for `blobs`, `usedBytes`,
  and `remainingBytes` and is not silently expanded with `totalBytes`,
  `maxBlobBytes`, or `blobMaxBytes`.

No compatibility field or silent schema change was introduced.

## Oversized request behavior: HTTP 413

`blob_create_handler()` compares `request->content_len` with
`APP_V2_BLOB_MAX_BYTES` and returns `413 Payload Too Large` directly.

The permanent source-contract proof verifies the following ordering:

1. read the request content length;
2. reject a body larger than 131,072 bytes with HTTP 413;
3. only then inspect `Content-Type`;
4. only then authenticate the request; and
5. only then initialize a storage upload.

An oversized body therefore cannot create a temporary file, initialize an upload,
or mutate storage. No generic `APP_ERROR_LIMIT` error was added. The gate scans
first-party firmware and host-test C sources and fails if that abandoned error
attempt reappears.

## Within-limit storage exhaustion: HTTP 507

The production path remains:

```text
LittleFS/write ENOSPC
→ APP_ERROR_STORAGE_FULL
→ 507 Insufficient Storage
```

The permanent gate verifies the production mapping and the host-test assertion
for HTTP 507. It also verifies the atomic upload invariants relevant to storage
exhaustion:

- upload creation opens only the temporary path;
- rename from temporary to final is the commit point;
- abort cleanup unlinks only the temporary path; and
- the host fake rejects any attempt to unlink a final path.

The existing `test_write_failures_abort_cleanly` host test injects `ENOSPC`,
expects `APP_ERROR_STORAGE_FULL`, verifies no final file exists, and verifies the
temporary file is removed. Previously committed final blobs are never selected
as cleanup targets, replaced, or deleted by this failure path.

## Permanent implementation

The accepted implementation consists of:

| Commit | Change |
| --- | --- |
| `bc45f8c2b8ce69c048d3c9b02355733cb8a7b517` | Correct the fail-closed V2-034 capacity and HTTP-contract proof. |
| `cdf7bf0be3e0f336ad728f920c5a31d623893495` | Add the V2-034 proof to `scripts/check-all.sh`. |
| `90ee2a235a95fe08b81e46e2c8d436161e8d9be9` | Remove the temporary capacity-probe workflow. |
| `8309a47ccb3dd4e7191356642a83eeb9f9f9f430` | Remove the failed temporary implementation workflow. |
| `5b138fe21c49b94938713a2433ca67b606f4f41e` | Install the pinned LittleFS dependency in the ESP-IDF Python environment used by Quality. |

Permanent files changed:

- `scripts/check-v2-034-capacity.py`
- `scripts/check-all.sh`
- `.github/workflows/quality.yml`

Temporary V2-034 probe and implementation workflows were deleted. No production
blob-storage or HTTP implementation was weakened or changed to make the gate pass.

## Acceptance correction

The first permanent exact-SHA attempt was commit
`8309a47ccb3dd4e7191356642a83eeb9f9f9f430`. Host Tests and Browser Tests passed,
but Quality run `31087004688`, job `92568721634`, failed closed because
`littlefs-python==0.15.0` was not installed in the ESP-IDF virtual environment
that executes `scripts/check-all.sh`.

The package had been considered only in the host Python environment. The gate did
not skip itself and did not accept a different library version. Commit
`5b138fe21c49b94938713a2433ca67b606f4f41e` corrected the authoritative Quality
setup by installing and verifying exactly `littlefs-python==0.15.0` after sourcing
the ESP-IDF environment. The corrected dependency step and the complete
authoritative gate then passed.

## Reproducible commands

The focused proof is:

```bash
python3 -m pip install littlefs-python==0.15.0
python3 ./scripts/check-v2-034-capacity.py
```

The complete authoritative local/CI gate is:

```bash
./scripts/check-all.sh
```

Host behavior is additionally exercised through:

```bash
./scripts/run-tests.sh
```

The Quality workflow verifies the pinned package version in the same ESP-IDF
Python environment that runs the authoritative gate.

## Exact-SHA validation

All authoritative workflows passed on
`5b138fe21c49b94938713a2433ca67b606f4f41e`:

| Workflow | Run | Job or jobs | Result |
| --- | ---: | --- | --- |
| Host Tests | `31087971174` | `92571859416`, `92571859440`, `92571859515`, `92571859528`, `92571859535` | Success |
| Browser Tests | `31087969416` | `92571853448` | Success |
| Device Test Build | `31087971115` | `92571859885` | Success |
| Quality | `31087971163` | `92571859901` | Success |

Quality completed the permanent V2-034 capacity proof through
`scripts/check-all.sh`; the failed-log upload step was correctly skipped.

## Boundary retained

This completion does not close V2-035 or the Phase 3 hardware exit gate. The
following still require execution and committed evidence on the reference
ESP32-S3R8:

- power-cycle persistence and byte-identical reload;
- multiple-blob numeric ordering;
- deletion preservation;
- interrupted-upload recovery;
- boot temporary-file cleanup;
- real storage-full behavior preserving every committed final blob; and
- mount-failure behavior without formatting.

No unchecked hardware-dependent task is claimed complete by this report.
