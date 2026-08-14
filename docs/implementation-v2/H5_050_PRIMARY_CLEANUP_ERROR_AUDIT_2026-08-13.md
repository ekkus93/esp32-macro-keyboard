# H5-050 — Primary/cleanup error provenance audit — 2026-08-13

## Scope

This document closes the audit portion of Phase H5 in
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.
The audited baseline is `773af25139109427c514d3a541ede7e457e1ec4e` on
`master`.

H5-050 is classification work only. It does not change the frozen
`docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`, and it does not claim the H5
runtime fixes are complete. The audit distinguishes four cases:

1. **primary overwritten by cleanup** — incorrect and must be fixed;
2. **primary preserved but secondary cleanup detail lost** — not an overwrite,
   but insufficient for H5's structured provenance goal;
3. **primary and cleanup already preserved separately** — correct pattern to
   reuse; and
4. **commit/durability uncertainty** — a separate semantic problem that must not
   be misclassified as ordinary cleanup failure.

## Search method

The audit inspected the current production paths and their host seams, including
repository searches for cleanup-result ternaries/returns, structured
`primary_error`/`cleanup_error` use, rollback helpers, mount/unmount paths,
blob-upload abort paths, settings recovery/unlock paths, startup teardown, web
server lifecycle rollback, Wi-Fi teardown, and executor shutdown/drain.

The minimum H5-050 categories from the TODO were all covered:

- atomic write staging cleanup;
- rename failure cleanup;
- blob upload temporary-file cleanup;
- mount/unmount rollback;
- shutdown/drain cleanup; and
- `cleanup == NONE ? primary : cleanup`-style replacement patterns.

## Findings

### A. Atomic write staging and rename cleanup — already structured correctly

**Path:** `firmware/components/storage/storage_atomic.c`

`stage_temporary_file()` creates an `app_operation_result_t` with the initiating
write/sync/close/verify failure in `primary_error`, then records unlink failure
separately with `operation_record_cleanup()`.

The rename-failure path in
`storage_atomic_write_with_ops_and_parent_sync_result()` uses the same pattern:
rename failure remains primary and failed temporary-file cleanup is retained as
cleanup detail.

The stable `app_error_code_t` wrapper returns `primary_error` when present, so a
later cleanup error does not replace the initiating error.

**Disposition:** no primary-overwrite defect remains in these two cleanup sites.
Reuse this result shape in H5-051/H5-052 rather than inventing another parallel
provenance type.

**Separate H5 issue:** after rename succeeds, parent-directory sync failure is
currently returned as an ordinary primary I/O error. The result type has no
commit-state field, so it cannot say "canonical rename happened, durability
acknowledgement is uncertain." That is not a cleanup-overwrite bug; it belongs
to H5-052/H5-053 commit-certainty work.

### B. Blob-upload commit cleanup — primary is still overwritten

**Path:** `firmware/components/storage/storage_blob_upload_core.c`

`cleanup_failure()` currently performs temporary-file unlink and returns the
cleanup error whenever unlink fails:

`cleanup_error == APP_ERROR_NONE ? primary_error : cleanup_error`

This means a flush/sync/close/final-path-check/next-ID-persist/rename failure can
be replaced by a later unlink failure.

The current host seam in `tests/host/test_storage_blob_upload.c` has operation
fault injection for unlink and commit stages, but the production API still
returns only `app_error_code_t`; H5-052/H5-054 must add a regression that injects
both an initiating failure and unlink failure and proves both are retained while
the initiating error stays authoritative.

**Disposition:** live H5 defect; fix with the common structured operation result.

### C. Blob HTTP abort after stream/commit failure — primary is still overwritten

**Path:** `firmware/components/web_server/web_server_blob.c`

`abort_uncommitted_upload()` calls `storage_blob_upload_abort()` after an
initiating stream or commit error and currently returns the abort error whenever
abort fails:

`abort_error == APP_ERROR_NONE ? primary_error : abort_error`

The handler then emits the replacement error and changes the user-facing message
to `blob upload cleanup failed`. The original upload/commit failure is no longer
the externally authoritative error.

**Disposition:** live H5 provenance defect. H5-051/H5-052 must preserve the
original stream/commit error and retain abort failure separately. The HTTP layer
should consume the structured result rather than reconstruct provenance from two
collapsed `app_error_code_t` values.

### D. Mount rollback — primary is still overwritten

**Path:** `firmware/components/storage/storage_mount_core.c`

Two rollback paths overwrite the initiating failure:

1. `mount_data()` failure followed by `unmount_web()` failure returns the
   unmount failure instead of the data-mount failure.
2. `prepare_directories()` failure followed by `storage_mount_core_unmount()`
   failure returns the cleanup failure instead of the directory-preparation
   failure.

The code intentionally keeps mount-state ownership flags set when unmount fails,
which is good residual-state tracking, but error provenance is still collapsed.

`tests/host/test_storage_mount.c` currently verifies successful rollback only;
its fake unmount functions always return `APP_ERROR_NONE`, so it does not cover
primary + rollback failure simultaneously.

**Disposition:** live H5 defect. Add a structured mount result or use the common
operation result internally, preserve residual ownership state, and add mount
rollback fault-injection regressions in H5-054.

### E. Device-settings NVS recovery — primary is still overwritten

