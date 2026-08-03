# ESP32 Macro Keyboard v2 — Phase 1 Contract Checkpoint

**Phase:** 1 — Shared contracts, limits, and schema foundations  
**Status:** In progress; exit gate not claimed  
**Date:** 2026-08-03  
**Contract implementation head before this report:**
`ed19a6b68a426d9ece53e3c1a2947e0c6ab7eed4`

## 1. Scope

This checkpoint records the v2 contract foundation added after the Phase 0
inventory. Production v1 package, macro, execution, and UI workflows remain in
place. The new v2 code is parallel contract infrastructure except where shared
constants are safely aliased into existing macro limits.

The product-owner decision for the first-run setup-state route has now been
resolved and incorporated into both authoritative documents and the shared
contracts. Phase 1 remains open because the checked-in clean-checkout gates have
not yet been executed successfully in an environment with the pinned toolchain.

## 2. Implemented contract artifacts

### 2.1 Strict repository contract

Added:

- `contracts/v2/repository/canonical.json`;
- `webapp/src/v2/repository.ts`;
- `webapp/src/v2/repositoryValidation.ts`;
- `webapp/tests/v2-repository.test.ts`;
- `webapp/tests/v2-repository-validation.test.ts`.

The validator enforces:

- exact root, package, and macro fields;
- no `activePackageId`;
- plain objects and dense arrays;
- canonical lowercase UUID v4 IDs;
- unique package IDs;
- globally unique macro IDs;
- UTF-8 byte limits;
- integer timing limits;
- TypeScript macro compilation;
- the 300-second configured-duration limit;
- deterministic canonical serialization.

### 2.2 Shared limits

Added one reviewed source and two verified mirrors:

- `contracts/v2/limits.json`;
- `firmware/components/app_contracts_v2/include/app_limits_v2.h`;
- `webapp/src/v2/limits.ts`.

Added `scripts/check-v2-limits.py`, which fails when either language mirror
changes independently of the JSON contract. `scripts/check-scripts.sh` and the
focused v2 gate run that check.

The limits live in the dependency-free `app_contracts_v2` component. This avoids
an invalid `macro_model` → `support` → `macro_model` dependency cycle.

`firmware/components/macro_model/include/macro_limits.h` currently aliases the
reusable macro limits to the v2 contract. Retired repository-only limits remain
explicitly labeled until their Phase 2 deletion.

### 2.3 API data contracts

Added:

- `contracts/v2/api/examples.json`;
- `webapp/src/v2/apiTypes.ts`;
- `webapp/src/v2/apiGuards.ts`;
- `webapp/src/v2/apiRequestGuards.ts`;
- `webapp/src/v2/apiContracts.ts`;
- `webapp/tests/v2-api-contracts.test.ts`;
- `webapp/tests/v2-api-requests.test.ts`;
- `firmware/components/app_contracts_v2/include/api_contracts_v2.h`.

The React guards reject unknown fields and validate the canonical request and
response examples for setup, login, sessions, status, limits, blobs, settings,
password changes, sends, reset operations, and diagnostics.

The approved setup-state response is now represented in the shared example,
TypeScript types and guards, C contract model, and response tests. Its exact
shape is:

```json
{
  "provisioned": false,
  "deviceName": "ESP32 Macro Keyboard"
}
```

The guard rejects `provisioned: true`, an empty device name, every unknown field,
and sensitive or expanded fields such as setup code, SSID, firmware details,
diagnostics, or repository data.

The C header is parser-neutral. It contains no cJSON, HTTP, filesystem, or route
handler dependency.

### 2.4 Shared macro-language contract

Added:

- `contracts/v2/macro-conformance.json`;
- `webapp/src/v2/macroCompiler.ts`;
- `webapp/src/v2/macroErrorClass.ts`;
- `webapp/tests/v2-macro-conformance.test.ts`;
- `webapp/tests/v2-macro-canonical-tokens.test.ts`;
- `firmware/components/macro_parser/macro_parser_v2.c`;
- `firmware/components/macro_parser/macro_plan_v2.c`;
- `firmware/components/macro_parser/include/macro_keymap_us_v2.h`;
- `firmware/components/macro_parser/macro_keymap_us_v2.c`;
- `tests/v2_contracts/test_macro_conformance.c`;
- `tests/v2_contracts/test_macro_canonical_tokens.c`;
- `scripts/generate-v2-macro-corpus.py`;
- `tests/scripts/test-generate-v2-macro-corpus.py`.

