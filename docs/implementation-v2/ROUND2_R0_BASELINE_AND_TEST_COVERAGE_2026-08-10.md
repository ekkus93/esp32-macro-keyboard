# Round 2 R0 baseline and test-coverage inventory

**Date:** 2026-08-10  
**Round 2 TODO:** `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_ROUND2_2026-08-10.md`  
**Round 2 spec:** `docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_SPEC_ROUND2_2026-08-10.md`

## R0-001 — Exact starting state

Round 2's reviewed product baseline is `44488753c9f4dc50d27cd4fefb4b21060c9c3948`.
The Round 2 planning documents then landed together at
`d5f3a41ea4d3c82e6797b336d91c27c449f61418`. A GitHub compare from
`44488753...` to `d5f3a41...` contains exactly one commit and exactly two added
files: the Round 2 specification and Round 2 TODO. There are no firmware,
webapp, test, workflow, contract, or script changes between those SHAs.

This Ralph-loop execution is connector-backed against committed GitHub state;
there is no separate local checkout carrying uncommitted changes. Immediately
before this evidence file was committed, `master` resolved exactly to
`d5f3a41ea4d3c82e6797b336d91c27c449f61418`.

The authoritative baseline command is:

```text
./scripts/check-all.sh
```

It passed on exact SHA `d5f3a41ea4d3c82e6797b336d91c27c449f61418`
in GitHub Actions Quality run `31414501339`, job `93540178710`. The
`Run authoritative checks` step completed with `success`. Full command output
is retained by GitHub Actions at:

```text
https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/31414501339/job/93540178710
```

The same exact SHA also passed the parallel push workflows:

- Host Tests: run `31414501463`, success.
- Browser Tests: run `31414501380`, success.
- Device Test Build: run `31414501834`, success.

No Round 2 product fix had landed before this baseline was established.

## R0-002 — Pre-fix test-suite state

The table below records what the current suite can and cannot prove before any
Round 2 product fix.

