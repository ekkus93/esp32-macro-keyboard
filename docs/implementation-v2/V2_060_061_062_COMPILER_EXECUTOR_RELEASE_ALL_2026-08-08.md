# Phase 6 (partial) — Compiler compliance, executor state machine, release-all

**Task:** V2-060, V2-061, V2-062 (V2-063 and V2-064 explicitly excluded — see
below)
**Branch:** `phase6-compiler-executor-release-all` (as instructed; not merged to
`master`, not pushed)
**Status:** Implemented and host-tested (including under ASan+UBSan). Firmware
builds and clang-tidy pass for both `firmware/` and `firmware/test_app/`. Not
claimed as physical-hardware-validated (no device access from this worktree).

## Scope actually delivered

### V2-060 — Compiler compliance

Audited, not rewritten: `macro_parser_v2.c`/`macro_plan_v2.c`/
`macro_keymap_us_v2.c` (already present before this task, adapted from the
retained v1 parser as the migration map instructed) already satisfy every
checklist item:

- The 21-case shared conformance corpus (`contracts/v2/macro-conformance.json`)
  is consumed by both a native C suite (`tests/v2_contracts/test_macro_conformance.c`,
  via a generated `.inc` from `scripts/generate-v2-macro-corpus.py`) and the
  TypeScript suite (`webapp/tests/v2-macro-conformance.test.ts`), wired into
  `scripts/check-v2-contracts.sh` (itself called from `check-all.sh`). Both
  pass in full (21/21 native, 30/30 combined with the canonical-tokens suite on
  the TypeScript side).
- `macro_compile_v2()` never partially populates `*out_plan`: the working plan
  is only assigned to the caller's plan after the entire source has been
  consumed without error, and any mid-loop failure frees the working buffer and
  clears `*out_plan` first.
- Every non-ASCII byte (`>= 0x80`) and the NUL byte are rejected as an
  unsupported character, which by construction rejects all multi-byte UTF-8
  sequences (both lead and continuation bytes fall in that range) — "reject all
  invalid Unicode" is satisfied without a separate UTF-8-aware validator.
- `v2_fail()`/`v2_locate()` compute exact byte offset, line, and column for
  every error, exercised by the corpus's `multiline error location` case.
- `v2_append_action()` enforces `APP_V2_COMPILED_ACTIONS_MAX` and
  `APP_V2_ESTIMATED_DURATION_MAX_MS` per-action, before the action is ever
  appended to the plan — i.e., before acceptance, not after.
- Compile failure emits no HID report by construction: `web_send.c` (owned by
  the concurrent Phase 5 stream, not touched here) only calls
  `macro_executor_submit()` after `macro_compile_v2()` returns
  `APP_ERROR_NONE`; a failed compile never reaches the executor at all, so no
  code path exists for it to type anything.

No firmware changes were needed for V2-060; this is an audit result with
existing, passing evidence.

### V2-061 — Single executor and state machine

- Added `EXECUTION_AWAITING_CONFIRMATION` to `execution_state_t`
  (`macro_executor.h`), the state SPEC_V2 §7.12 and the PHASE_5 report's Known
  gap #3 identified as missing. Placed between `EXECUTION_IDLE` and
  `EXECUTION_RUNNING` in the enum's declaration order (matching the spec's
  listed order); this renumbers the later constants, which is safe because
  every consumer (including `firmware/components/web_server`, deliberately not
  touched by this task) compares by symbolic name only — confirmed by a full
  `check-firmware.sh` pass that builds `web_server` against the changed header.
- Added a `require_confirmation` field to `macro_execution_request_t`, defaulting
  to `false` via zero-initialization so every existing caller (`web_send.c`'s
  designated-initializer construction, the host `web_send_tests` fakes) is
  unaffected. This is the seam a future settings-aware caller uses to opt a
  request into the confirmation flow; wiring `requireSerialConfirmation` from
  device settings into `web_send.c`'s request construction, and dispatching the
  serial-console `confirm` command to the new `macro_executor_confirm()`, is
  explicitly *not* done here (see Known gaps).