The existing v1 `macro_compile` remains untouched. The parallel
`macro_compile_v2` entry point implements the v2 zero-through-10,000 ms timing
range and is not yet used by production execution.

The shared corpus defines:

- source and timing;
- expected compiled actions;
- expected duration;
- exact error code, byte offset, line, and column;
- stable error message class.

The C suite no longer requires cJSON. CMake invokes the Python generator to
produce a native include from the same checked-in JSON consumed by Vitest.

Cross-language review found and fixed two drift cases before integration:

1. C initially accepted lowercase and punctuation chord keys that TypeScript
   rejected.
2. C initially accepted `{A}` and `{1}` as standalone named directives, although
   letters and digits are valid only as chord ordinary-key tokens.

Dedicated C and TypeScript tests now cover these cases.

### 2.5 Fixed-length device-settings contract

Added:

- `contracts/v2/device-settings.json`;
- `firmware/components/app_contracts_v2/include/device_settings_v2.h`;
- `firmware/components/app_contracts_v2/device_settings_v2.c`;
- `tests/v2_contracts/test_device_settings.c`;
- `scripts/check-v2-settings-schema.py`.

The record is 344 bytes, little-endian, and uses explicit offsets rather than C
struct layout. It includes:

- record and credential versions;
- PBKDF2 algorithm metadata;
- salt and verifier fields;
- optional next-blob counter;
- Quick Send or Preview mode;
- advisory retention target;
- source-preview and serial-confirmation preferences;
- provisioned and station-configured flags;
- opaque last-selected-package UUID;
- device, access-point, and station fields.

The codec rejects wrong length, magic, version, enum, boolean, reserved bytes,
UUID, UTF-8, string-tail, credential, and station invariants. Reset-settings
preserves provisioning state, AP credentials, administrator verifier, and the
next-blob counter while clearing station configuration and restoring UI defaults.

### 2.6 Approved setup-route policy

The product owner approved unprovisioned-only `GET /api/v1/setup`. The decision
is recorded in:

- `docs/SPEC_V2.md`;
- `docs/TODO_V2.md`;
- `docs/implementation-v2/PHASE_1_ROUTE_POLICY_BLOCKER.md`, now a resolved
  decision record.

Added:

- `contracts/v2/api/setup-route-policy.json`;
- `webapp/src/v2/setupRoutePolicy.ts`;
- `webapp/tests/v2-setup-route-policy.test.ts`;
- `firmware/components/app_contracts_v2/include/setup_route_policy_v2.h`;
- `tests/v2_contracts/test_setup_route_policy.c`;
- `scripts/check-v2-setup-route-policy.py`.

The contract fixes this access matrix:

```text
unprovisioned GET  /api/v1/setup  -> 200
unprovisioned POST /api/v1/setup  -> 202
all other unprovisioned API routes unavailable
provisioned   GET  /api/v1/setup  -> 404
provisioned   POST /api/v1/setup  -> 409
```

Both setup operations are unauthenticated only before provisioning. The GET has
no request body. The POST accepts bounded `application/json`. Both successful
responses use `application/json`.

The TypeScript validator rejects route reordering, extra API routes, changed
authentication, unbounded bodies, changed content types, changed statuses, sparse
arrays, and unknown fields. The Python drift checker verifies that the C constants
match the reviewed JSON contract. The native C test compiles and executes those
constants under the warning-as-error policy.

### 2.7 Focused contract gate

Added `scripts/check-v2-contracts.sh` and registered its native mode in
`./scripts/check-all.sh` before the ESP-IDF build.

The focused gate performs:

1. limit drift validation;
2. settings-layout drift validation;
3. setup-route-policy drift validation;
4. native CMake build;
5. native CTest execution;
6. optionally, clean npm installation and the focused v2 Vitest files.

`check-format.sh` includes `tests/v2_contracts`, and `check-scripts.sh` runs the
new drift and generator checks. The focused Vitest list now includes the setup
route-policy suite.

## 3. Executed evidence

The following evidence was executed in isolated local harnesses because the
available GitHub connector does not provide an executable checkout and the
container cannot resolve `github.com` for cloning.

### 3.1 TypeScript

Environment observed:

- Node.js `22.16.0`;
- TypeScript `5.8.3`.

