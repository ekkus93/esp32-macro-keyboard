# H9 — Cross-cutting secret, fallback, and regression audit evidence

**Date:** 2026-08-11
**Phase:** `H9 — Cross-cutting secret, fallback, and regression audit`
**Initial mechanical audit:** run `31544238722`, job `93953189964`
**H9 product/test correction:** `f36b48eef170b84085f1a978b25fb8c14de99574` (`fix: close H9 silent safety gaps`)
**Targeted behavior validation:** run `31546096618`, job `93958810321`
**Final mechanical audit:** run `31546340375`, job `93959559165`

## Result

The cross-cutting audit found and fixed four correctness/security defects rather than merely classifying them:

1. startup carried the manufacturing-label setup code into the ordinary UART logging boundary and printed it;
2. real `POST /api/v1/send` construction did not bind the authoritative `requireSerialConfirmation` device setting into `macro_execution_request_t`, so the setting could be silently bypassed;
3. if `httpd_start()` succeeded, async-worker startup failed, and the compensating `httpd_stop()` also failed, the live HTTP server handle could be lost, creating an unrecoverable ghost server;
4. host-test assertion failures printed compared string/integer/buffer values, allowing a deliberately secret-bearing negative test to disclose a password, token, salt/verifier fragment, or related value in CI output.

All four are corrected at the H9 product SHA. The final audit re-ran the permanent guards against the resulting production tree and found zero empty catches, zero empty Promise catches, and zero best-effort markers.

## H9-090 — Silent/ignored/fallback classification

The mechanical scan covered production `firmware/components`, `firmware/main`, and `webapp/src` for the required categories. The final inventory reported:

- `catch {}`: **0**;
- `.catch(() => {})`: **0**;
- `best-effort`: **0**;
- `fallback`: **4 textual hits**, all non-silent and non-dangerous;
- stale/retry: **79 textual hits**, dominated by declarations/comments/UI labels plus the reviewed bounded/visible retry mechanisms;
- cleanup-related: **159 textual hits**, mostly structured health/result code plus the reviewed secondary-cleanup cases;
- `(void)`: the raw textual scan intentionally over-counts declarations such as `foo(void)` and unused-context casts; actual fallible-result discards were inspected individually.

### Classified fallible-result discards

| Site/class | Disposition |
| --- | --- |
| `web_server_lifecycle.c` compensating `httpd_stop()` after async startup failure | **Fixed.** Stop failure now preserves the live handle in lifecycle ownership; another start is rejected until cleanup succeeds. Regression covers failed start with a live handle followed by successful cleanup retry. |
| `web_server_blob.c` reader close after response-header failure | **Intentional secondary cleanup.** The request has already failed visibly; close failure cannot turn it into success. Richer primary/cleanup provenance remains H5 scope. |
| `storage_fs_ops.c` descriptor close after parent-directory `fsync` failure | **Intentional secondary cleanup.** The durable-operation failure is already returned; ignored close cannot create false success. H5 remains responsible for generalized durability/cleanup provenance. |
| `web_server_static.c` `fclose()` after HTTP header failure | **Intentional secondary cleanup.** The HTTP operation already fails; close failure cannot change a success into a silent failure. |
| `web_server_api.c` ignored return from `set_error_response()` while handling response-allocation/encoding failure | **Terminal error-reporting fallback.** The original operation already failed and there is no safer richer body if error-envelope construction itself fails; no success is reported. |
| `wifi_ap.c` `esp_wifi_disconnect()` before checked reconnect | **Safe sequencing.** The following checked `esp_wifi_connect()` is the authoritative operation and determines visible success/failure. |
| `wifi_ap.c` zero-time `xSemaphoreTake()` | **Intentional drain/probe**, not a fallible business operation. |
| executor timed `ulTaskNotifyTake()` | **Intentional wait primitive.** Timeout/notification both return control to the engine's own cancellation/timing checks; no domain result is being discarded. |
| executor atomic compare/exchange return | **Intentional first-fault-wins latch.** The release fault is retained atomically; H7 regressions prove subsequent sends are rejected. |
| fixed literals copied through `bounded_length`, `memcpy`, keymap helpers, build-id/fixed-buffer formatting | **Invariant/informational operations.** Inputs and destination sizes are construction-time bounded; none is a hidden user-operation result. |

### Cleanup-result replacement patterns

Most cleanup hits use the project's structured primary/cleanup health/result model and explicitly retain both errors. Two older storage-core patterns (`storage_blob_upload_core.c` and mount cleanup) can still choose a cleanup code as the single returned code after a primary failure. They never convert failure to success and therefore are not silent correctness failures. Their loss of exact provenance is explicitly left open under **H5**, which owns the generalized structured storage-result/durability-certainty work; H9 does not falsely close H5.

### Fallback/retry classification

The four `fallback` text hits are safe by construction: `http_health.c` explicitly says the pthread branch is compile-time host portability and **not** a runtime fallback; `gzip.ts` explicitly rejects unsupported compression rather than using an uncompressed fallback; the two routing hits are the normal safe-screen default for an invalid hash, not a persistence/security fallback.

