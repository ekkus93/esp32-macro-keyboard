# FIX1 Spec/TODO Review — Responses

Covers the review of:

- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
- `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`

Fill in each `A:` line. Questions 1–5 are the primary decision points; 6–8 are
confirmations that unblock specific tasks.

---

1. **clang-tidy fail-closed vs. the third-party FreeRTOS diagnostic (SPEC §2.1 / §6.1)**

   Q: `run-clang-tidy` exits nonzero because of an unfixable `misc-header-include-cycle`
   in ESP-IDF's FreeRTOS headers (`idf_additions.h`), plus TinyUSB header findings, that
   `-header-filter` does not suppress. The current `scripts/check-firmware.sh` therefore
   uses `run-clang-tidy … || true` and greps by *finding location* for first-party paths.
   §6.1 requires the script to instead preserve `run-clang-tidy`'s exit status and not
   treat output grep as the sole source of truth — but doing that verbatim would fail the
   gate on the third-party diagnostic, contradicting §3.4. To make exit status trustworthy,
   do you want me to **disable `misc-header-include-cycle`** (it fires only on third-party
   framework headers) and any similar third-party-only checks so `run-clang-tidy`'s exit
   code becomes meaningful — or keep a location-based first-party filter but *also* honor
   exit status for first-party translation units?

   A:

2. **The three clang-tidy checks disabled this session (SPEC §2 / §3.4)**

   Q: `.clang-tidy` currently disables three checks, each documented as un-actionable on
   this toolchain: `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`
   (newlib has no C11 Annex-K `*_s` functions), `readability-non-const-parameter` (the
   TinyUSB `tud_hid_get_report_cb` `uint8_t* buffer` is mandated by the callback prototype),
   and `concurrency-mt-unsafe` (`readdir`; its only mt-safe variant `readdir_r` is deprecated
   and undeclared on the glibc host). FIX1's fail-closed / no-hidden-suppression posture may
   contest these, and the storage rewrite (§7) still needs `readdir` while §2.3 does not touch
   the TinyUSB callback. Keep them disabled-with-documentation, or must FIX1 eliminate them
   another way (e.g., checked wrapper functions, restructuring)?

   A:

3. **Execution timeout representation (SPEC §12.4 vs §17.2 / §17.8)**

   Q: The current terminal-state enum is `EXECUTION_{IDLE,RUNNING,COMPLETED,CANCELLED,FAILED}`
   with no `TIMED_OUT` (timeout maps to `EXECUTION_FAILED` + `APP_ERROR_TIMEOUT`). §12.4 allows
   either approach, but the frontend guard (`isExecutionStatus`, §17.2) and result labeler
   (§17.8) hard-code a distinct `"timed_out"` state. Add a distinct `EXECUTION_TIMED_OUT` state
   end-to-end (enum + API + frontend), or keep `EXECUTION_FAILED` + `APP_ERROR_TIMEOUT` on the
   backend and have the frontend derive "timed out" from the error code?

   A:

4. **Branch vs. master (TODO §1.1)**

   Q: The baseline commit `992f2a018aff97e5b167c98d6a0d469d6a7c84ff` is current `HEAD`. §1.1 says
   to create a dedicated implementation branch "unless the operator explicitly instructs direct
   work on `master`." This session worked directly on `master` per your convention. Continue FIX1
   directly on `master`, or create a dedicated implementation branch for this program?

   A:

5. **Scope and sequencing of this engagement**

   Q: FIX1 is a 24-phase, ~469-checkbox program, and its release gate requires physical hardware
   (eFuse/flash-encryption provisioning, the Linux+ChromeOS USB matrix, SoftAP/browser integration,
   real power-interruption, cancellation-latency measurement) that cannot be exercised in this
   environment. How do you want to proceed: (a) work the host-testable phases in order and clearly
   mark hardware/HIL/crypto-provisioning items as environment-blocked; (b) do just an early slice
   (e.g., Phase 2 gate hardening, then Phases 3–4 ownership) and reassess; or (c) something else?
   Recommendation: (b) — land Phase 2 first because it redefines what a "green" gate means.

   A:

6. **Concurrency-test mechanism (TODO §9.3)**

   Q: The host test harness is currently single-threaded and uses a custom assert framework (no
   pthreads). §9.3 requires proving serialization ("two updates with the same expected revision
   cannot both succeed", etc.) via "host threads or deterministic fake scheduling." Confirm the
   approach: deterministic fake scheduling / an injected lock seam (safer, reproducible), or real
   host threads?

   A:

7. **Intentional correction of tests that encode current (unsafe) behavior (SPEC §2)**

   Q: §2 authorizes correcting tests that assert unsafe or false-success behavior. Concretely this
   will rewrite existing *passing* tests, including: the auth verify tests (as the API changes from
   `bool` to `app_error_code_t` + `out_matches`), the frontend cancellation label in
   `webapp/tests/app-execution.test.tsx` ("finished" → "cancelled"), and any test asserting the
   current partial-cleanup / false-idle behavior. Confirm these should be rewritten rather than
   preserved (so the changes aren't mistaken for regressions).

   A:

8. **FIX1 precedence over `docs/SPEC.md` (SPEC §2)**

   Q: FIX1 outranks `docs/SPEC.md`. Where they diverge (e.g., §15.4 persists settings/UI preferences
   "only when specified by `docs/SPEC.md`"), I will follow FIX1. Confirm that FIX1 is authoritative on
   any such divergence.

   A:

---

Fill in the `A:` lines above, then share this file (or paste the answers back) and I'll
proceed with implementation per your decisions.