The v2 compiler and repository-validation modules passed an isolated strict
compile with:

- `strict`;
- `noUncheckedIndexedAccess`;
- `exactOptionalPropertyTypes`;
- unused-local and unused-parameter checks.

Representative TypeScript execution passed for:

- zero key and inter-key timing;
- CRLF and Tab normalization;
- named keys and chords;
- escaped braces;
- delays;
- non-ASCII rejection;
- exact multiline error location;
- 300,000 ms acceptance and longer-duration rejection;
- canonical chord tokens;
- rejection of bare alphanumeric directives.

The setup route-policy module additionally passed a strict TypeScript 5.8.3
compile and runtime checks for:

- acceptance of the canonical fixture;
- rejection of an unknown root field;
- rejection of an added `/api/v1/status` route while unprovisioned;
- rejection of swapped provisioned GET/POST statuses.

The first runtime invocation used an incorrect path to the temporary JSON fixture
and exited with `MODULE_NOT_FOUND`. The path was corrected and the same compiled
module then passed all runtime checks. This was a harness-path error, not a
contract failure.

This is not the pinned Node.js `24.18.0` Vitest gate and is not presented as such.

### 3.2 Native C

The v2 macro compiler and settings codec were compiled in isolated harnesses
with:

```text
-std=c11
-Wall
-Wextra
-Werror
-Wshadow
-Wconversion
-Wsign-conversion
-Wformat=2
-Wdouble-promotion
-Wmissing-declarations
-Wstrict-prototypes
```

Representative native execution passed for:

- zero timing;
- printable HID mapping;
- CRLF normalization;
- named keys, chords, and delays;
- exact source error positions;
- canonical chord-token rejection;
- 300,000 ms duration boundary;
- provisioned and unprovisioned settings round trips;
- truncated, version, enum, boolean, reserved-byte, UUID, UTF-8, and tail-byte
  rejection;
- reset-settings preservation behavior.

The new setup-route policy passed:

- Python 3.13 syntax compilation;
- exact JSON validation;
- JSON-to-C macro synchronization;
- GCC 14.2 compilation with the warning-as-error flags used by the native suite;
- native runtime verification of path, methods, authentication label, content
  type, body-limit label, route count, route isolation flag, and four statuses.

This is meaningful source-level evidence but is not the checked-in CMake/CTest
run, ESP-IDF build, clang-tidy result, or full clean-checkout gate.

### 3.3 Script and checkout attempts

The focused shell gate passed `bash -n` in the isolated environment before the
setup-route update. The route-policy Python checker separately passed syntax and
runtime validation after the update.

A clean repository download/clone was attempted. The container could not resolve
`github.com`, so no checkout was produced. An earlier attempted snapshot command
therefore returned exit `127` with:

```text
bash: scripts/check-v2-contracts.sh: No such file or directory
```

That result means the checkout did not exist; it is not recorded as a contract
test failure or pass.

The local container does not provide the repository-pinned Node.js 24.18.0,
ESP-IDF 5.5.5, clang-format, cmake-format, cmake-lint, shfmt, shellcheck, or the
installed npm dependency tree required by the authoritative gates.

## 4. Resolved specification decision

The setup-state route ambiguity is resolved. The approved route and response are
now authoritative and contract-tested as described in §2.6.

No other route authentication or access policy was inferred as part of this
decision. The setup policy fixture deliberately covers only the approved
pre-provisioning surface and post-provisioning setup statuses.

## 5. Phase 1 exit items still open

- Clean-checkout execution of `bash scripts/check-v2-contracts.sh`.
- Pinned Node.js `24.18.0` typecheck, lint, format, and Vitest evidence.
- Checked-in native CMake/CTest execution.
- ESP-IDF `v5.5.5` firmware build and clang-tidy evidence.
- Full `./scripts/check-all.sh` result.
- Confirmation that the exact checked-in CMake and formatting configuration
  accepts every new C, CMake, TypeScript, Python, shell, JSON, and Markdown file.
- PBKDF2 iteration-count hardware measurement remains intentionally deferred to
  Phase 4 and final hardware acceptance.

Production v1 routes and repository serializers still exist and are scheduled
for Phase 2 deletion. Therefore the Phase 1 exit item stating that no production
route or serializer depends on a v1 shape cannot yet be claimed from the current
production build.

Phase 1 is not marked complete, and Phase 2 production deletion work has not
begun.
