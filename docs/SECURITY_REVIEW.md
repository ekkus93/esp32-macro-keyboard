# Security and failure review

**Status:** Historical snapshot from early implementation (pre-FIX1). Most
"Blocking open findings" below have since been resolved by the FIX1 effort -
see `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`,
now the authoritative, currently-maintained status document. Two findings
below are confirmed stale as of this note and corrected inline rather than
silently removed, per that doc's own rule to keep historical findings but
mark when and how they were fixed; the remaining findings in this file were
not individually re-audited when this note was added and should not be
trusted for current status either way.

## Enforced foundations

- Protected SoftAP configuration rejects passphrases shorter than 12 bytes.
- No open-AP fallback, NAT, or internet routing exists.
- Password records use random salts and PBKDF2-HMAC-SHA-256.
- Sessions and CSRF tokens are random, RAM-only, bounded, compared in constant
  time, and invalidated by reboot.
- Repeated login failures use bounded monotonic throttling.
- Mutations currently exposed by HTTP require session, CSRF, Host, and Origin.
- Static paths use a character allowlist, reject traversal, stream bounded
  chunks, support pre-generated gzip, and never map into `/data`.
- Macro parsing completes before execution and failed parsing returns no plan.
- The executor has one-item ownership transfer, rejects busy state, remains
  cancellable during delays, applies a watchdog, and records release-all failure
  separately from the original terminal error.
- LittleFS mounts set `format_if_mount_failed = false`.
- Atomic writes use unique exclusive temporary files, check short writes, sync,
  close, full byte-for-byte readback, rename, and cleanup errors.
- Startup refuses the production network path until persistent secure
  provisioning exists; an explicit development option uses generated temporary
  credentials rather than fixed defaults.

## Blocking open findings

- Production provisioning and encrypted NVS configuration persistence are not
  implemented. The release build must keep refusing to start a network until
  this is complete and validated.
- ~~Set, macro, procedure, progress, import, restore, and diagnostic
  repositories and routes are not implemented.~~ **Stale as of FIX1 Phase 15/16/18/19**:
  all of these are implemented - e.g. `firmware/components/web_server/web_api_sets.c`
  (`web_api_handle_sets`) dispatches full set CRUD (create/read/update/delete/
  reorder/select/duplicate/export/import) backed by
  `storage_repository_sets.c`; macro/procedure/progress repositories
  similarly exist (`storage_repository_macros.c`/`storage_repository_procedures.c`/
  `storage_repository_progress.c`); the diagnostics route landed in FIX1
  §19.2 (`web_server_diagnostics.c`).
- Transaction recovery preserves unknown manifests but lacks operation-specific
  deterministic roll-forward/rollback.
- Login throttling is global rather than the final bounded global-plus-client
  policy.
- The current Origin comparison intentionally supports only the HTTP SoftAP
  origin and needs browser integration tests for IPv4 and any later mDNS name.
- USB descriptor/TinyUSB integration needs an actual ESP-IDF `v5.5.5` build and
  enumeration tests.
- ~~Dependency lockfiles are not generated in this environment.~~ **Stale**:
  `firmware/dependencies.lock` has been committed and tracked since commit
  `0b64ee6` (2026-07-25); `webapp/package-lock.json` is likewise committed.
- Factory reset, credential reset, and destructive repair gestures are not
  implemented.

Every open item is release-blocking. No security exception, silent fallback, or
lint suppression is accepted.
