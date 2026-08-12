# H3-030 — Durable factory-reset state

## Scope

This evidence covers H3-030 only. H3-031 (fully idempotent/resumable cleanup), H3-032 (accepted/recovery HTTP semantics), H3-033 failure/reboot matrix, H3-034 reset-settings audit, and H3-035 hardware validation remain separate work.

## Verified pre-H3 defect

The previous factory-reset engine wrote the unprovisioned settings record first, then attempted session invalidation and blob deletion, and scheduled restart. There was no durable record that destructive cleanup was incomplete. A power loss or a post-settings cleanup failure could therefore reboot into setup with no persistent evidence that reset cleanup still owned the device state.

## H3-030 state machine

The durable journal has exactly two logical states:

- `NONE`: no factory-reset recovery owns startup.
- `PENDING`: a factory reset crossed its durable acceptance boundary and cleanup must be treated as incomplete until the marker is explicitly cleared.

The journal is stored independently from both the device-settings record and repository/blob storage:

- NVS namespace: `reset_journal`
- key: `factory_reset`
- encoded pending value: `1`
- missing namespace/key: `NONE`
- any unknown stored value: `APP_ERROR_STORAGE_CORRUPT` (fail closed)

`factory_reset_state_mark_pending()` performs `nvs_set_u8()` followed by `nvs_commit()`. It cannot return success unless both succeed. Clearing likewise erases the key and commits the erase; an already-absent key is idempotently clear. Each public operation uses a short-lived NVS handle.

## Transaction boundary

The factory-reset orchestration order is now:

1. durably mark `PENDING`;
2. erase settings/credentials/provisioning state;
3. invalidate sessions;
4. delete opaque repository blobs;
5. clear `PENDING` only if every required destructive effect succeeded;
6. schedule reboot.

A marker-write/commit failure aborts before any destructive effect and does not schedule restart. Once `PENDING` has committed, every later failure keeps the marker and schedules restart so the next boot cannot silently resume ordinary service.

This is the H3-030 ownership boundary, not yet the full H3-031 recovery engine. H3-031 will make each cleanup stage explicitly repeatable/resumable after a pending boot.

## Boot behavior

After NVS initialization and before `device_settings_init()`, app-core reads the factory-reset journal.

- `NONE`: ordinary boot continues.
- `PENDING`: startup returns `APP_ERROR_RESET_RECOVERY_REQUIRED`; device settings, storage, auth, USB, macro executor, controls, Wi-Fi, and HTTP are not started.
- journal read/corruption error: the underlying storage error propagates and startup also fails closed.

The existing lifecycle failure path deinitializes NVS, drives the fatal indicator, and `app_main` still attempts to start the UART debug console. H3-032 will define richer recovery/UI visibility; H3-030 deliberately prevents normal/setup service from lying about readiness.

## Repository independence

The journal component depends only on the app error model and NVS. It does not import `storage`, `storage_blob`, repository models, macro packages, or web-server semantics. Repository blobs remain opaque data deleted by the existing reset adapter.

## Regression evidence

Permanent host coverage includes:

- core state semantics: missing marker -> `NONE`, pending round-trip, corrupt value fails closed;
- production NVS adapter: `mark_pending` requires successful `nvs_set_u8` + `nvs_commit`; injected set/commit failures propagate; clear requires a committed erase unless already absent;
- reset orchestration: marker precedes destruction, marker failure is nondestructive, post-marker settings/session/blob/clear failures retain recovery ownership and schedule reboot;
- startup: `APP_ERROR_RESET_RECOVERY_REQUIRED` stops before settings read, storage, auth, USB, executor, controls, Wi-Fi, and HTTP.

The publish gate for this commit runs the H3 architecture guard, credential-log guard, startup/controls host labels, the same labels under ASan+UBSan, and the full host suite before committing to `master`. Hardware evidence is intentionally not claimed.