| Finding | Current test-suite state at `d5f3a41...` | Missing regression evidence |
| --- | --- | --- |
| F-014 password-record race | `web_api_administration.c` is linked into host administration tests, and the async-confirmation host target links the real administration/settings code. That async target explicitly does not start the real FreeRTOS worker and uses dead-path-only FreeRTOS stubs. `web_server_login.c` is shipped firmware, but there is no host target exercising it concurrently with password-record refresh. Repository search found no `-fsanitize=thread` or `pthread_create` harness. | No test can currently detect a torn concurrent read/write of `server_configuration.password_record`; single-threaded host success is insufficient. |
| F-015 parsed password heap copies | `web_settings.c` and `web_device_actions.c` are directly host-linked. Existing settings tests verify request-buffer wiping on several paths, including error paths, but do not instrument cJSON's separately allocated string copies before `cJSON_Delete()`. | No assertion proves parsed current/new/factory-reset password strings are zeroed before their cJSON tree is freed. |
| F-016 post-rename blob accounting | Production `firmware/components/storage/CMakeLists.txt` ships both `storage_blob_upload_core.c` and `storage_blob_upload.c`. Host `storage_blob_upload_tests` links `storage_blob_upload_core.c` but not `storage_blob_upload.c`. | Zero host coverage of the wrapper whose `upload->committed` check advances live inventory after the core returns. No wrapper-level `sync_parent` failure regression exists. |
| F-017 stuck executor shutdown | Production ships both `macro_executor.c` and `macro_executor_engine.c`. Host `macro_executor_tests` links only `macro_executor_engine.c`; comments elsewhere likewise treat the FreeRTOS-backed public executor entry points as not host-linkable. | No automated test drives `macro_executor_deinit()` through `SHUTDOWN_WAIT_MS` timeout and observes the resulting `shutting_down` state. |
| F-018 tracker outlives `MacrosPage` | `v2-macros-page.test.tsx` covers ordinary Quick Send, confirmation, cancellation, terminal acknowledgements, and unmounts, but its ordinary send tests advance tracking to a terminal status before unmount. | No test unmounts while a self-initiated tracker or 409-recovered tracker is still active and then proves `.stop()`/poll termination. |
| F-019 sibling settings update loses edits | `v2-settings-page.test.tsx` covers Identity, AP, and Station form submissions independently. | No test edits Identity locally, submits AP/Station, then asserts the unsaved Identity fields survive the sibling response. |
| F-020 unbounded poll failure retry | `sendClient.ts` implements the tracker; existing page tests exercise successful/terminal polling. No current test establishes a bounded retry/give-up signal for persistent malformed/5xx/network failure. | No persistent-failure regression proves polling eventually surfaces/stops rather than rescheduling forever. |
| F-021 cleanup masks primary storage error | `storage_atomic.c` is directly linked by `storage_atomic_tests`. Existing failure coverage injects write/sync/read/rename failures one at a time and checks destination preservation. | No dual-failure test injects a primary stage/rename failure followed by cleanup `unlink` failure and asserts the primary error remains authoritative. |
| F-022 dead setup code with live tests | Shipped `firmware/components/web_server/CMakeLists.txt` includes `web_server_setup.c` and `web_server_setup_submit.c` but not `web_setup_core.c`/`web_setup_json.c`. `tests/host/cmake/extra_tests.cmake` nevertheless builds `web_setup_tests` and `web_setup_json_tests` directly against those excluded files. | The suite currently makes dead setup code look intentionally production-covered; no guard/documentation distinguishes that coverage from the live v2 setup path. |
| F-023 duplicated routing pipelines | `web_server_lifecycle_tests` directly exercise registration/resolution of `normal_routes[]`; `web_api_administration_tests` separately exercise the wildcard administration dispatch. | No test ties the exact-match table and wildcard switch together so removing/moving an exact route cannot silently turn it into a wildcard 404 or change request-policy ordering. |
| F-024 health globals synchronization | `executor_health.c` and `storage_health.c` are directly host-linked. Their tests are sequential reset/record/snapshot unit tests. | No concurrent read/write regression exists; current tests prove state semantics only, not synchronization. |
| F-025 submission-cleanup release error | `macro_executor_engine.c` is directly host-tested. `executor_terminal_tests.inc` already proves `status.release_error` for normal terminal/final-release failures. The two submission-cleanup branches still discard `usb_release_all()` and then reset flags. | No regression injects release-all failure during submission unlock/queue failure and asserts the published status carries that `release_error`. |
| §4 minor/quality cluster | Coverage is mixed: `device_controls_logic.c`, `web_api_core.c`, and `web_request_policy.c` have direct host targets; `web_server_static.c` has no equivalent direct target in the host CMake list; administration/integration tests cover restart actions but not the documented restart asymmetry; Settings/Snapshots/Diagnostics each have UI coverage but no architecture test for duplicated file-save helpers. | Each behavior-changing cleanup needs a focused regression where applicable; pure comment/dead-code/helper-consolidation items need clean full-gate evidence rather than invented behavioral tests. |

## Cross-round coordination recorded at baseline

- Round 1 H5 has not been implemented at this baseline; F-016 therefore remains
  eligible for the Round 2-approved internal interim accounting fix if R2 is
  reached before H5.
- Round 1 H7/F-009 has not yet supplied the capture half required by F-025;
  R3-031 must either be implemented together with that capture or wait until it
  exists.
- Round 1 H4-042 has not yet been implemented; R4-042 must not assume F-020 was
  resolved as a side effect.

## R0 conclusion

The pre-fix baseline is green and the important coverage gaps are explicit.
No Round 2 finding is considered fixed by this document. In particular,
F-014 and F-017 retain their non-host-verification constraints, and F-016's
wrapper coverage gap remains open until a wrapper-level regression is added.
