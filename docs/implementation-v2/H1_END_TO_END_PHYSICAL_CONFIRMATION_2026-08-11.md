# H1 — End-to-end physical confirmation for real sends

**Date:** 2026-08-11  
**Phase:** `H1 — End-to-end physical confirmation for real sends`  
**Baseline audited:** `2b09661a4d6971c6af4ee5ee0c76c451f1d95719`  
**Software implementation SHA:** `697f44ef10aa5441b09e5858d7fab41631188d8f` (`fix: complete H1 software confirmation path`)  
**Hardware-harness contract correction SHA:** `f7d615d7a40f5a7617da8e78b8b3cddb0299041a` (`fix: align H1 hardware harness with v2 settings contract`)  
**Architecture-guard follow-up SHA:** `7f6d3be66f54f29c5e939e150bb5829c51eef459` (`test: guard H1 serial confirmation production binding`)

## 1. Audit result

H1 was not a greenfield implementation. The H9 hardening work had already wired the real send boundary to the authoritative persisted `device_settings_t.require_serial_confirmation` value and made a settings-read failure reject the send rather than defaulting confirmation off. The executor also already had a confirmation wait state, cancellation, timeout, and no-keypress-before-confirmation tests, and the React Macros page already had an inline waiting state.

The H1 audit nevertheless found two production defects that prevented the real feature from satisfying the v2 contract end to end:

1. **The HTTP send state was wrong/incomplete.** A confirmation-gated accepted POST was always serialized as `"state":"running"`, and GET state serialization had no `EXECUTION_AWAITING_CONFIRMATION` mapping. The executor could be waiting correctly while the API lied or returned an empty state string.
2. **The documented UART `confirm` command was disconnected from macro execution.** `serial_console.c` signalled only the administrative `device_controls` confirmation primitive and never called `macro_executor_confirm()`. A real send could therefore remain in `awaiting_confirmation` until timeout even after the owner typed the documented serial `confirm` command.

Both defects are corrected below. The second fix deliberately prevents one generic `confirm` command from authorizing two unrelated confirmation domains.

## 2. Authoritative setting capture and fail-closed behavior

The existing H9 implementation remains the correct H1-010 design:

- `web_server_send.c` reads `device_settings_t` through the authoritative settings API;
- `require_serial_confirmation` is copied into the send policy result;
- `web_send_create_handle()` captures that boolean before submission and stores it in `macro_execution_request_t.require_confirmation`;
- the accepted result now also records the same captured value so response serialization cannot be changed by a later settings mutation;
- failure to read the setting returns `WEB_SEND_CREATE_BACKEND_UNAVAILABLE`; there is no confirmation-off fallback.

The host regression now explicitly changes the fake setting after acceptance and proves both the submitted request and accepted response retain the original captured `true` value.

## 3. API state correction

`web_send.c` now:

- maps `EXECUTION_AWAITING_CONFIRMATION` to the exact wire value `awaiting_confirmation` for GET status;
- serializes a confirmation-gated accepted POST with `state=awaiting_confirmation`;
- keeps confirmation-disabled accepted POSTs at `state=running`;
- continues using the existing redacted status object, which contains execution metadata but no macro source or key content.

The checked-in canonical accepted-response example remains valid because that fixture represents the confirmation-disabled path; the authoritative spec already defines `awaiting_confirmation`, so no schema expansion or speculative contract change was required.

Host route regressions exercise the real send handlers for confirmation-required POST, GET while awaiting, and DELETE while awaiting.

## 4. Serial `confirm` routing

A new host-testable `serial_console_route_confirmation()` seam gives a pending macro send precedence:

- `APP_ERROR_NONE` from `macro_executor_confirm()` consumes the command;
- `APP_ERROR_CONFLICT` also consumes the command, preventing a duplicate send-confirm command from falling through and authorizing an unrelated administrative action;
- only `APP_ERROR_NOT_FOUND` means no send confirmation is pending and permits fallback to `device_controls_signal_confirmation()`;
- any other send-confirmation error fails closed and does not authorize the administrative confirmation domain.

`serial_console.c` routes the production `confirm` command through this seam. The permanent architecture guard additionally requires the production file to include the router, call `macro_executor_confirm()`, and invoke `serial_console_route_confirmation(&operations)` so the helper cannot become disconnected while its unit tests continue passing.

## 5. Executor safety regressions

Existing executor confirmation tests already prove:

