# H5-051 — Standardized structured operation results — 2026-08-13

## Scope

This document records the H5-051 implementation from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.

Starting audit SHA: `118ea60ce5efc148d7abccce31d7e81dbc8b71ae`.
Implementation candidate before this evidence commit:
`830a5f5efdd2f3900b2d9bc529386a8e200d390d`.

This task standardizes the internal result mechanism only. It does not change
`docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`, does not add a new public HTTP
response, and does not claim H5-053 retry/reconciliation semantics are complete.

## Common result contract

`firmware/components/support/include/app_operation_result.h` remains the single
structured-result vocabulary and now carries the minimum fields required by the
post-v2 hardening specification:

- `primary_error` — the initiating operation failure;
- `cleanup_error` — the first independent cleanup/release/durability failure;
- `cleanup_incomplete` — residual cleanup state remains; and
- `commit_state` — explicit commit/durability certainty.

`app_operation_commit_state_t` defines:

- `APP_OPERATION_COMMIT_NOT_APPLICABLE`;
- `APP_OPERATION_NOT_COMMITTED`;
- `APP_OPERATION_COMMITTED`; and
- `APP_OPERATION_COMMIT_UNCERTAIN`.

`app_operation_result_ok()` is fail-closed for both `NOT_COMMITTED` and
`COMMIT_UNCERTAIN`; neither can accidentally become success merely because an
error field was omitted.

The shared helpers preserve first-primary and first-cleanup semantics and provide
one compatibility mapping, `app_operation_result_error()`, which returns the
primary error when one exists and only falls back to cleanup error when there was
no primary failure.

The tiny helpers are header-inline. This is intentional: the repository's
focused host storage targets compile production storage state machines directly
rather than link the full `support` component. Keeping these operations inline
preserves that isolation instead of adding unrelated CMake coupling. The existing
`app_operation_result.c` translation unit remains in source lists as a stable
home for future non-inline logic.

## Storage consumers standardized

### Atomic writes

`storage_atomic_write_with_ops_and_parent_sync_result()` now uses the common
cleanup recorder and sets commit state at the actual activation/durability
boundary:

- validation/staging/rename failure -> `NOT_COMMITTED`;
- rename + parent sync success -> `COMMITTED`;
- rename success + parent sync failure -> `COMMIT_UNCERTAIN`.

The existing `app_error_code_t` wrapper remains source-compatible and maps
through `app_operation_result_error()`.

This supplies the mechanism H5-052 needs; H5-052/H5-054 still own the full
failure-injection proof for write/verify/rename/parent-sync cases.

### Mount and rollback

`storage_mount_core_mount_result()` and
`storage_mount_core_unmount_result()` are detailed internal variants.

For mount rollback, the mount/directory-preparation failure remains
`primary_error`; rollback failure is recorded as `cleanup_error`; ownership flags
continue to identify partitions that remain mounted after failed cleanup.

The old `storage_mount_core_mount()` / `storage_mount_core_unmount()` signatures
remain available and map detailed results through the primary-first compatibility
helper.

`tests/host/test_storage_mount.c` now injects:

- data-mount failure + web-unmount failure; and
- directory-preparation failure + failures unmounting both partitions.

The assertions require the initiating error to remain primary, the first rollback
failure to remain separately visible, and residual mount ownership to stay true.

### Blob upload

`storage_blob_upload_commit_with_ops_result()` and
`storage_blob_upload_abort_with_ops_result()` are detailed internal variants.

The commit result now distinguishes:

- pre-activation failure and failed temporary cleanup -> primary + cleanup,
  `NOT_COMMITTED`;
- successful rename followed by parent-sync failure ->
  `COMMIT_UNCERTAIN`; and
- full success -> `COMMITTED`.

The existing `storage_blob_upload_commit_with_ops()` and abort wrappers remain
source-compatible and use the common primary-first mapping.

The production `storage_blob_upload_commit()` wrapper consumes the detailed
result while retaining its existing inventory rule: live inventory is advanced
only when `upload->committed` actually transitions to true.

H5-053 still owns externally visible reconciliation/retry behavior for an
uncertain blob commit. This task deliberately does not teach the HTTP/frontend
layer to blind-retry or invent a new response contract.

## Compatibility audit

The storage component already declares `support` as an ESP-IDF `REQUIRES`
dependency, so the firmware build gains no new component dependency.

The host storage targets do not link the support library. Storage-private headers
therefore include the common result header by a component-relative path, matching
the pre-existing pattern in `storage_atomic_internal.h`; result helpers are
header-inline, so no new host linker dependency is required.

No existing public storage function was removed or had its signature changed.
Detailed variants are additive internal APIs.

## Validation performed here

The sandbox cannot clone/fetch the repository through normal git because DNS for
`github.com` is unavailable, so a full checkout and the literal repository gates
cannot be run locally in this session.

A standalone strict-warning compile/run of the exact common-result semantics was
performed with:

```text
cc -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
  -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations \
  -Wstrict-prototypes ...
```

Result:

```text
h5 common result smoke passed
```

The smoke covers successful/not-applicable state, committed state,
not-committed fail-closed behavior, uncertain fail-closed behavior, cleanup-only
mapping, and primary-over-cleanup mapping.

Connector-backed source inspection also verified the focused host storage target
wiring and the production storage component dependency before choosing the
header-inline helper implementation.

## Required authoritative validation

Before checking H5-051 complete in the authoritative TODO, run on the exact
candidate/evidence descendant:

```text
./scripts/run-tests.sh storage
./scripts/run-tests.sh --sanitizers storage
./scripts/check-all.sh
```

If the repository's targeted workflow splits these commands, record the exact
run/job IDs and tested SHA rather than inferring success from source inspection.

## Remaining H5 work

H5-051 supplies one shared result vocabulary and compatibility policy. It does
not close the phase.

Next:

1. H5-052 — complete/verify atomic-write provenance at every injected failure
   boundary and carry the remaining H5-050 overwrite sites into the same policy;
2. H5-053 — make commit-uncertain retry/reconciliation explicit end-to-end;
3. H5-054 — full storage failure-injection matrix, including blob and mount
   rollback; and
4. H5-055 — physical interrupted-write/power-cycle durability sanity.

No hardware claim is made by H5-051.
