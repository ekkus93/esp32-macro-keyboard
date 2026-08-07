# V2-040 — First-run provisioning host preparation

**Phase:** 4 — Authentication, provisioning, and device settings  
**Task:** V2-040 — First-run provisioning  
**Status:** Host-side contract preparation only; V2-040 remains open  
**Implementation commit:** `904f9dc1e40b92ace3cc77382c81285a24785c1e`  
**C/C-header formatter repair:** `fc3e997c3e74390de527cb19b371d9d9bd198943`  
**CMake formatter repair:** `238b7299b2bdff4603ffc21910efaffb258e6c7e`

## Scope

This change prepares the fail-closed C contract and native regression coverage needed before the live firmware route migration. It deliberately does **not** register or expose the V2 setup HTTP handlers and does not claim any unchecked V2-040 item complete.

The host-preparation implementation changed exactly these permanent files:

- `firmware/components/app_contracts_v2/CMakeLists.txt`
- `firmware/components/app_contracts_v2/include/setup_contract_v2.h`
- `firmware/components/app_contracts_v2/setup_contract_v2.c`
- `scripts/check-v2-contracts.sh`
- `scripts/check-v2-setup-contract.py`
- `tests/v2_contracts/CMakeLists.txt`
- `tests/v2_contracts/test_setup_contract.c`

The two subsequent repair commits contain formatter-only changes to those new host-prep files. Neither repair changes contract behavior or test semantics. Both temporary materializer workflows removed themselves and are not part of the permanent repository state.

## Contract behavior prepared

The new `app_contracts_v2` setup contract provides host-testable primitives for the later live firmware integration:

- generation of an eight-digit ASCII decimal one-time setup code from caller-supplied randomness;
- rejection sampling instead of biased modulo-only selection;
- fail-closed behavior on random-source failure or excessive rejected samples;
- explicit setup-session initialization and one-time consumption;
- constant-time comparison of a well-formed supplied setup code;
- rejection of malformed, mismatched, consumed, and already-provisioned states;
- derivation of the minimal unprovisioned setup response without secret fields;
- strict UTF-8 and byte-bound validation for device name, AP SSID, AP passphrase, and administrator password inputs;
- validation of versioned password material supplied by the later password-verifier integration;
- preparation of a candidate settings record while preserving unrelated existing settings;
- separation between candidate preparation and setup-code consumption so a caller can consume the code only after a successful settings commit;
- exact non-secret accepted/restart/reconnect response semantics.

The fixture validator `scripts/check-v2-setup-contract.py` fails closed if the reviewed setup examples drift from their exact field sets, if the setup code is not exactly eight ASCII decimal digits, if retired setup fields reappear, or if the accepted response changes from the reviewed V2 semantics.

## Native regression coverage

`tests/v2_contracts/test_setup_contract.c` covers:

- unbiased random-code generation including `00000000`;
- random-source and rejection-budget failure with zeroized output state;
- minimal unprovisioned setup state and rejection after provisioning;
- preservation of unrelated device settings;
- code consumption only after the caller elects to commit success;
- wrong, malformed, and reboot-stale code rejection;
- strict field boundaries including malformed UTF-8;
- invalid password material;
- already-provisioned rejection;
- exact accepted/restart/reconnect response flags.

The native V2 CTest suite contains six targets:

1. `v2_macro_conformance`
2. `v2_macro_canonical_tokens`
3. `v2_device_settings`
4. `v2_setup_contract`
5. `v2_setup_route_policy`
6. `v2_api_routes`

All native targets compile with the repository's warning-as-error policy.

## Quality failures and repairs

### 1. C/C-header formatting

The first permanent Quality run on `904f9dc1e40b92ace3cc77382c81285a24785c1e` failed in the authoritative checks because the three new setup-contract C/C-header files were not fully `clang-format` compliant. This was a formatting defect, not a test, algorithm, or analyzer failure.

Failing evidence:

- workflow run: `31167622201`
- job: `92831983100`
- failing step: `Run authoritative checks`
- uploaded artifact: `failed-quality-31167622201-1`
- artifact SHA-256: `79b12e0969015efab75477bdb9e7fa48c44ca4899ebbc0aabdaf496edcf68734`