Runtime retry behavior was reviewed rather than banned: authentication lockout exposes bounded `Retry-After`; station connection is explicitly bounded; send polling stops after a fixed consecutive-failure bound and makes state unavailable; device reconnect has terminal error states; stale USB status disables sending; startup, diagnostics, selection persistence, and send recovery expose explicit user-driven Retry actions. No hidden unbounded correctness retry remains.

## H9-091 — No-secret audit

### Surface audit

| Surface | Evidence/disposition |
| --- | --- |
| Serial logs | Setup code value removed from the startup logging event; UART now says only that setup credentials are on the manufacturing label. Serial command help contains words such as `<password>` but never prints the supplied password value. |
| Firmware logs | `scripts/check-credential-logging.sh` now rejects password, passphrase, setup-code, session-token, salt, or verifier values even when passed through a generic format string. Its regression suite passed **9/9**. |
| HTTP success/error bodies | Response builders expose fixed allowlisted fields and sanitized fixed-vocabulary errors. Password/passphrase/setup-code/password-record bytes are not response fields. A session token is intentionally transported only in the session cookie boundary; it is not reflected in JSON bodies, diagnostics, or exports. |
| Diagnostics | Diagnostics JSON has an exact allowlist and dedicated forbidden-field/secret-sentinel regressions; it excludes password, passphrase, setup code, session token/cookie, macro source, and repository content. |
| Browser console | The production-V2 static scan now covers `AppV2.tsx`, all `src/v2`, and every `src/features/**/v2` family and rejects `console.log/info/warn/error/debug`. |
| Repository export | The `Repository` schema contains only `format`, `schemaVersion`, packages, macro metadata/timing, and user-authored macro source. Device credentials/auth records are not representable in this schema. |
| Snapshot export | Snapshot save/export serializes the same typed repository working copy; it does not serialize device settings/auth/session state. |
| Test failure output | Shared string, integer, and buffer assertion helpers now report only generic mismatch descriptions/length, never compared values or secret-bearing macro arguments. `check-h9-architecture.py` prevents those value-printing formats from returning. |

User-authored macro source is intentionally repository data and therefore is expected to appear in repository/snapshot exports. That is distinct from device-managed administrator/AP/setup/session/password-record secrets.

### Secret-sentinel disposition

- **Administrator password:** accepted only as request/input material, never returned in settings/diagnostics/export/log output; test failure values are redacted.
- **AP passphrase:** used for Wi-Fi/provisioning input and persisted credential state, but omitted from public settings, diagnostics, exports, and ordinary logs.
- **Setup code:** generated/read for bootstrap verification but no longer crosses the ordinary logging boundary; manufacturing label/QR remains the delivery path.
- **Session token:** intentionally exists in the secure session-cookie transport boundary and request cookie parsing; it is not mirrored into HTTP JSON, logs, diagnostics, repository/snapshot export, or browser console.
- **Password salt/verifier bytes:** remain internal password-record state; diagnostics/settings/export schemas exclude them and shared test failures no longer print compared values.

## H9-092 — Architectural guards

Permanent guards now enforce all four required architecture properties:

1. `scripts/check-v2-phase2-architecture.py` scans firmware components/main and fails if retired firmware package/macro repository ownership or APIs return;
2. `v2-browser-storage-prohibition.test.tsx` scans `AppV2.tsx`, all production `src/v2`, and every production `src/features/**/v2` directory for forbidden browser persistence, and also prohibits browser-console output;
3. `scripts/check-h9-architecture.py` verifies the worker-unavailable confirmation path remains HTTP 503 and cannot call the synchronous handler fallback;
4. the same guard plus permanent host tests require the real send boundary to read `device_settings.require_serial_confirmation`, bind it to `macro_execution_request_t.require_confirmation`, and fail closed without submitting if the setting cannot be read.

## Validation

Targeted run `31546096618`, job `93958810321`, validated the product/test correction before committing `f36b48eef170b84085f1a978b25fb8c14de99574`:

- credential logging policy: passed;
- credential-logging regression script: **9/9**;
- H9 architecture guard: passed;
- Phase-2 repository-ownership guard: passed;
- `./scripts/run-tests.sh web`: **29/29**;
- `./scripts/run-tests.sh startup`: **10/10**;
- `./scripts/run-tests.sh --sanitizers web`: **29/29** under ASan+UBSan;
- `./scripts/run-tests.sh --sanitizers startup`: **10/10** under ASan+UBSan;
- pinned Node **24.18.0** frontend format/typecheck/ESLint/stylelint: passed;
- frontend Vitest: **46/46 files, 518/518 tests**;
- frontend production build: passed;
- `git diff --check`: passed.

Final read-only audit run `31546340375`, job `93959559165`, then re-ran the permanent credential/H9/Phase-2 guards and the complete mechanical classification inventory against the same production source (the audit SHA differs from the product SHA only by its temporary workflow). All guards passed; empty catches, empty Promise catches, and best-effort hits remained zero.

## Phase H9 disposition

Every reviewed production best-effort/ignored-error/fallback/retry/cleanup site is either fixed or classified above. No known **critical silent** failure from H9 remains. The complete no-secret surface/sentinel audit passes with committed guards and regression evidence.

H9 deliberately does not claim H5's remaining generalized storage provenance work, H1's remaining browser/hardware confirmation evidence, or H10's full real-Chrome/device/final-candidate gates.
