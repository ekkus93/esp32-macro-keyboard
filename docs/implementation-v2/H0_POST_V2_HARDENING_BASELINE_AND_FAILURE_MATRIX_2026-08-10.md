# H0 — Post-v2 hardening baseline and failure matrix

**Date:** 2026-08-10  
**Hardening TODO:** `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`  
**Starting `master` SHA:** `78c356f35db252f15e37a7508a859b20517e45e0`  
**Last product-code SHA before this H0 documentation work:** `0c51b7675255ce153c2fabd936160eb96bc90b8b`  
**Execution environment for baseline CI:** GitHub Actions clean checkout on Ubuntu 24.04  
**Reference board from the immediately preceding V2-150/V2-151 evidence:** ESP32-S3 QFN56 rev v0.2, 8 MiB embedded PSRAM, MAC `9c:13:9e:a8:77:38`

## 1. Purpose and evidence rules

This document is the Phase H0 failure inventory for the post-v2 correctness hardening pass. It does not claim that the failures below are fixed. It records the current behavior that the code review found, the state transitions that make each failure dangerous or ambiguous, and the regression evidence required before a later hardening task may be closed.

Historical implementation evidence remains historical. Where an older report proves a happy path but not a failure path, this document does not rewrite that report; it records the missing failure semantics separately.

The starting SHA above is the first commit after Claude Code corrected H2-021/H2-022 to match the verified password-change behavior. That commit changes documentation only. The relevant production source is therefore unchanged from the preceding reviewed product code.

## 2. Tool and platform baseline

The permanent `Quality` workflow for this SHA uses:

- Ubuntu 24.04 runner;
- Node from `.nvmrc`: **24.18.0**;
- ESP-IDF **v5.5.5**;
- `cmakelang==0.6.13`;
- `yamllint==1.38.0`;
- `actionlint@v1.7.12`;
- `shfmt@v3.11.0`;
- `littlefs-python==0.15.0`;
- distro `clang-format`, `clang-tidy`, `jq`, `libcjson-dev`, and `shellcheck` installed by the workflow.

The immediately preceding clean V2-150/V2-151 evidence records Node 24.18.0, ESP-IDF 5.5.5, Python 3.10.10, gcovr 8.6, the reference ESP32-S3 above, a real-Chrome browser test run, and a fully green local gate at `de47eee5f7544e6c4c2d686ac4cbb07abc08736b`. Those values are useful continuity evidence but are not substituted for the current H0 baseline run; H0 remains open until the current SHA's required commands/results are recorded.

### Current push-CI snapshot for starting SHA

At the time H0 evidence was first written:

- `Host Tests`, run `31387710687`: **completed / success**;
- `Quality`, run `31387710689`: still **in progress** while installing ESP-IDF v5.5.5;
- `Device Test Build`, run `31387710705`: still **in progress**;
- the fourth push workflow had not yet been used as evidence here.

`Publish CI Status` workflow-run fanout is deliberately not treated as the product gate because it includes success/failure/cancelled meta-runs for individual upstream workflows. H0 uses the actual push workflows and their jobs.

## 3. Failure matrix

Legend:

- **Durable state:** flash/NVS/LittleFS state surviving restart.
- **Runtime state:** RAM/session/executor/UI state.
- **Current external result:** behavior observed from current source/review, not the desired behavior.
- **Required regression:** minimum test obligation for the later hardening phase.

