# V2-031 — Atomic Blob Add

**Status:** Complete  
**Task ID:** V2-031  
**Phase baseline SHA:** `69fd904cf1897870dccc6d633ea57e731d8f3782`  
**Initial upload-adapter SHA:** `8d4b3fee9f1504d7d6eb0606420ae2bfd26aa86c`  
**Implementation evidence SHA:** `52d19f459b0e24f71a6c271fd92a6a7897009b6a`  
**Evidence date:** 2026-08-05

## 1. Completion scope

V2-031 implements the authenticated `POST /api/v1/blob` operation as a bounded,
opaque `application/gzip` upload. Firmware streams request bytes directly into a
temporary LittleFS file, synchronizes the staged data, reserves the next blob ID,
and uses rename as the final-file commit point.

The implementation does not decompress the payload, inspect gzip headers, parse
repository JSON, derive repository metadata, or reinterpret the uploaded bytes.

The completed task includes:

- an exact `POST /api/v1/blob` route registered before the wildcard API route;
- strict `application/gzip` media-type enforcement;
- the contract maximum of 131,072 bytes;
- 1,024-byte bounded receive/write chunks;
- exclusive `<id>.gz.tmp` staging-file creation;
- explicit handling of receive, short-write, write, flush, file-sync, close,
  counter-persistence, pre-rename stat, rename, cleanup, and parent-sync errors;
- rename as the final-file commit point;
- `201 Created` only after the final file exists and the commit sequence has
  completed;
- an API success envelope containing the decimal blob ID and stored byte size;
- preservation of every existing final blob on all failure paths.

## 2. HTTP boundary

The HTTP server registers this exact route:

```text
POST /api/v1/blob
Content-Type: application/gzip
Authentication: MKSESSION cookie
Maximum body: 131072 bytes
```

The route is separate from the JSON request allocator. It receives no more than
`STORAGE_BLOB_UPLOAD_CHUNK_BYTES` (1,024 bytes) at a time and immediately passes
each chunk to the storage transaction. The complete blob is never buffered in
RAM.

The handler rejects:

- an empty body with `400`;
- an oversized body with `413`;
- a missing or noncanonical media type with `415`;
- a missing or invalid session with `401`;
- storage-unavailable or collision/degraded conditions with `503`;
- storage exhaustion with `507`;
- internal I/O or response failures with `500`.

A supplied request ID is validated; otherwise a UUID request ID is generated.
Invalid supplied IDs return `400`. Internal UUID-generation failures return
`500` and are not mislabeled as client errors.

## 3. Atomic storage transaction

### 3.1 Begin

`storage_blob_upload_begin()` snapshots the next ID established by V2-030 and
constructs:

```text
/data/repository/<20-digit-id>.gz.tmp
/data/repository/<20-digit-id>.gz
```

Both the final and temporary paths must be absent. The temporary file is opened
with exclusive-create semantics (`wbx`), so concurrent requests cannot own the
same staged ID and no existing temporary or final file is truncated.

### 3.2 Streaming writes

Every receive chunk is passed to `storage_blob_upload_write()`. The transaction
tracks the declared content length and rejects writes that would exceed it.
A write is successful only when the filesystem reports the complete requested
length. A zero-length, short, or failed write is not accepted as progress.
`ENOSPC` maps to `APP_ERROR_STORAGE_FULL`; other write failures map to explicit
I/O failure.

### 3.3 Pre-commit durability and ID reservation

After exactly the declared byte count has been written, commit performs this
ordered sequence:

1. flush the temporary stream;
2. synchronize the temporary file;
3. close the temporary stream;
4. recheck that the final path is absent;
5. persist `next_blob_id = current_id + 1` in NVS;
6. rename the temporary path to the final path;
7. synchronize the parent directory where the platform supports directory
   synchronization.

The counter is reserved before rename. This prevents a counter-persistence
failure from creating a final blob while reporting a pre-commit error. A later
rename failure can leave a skipped numeric ID, but it cannot overwrite a final
file or reuse a committed ID after reboot.

### 3.4 Commit point and parent synchronization

Rename is the commit point. Before a successful rename, failures remove the
staged temporary file and leave existing final blobs unchanged. Cleanup errors
are returned explicitly rather than hidden behind the primary failure.

POSIX host builds open and `fsync` the parent directory. ESP-IDF/LittleFS does
not expose directory `fsync`; that platform adapter explicitly documents the
completed LittleFS rename as its available metadata boundary. This is an
intentional capability decision, not an ignored runtime error.

