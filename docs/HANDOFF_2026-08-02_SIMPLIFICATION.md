# Handoff: simplification decisions, 2026-08-02

Owner decisions from this session. These override earlier documents, including
`docs/SPEC.md`, which is stale wherever it conflicts.

## The product

A macro set is **a name and an ordered list of macros**. That is the whole data
model.

Explicitly **not** part of this product:

- procedures, instruction steps, checkpoint steps;
- per-procedure progress tracking;
- buttons of any kind, or any added hardware;
- quarantine of damaged files.

All of these came from the founding spec commit `686c95d` (2026-07-22). None
were requested. Procedures alone account for ~1,276 lines in three core files
and 92 files repo-wide; the device has zero of them.

## Storage design

The device has **512 KiB** of user storage (`userdata`, LittleFS). Measured
today: 98,304 bytes consumed by *empty directories* while holding 1,370 bytes of
data. LittleFS charges 8 KiB per directory (a metadata pair) and inlines files
under ~512 bytes for free, so on this device directories are the entire cost.

Target layout:

```text
/data/sets/<set-id>.json     name + ordered macros, inline
/data/index.json             set order
```

Writes are `write .tmp` then `rename()` over the target. POSIX rename is atomic,
so a crash leaves either the old complete file or the new one. Boot deletes any
stray `.tmp`; that is the entire recovery routine.

One file per set, not one file for everything: a write then duplicates only the
set being edited rather than the whole repository. Usable space goes from
~252 KiB (single file, every write duplicates all data) to ~480 KiB.

Deleted by this design: `staging/`, `trash/`, `transactions/`, transaction
manifests, the tree walker, per-set directories, order files, and the
reference-integrity machinery (which exists only to keep procedure steps
pointing at real macros). That is the 3,492 lines still in
`storage_transaction*.c`, `storage_atomic*.c`, and `storage_package_restore.c`,
plus most of what surrounds them, against a storage component of 15,130 lines.

Restore touches every set, so it is not atomic across sets. Accepted: restore
each set file atomically and report per-set success/failure to the client. No
staging directory.

Corrupt data is **deleted and the failure reported**. It is never archived.

## Done this session

- PSRAM enabled (ESP32-S3R8, octal). Free heap 208,804 -> 8,465,455.
- Physical confirmation no longer forced: all routes honour the setting, which
  defaults off. `confirm` and `cancel` serial commands replace the buttons.
- Button code removed; `device_controls` keeps only the status indicator and the
  confirmation signal. FIX1 20.5 struck from scope.
- `restore_locked` manifest heap-allocated (was overflowing the httpd stack).
- Main task stack 8192 -> 16384 (boot recovery of an interrupted restore was
  bricking the device into a boot loop).

## Uncommitted work in the tree

Quarantine removal is **half-landed and not committed**:

- Done: all read paths delete corrupt files via
  `storage_repository_discard_corrupt_file()` and report the error;
  `storage_quarantine.c`, its header, its 997-line host test, the 41 KB
  `storage_quarantine_list_t`, the `/data/quarantine/` directory, the
  `/api/v1/diagnostics/quarantine` route, the diagnostics JSON block, and the
  boot-time `storage_quarantine_recover_all()` stage are gone. **Firmware
  builds.**
- Not done: 9 webapp files still reference quarantine (`DiagnosticsPage.tsx`,
  `models.ts`, `routes.ts`, `managementGuards.ts`, and four test files), plus
  remaining host tests. `./scripts/check-all.sh` fails until those are cut.

Finish that to green and commit before starting the storage rewrite.

## Open defect

`POST /api/v1/restore` does not work and never has. The stack overflow is fixed;
it now trips the task watchdog because it performs the whole tree rewrite
synchronously inside the HTTP handler, starving the idle task for over five
seconds. The single-file-per-set design removes the cause rather than treating
it - do not patch restore before the storage rewrite.