- Implemented the state machine itself in `macro_executor_engine.c`:
  - `macro_executor_engine_execute()` publishes `EXECUTION_AWAITING_CONFIRMATION`
    (not `EXECUTION_RUNNING`) when `request->require_confirmation`, then calls
    the new `run_confirmation_phase()` before any action executes.
  - `await_confirmation()` polls in `CANCELLATION_SLICE_MS` (10 ms) slices
    against a wall-clock deadline (`now_ms() + CONFIRMATION_TIMEOUT_MS`, using
    `APP_V2_SERIAL_CONFIRMATION_TIMEOUT_SECONDS` from `app_limits_v2.h` — the
    exact 60-second bound SPEC_V2 §7.12 requires), the same
    `deadline_expired()`/slice idiom `cancellable_delay()` already uses for
    in-run cancellation responsiveness, rather than a countdown of a fixed slice
    count (an earlier version of this used a countdown and blew a test
    fixture's 1024-entry call-log capacity simulating a full 60-second timeout;
    the wall-clock design lets a test jump the clock in one call, exactly like
    the existing watchdog test does).
  - `macro_executor_engine_confirm()` (new; wrapped by `macro_executor_confirm()`
    at the component's public API) mirrors `macro_executor_engine_cancel()`'s
    contract: `APP_ERROR_NOT_FOUND` when no send is awaiting confirmation,
    `APP_ERROR_CONFLICT` on a second confirm of the same send (idempotent
    no-op), `APP_ERROR_NONE` on success, notifying the executor task the same
    way cancellation does.
  - A cancellation while awaiting confirmation is honored immediately (the
    existing `cancellation_requested` flag and `macro_executor_engine_cancel()`
    already work correctly here with no changes, since `engine->busy` is set
    before the confirmation wait begins).
  - Expiry without confirmation or cancellation returns `APP_ERROR_TIMEOUT`,
    which `macro_executor_engine_execute()` maps to `EXECUTION_TIMED_OUT` —
    matching SPEC_V2 §7.12's "Expiry produces `timed_out` and types nothing"
    exactly (no action ever runs in this path; `engine->ops.plan_free()` is
    called and `finish_execution()` still attempts release-all per V2-062).
- Fixed PHASE_5 report's Known gap #4: `validate_request()` rejected
  `key_press_ms == 0`, a retired v1-shaped lower bound the v2 contract
  (`macro_compile_v2()`, `APP_V2_KEY_PRESS_MAX_MS`) does not share — the
  documented range is the complete 0–10,000 ms. Removed the `== 0U` rejection;
  `key_press_ms > APP_DELAY_MAX_MS` remains the only bound.
- Replaced the per-request watchdog deadline (`estimated_duration_ms + 1000 ms`
  margin, a FIX1-era heuristic not derived from any SPEC_V2 number) with the
  literal SPEC_V2 §7.11 "absolute executor deadline 310,000 ms" constant
  (`APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS`), computed once from the moment
  execution actually starts (post-confirmation, if any) — a fixed ceiling
  independent of any particular request's own smaller estimate, matching "a
  10-second safety margin beyond the *maximum accepted* estimate" (300,000 ms)
  rather than beyond each request's own estimate.
- Audited and confirmed already correct, no changes needed:
  - Exactly one executor task (`macro_executor.c`'s single `xTaskCreate` call,
    unchanged).
  - HTTP handlers cannot type directly — `web_send.c` only reaches USB through
    `ops->submit()`/`ops->cancel()`/`ops->get_status()` function pointers into
    this component; the actual `usb_keyboard_press()`/`_release_all()` calls are
    private to `macro_executor_engine.c`'s worker-task-only code path.
  - No second send is queued: `macro_executor_engine_submit()`'s `engine->busy`
    check returns `APP_ERROR_EXECUTOR_BUSY` (mapped to `409` by `web_send.c`)
    while a send is non-terminal, whether awaiting confirmation or running.
  - The current or most recent send survives for `GET`-based status recovery:
    `engine->status` is left in its terminal state after `finish_execution()`
    and is not cleared until the next `macro_executor_engine_execute()`
    publishes a fresh status — unaffected by this task's changes, exercised by
    the pre-existing `test_recovery_after_each_terminal_outcome` test.

### V2-062 — Release-all invariant

- Audited and confirmed already correct: `execute_action()` unconditionally
  calls `ops.usb_release_all()` after every key/chord press, regardless of
  outcome; `finish_execution()` unconditionally attempts release-all on every
  terminal transition (completion, cancellation, failure, timeout) and always
  clears `engine->busy`/`cancellation_requested`/`confirmed_requested` even when
  the release-all transport call itself fails (`test_key_release_failure_after_success`,
  `test_press_release_and_final_release_errors` already cover this); the
  `status.release_error` field is always separate from `status.error`, never
  conflated.
- Extended `finish_execution()`'s reach to the new confirmation-phase
  cancellation/expiry paths (via `run_confirmation_phase()`) — both attempt
  release-all before the send reaches a terminal state, even though nothing was
  ever pressed.
- Found and fixed a genuine, previously-unaddressed gap:
  `macro_executor_engine_submit()`'s failure paths for an unlock failure and a
  `queue_send()` failure — the two failures SPEC_V2 §7.3 names explicitly
  ("internal error", "queue failure") — never attempted release-all; they only
  reset the busy flag. Added an unconditional `ops.usb_release_all()` attempt
  (result intentionally not folded into the returned error code, matching
  `finish_execution()`'s own "attempt, don't gate on success" semantics) to
  both paths. New assertions in `test_submission_ownership_and_recovery`
  confirm `release_index` increments exactly on these two paths and not on the
  earlier lock-failure path (where `busy` was never claimed and nothing needs
  releasing).

## Files changed

- `firmware/components/macro_executor/include/macro_executor.h` —
  `EXECUTION_AWAITING_CONFIRMATION`, `require_confirmation` field,
  `macro_executor_confirm()` declaration.
- `firmware/components/macro_executor/macro_executor_engine.h` —
  `confirmed_requested` field, `macro_executor_engine_confirm()` declaration.
- `firmware/components/macro_executor/macro_executor_engine.c` — the state
  machine, timeout, deadline, and release-all-on-submission-failure changes
  above; `run_confirmation_phase()` extracted from
  `macro_executor_engine_execute()` to keep its clang-tidy
  `readability-function-cognitive-complexity` score under the enforced
  threshold (25) after the confirmation branch was added (it briefly hit 31;
  extraction, not suppression, brought it back under).
- `firmware/components/macro_executor/macro_executor.c` — `macro_executor_confirm()`
  wrapper.
- `firmware/components/macro_executor/CMakeLists.txt` — added `app_contracts_v2`
  to `REQUIRES` (for `app_limits_v2.h`'s `APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS`
  and `APP_V2_SERIAL_CONFIRMATION_TIMEOUT_SECONDS`).
- `tests/host/CMakeLists.txt` — added the same include directory to
  `macro_executor_tests`.
- `tests/host/executor_test_fixture.h`/`.inc` — `confirm_on_wait`,
  `capture_second_confirm`/`second_confirm_result` fake-harness fields, mirroring
  the existing `cancel_on_wait` mechanism.
- `tests/host/executor_validation_tests.inc` — removed the assertion that
  `key_press_ms == 0` is rejected (it now must be accepted); added
  `test_zero_key_press_ms_is_accepted`; added release-all assertions to
  `test_submission_ownership_and_recovery`'s existing failure-path cases.
- `tests/host/executor_execution_tests.inc` — `test_watchdog_and_wait_failures`
  updated for the new fixed-310s deadline (two delay slices plus a
  clock-jumping `extra_advance_on_wait_ms`, since a flat per-request-estimate
  deadline no longer applies).
- `tests/host/executor_confirmation_tests.inc` (new) — five tests: confirm
  before any send exists (`NOT_FOUND`), confirm on a non-confirmation-gated
  running/completed send (`NOT_FOUND`), confirmation granted (runs to
  completion, second confirm is `CONFLICT`), cancellation while awaiting
  confirmation (types nothing), and expiry (types nothing, terminal
  `EXECUTION_TIMED_OUT`).
- `tests/host/test_macro_executor.c` — wires the new test file in and includes
  `app_limits_v2.h`.
- `docs/SPEC_V2_TEST_TRACEABILITY.md` — regenerated
  (`python3 scripts/generate-spec-traceability.py`) to pick up the new
  `SPEC_V2 §7.3`/`§7.11`/`§7.12` citations this task's tests and comments add.

## Known gaps and deliberate scope boundaries

1. **`POST /api/v1/send` still does not gate on physical confirmation.**
   `require_confirmation` exists and is fully functional in
   `macro_executor_engine`, but nothing sets it to `true`: doing so needs
   `web_send.c` (owned by the concurrent Phase 5/V2-055 stream, explicitly on
   this task's do-not-touch list) to read `requireSerialConfirmation` from
   device settings and populate the new request field. This is exactly what
   PHASE_5_EXACT_V2_HTTP_API_2026-08-08.md's Known gap #3 described as "Phase 6
   executor-internals work" — the internals are now done; the HTTP-layer wiring
   remains open, deliberately, for whichever stream next owns `web_send.c`.
2. **The serial-console `confirm` command is not wired to
   `macro_executor_confirm()`.** `firmware/components/serial_console/serial_console.c`'s
   existing `confirm` command currently only calls
   `device_controls_signal_confirmation()`, a pre-existing, separate
   confirmation primitive used for physical-button-style admin confirmations
   (e.g. credential reset) — a different mechanism from the per-send
   confirmation this task implements. Wiring the same console command to also
   (or instead, for a send specifically) call `macro_executor_confirm()` was
   not done: it's dead code without gap 1 above being resolved first, and
   touching `serial_console.c` for a currently-unreachable code path seemed
   like scope creep beyond "V2-060/061/062 only, in `macro_parser`/
   `macro_executor`". Flagged here rather than silently left implicit.
3. **V2-063 (cancellation responsiveness) and V2-064 (USB identity and HIL
   evidence) are untouched**, per the assigning task's explicit scope boundary.
   Cancellation during typing and delay is already responsive (pre-existing
   `cancellable_delay()`/`CANCELLATION_SLICE_MS` design, now reused unchanged
   for the confirmation wait too), and is host-tested, but no real-device
   last-keystroke-latency measurement (V2-063's explicit checklist item) was
   taken — that requires the physical board.

## Test evidence

All commands run from the repository root, worktree
`.claude/worktrees/agent-aeec2f41a9edb2f0b`, branch
`phase6-compiler-executor-release-all`.

- `./scripts/run-tests.sh` (all 45 host suites, all labels): **100% tests
  passed, 0 tests failed out of 45.**
- `./scripts/run-tests.sh --sanitizers executor` (ASan+UBSan): **100% tests
  passed, 0 tests failed out of 2** (`macro_executor`, `executor_health`).
- `bash scripts/check-v2-contracts.sh --native-only`: **100% tests passed, 0
  tests failed out of 6**, including `v2_macro_conformance` (21/21 corpus
  cases).
- `npm --prefix webapp run test -- tests/v2-macro-conformance.test.ts
  tests/v2-macro-canonical-tokens.test.ts`: **2 files, 30 tests passed.**
- `bash scripts/check-format.sh`: clean (clang-format, cmake-format/cmake-lint,
  shfmt, prettier all pass).
- `bash scripts/check-firmware.sh`: GCC build succeeds for `firmware/` and
  `firmware/test_app/`; clang-tidy (esp-clang LLVM 19, `WarningsAsErrors: '*'`)
  reports zero first-party findings for both, after fixing one genuine
  cognitive-complexity violation this task introduced (see above).
- `./scripts/check-all.sh` (the full gate): run three times, two real failures
  found and fixed, neither suppressed:
  1. The first full run failed at `check-docs.sh`'s
     `generate-spec-traceability.py --check` step because this task's new
     `SPEC_V2 §7.3`/`§7.11`/`§7.12` test citations made the committed
     `docs/SPEC_V2_TEST_TRACEABILITY.md` stale. Fixed by running
     `python3 scripts/generate-spec-traceability.py` and committing the
     regenerated file (a three-line diff, verified with `git diff`).
  2. The second full run failed at the same `check-docs.sh` step's
     `markdownlint-cli2` pass, on this report file itself: its `#` heading
     wrapped across two source lines with trailing punctuation
     (`MD022`/`MD026`). Fixed by rewording the heading onto one line.
  3. The third, complete run passed end to end: `./scripts/check-firmware.sh`
     (GCC + clang-tidy clean for both `firmware/` and `firmware/test_app/`),
     `./scripts/check-stack-usage.sh` (559 first-party frames analyzed, largest
     1536 bytes, 0 allowlisted), `./scripts/build-webfs-image.sh`,
     `./scripts/check-release-budgets.sh`, the full `./scripts/check-webapp.sh`
     chain (ci → format:check → typecheck → lint → stylelint → 298/298 vitest
     tests → coverage → build → browser tests), `./scripts/check-scripts.sh`'s
     regression suites, `./scripts/check-docs.sh`, and the default
     (non-sanitizer) `./scripts/run-tests.sh` (all 45 host suites).

No physical hardware was available in this worktree; nothing here is claimed as
HIL-validated.