If parent synchronization fails after rename on a platform that supports it,
the transaction remains marked committed and the final file is not deleted.
The operation reports the durability error instead of pretending the rename did
not occur.

## 4. Failure-path guarantees

The implementation and native tests verify that existing final blobs remain
unchanged for:

- an existing final path;
- an existing temporary path;
- receive timeout or receive failure;
- declared-length mismatch;
- oversized write attempt;
- short write;
- ordinary write error;
- storage-full write error;
- flush failure;
- temporary-file synchronization failure;
- close failure;
- pre-rename stat failure;
- NVS counter-persistence failure;
- rename failure;
- staged-file cleanup failure.

A cleanup failure is surfaced and may leave only the uncommitted `.tmp` file.
Startup recovery and stale-temporary cleanup belong to V2-033 and remain open.
No code treats a temporary file as a valid final blob.

## 5. Test and analysis coverage

`test_storage_blob_upload.c` provides an operation-recording filesystem/NVS fake
and verifies:

- exact successful operation ordering;
- multiple bounded writes and byte preservation;
- canonical temporary and final paths;
- exclusive ownership and conflict behavior;
- maximum-ID and argument validation;
- all pre-commit failures listed above;
- cleanup-error visibility;
- post-commit parent-sync failure semantics;
- persisted next-ID advancement;
- no final-file creation on pre-commit failures.

The web adapter tests verify bounded binary streaming, timeout retry limits,
receive errors, oversized bodies, consumer failures, and maximum chunk size.
The API-core and request-policy tests verify the exact blob route,
`application/gzip` matching, body requirements, session requirements, and
contract status mappings.

The browser launcher remains fail-closed. Its DevTools startup timeout was
increased from 15 to 30 seconds after the dedicated Browser workflow passed but
the same Chrome startup exceeded 15 seconds under the fully loaded Quality
matrix. There is no retry, alternate browser, skipped test, or silent fallback.

## 6. Exact CI evidence

All primary workflows completed successfully on implementation evidence SHA
`52d19f459b0e24f71a6c271fd92a6a7897009b6a`:

- [Host Tests run 31059119008](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31059119008)
- [Browser Tests run 31059118803](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31059118803)
- [Device Test Build run 31059118741](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31059118741)
- [Quality run 31059118709](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31059118709)

The exact-SHA evidence includes:

- native host tests, including the V2-031 upload transaction target;
- ASan and UBSan;
- native coverage enforcement;
- frontend tests, lint, type checking, formatting, and coverage;
- real-Chrome workflows;
- strict C/CMake formatting;
- compiler warnings and clang-tidy;
- first-party stack-frame checks;
- the ESP32-S3 device-test firmware build with ESP-IDF v5.5.5;
- the complete authoritative `./scripts/check-all.sh` gate.

Quality's failed-log upload step was skipped because the authoritative checks
passed.

## 7. Hardware and observed-value applicability

No physical ESP32-S3R8, serial port, browser-to-device HTTP connection, LittleFS
power-interruption fixture, or USB HID host was used for V2-031 closeout. The
Device Test Build workflow compiled the production and device-test firmware but
did not execute the upload on hardware.

Therefore this report does not claim measured device throughput, physical flash
persistence, real power-loss behavior, or observed LittleFS capacity behavior.
Those physical storage and power-cycle requirements remain in V2-035.

## 8. Deferred work and limitations

The following work remains explicitly open:

- V2-032 list, byte-identical load, and explicit deletion;
- V2-033 startup cleanup of interrupted `.tmp` files and degraded-state handling;
- V2-034 capacity accounting, image evidence, `413`, and `507` boundary evidence;
- V2-035 physical storage, reboot, and power-cycle evidence;
- the Phase 3 exit gate.

A pre-rename failure after successful NVS reservation may skip an ID. IDs are
opaque monotonic identifiers, not a gap-free sequence. Skipping is safer than
creating a final blob whose counter reservation failed.

## 9. Completion statement

Every V2-031 checkbox in `docs/TODO_V2.md` is backed by the production
implementation and reproducible exact-SHA evidence above. No V2-032 through
V2-035 task, Phase 3 exit-gate item, or physical-device behavior is claimed
complete. No compatibility adapter, analyzer suppression, warning exemption,
ignored write, hidden cleanup failure, overwrite fallback, gzip parser, or
whole-blob RAM buffer was introduced to satisfy this task.
