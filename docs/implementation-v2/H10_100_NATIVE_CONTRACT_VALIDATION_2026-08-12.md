# H10-100 Native/Contract Validation — 2026-08-12

## Scope

This report records the first H10-100 validation pass from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.
It intentionally records partial completion only. The full clang-tidy gate and the
authoritative gcovr coverage report remain open.

## Candidate and sandbox provenance

- GitHub product/code candidate after the H10-100 route-guard repair:
  `1c83dbb4cc143452a5a498eb13966d2beacae669`.
- The sandbox tree was reconstructed from the user-provided master archive at
  `d11089e141490356ce93c369c73e1c07fc9820e8` plus the GitHub deltas through the
  candidate above.
- The final local blobs for `scripts/check-web-route-dispatch-sync.py` and
  `tests/scripts/test-check-web-route-dispatch-sync.sh` matched the GitHub blobs
  at the candidate SHA exactly.
- Host builds used a sandbox-local cJSON 1.7.18 development header/pkg-config
  shim under `/tmp/cjson-shim`; the shim did not modify any repository file.

## H10-100 results

### Complete host suite

Command:

```text
./scripts/run-tests.sh
```

Result: **PASS — 66/66 tests**.

Label counts observed in the complete run included auth 5, controls 6, executor
3, parser 1, startup 6, storage 14, web 31, USB 2, Wi-Fi 2, plus support/model
coverage.

### Complete host suite under ASan + UBSan

Command:

```text
./scripts/run-tests.sh --sanitizers
```

Result: **PASS — 66/66 tests** with the repository's configured ASan/UBSan fatal
settings.

### V2 contract corpus/native checks

Command:

```text
bash scripts/check-v2-contracts.sh --native-only
```

Result: **PASS — 6/6 native contract tests**:

- `v2_macro_conformance`
- `v2_macro_canonical_tokens`
- `v2_device_settings`
- `v2_setup_contract`
- `v2_setup_route_policy`
- `v2_api_routes`

The pre-build contract synchronization checks for limits, device settings,
setup policy/examples, and the 21-route API manifest also passed.

### Route-manifest and setup isolation

Commands:

```text
bash scripts/check-setup-route-isolation.sh
python3 scripts/check-web-route-dispatch-sync.py
bash tests/scripts/test-check-web-route-dispatch-sync.sh
```

Final result: **PASS**.

The first H10-100 pass exposed a real regression: H3-034 had correctly inserted
`reset_guarded_*` HTTP wrappers, but `check-web-route-dispatch-sync.py` still
compared registered handler names directly against the pre-wrapper delegates.
This caused the authoritative route synchronization check to fail even though
the registered routes delegated to the intended handlers.

Repair commits:

- `b203d470f18bc69ab212cef9c62c4e0740747a6a` — teach the route synchronization
  checker to resolve `DEFINE_RESET_GUARDED_HANDLER(wrapper, delegate)` mappings
  before validating the route partition;
- `1c83dbb4cc143452a5a498eb13966d2beacae669` — add negative regressions proving
  a wrong wrapper delegate and a registered wrapper with no delegate mapping
  both fail closed.

The repair does not whitelist guarded handler names. It verifies that each
registered reset wrapper resolves to the expected underlying route handler and
fails when the mapping is missing or wrong.

### Static analysis

Command completed locally:

```text
./scripts/check-static-analysis-policy.sh
```

Result: **PASS**. The approved disabled-check set, `WarningsAsErrors: '*'`, and
first-party no-suppression policy remain intact. The policy regression suite also
passed 6/6.

The actual clang-tidy execution required by H10-100 is **OPEN**. This sandbox
does not currently contain `clang-tidy`, `run-clang-tidy`, or an installed
ESP-IDF v5.5.5 clang environment. The include-cycle clang-tidy regression
therefore stopped explicitly with `error: no clang-tidy found`; no suppression,
fake success, or downgraded gate was used.

### Native coverage

Coverage-instrumented host command:

```text
./scripts/run-tests.sh --coverage
```

Result: **PASS — 66/66 tests**.

The authoritative `scripts/generate-native-coverage.sh` report remains **OPEN**
because `gcovr` is not installed and the sandbox has no network access to
install it. As a non-authoritative cross-check only, GCC 14 `gcov --json-format`
data from the coverage build was aggregated for the same pure-policy source set:

- line coverage: **2564 / 2668 = 96.10%**;
- branch coverage: **1866 / 2261 = 82.53%**.

Those values are above the repository's 90% line / 80% branch thresholds, but
this report does **not** substitute them for the required gcovr output and does
not mark the native coverage-policy checkbox complete.

## Additional architecture checks

The following current-tree guards also passed during this pass:

```text
python3 scripts/check-v2-auth-policy.py
python3 scripts/check-h3-architecture.py
python3 scripts/check-h9-architecture.py
```

Results: authentication policy passed; H3 factory-reset and H3-034 reset-settings
architecture guards passed; H9 architecture guard passed.

## H10-100 disposition

Completed and evidenced:

- complete host suite;
- complete ASan + UBSan host suite;
- v2 native/contract corpus;
- route-manifest and setup-isolation checks.

Still open:

- real clang-tidy/static-analysis execution with warnings fatal;
- authoritative gcovr native coverage report and exact gcovr values.

H10-100 is therefore **not complete** and the H10 phase exit gate is not claimed.
No unchecked H10 task is being represented as complete.