- confirmation-required work enters `EXECUTION_AWAITING_CONFIRMATION`;
- zero key presses occur before confirmation;
- confirmation starts execution once;
- cancel-before-confirmation terminates without typing;
- confirmation timeout terminates without typing.

H1 strengthens the failure-path assertions so confirmation read-lock failure, wait failure, and confirm-lock timeout also assert that defensive release-all ran. Together with the existing H7 release-fault latch, a failed defensive release cannot silently permit subsequent sends.

## 6. React/browser behavior

`MacrosPage` now tells the user the actual required action without exposing any secret:

`Run confirm in the device serial console to continue.`

The added frontend regressions assert that:

- the awaiting-confirmation state remains cancellable;
- polling can transition `awaiting_confirmation -> running` without another POST;
- the real-browser scenario keeps Cancel reachable while awaiting and verifies exactly one POST across the confirmation transition.

These test sources were inspected and the browser harness passes `node --check`, but this sandbox has Node 22.16.0 rather than the pinned Node 24.18.0 and has no installed frontend dependencies. Therefore Vitest and the real-Chrome scenario were **not executed locally**. H1-014 remains open until executable pinned-Node/real-browser evidence is supplied; source-level implementation alone is not being promoted to browser evidence.

## 7. Real-device acceptance harness

`tests/hardware/test_send_confirmation.py` is the committed H1-015 harness. Follow-up `f7d615d7a40f5a7617da8e78b8b3cddb0299041a` aligns its settings mutation with the actual v2 partial-update contract (no stale `expectedRevision` field) and reads the nested `settings` object from the PUT response. It requires the exact flashed Git SHA and refuses anonymous evidence. On an already-provisioned reference ESP32-S3R8 it will:

1. preserve the existing `requireSerialConfirmation` setting and enable it;
2. start native USB HID capture;
3. POST a send and require the accepted API state to be `awaiting_confirmation`;
4. prove zero key-down HID reports before confirmation;
5. issue the real UART `confirm` command and prove the expected text is typed and execution ends released;
6. repeat with cancel-before-confirmation and prove zero typed reports;
7. repeat with the real 60-second confirmation expiry and prove `timed_out` plus zero typed reports;
8. restore the owner's original confirmation setting on exit.

This intentionally uses the real 60-second timeout instead of a shortened hardware-only substitute. The harness was syntax-checked locally but **not run on hardware in this sandbox**, so H1-015 and the full H1 exit gate remain open.

## 8. Local validation

Using the same temporary, uncommitted cJSON development shim documented by H0/H9 against the installed cJSON 1.7.18 runtime:

- `./scripts/run-tests.sh executor` — **3/3 passed**;
- `./scripts/run-tests.sh web` — **30/30 passed**;
- `./scripts/run-tests.sh --sanitizers executor` — **3/3 passed** under ASan+UBSan;
- `./scripts/run-tests.sh --sanitizers web` — **30/30 passed** under ASan+UBSan;
- `./scripts/run-tests.sh` — **61/61 passed**;
- `./scripts/run-tests.sh --sanitizers` — **61/61 passed** under ASan+UBSan;
- `python3 scripts/check-h9-architecture.py` — passed, including the H1 serial-console production-binding guard;
- `python3 scripts/check-v2-phase2-architecture.py` — passed;
- `bash scripts/check-credential-logging.sh firmware` — passed;
- `bash tests/scripts/test-check-credential-logging.sh` — **16/16 passed**;
- `bash tests/scripts/test-test-assert-redaction.sh` — **3/3 passed**;
- `python3 -m py_compile tests/hardware/test_send_confirmation.py` — passed;
- `node --check webapp/tests/browser/run-browser-tests.mjs` — passed.

No GitHub CI job was monitored for this H1 work.

## 9. H1 disposition

Software implementation and native evidence close H1-010, H1-011, H1-012, and H1-013. They do **not** close H1-014 or H1-015:

- **H1-014 remains open** for pinned-Node Vitest and real-Chrome execution of the confirmation workflow after these changes.
- **H1-015 remains open** for the exact-SHA ESP32-S3R8/HID/UART run of `tests/hardware/test_send_confirmation.py`.
- The H1 phase exit gate remains open until the browser and hardware evidence exist on an exact candidate SHA.

There is no known software fallback that disables required confirmation after a settings-read or serial-routing failure; those paths are fail-closed. This report does not convert host fakes or source inspection into real-device evidence.
