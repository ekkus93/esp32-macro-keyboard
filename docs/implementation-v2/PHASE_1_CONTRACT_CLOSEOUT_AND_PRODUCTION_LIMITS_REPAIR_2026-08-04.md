# ESP32 Macro Keyboard v2 — Phase 1 Contract Closeout and Production Limits Repair

**Status:** Software repair validated; hardware completion not claimed  
**Date:** 2026-08-04  
**Target branch:** `master` only  
**Starting SHA:** `cbfa0c0ca3b07ac7592c1463a9b326dd6e2bf074`  
**Validated software SHA:** `195806e1fca76777fe86a7d6b9873dc3ddb32efd`  
**Specification:**
[`docs/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_SPEC_2026-08-04.md`](../PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_SPEC_2026-08-04.md)  
**Checklist:**
[`docs/PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_TODO_2026-08-04.md`](../PHASE_1_CONTRACT_CLOSEOUT_AND_PRODUCTION_LIMITS_REPAIR_TODO_2026-08-04.md)

## 1. Scope and boundary

This repair is limited to the Phase 1 contract-layer closeout and the active
production `/api/v1/limits` response. It does not delete firmware-owned package
or macro repositories, package/macro CRUD routes, revision behavior, legacy
production parser entry points, legacy production NVS structures, or legacy React
setup paths. Those remain Phase 2 or later work.

No feature branch or pull request was created. Changes were committed directly to
`master`. No workflow or validation gate was weakened.

## 2. Starting finding

The authoritative limits contract contained 18 fields, but three consumers
exposed only 14:

- `firmware/components/web_server/web_server_adapter_json.c`;
- `contracts/v2/api/examples.json`;
- `webapp/src/v2/apiGuards.ts` and the associated `LimitsResponse` type.

The omitted fields were:

- `activeSessionsMax`;
- `sessionIdleLifetimeSeconds`;
- `sessionAbsoluteLifetimeSeconds`;
- `serialConfirmationTimeoutSeconds`.

Because the serializer, example, guard, and tests agreed with each other, the
existing tests preserved the incomplete response instead of detecting the drift.

## 3. Implemented software repair

### 3.1 Full production limits response

`web_adapter_build_limits_json()` now emits all 18 fields in contract order. The
new fields are sourced from the existing `APP_V2_*` constants. The response
remains a top-level JSON object with no `ok` or `data` wrapper and no retired
`macrosPerPackage`, `packages`, or `importBytes` fields.

The serializer preserves fail-closed truncation behavior: an undersized output
buffer produces `APP_ERROR_INTERNAL` and is cleared to an empty string.

### 3.2 TypeScript and example parity

The canonical API example, `LimitsResponse` type, and `isLimitsResponse()` now
require all 18 fields and exact values from `v2Limits`. The guard continues to
reject unknown fields.

The TypeScript contract test iterates over every limits key and proves that:

- the canonical 18-field example is accepted;
- removing any one field is rejected;
- changing any one value is rejected;
- adding an unknown field is rejected.

### 3.3 Drift gate

`scripts/check-v2-limits.py` now verifies the canonical API limits example in
addition to the C header, temporary legacy constants, and TypeScript limits
mirror. A future omission or value change in `contracts/v2/api/examples.json`
therefore fails the focused limits gate.

### 3.4 Hardware benchmark documentation

`firmware/test_app/README.md` now documents the `[benchmark]` Unity tag and the
required `PBKDF2_BENCH` evidence fields. The existing test harness:

- uses the production `auth_password_verify()` adapter;
- reaches mbedTLS `PBKDF2-HMAC-SHA-256` through the same production derive
  function;
- uses a 16-byte salt and 32-byte output;
- measures 60,000, 90,000, 120,000, and 150,000 iterations;
- records ten samples per candidate;
- prints median, p90, and worst-case microseconds.

The repository's legacy auth header currently defines 120,000 iterations, but no
committed ESP32-S3R8 measurement proves that value meets the v2 250–500 ms target.
It has therefore not been copied into the v2 settings contract as a measured and
frozen value.

## 4. Commits

