# V2-032 — List, Load, and Delete

**Status:** Complete  
**Task ID:** V2-032  
**Implementation evidence SHA:** `5881139a0e0d80dc05c2dd9a87b6ff7ba2f09f1c`  
**Evidence date:** 2026-08-05

## 1. Completion scope

V2-032 implements authenticated listing, byte-identical loading, and explicit
deletion for opaque repository blobs. The firmware treats every final `.gz` file
as uninterpreted bytes. It does not inspect gzip headers, decompress data, parse
repository JSON, derive package or macro metadata, or compute a checksum, hash,
digest, or CRC.

The completed task includes:

- newest-first `GET /api/v1/blob` listing with decimal ID and stored byte size;
- byte-for-byte `GET /api/v1/blob/{blob_id}` streaming as `application/gzip`;
- exact `DELETE /api/v1/blob/{blob_id}` removal with `204 No Content`;
- deletion of the final remaining blob;
- no replacement selection, implicit load, or automatic fallback;
- strict canonical decimal blob-ID parsing;
- explicit errors for missing, nonregular, truncated, or inconsistent files;
- host tests for list, load, delete, route, parser, and hostile callback paths.

## 2. HTTP boundary

The HTTP server registers these authenticated routes:

```text
GET    /api/v1/blob
GET    /api/v1/blob/{blob_id}
DELETE /api/v1/blob/{blob_id}
```

The collection response contains only the blob records and storage accounting
required by the v2 contract:

```json
{
  "blobs": [
    {"id": "3", "sizeBytes": 1332},
    {"id": "2", "sizeBytes": 1298}
  ],
  "usedBytes": 2630,
  "remainingBytes": 492000
}
```

Each blob record contains only `id` and `sizeBytes`. Unsupported methods on a
valid blob-item path are classified as method errors rather than falling through
to an unrelated wildcard `404`.

Blob IDs must be canonical positive decimal values. The parser rejects zero,
leading zeroes, signs, non-digits, query strings, fragments, percent-encoded
paths, extra slashes, traversal forms, values wider than the configured decimal
identifier contract, and unsigned 64-bit overflow.

## 3. Newest-first listing

`storage_blob_list()` rescans the repository directory and returns valid final
blob files in descending numeric-ID order. Ordering is numeric, not
lexicographic. Temporary files, malformed names, directories, and other
nonregular entries are not returned as blobs.

The list reports the stored byte size obtained from the filesystem. It does not
open or inspect the payload to infer repository meaning. The list operation also
refreshes storage scan state so diagnostics and subsequent operations do not rely
on a stale in-memory catalog.

The JSON builder transfers cJSON ownership only after all required fields are
created. Failure to attach the entries array cannot leak the detached array.

## 4. Byte-identical loading

The selected final blob is opened through a bounded storage reader. The reader
tracks the expected file size from `stat`, emits no more than the caller's
requested capacity, and returns the exact stored bytes without transformation.

The HTTP handler sends `application/gzip` and streams bounded chunks. It does not
buffer the complete blob in RAM.

Failure behavior is fail-closed:

- missing IDs return an explicit not-found result;
- nonregular files are rejected;
- read errors are surfaced;
- premature EOF or truncation is reported as storage corruption;
- a filesystem callback that claims more bytes than requested is rejected before
  counters advance or memory is compared outside the valid region;
- close and response-send failures remain visible.

The hostile callback regression test proves that an impossible over-reported
read cannot be treated as successful progress.

## 5. Exact deletion

Deletion constructs only the canonical final path for the selected numeric ID and
unlinks exactly that file. It never deletes another final blob, never treats a
temporary file as the selected final blob, and never chooses a replacement after
success.

Deleting the sole remaining blob is valid. The in-memory valid-blob count is
updated without lowering or reusing the monotonic next-ID counter.

On platforms supporting directory synchronization, the parent directory is
synchronized after unlink. If parent synchronization fails after the unlink, the
deletion remains committed and the durability failure is returned explicitly.
The code does not pretend the file still exists or silently discard the error.

## 6. Defensive hardening found during the Ralph loop

The validation loop found and corrected several issues rather than suppressing
them:

1. Blob-item paths were added to common API route classification, so unsupported
   methods do not silently fall through to a wildcard `404`.
2. The list-response builder's cJSON ownership order was fixed to prevent a
   detached-array leak on attachment failure.
3. Filesystem read/write callback results are handled through a width-safe signed
   boundary, with explicit rejection when a callback reports more bytes than the
   request permitted.