A temporary self-removing formatter workflow materialized the Ubuntu 24.04 `clang-format` result. It ran `clang-format -i`, immediately required `clang-format --dry-run --Werror`, ran `git diff --check`, and ran the native V2 contract suite before pushing the formatter-only repair.

Repair evidence:

- trigger commit: `5139f8f61f8827e65e69a305dedf6a6343831af7`
- workflow run: `31178562547`
- job: `92866103555`
- formatter validation: pass
- native V2 contract suite: pass
- resulting repair: `fc3e997c3e74390de527cb19b371d9d9bd198943`

### 2. CMake formatting

The first documentation candidate `ec7c658e72f71939fa68a062935f65bc26bc23b4` then reached the permanent gates. Browser Tests, Host Tests, and Device Test Build passed, while Quality correctly rejected the new `tests/v2_contracts/CMakeLists.txt` because it was not `cmake-format` compliant.

Intermediate permanent-CI evidence on `ec7c658e72f71939fa68a062935f65bc26bc23b4`:

- Browser Tests run `31178807291`: pass
- Host Tests run `31178807966`: pass
- Device Test Build run `31178807559`, job `92866937498`: pass, including device-test source lint and ESP32-S3 device-test firmware build
- Quality run `31178808406`, job `92866956767`: fail at `Run authoritative checks`
- failed-quality artifact: `failed-quality-31178808406-1`
- artifact SHA-256: `28335d552a1a7ff381e7ea9a6308d768badc0994767ff2563c0abf1664b4141e`
- authoritative log finding: `Check failed: tests/v2_contracts/CMakeLists.txt`

A second temporary self-removing workflow installed the repository-pinned `cmakelang==0.6.13`, formatted only `tests/v2_contracts/CMakeLists.txt`, then required `cmake-format --check`, `cmake-lint`, `git diff --check`, and the complete native V2 contract suite before committing.

CMake repair evidence:

- trigger commit: `2c6c89786d7d784d127a09ff94a96fb779acdf80`
- workflow run: `31179336283`
- job: `92868598089`
- pinned formatter installation: pass
- `cmake-format --check`: pass
- `cmake-lint`: pass
- `git diff --check`: pass
- native V2 contract suite: pass
- resulting repair: `238b7299b2bdff4603ffc21910efaffb258e6c7e`

No warning suppression, ignored exit code, analyzer exclusion, compatibility fallback, or quiet failure was introduced to make either gate pass.

## Deferred live-firmware work

The following V2-040 behavior remains intentionally unimplemented or unproven by this host-preparation cycle:

- live unprovisioned route exposure limited to `GET /api/v1/setup`, `POST /api/v1/setup`, and required static assets;
- `GET /api/v1/setup` returning the exact response over the real HTTP server and returning `404` after provisioning;
- every other `/api/v1` route being unavailable while unprovisioned;
- boot-time generation and serial presentation of the real one-time setup code;
- real setup-code lifetime/reboot semantics in production firmware;
- parsing and strict unknown-field rejection in the live POST handler;
- password-verifier derivation and benchmark integration, which belongs to V2-041;
- atomic NVS/settings commit integration followed by setup-code consumption;
- live `409` behavior after provisioning;
- restart, disconnect, AP credential transition, and reconnect behavior;
- device/hardware validation on the reference ESP32-S3R8.

No fallback route, compatibility alias, or alternate success path is being added for those deferred behaviors.

## Hardware evidence

No hardware was used in this host-preparation cycle. Therefore there are no hardware model, serial port, firmware build ID, timing, reconnect, storage, or memory observations to report here.

## Final exact-SHA CI boundary

The commit containing this revised report is the final host-preparation candidate. The cycle closes only if the permanent Browser Tests, Host Tests, Device Test Build, and Quality workflows all complete successfully on that exact commit. GitHub's immutable workflow/run records are the exact-SHA evidence for that final gate; a further documentation commit is not required merely to copy those IDs into this file.

If any of the four permanent gates fails, this report is not completion evidence and the Ralph loop remains open until the defect is repaired and all four gates pass on a new exact final candidate SHA.

## Completion statement

This report records preparation for V2-040 only. **V2-040 remains unchecked and no unchecked TODO item is being claimed complete.** Live firmware integration and hardware evidence remain subsequent Ralph-loop work.