| Commit | Change |
| --- | --- |
| `49424659d9530f3139101a970245e2252f66e8fa` | Emit full v2 limits response. |
| `c580238589c507aa04646a3e9d41932b3ce5a91c` | Test full response, wrapper absence, retired-key absence, and truncation. |
| `75f29c7bec5c84895b52558ab3b782657184be44` | Expand canonical API limits example. |
| `12dc26dbb0389fc72cacdb735e4d96fafa417410` | Expand the `LimitsResponse` type. |
| `ed19096a98cfe7232afaa31199f7e5243714cc73` | Require all fields in the runtime guard. |
| `c3ca5de31562cbe9440a1a3c7f30762d16271c2b` | Test every required limits field and value. |
| `a29f646fd93fb6d10f70c5f70d4a68d84614d594` | Gate the canonical API limits example. |
| `f41daaa2a5e11a8f33067a8c26253f58e83e895b` | Document physical PBKDF2 benchmark execution. |
| `dad05e65a00f4061e3361cdba502edc07eea6512` | Replace the lint-invalid dynamic deletion test. |
| `44207461ae202f7c53531cbd59e9c576882f0c06` | Normalize the omission-test layout. |
| `74bcb6b72f1d5aebe6ee8b4283cbbfe0c96d8bae` | Simplify missing-field construction. |
| `195806e1fca76777fe86a7d6b9873dc3ddb32efd` | Apply the formatter-required limits-guard layout. |

## 5. Files changed

- `firmware/components/web_server/web_server_adapter_json.c`
- `tests/host/test_web_server_adapter_json_static.inc`
- `contracts/v2/api/examples.json`
- `webapp/src/v2/apiTypes.ts`
- `webapp/src/v2/apiGuards.ts`
- `webapp/tests/v2-api-contracts.test.ts`
- `scripts/check-v2-limits.py`
- `firmware/test_app/README.md`
- this implementation report

## 6. PBKDF2 hardware evidence still required

No physical device is connected to this GitHub connector session. Consequently,
the following actions were not performed and are not claimed:

- building and flashing a specific ESP32-S3R8 from this session;
- running the `[benchmark]` Unity test on hardware;
- recording a board model or serial port;
- collecting real `PBKDF2_BENCH` timing lines;
- selecting a measured iteration count;
- replacing the v2 settings-contract placeholder;
- proving encode/decode/reset behavior for a newly frozen value.

The required operator procedure is:

```bash
bash ./scripts/build-device-tests.sh
cd firmware/test_app
idf.py -B build -p /dev/ttyUSB0 flash monitor
```

Press Enter in the Unity monitor and select `[benchmark]`. Capture every
`PBKDF2_BENCH` line and record the actual port, board model, host OS, and exact
commit SHA. The selection rule remains: choose the highest measured candidate
whose p90 is at or below 500 ms and whose median is at or above 250 ms. Do not
freeze an unmeasured interpolated value without explicit product-owner approval.

## 7. Validated software evidence

The complete software repair was validated on exact SHA
`195806e1fca76777fe86a7d6b9873dc3ddb32efd`. All four permanent `master`
workflows completed successfully on that same SHA:

| Workflow | Run | Job or jobs | Conclusion |
| --- | --- | --- | --- |
| Quality | `30955100062` | `92146219009` | success |
| Host Tests | `30955100784` | `92146169171`, `92146169229`, `92146169249`, `92146169288`, `92146169337` | success |
| Browser Tests | `30955100871` | `92146169221` | success |
| Device Test Build | `30955099974` | `92146201274` | success |

The Quality job's `Run authoritative checks` step completed successfully. That
step executes `./scripts/check-all.sh`, which fail-closed invokes
`./scripts/check-v2-contracts.sh --native-only`. The focused native contract gate
therefore ran and passed:

- `python3 scripts/check-v2-limits.py`;
- `python3 scripts/check-v2-settings-schema.py`;
- `python3 scripts/check-v2-setup-route-policy.py`;
- `python3 scripts/check-v2-api-routes.py`;
- native v2 CMake build and CTest;
- the remaining authoritative firmware, frontend, script, documentation, and
  test gates in `check-all.sh`.

Host Tests independently passed frontend type-checking, TypeScript lint,
stylelint, Prettier formatting, frontend tests, frontend coverage, host native
tests, ASan/UBSan, and native coverage. Browser Tests passed the real Chrome
workflow. Device Test Build passed device-source lint and ESP32-S3 firmware
compilation.

This is exact-SHA evidence for the completed software repair. It is not the final
Phase 1 closure SHA because the required physical benchmark will necessarily add
measured contract, firmware, test, and documentation changes.

## 8. Current conclusion

The production limits contract defect is repaired, regression-covered, and green
across all four permanent workflows on the same validated software SHA. Phase 1
closeout is **not complete** because the mandatory ESP32-S3R8 PBKDF2 measurement,
frozen v2 iteration value, and dependent tests remain open. Production v1
architecture deletion has not begun and is not claimed.

No unchecked hardware task or final Phase 1 closure is claimed complete.