| ID | Initiating operation | Primary failure point / current defect | Durable state at failure | Runtime state at failure | Cleanup / secondary failure behavior | Current external result | Retry semantics today | Reboot semantics today | Required regression |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| F-H1-01 | `POST /api/v1/send` with `requireSerialConfirmation=true` in device settings | Real HTTP send construction does not copy the authoritative setting into `macro_execution_request_t.require_confirmation` | No relevant durable change | Executor accepts ordinary send semantics | Confirmation subsystem is bypassed rather than failed | Send can run even though owner enabled confirmation | Retrying repeats bypass | Setting persists, but next send is still vulnerable until wiring is fixed | false setting -> normal; true -> awaiting confirmation; settings-read failure -> no accepted send; zero HID before confirmation |
| F-H2-01 | Password change | Durable credential replacement succeeds, then best-effort NVS reread used to refresh RAM verifier fails | **New password is durable** | RAM verifier may still contain **old** password | Refresh helper returns without propagating failure | Operation can appear successful while immediate login authority disagrees with durable state | User can receive contradictory old/new login behavior until reboot | Reboot rereads durable new password and changes authority again | Inject verifier-refresh/read failure and prove no `204` can coexist with old verifier authority |
| F-H2-02 | Password change | Session invalidation fails after durable password write | **New password is durable** | Existing sessions may remain valid; credential runtime transition is not represented as a distinct partial-commit state | No rollback of durable password; ordinary backend failure is returned | Error does not tell caller that password changed but invalidation is incomplete | Blind retry/"old password still authoritative" assumptions are unsafe | Durable new password remains authoritative after reboot; old sessions disappear only as runtime state resets | Inject invalidation failure after durable write; API outcome must distinguish partial commit; assert old/new password and session authority explicitly |
| F-H3-01 | Factory reset | Session invalidation fails after settings/credentials erase stage | Credentials/settings may already be erased | Some sessions may still exist until restart/reset | Reset engine continues later stages and schedules restart; first error is returned | Caller sees reset failure although destructive work already occurred | Repeating may operate on already-erased state | Restart can enter setup/unprovisioned behavior after a reported error | Durable reset marker/state machine; interruption at each stage resumes safely; old session never regains authority after accepted reset |
| F-H3-02 | Factory reset | Blob deletion fails after credential/settings erase | Credentials can be gone while one or more old opaque blobs survive | Normal auth/session state may already be invalidated | Restart is still scheduled after attempted stages | Reset may report error and reboot with surviving old data | Reprovisioning without a reset-recovery barrier risks exposing old blobs to a new owner | Surviving blobs persist across reboot unless cleanup resumes | Fail first/middle/final blob deletion; reset marker must survive and block normal/setup completion until all old blobs are gone |
| F-H3-03 | Factory reset | Reboot/power loss between destructive stages | Prefix of reset stages may be durable | Runtime state is lost | No durable reset transaction marker in current design | Boot cannot reliably distinguish intentional incomplete reset from ordinary partially erased state | Manual retry depends on what survived | Can present setup/normal behavior inconsistent with remaining storage | Simulated reboot between every stage; boot resumes idempotent reset and never exposes ambiguous normal operation |
| F-H4-01 | React startup/reload while send may be active | `recoverSendState()` throws because of network/timeout/5xx/malformed response | Device execution unchanged | Frontend catches exception and replaces recovery with `null` | No explicit degraded execution state | UI can look as if no send exists; progress/cancel visibility disappears while typing may continue | User must manually retry/reload or provoke a later conflict; no explicit recovery state | Device execution may still be active after browser reload | First recovery request fails while send active; UI must show unknown/unavailable state, keep recovery/cancel affordance, and recover on retry without duplicate POST |
| F-H5-01 | Atomic storage write/upload | Primary stage/write/rename failure plus cleanup failure | Depends on failure point; canonical may be unchanged | Operation has two independent errors | Cleanup error can replace the primary error in return value | Caller loses the actual initiating failure | Retry diagnosis can target wrong cause | Durable state follows primary operation, not the overwritten error code | Inject primary+cleanup failure together; structured result preserves primary and secondary errors separately |
| F-H5-02 | Atomic storage activation | `rename()` succeeds, then parent directory sync fails | New canonical name/content is already visible; durability is uncertain | Caller receives error | No rollback can truthfully restore pre-rename visibility | Ordinary failure is indistinguishable from "nothing committed" | Blind retry can duplicate/repeat an already visible commit | Reboot may retain or lose directory entry depending on durability outcome | Inject parent-sync failure after rename; expose `COMMIT_UNCERTAIN`/equivalent state and idempotent reconciliation behavior |
| F-H6-01 | HTTP send requiring asynchronous confirmation | Async worker/queue unavailable | No durable change | Required worker is absent | Current adapter falls back to synchronous `web_api_handle_call()` on the httpd task | HTTP server can block for the confirmation wait instead of failing closed | Repeated requests can continue degraded blocking behavior | Restart may recreate worker, but no explicit health contract distinguishes state | Worker unavailable -> bounded 503/internal-unavailable, no send, no synchronous long wait; health/diagnostics expose failure |
| F-H6-02 | Executor submit/error cleanup | Queue/unlock/submit path fails and `usb_release_all()` also fails | No durable change | HID may remain uncertain/stuck; release failure discarded on exceptional paths | `(void)` ignores release result in identified branches | Caller sees primary submission failure only; key-release safety failure is hidden | Retry can proceed without knowing HID safety state | USB reset/reboot may clear host/device key state, but software gives no fault signal | Inject submit failure + release-all failure; preserve both and latch/report HID safety fault before later sends |
| F-H7-01 | Package selection/open package | UI-state persistence write fails | Selected-package preference remains old/not saved | Current in-memory selection still changes | Exception is intentionally swallowed as best-effort | User sees selection as successful with no warning it will not survive reload/reconnect | Later selection may retry implicitly; user has no explicit recovery signal | Reload can restore older selection | Inject persistence failure; continue local selection only with visible warning/retry and no dirty working-copy mutation |
| F-H7-02 | Snapshot export | Compression/file-save/export operation rejects | Repository/snapshot state unchanged | Export promise fails | Caller uses `void exportWorkingCopy()`; no normal user-facing catch in reviewed path | Export may simply fail with no actionable UI error | User may click again without understanding cause | No persistent effect | Inject compression/save failure; visible error, retry path, dirty state unchanged, no partial success claim |
| F-H4-02 | Send/USB status polling | Repeated network/invalid-response failures | Device state unchanged | Last known UI value is retained indefinitely without freshness/degraded state | Poll loop silently retries | Stale USB-ready/send state can appear current; active-send degradation is especially misleading | Automatic retries occur but are invisible | Reload restarts the same behavior | Bounded consecutive failures mark data stale/degraded, successful poll clears warning, no duplicate send POST |

