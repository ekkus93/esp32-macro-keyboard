# V2-030 — Storage Layout and Scanning

**Status:** Complete  
**Task ID:** V2-030  
**Implementation start SHA:** `4ede23f503cf24a441a7dcd5beca11f2da1045f4`  
**Implementation evidence SHA:** `928472dd2e501d752c7b49db042f3ec9c6dd83e6`  
**Evidence date:** 2026-08-05

## 1. Completion scope

V2-030 establishes the filesystem layout, startup scan, diagnostics, ordering, and
collision-safe next-ID derivation for the v2 opaque snapshot repository. Firmware
treats each final blob as uninterpreted bytes and does not inspect gzip headers,
decompress content, parse repository JSON, or derive repository metadata.

The completed task includes:

- non-formatting userdata LittleFS mount behavior;
- `/data/repository/` as the single final-blob directory;
- fixed-width 20-digit decimal `<id>.gz` final filenames;
- startup scanning of final regular files only;
- explicit diagnostics for invalid names and nonregular entries;
- numeric newest-first ordering;
- next-ID derivation from both NVS and the maximum scanned final ID;
- collision prevention after a stale, absent, or erased NVS counter.

## 2. Production changes

### 2.1 Filesystem layout and mount policy

`storage_mount.c` mounts the userdata LittleFS partition with
`format_if_mount_failed = false`. A mount failure remains an explicit storage
failure and cannot silently format user data.

`storage_blob.h` defines `/data/repository` and the canonical filename contract:
20 decimal digits followed by `.gz`. ID zero, short names, long names,
non-decimal characters, incorrect suffixes, and integer overflow are rejected as
final blob names.

### 2.2 Startup scanning and ordering

The startup topology creates the repository directory when absent, rejects a
non-directory at that path, reads the persisted next-ID counter, and scans the
final directory. The scanner:

- ignores `.` and `..`;
- accepts only canonical final names that resolve to regular files;
- records byte size without reading file contents;
- counts and reports invalid names;
- derives the maximum numeric ID;
- sorts valid entries by numeric ID, newest first;
- closes the directory and propagates read, stat, close, allocation, and observer
  failures instead of dropping them silently.

The scan implementation was split into bounded helpers and a typed scan context
to satisfy strict complexity analysis without suppressing the finding.

### 2.3 NVS counter and overwrite prevention

The device build reads the optional `next_blob_id` value from the `storage` NVS
namespace. A missing namespace or key means that no counter has been persisted.
A type mismatch is reported as storage corruption, and other NVS access failures
are reported as storage unavailable.

The next ID is:

```text
max(normalized persisted next ID, maximum scanned final ID + 1)
```

A missing or erased counter normalizes to ID 1. Existing final files remain
authoritative for collision prevention, so a stale or erased counter cannot
cause a later add operation to select an existing final filename. A scanned
`UINT64_MAX` final ID produces `APP_ERROR_STORAGE_FULL` rather than wrapping.

### 2.4 Diagnostics and stack budget

Startup scan state exposes the valid count, invalid count, maximum ID, next ID,
and bounded invalid-name details to diagnostics. Diagnostics reject internally
inconsistent or truncated scan snapshots instead of silently returning partial
results.

The bounded storage and web diagnostics snapshots are allocated with checked heap
allocations and freed on every success and failure path. This keeps
`web_diagnostics_handle` below the repository's 4,096-byte first-party stack
frame limit without adding an analyzer exemption or weakening the gate.

## 3. Test and analysis coverage

The native storage tests cover:

- minimum, maximum, malformed, overflow, zero, suffix, and buffer-size filename
  cases;
- no-file, stale-counter, erased-counter, ahead-counter, and maximum-ID next-ID
  derivation;
- repository directory creation and non-directory rejection;
- regular final files, invalid names, temporary names, and nonregular entries;
- stored byte sizes and numeric newest-first ordering;
- scan observer results and persisted-counter precedence;
- storage mount success and rollback behavior;
- existing generic atomic-file and parent-directory synchronization behavior.

The web adapter and frontend diagnostics tests cover scan-state serialization,
invalid-name reporting, truncation rejection, and diagnostics presentation.
Strict formatting, compiler warnings, clang-tidy, stack-frame analysis, native
coverage, frontend coverage, sanitizers, real-Chrome tests, and the ESP32-S3
firmware build all passed.

## 4. Commands and results

The authoritative evidence was produced from clean GitHub Actions checkouts of
`928472dd2e501d752c7b49db042f3ec9c6dd83e6` on Ubuntu 24.04.4.

| Command or gate | Result |
| --- | --- |
| `./scripts/run-tests.sh` | 34 of 34 native host tests passed; 8 storage-labeled tests |
| Host ASan and UBSan job | Passed the complete host suite |
| Native coverage job | Passed all committed native coverage gates |
| Frontend test, typecheck, lint, stylelint, and formatting gates | Passed |
| Frontend coverage job | Passed all committed frontend coverage gates |
| Real Chrome workflow | Passed |
| ESP32-S3 device-test firmware build | Passed; build only, not device execution |
| `./scripts/check-all.sh` | Passed in the Quality workflow |

Toolchain verified by CI:

- ESP-IDF `v5.5.5`;
- target `esp32s3`;
- Node.js `v24.18.0`;
- Ubuntu `24.04.4` runner image.

## 5. Exact CI evidence

All primary workflows completed successfully on the implementation evidence SHA:

- [Browser Tests run 31050879749](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31050879749)
- [Host Tests run 31050879747](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31050879747)
- [Device Test Build run 31050879803](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31050879803)
- [Quality run 31050879754](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31050879754)

The Quality job completed `Run authoritative checks` successfully. Its failed-log
upload step was skipped because no failure occurred.

## 6. Hardware and observed-value applicability

No physical ESP32-S3R8, serial port, USB HID host, or power-interruption fixture
was used for V2-030 closeout. The Device Test Build workflow compiled the device
firmware but did not execute it on hardware. No persistence, power-cycle,
filesystem-capacity, or mount-failure runtime claim is made from that build.

The hardware evidence required by V2-035 remains open. V2-030 is limited to the
implemented scan contract and reproducible host, browser, static-analysis, and
firmware-build evidence described above.

## 7. Deferred work and limitations

The following work remains explicitly open and is not claimed by this report:

- V2-031 bounded streaming gzip upload and atomic final-file commit;
- V2-032 list, byte-identical load, and explicit deletion;
- V2-033 interrupted-temporary-file cleanup and degraded storage states;
- V2-034 capacity accounting, `413`, `507`, and LittleFS image evidence;
- V2-035 physical storage and power-cycle evidence;
- the Phase 3 exit gate.

The NVS counter is read during V2-030 startup. Persisting the advanced counter as
part of a committed add belongs to V2-031 and is not claimed here.

## 8. Completion statement

Every V2-030 checkbox in `docs/TODO_V2.md` is backed by the implementation and
reproducible evidence identified above. No unchecked V2-031 through V2-035 task,
Phase 3 exit-gate item, or physical-device behavior is being claimed complete.
No compatibility adapter, silent fallback, ignored error, warning suppression,
analyzer exclusion, or format-on-failure path was introduced to satisfy this
task.