4. Hostile over-reporting tests were added for blob streaming and atomic upload
   verification.
5. The API blob-ID width is an API contract constant, while the integrated web
   handler uses a compile-time assertion to prevent drift from the storage
   filename width. This preserves standalone API-core testability without a
   hidden cross-component dependency.
6. Analyzer findings about swappable string parameters, unclear variable names,
   and duplicated numeric constants were fixed in source. No analyzer
   suppression, warning exemption, ignored exit code, or compatibility shim was
   added.

## 7. Test and analysis coverage

V2-032 behavior is covered by the following native targets and related suites:

- `storage_blob_access` — arbitrary and empty byte identity, missing and
  nonregular files, exact deletion, deletion of the final blob, preservation of
  unrelated final and temporary files, and hostile over-reported reads;
- `storage_blob_scan` — valid final-file discovery and numeric newest-first
  ordering;
- `web_api_core` — collection/item routing, canonical decimal parsing, maximum
  unsigned ID, malformed/overflow IDs, session policy, body policy, and exact
  method policy;
- `web_server_adapter` and request-policy coverage — bounded binary streaming and
  HTTP contract behavior;
- `storage_blob_upload` — hostile over-reported filesystem callback rejection at
  the atomic verifier boundary.

The exact Host workflow completed all five jobs: Host Tests, Host ASan/UBSan,
Native Coverage, Frontend Tests, and Frontend Coverage. The repository's workflow
logs expose job-level pass/fail evidence rather than one aggregate C assertion
count; all five jobs passed.

Commands represented by the authoritative workflows include:

```bash
./scripts/run-tests.sh
./scripts/run-tests.sh --sanitizers
bash ./scripts/generate-native-coverage.sh
npm --prefix webapp run typecheck
npm --prefix webapp run lint
npm --prefix webapp run stylelint
npm --prefix webapp run format:check
npm --prefix webapp test
bash ./scripts/generate-frontend-coverage.sh
./scripts/check-all.sh
```

The Browser workflow also built the frontend and executed the real-Chrome
workflow suite. The Device Test Build workflow linted device-test sources and
compiled the ESP32-S3 device-test firmware with ESP-IDF v5.5.5.

## 8. Exact CI evidence

All primary workflows completed successfully on exact implementation evidence
SHA `5881139a0e0d80dc05c2dd9a87b6ff7ba2f09f1c`:

- [Host Tests run 31069702496](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31069702496)
- [Browser Tests run 31069702406](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31069702406)
- [Device Test Build run 31069702401](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31069702401)
- [Quality run 31069702425](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31069702425)

The exact-SHA evidence includes strict compiler warnings, C/CMake formatting,
clang-tidy/static analysis, first-party stack-frame checks, ASan, UBSan, native
coverage, frontend type checking/lint/formatting/tests/coverage, real Chrome,
ESP32-S3 device-test compilation, and the complete authoritative quality gate.

## 9. Hardware and observed-value applicability

No physical ESP32-S3R8, serial port, browser-to-device HTTP connection, LittleFS
power-cycle fixture, or USB HID host was used for V2-032 closeout. The Device
Test Build workflow compiled production/device-test code but did not execute
list, load, or delete on hardware.

Therefore this report does not claim measured device throughput, physical flash
persistence, power-loss durability, observed filesystem capacity, or
browser-to-device byte identity. Those physical requirements remain open in
V2-035.

## 10. Deferred work and limitations

The following work remains explicitly open:

- V2-033 boot cleanup of interrupted `.tmp` files and degraded-state behavior;
- V2-034 capacity/image evidence and final candidate blob-limit validation;
- V2-035 physical add/reboot/load/delete/interruption/full-storage evidence;
- the Phase 3 exit gate;
- later Phase 5 consolidation of the complete documented HTTP API.

A post-unlink directory-sync error reports failure even though deletion is
already committed. This is intentional fail-closed durability reporting, not a
rollback claim or silent success.

## 11. Completion statement

Every V2-032 checkbox in `docs/TODO_V2.md` is backed by production
implementation, native regression tests, and reproducible exact-SHA evidence
above. No V2-033 through V2-035 task, Phase 3 exit-gate item, Phase 5 API task, or
physical-device behavior is claimed complete. No gzip parser, repository JSON
parser, automatic replacement selection, compatibility adapter, analyzer
suppression, warning exemption, ignored filesystem error, whole-blob RAM buffer,
or quiet fallback was introduced to satisfy this task.