## 4. Required hardened invariants derived from the matrix

1. **Safety/security controls fail closed.** Missing confirmation/settings/worker support never silently disables confirmation or converts to a long synchronous fallback.
2. **Durable commit is never described as "nothing changed."** Password/reset/storage responses must distinguish partial commit, accepted recovery, and commit-uncertain outcomes where relevant.
3. **Primary and secondary failures are both preserved.** Cleanup/release/durability errors supplement rather than overwrite the initiating error.
4. **Active execution uncertainty is visible.** The frontend must never map inability to recover send state to confirmed absence of a send.
5. **Destructive reset is resumable.** Once accepted durably, reset completes or remains in an explicit reset-recovery state across reboot; newly provisioned users cannot inherit old blobs.
6. **Persistence degradation is visible.** Continuing safely in RAM is permitted for non-safety UI state such as package selection only when the failure is disclosed and retryable.
7. **Every row gets failure injection.** A happy-path unit test or historical hardware run cannot close a failure row.

## 5. `TODO_V2.md` reconciliation decisions

The post-v2 review identified these ledger corrections. They must be applied to `docs/TODO_V2.md` without rewriting historical evidence:

- **V2-154 authentication matrix:** split the checked compound item. Login/logout/lockout have hardware evidence; idle expiry and absolute expiry do not have independent hardware evidence and remain open.
- **V2-154 secret-leak matrix:** the existing hardware work is a spot-check, explicitly not a full audit. The broad compound "no secret appears" completion claim remains open until the full audit is performed.
- **V2-061 / Phase 6:** executor confirmation mechanics are implemented and tested, but real `POST /api/v1/send` does not yet wire `requireSerialConfirmation`; end-to-end product confirmation remains open under H1.
- **V2-055 / V2-154 password change:** historical happy-path evidence remains valid. It does not prove stale-verifier-refresh failure semantics or durable-password/session-invalidation partial-commit semantics; those remain open under H2.
- **V2-153 factory reset:** historical happy-path factory-reset/reprovisioning evidence remains valid. It does not prove interrupted/partial destructive reset semantics; those remain open under H3.

No historical evidence document should be edited to claim that it covered these failure paths.

## 6. H0 status at this commit

Completed by this document:

- exact starting `master` SHA recorded;
- clean-checkout CI execution context recorded;
- tool/platform configuration currently known from permanent workflow and immediately preceding hardware evidence recorded without presenting older evidence as the new baseline;
- all required reviewed failure cases inventoried with state, cleanup, external behavior, retry/reboot semantics, and regression obligation;
- reconciliation decisions for the known `TODO_V2.md` contradictions recorded.

Still open before the H0 exit gate:

- current-sha `Quality` must finish and its authoritative `check-all.sh` result must be recorded;
- current-sha sanitizer and native-coverage commands must be run/recorded for this hardening baseline (a plain Host Tests success is not a substitute unless the workflow demonstrably executes those exact gates);
- exact current Python/compiler/static-analysis/browser versions should be captured from the current gate/logs where available;
- `docs/TODO_V2.md` itself must be edited to match the reconciliation decisions above;
- the hardening TODO checkboxes must only be marked after those pieces are proved.
