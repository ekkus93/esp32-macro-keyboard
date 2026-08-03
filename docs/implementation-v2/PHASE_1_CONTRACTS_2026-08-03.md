# ESP32 Macro Keyboard v2 — Phase 1 Contract Checkpoint

**Phase:** 1 — Shared contracts, limits, and schema foundations  
**Status:** In progress; exit gate not claimed  
**Date:** 2026-08-03

## 1. Scope

This checkpoint records the v2 contract foundation added after the Phase 0
inventory. Production v1 package, macro, execution, and UI workflows remain in
place. The new v2 code is parallel contract infrastructure except where shared
constants are safely aliased into existing macro limits.

The exact commit containing this report is the checkpoint commit. A subsequent
clean-checkout evidence update must record its full commit SHA before Phase 1 is
marked complete.

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

Added one reviewed source and two generated mirrors:

- `contracts/v2/limits.json`;
- `firmware/components/app_contracts_v2/include/app_limits_v2.h`;
- `webapp/src/v2/limits.ts`.

Added `scripts/check-v2-limits.py`, which fails when either language mirror
changes independently of the JSON contract. `scripts/check-scripts.sh` runs that
check.

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

### 2.6 Focused contract gate

Added `scripts/check-v2-contracts.sh` and registered its native mode in
`./scripts/check-all.sh` before the ESP-IDF build.

The focused gate performs:

1. limit drift validation;
2. settings-layout drift validation;
3. native CMake build;
4. native CTest execution;
5. optionally, clean npm installation and the focused v2 Vitest files.

`check-format.sh` now includes `tests/v2_contracts`, and `check-scripts.sh` runs the
new drift and generator tests.

## 3. Executed evidence

The following evidence was executed in an isolated local harness because the
available repository connector cannot provide an executable checkout.

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

This is not the pinned Node.js `24.18.0` Vitest gate and is not presented as such.

### 3.2 Native C

The v2 macro compiler and settings codec were compiled in an isolated harness
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

This is meaningful source-level evidence but is not the checked-in CMake/CTest
run, ESP-IDF build, clang-tidy result, or full clean-checkout gate.

### 3.3 Script checks

The focused shell gate passed `bash -n` in the isolated environment. The local
container did not provide the repository-pinned clang-format, cmake-format,
cmake-lint, shfmt, or shellcheck toolchain, so those exact checks remain open.

## 4. Specification blocker

Phase 1 route policy cannot be frozen yet.

`docs/SPEC_V2.md` says an unprovisioned device exposes setup state and setup
submission, but the route table defines only `POST /api/v1/setup`. It does not
identify the setup-state read route or authorize unauthenticated
`GET /api/v1/status`.

The blocker and a non-normative recommendation are recorded in:

- `docs/implementation-v2/PHASE_1_ROUTE_POLICY_BLOCKER.md`.

A prematurely inferred machine-readable route table was deleted in a forward
commit. No unauthenticated read route has been implemented.

## 5. Phase 1 exit items still open

- Product-owner resolution of the setup-state read route.
- Exact route method, access, content-type, body-limit, and success-status table
  after that resolution.
- Clean-checkout execution of `bash scripts/check-v2-contracts.sh`.
- Pinned Node.js `24.18.0` typecheck, lint, format, and Vitest evidence.
- Checked-in native CMake/CTest execution.
- ESP-IDF `v5.5.5` firmware build and clang-tidy evidence.
- Full `./scripts/check-all.sh` result.
- Exact full checkpoint commit SHA in this report.
- PBKDF2 iteration-count hardware measurement remains intentionally deferred to
  Phase 4 and final hardware acceptance.

Phase 1 is therefore not marked complete, and Phase 2 production deletion work
has not begun.
