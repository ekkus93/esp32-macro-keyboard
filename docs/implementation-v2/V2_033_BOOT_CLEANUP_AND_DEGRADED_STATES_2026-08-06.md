# V2-033 — Boot cleanup and degraded storage states

**Status:** Complete  
**Task:** V2-033  
**Authoritative implementation commit:** `cea87fc2aa43ff0acd8ced27edd0cbbc45052cb3`

## Scope

V2-033 hardens BlobFS startup recovery and storage-state reporting without
introducing capacity enforcement or retention behavior from V2-034 or V2-035.

## Implemented behavior

### Recovery starts only after `/data` mounts

The storage mount topology mounts the payload filesystem first. BlobFS recovery
runs only after that mount succeeds. A recovery failure is returned to the
caller and the mounted payload is cleaned up rather than being treated as a
successful startup.

### Boot cleanup is narrow and fail-closed

Recovery removes only regular files with the canonical interrupted-write name:

```text
<20-digit-positive-blob-id>.gz.tmp
```

It does not remove final `.gz` files, non-canonical `.tmp` files, directories,
symlinks, or other unexpected entries. Directory iteration, metadata lookup,
unlink, close, and directory-sync failures propagate as errors.

After deleting one or more interrupted temporary files, recovery synchronizes
the BlobFS parent directory so the cleanup is not treated as complete before
the directory update is durable.

### Invalid final entries are preserved and reported

Final `.gz` files are not decoded or deleted during boot recovery. Invalid or
unexpected entries remain available for diagnosis and are counted by the
content scan rather than silently repaired, renamed, or discarded.

### Diagnostics expose recovery state

The fixed diagnostics model reports:

- `storage.content.temporaryFiles`
- `storage.content.invalidFiles`
- `storage.content.scanFailed`

A failed content scan is therefore distinguishable from a clean scan that found
zero temporary or invalid files.

### Mount and recovery failures remain visible

Primary storage mount and recovery errors remain explicit lifecycle failures or
degraded states. Cleanup errors do not overwrite the original failure that made
storage unavailable.

LittleFS registration keeps `format_if_mount_failed = false`; startup never
silently formats storage to recover from a mount failure.

## Regression corrected during acceptance

The first no-format architecture guard recursively scanned vendored ESP-IDF
managed-component examples. That caused Quality run `31077783078` to reject a
LittleFS example that is not linked into the production firmware.

Commit `cea87fc2aa43ff0acd8ced27edd0cbbc45052cb3` corrected the guard so it:

1. explicitly requires exactly one `format_if_mount_failed = false` assignment
   in the production storage mount source;
2. scans only first-party `firmware/components` and `firmware/main` sources;
3. rejects any first-party `format_if_mount_failed = true` assignment; and
4. rejects any first-party `esp_littlefs_format(...)` call.

A vendored example can no longer create a false positive, while a production
formatting regression still fails the test.

## Acceptance evidence

| Requirement | Evidence |
| --- | --- |
| Start recovery only after `/data` is mounted | Mount-topology ordering and host tests verify mount-before-recovery behavior. |
| Remove only interrupted `.tmp` files during boot recovery | Recovery filename/type filtering and host tests cover canonical deletion plus preservation cases. |
| Preserve final `.gz` files and fail closed on invalid entries | Recovery never decodes or deletes final blobs; filesystem-operation failures propagate. |
| Report temporary and invalid files in diagnostics | Diagnostics contract exposes temporary, invalid, and scan-failure fields with host/browser coverage. |
| Keep mount failures explicit and never silently format | Lifecycle tests preserve the primary failure; the no-format architecture guard protects production sources. |

## Exact-SHA validation

All authoritative workflows passed on
`cea87fc2aa43ff0acd8ced27edd0cbbc45052cb3`:

| Workflow | Run | Result |
| --- | ---: | --- |
| Host Tests | `31080685446` | Success |
| Browser Tests | `31080685482` | Success |
| Device Test Build | `31080685450` | Success |
| Quality | `31080685489` | Success |

## Boundary retained

This completion does not close:

- V2-034 BlobFS capacity accounting and admission control;
- V2-035 advisory retention visibility; or
- the Phase 3 real-device recovery and storage-pressure exit gate.