**Path:** `firmware/components/device_settings/device_settings.c`

`replace_record_atomic()` preserves the NVS set/commit failure only if
`reopen_settings_handle()` succeeds. If reopening also fails, it returns the
reopen failure. The initiating durable-write failure is therefore lost.

This is a recovery/cleanup operation, not a new primary mutation. The original
set/commit result must remain primary while reopen failure is retained
separately.

**Disposition:** live H5 defect. H5-051 should define how this component exposes
both values without destabilizing its public `app_error_code_t` API.

### F. Device-settings mutex release — primary can be overwritten

**Path:** `firmware/components/device_settings/device_settings.c`

`finish_locked(result)` returns `APP_ERROR_INTERNAL` whenever `xSemaphoreGive()`
fails, even when `result` already contains the real settings read/replace/reset
failure. Thus an unlock failure can replace the initiating operation error.

This affects the read, replace, reset-noncredential, and factory-reset wrappers
that use `finish_locked()`.

**Disposition:** live H5 provenance defect. Preserve the operation result as
primary, record unlock failure as cleanup/release detail, and keep cache/output
invalidation behavior fail-closed.

### G. App-core startup teardown — already preserves primary and cleanup

**Path:** `firmware/components/app_core/app_core_sequence.c`

`cleanup_after_failure()` records the startup failure with
`app_operation_record_primary()`, runs every teardown stage, and records cleanup
failures with `app_operation_record_cleanup()`. `finish_failure()` logs both and
returns `result.primary_error`.

**Disposition:** correct reference pattern. No H5 primary-overwrite fix required.
This is the preferred model for multi-stage cleanup where the public API must
remain `app_error_code_t`.

### H. Wi-Fi startup rollback — already preserves primary and cleanup

**Path:** `firmware/components/wifi_ap/wifi_ap_state.c`

`fail_start()` runs all acquired-resource cleanup, publishes the original error
as `last_error`, publishes cleanup independently as `cleanup_error`, and returns
the original error. Cleanup continues past individual teardown failures and
ownership flags clear only on successful release.

**Disposition:** correct reference pattern. No H5 primary-overwrite fix required.

### I. Web-server route-registration rollback — primary provenance is collapsed

**Path:** `firmware/components/web_server/web_server_adapter_lifecycle.c`

If route registration fails and the subsequent server `stop()` also fails, the
lifecycle retains `cleanup_error = APP_ERROR_IO` and retained ownership, but the
function returns `APP_ERROR_IO`; the registration failure itself is not retained
as a separate primary value.

Because both failures are currently mapped to coarse fixed-vocabulary errors,
the immediate return may sometimes have the same enum value, but the provenance
is still structurally lost and cannot remain correct if either mapping becomes
more specific.

**Disposition:** H5 structured-result gap. Carry forward to H5-051. Residual
handle ownership behavior is correct and must be preserved.

### J. Executor shutdown/drain — first error is preserved; later detail can be lost

**Path:** `firmware/components/macro_executor/macro_executor.c`

`stop_executor_task()` and `macro_executor_deinit()` use a first-error policy, so
later release/shutdown problems do not overwrite an existing initiating error.
The shutdown latch also remains fail-closed when worker stop is unconfirmed.

However, because the public result is a single `app_error_code_t`, a later
`usb_keyboard_release_all()` failure is not independently retained when a prior
shutdown error already exists. This is **not** a primary-overwrite bug, but it is
secondary-detail loss relevant to H5-051's structured provenance objective.

`drain_pending_plans()` itself has no fallible cleanup return: it only frees
queued plans after worker ownership is resolved.

**Disposition:** no overwrite fix required for H5-050; evaluate whether existing
executor health is sufficient to retain simultaneous secondary cleanup detail
when H5-051 standardizes result handling.

## Classification summary

| Area | Primary overwritten? | Cleanup/detail retained separately? | H5 follow-up |
| --- | --- | --- | --- |
| Atomic stage cleanup | No | Yes | Keep pattern |
| Atomic rename cleanup | No | Yes | Keep pattern |
| Atomic post-rename parent sync | N/A — commit certainty | No commit-state field | H5-052/H5-053 |
| Blob core cleanup | **Yes** | No | H5-051/H5-052/H5-054 |
| Blob HTTP abort | **Yes** | No | H5-051/H5-052/H5-054 |
| Mount rollback | **Yes** | Residual state only | H5-051/H5-054 |
| Settings NVS reopen | **Yes** | No | H5-051/H5-054 |
| Settings unlock | **Yes** | No | H5-051/H5-054 |
| App-core startup teardown | No | Yes | Reference pattern |
| Wi-Fi startup rollback | No | Yes | Reference pattern |
| Web-server start rollback | Provenance collapsed | Cleanup + ownership only | H5-051 |
| Executor shutdown/drain | No | Partial; simultaneous detail can be lost | H5-051 review |

## H5-050 outcome

The audit is complete. The next implementation step is H5-051: standardize on
the existing `app_operation_result_t` where possible, extending it only for the
commit-certainty state that the atomic/blob post-rename paths require. H5-052
then applies that result consistently to the live overwrite sites and the
post-rename durability boundary. H5-053 remains responsible for externally
visible retry/reconciliation semantics; those semantics must not be guessed by
this audit or silently added to the frozen specification.

No hardware claim is made by H5-050.