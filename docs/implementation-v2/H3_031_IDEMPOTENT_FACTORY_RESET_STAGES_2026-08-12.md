# H3-031 — Idempotent factory-reset stages

## Scope

This evidence covers H3-031 only. H3-032 response/recovery visibility, H3-033 full cut-point/failure matrix, H3-034 reset-settings audit, and H3-035 hardware interruption evidence remain separate work.

Starting master SHA: `65d3c0334728e74d8537b6506fe0632c35de9683`.

## Audit findings

H3-030 established a durable `PENDING` ownership boundary but intentionally stopped startup when that marker was present. The stage audit found:

- settings/credential factory reset already converges to the same unprovisioned settings record; replay becomes a no-op;
- `auth_core_session_logout_all()` already converges to an empty RAM session table and an empty table is a successful no-op;
- blob bulk deletion scans only blobs that still exist, so rerunning naturally retries survivors and an empty repository is a successful no-op;
- canonical `.gz.tmp` cleanup is replay-safe, but was not part of live factory-reset cleanup;
- bulk blob deletion did not sync the repository parent directory after unlinking, unlike single-blob deletion, leaving a power-loss durability gap;
- H3-030 `PENDING` boot state blocked forever instead of completing the repeatable cleanup;
- repeated restart scheduling could allocate another timer before the first reboot fired.

## H3-031 recovery model

A new `factory_reset_recovery` component runs before ordinary settings initialization. It is separate from the durable journal component so `factory_reset_state` remains NVS-only and independent from repository semantics.

When the journal is `NONE`, recovery is a no-op. When it is `PENDING`, boot performs only the minimum reset-recovery work:

1. initialize device settings;
2. reapply factory-reset settings/credential erase;
3. deinitialize settings;
4. mount storage;
5. delete all remaining valid repository blobs;
6. remove canonical temporary upload debris;
7. unmount storage;
8. clear `PENDING` only after every prior stage reports success;
9. continue ordinary startup, which necessarily enters unprovisioned setup.

Previous authenticated sessions require no persistent boot cleanup: the session table is RAM-only and a reboot constructs a fresh auth core before `auth_init()` is ever called. The live accepted factory-reset path still explicitly calls `auth_session_logout_all()` before reboot.

Any recovery error leaves `PENDING` durable and returns startup failure before normal/setup services start. Rebooting or retrying therefore repeats convergent stages rather than exposing partially reset state.

## Durability and repetition corrections

- Live factory reset now removes canonical temporary upload debris in addition to valid blobs.
- Blob deletion and temporary cleanup are both attempted even when one fails; the first error remains primary and the marker is not cleared.
- Bulk valid-blob deletion syncs the repository parent directory whenever it unlinks one or more entries. A parent-sync failure returns an error, retaining reset ownership for another boot/retry.
- Reset-marker clear remains idempotent: an absent key is success.
- Restart scheduling is latched so repeated restart requests before reboot do not allocate multiple timers.

## Regression evidence

Permanent host coverage proves:

- factory-reset settings erase can be applied twice; the second application reports unchanged and performs no extra durable write;
- `logout_all` succeeds repeatedly after the table is already empty;
- bulk blob deletion succeeds on an already-empty repository;
- temporary-file recovery succeeds again after all canonical temporary files are gone;
- the recovery engine retries successfully after an injected failure at settings init, settings erase, settings teardown, storage mount, blob deletion, temporary cleanup, storage teardown, or marker clear;
- marker clear occurs only after every required stage succeeds;
- a successful pending recovery followed by re-entry is a no-op rather than another destructive reset.

The implementation publish gate runs ClangFormat, H2/H3/H9 architecture guards, the credential logging guard, focused `controls`, `startup`, `auth`, and `storage` host labels normally and under ASan+UBSan, and the full host suite. Hardware completion is not claimed.
