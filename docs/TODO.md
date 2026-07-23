# ESP32 Macro Keyboard — Authoritative Implementation TODO

**Document status:** Execution plan for `docs/SPEC.md`
**Target:** ESP32-S3, ESP-IDF v5.5.5
**Implementation order:** Mandatory unless a dependency requires a documented change
**Last updated:** 2026-07-22

## 0. Rules for the implementation agent

Before changing code, read `docs/SPEC.md` completely.

The following rules are non-negotiable:

- Implement only behavior defined by `docs/SPEC.md`.
- Do not invent silent fallback behavior.
- Do not automatically format LittleFS after any mount or integrity failure.
- Do not start an open Wi-Fi AP if protected AP setup fails.
- Do not type directly from an HTTP handler.
- Do not start a macro until the whole source has parsed and compiled.
- Do not queue a second user execution while one is active.
- Do not return success after partial completion.
- Do not ignore an `esp_err_t`, short write, close error, rename error, parser
  error, task error, or linter result.
- Do not store passwords, setup codes, sessions, or CSRF tokens in plaintext.
- Do not log credentials, cookies, tokens, or raw macro source.
- Do not modify ESP-IDF or third-party dependencies to satisfy project linting.
- Do not suppress first-party warnings or lint findings.
- Do not use `NOLINT`, `eslint-disable`, `@ts-ignore`, `@ts-nocheck`,
  diagnostic pragmas, `-Wno-*`, `-Wno-error=*`, ignored exit codes, `|| true`,
  or hidden stderr to make checks pass.
- Every first-party warning or lint finding is a bug. Fix the code.
- Keep every referenced assistant-created handoff file in the repository at the
  exact path named by the documents.
- Do not reference mockup SVG or PNG files until they actually exist.

Each task below has a completion gate. Do not mark a task complete until its
gate passes.

## 1. Phase 0 — Repository and toolchain bootstrap

### 1.1 Create the monorepo directory structure

Create these paths:

```text
firmware/
firmware/main/
firmware/components/app_core/
firmware/components/auth/
firmware/components/device_controls/
firmware/components/macro_executor/
firmware/components/macro_model/
firmware/components/macro_parser/
firmware/components/storage/
firmware/components/usb_keyboard/
firmware/components/web_server/
firmware/components/wifi_ap/
firmware/test_app/
firmware/webfs/
webapp/src/api/
webapp/src/components/
webapp/src/features/auth/
webapp/src/features/execution/
webapp/src/features/macros/
webapp/src/features/procedures/
webapp/src/features/sets/
webapp/src/features/settings/
webapp/src/pages/
webapp/src/types/
webapp/tests/
scripts/
tests/
.github/workflows/
```

A bootstrap script may use:

```bash
#!/usr/bin/env bash
set -euo pipefail

directories=(
  firmware/main
  firmware/components/app_core
  firmware/components/auth
  firmware/components/device_controls
  firmware/components/macro_executor
  firmware/components/macro_model
  firmware/components/macro_parser
  firmware/components/storage
  firmware/components/usb_keyboard
  firmware/components/web_server
  firmware/components/wifi_ap
  firmware/test_app
  firmware/webfs
  webapp/src/api
  webapp/src/components
  webapp/src/features/auth
  webapp/src/features/execution
  webapp/src/features/macros
  webapp/src/features/procedures
  webapp/src/features/sets
  webapp/src/features/settings
  webapp/src/pages
  webapp/src/types
  webapp/tests
  scripts
  tests
  .github/workflows
)

mkdir -p "${directories[@]}"
```

Do not leave empty directories as the final result. Add the appropriate build,
source, test, or README file as each component is implemented.

**Completion gate**

- The intended directories exist.
- No unrelated generated files are committed.
- ShellCheck and shfmt pass for the bootstrap script.

### 1.2 Pin ESP-IDF v5.5.5

Add:

- `.env.example`;
- `scripts/install-esp-idf.sh`;
- `scripts/verify-toolchain.sh`;
- developer setup documentation.

`verify-toolchain.sh` MUST compare the active IDF version against `v5.5.5` and
fail with a clear message on mismatch.

Example logic:

```bash
#!/usr/bin/env bash
set -euo pipefail

expected="v5.5.5"
actual="$(git -C "${IDF_PATH:?IDF_PATH is not set}" describe --tags --exact-match 2>/dev/null)"

if [[ "${actual}" != "${expected}" ]]; then
  printf 'error: ESP-IDF %s is required; active version is %s\n' \
    "${expected}" "${actual:-unknown}" >&2
  exit 1
fi
```

Do not silently accept `release/v5.5`, `master`, or another tag.

**Completion gate**

- A clean environment can install and activate the exact recursive tag.
- The verification script rejects a deliberately wrong tag.
- Script lint and formatting pass.

### 1.3 Create the ESP-IDF project skeleton

Create:

```text
firmware/CMakeLists.txt
firmware/sdkconfig.defaults
firmware/partitions.csv
firmware/main/CMakeLists.txt
firmware/main/idf_component.yml
firmware/main/app_main.c
```

Set the target to `esp32s3`.

Add managed dependencies through the component manager. Pin selected compatible
versions and commit `firmware/dependencies.lock`.

Do not commit `firmware/managed_components/`.

**Completion gate**

- `idf.py set-target esp32s3` succeeds.
- `idf.py build` succeeds against ESP-IDF v5.5.5.
- The dependency lockfile is present.
- No unbounded floating production dependency remains.

### 1.4 Define the partition table

Create an OTA-ready layout for at least 8 MiB flash with:

- NVS;
- OTA data;
- PHY init if required;
- app slot A;
- app slot B;
- `webfs` LittleFS partition;
- `userdata` LittleFS partition;
- core dump partition.

Use blank offsets where ESP-IDF can calculate safe aligned offsets. Add a build
check that validates total size against the configured flash size.

Do not use one mutable partition for both firmware web assets and user data.

**Completion gate**

- Partition generation succeeds.
- Both application slots fit the firmware budget.
- Web assets and user data have independent partitions.
- A deliberate overflow fails the build.

### 1.5 Bootstrap the frontend

Create a Vite React TypeScript application under `webapp/`.

Required configuration:

- strict TypeScript;
- React;
- Tailwind CSS;
- relative or device-safe asset paths;
- hash routing;
- no CDN;
- no remote font;
- no server-side runtime;
- committed Node version pin;
- committed package lockfile.

**Completion gate**

- `npm ci` succeeds.
- `npm run build` produces static assets.
- The production build contains no remote HTTP/HTTPS asset dependency.
- The build works when opened without internet access.

## 2. Phase 1 — First-party quality gate

### 2.1 Add formatting and lint configuration

Add project-owned configuration for:

- clang-format;
- clang-tidy;
- first-party C compiler warnings;
- ESLint;
- TypeScript;
- Prettier;
- Stylelint;
- ShellCheck;
- shfmt;
- Ruff if Python is present;
- CMake lint/format;
- markdownlint;
- YAML and JSON validation.

Scope tools to first-party paths. Exclude only the exact third-party and
generated paths listed in `docs/SPEC.md`.

For clang-tidy, use a first-party header filter similar to:

```text
^(firmware/main|firmware/components|firmware/test_app)/
```

Do not add source-level suppressions.

### 2.2 Configure strict firmware warnings

Add strict warning options to each first-party component target rather than
modifying ESP-IDF component targets globally.

Start with a validated subset such as:

```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wshadow
  -Wconversion
  -Wsign-conversion
  -Wformat=2
  -Wundef
  -Wdouble-promotion
  -Wmissing-declarations
  -Wstrict-prototypes
)
```

Validate each option against the ESP-IDF v5.5.5 compiler. Fix first-party source
findings. If a finding is exclusively in a third-party header, correct tool
scope or system-header classification without weakening first-party checks.

### 2.3 Add the authoritative check script

Create `scripts/check-all.sh`.

Suggested structure:

```bash
#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

scripts/verify-toolchain.sh
scripts/check-format.sh
scripts/check-firmware.sh
scripts/check-webapp.sh
scripts/check-scripts.sh
scripts/check-docs.sh
scripts/run-tests.sh
```

Every child script MUST propagate failure. No child may redirect or ignore a
diagnostic.

### 2.4 Add CI

Create a GitHub Actions workflow that:

1. checks out submodules when required;
2. installs the pinned ESP-IDF tag and Node version;
3. restores only safe dependency caches;
4. runs `./scripts/check-all.sh`;
5. uploads test reports only after the command runs;
6. fails on every first-party warning, lint finding, type error, format
   difference, or failed test.

CI MUST call the same local script developers use.

**Phase completion gate**

- A deliberately introduced C warning fails locally and in CI.
- A deliberately introduced ESLint warning fails locally and in CI.
- A formatting difference fails.
- A failing test cannot be hidden.
- Third-party source is not edited or linted as first-party.
- Removing the deliberate defects returns the build to green.

## 3. Phase 2 — Shared limits, result types, and data model

### 3.1 Implement centralized limits

Create:

```text
firmware/components/macro_model/include/macro_limits.h
webapp/src/types/limits.ts
```

Firmware is authoritative. Expose limits through `GET /api/v1/limits`.
Generate or test the frontend constants against the API representation to avoid
drift.

Include every limit from Section 10.7 of `docs/SPEC.md`.

### 3.2 Implement IDs and revisions

Add bounded UUID version 4 generation using the hardware RNG.

Define typed IDs rather than passing unchecked paths or arbitrary strings.

Example:

```c
#define APP_UUID_STRING_LENGTH 36U
#define APP_UUID_BUFFER_LENGTH 37U

typedef struct {
    char value[APP_UUID_BUFFER_LENGTH];
} app_uuid_t;
```

Provide strict parse, validate, compare, and format helpers.

Never concatenate an unvalidated user ID into a filesystem path.

### 3.3 Define first-party error codes

Create a stable error enum and conversion to API error strings.

Example categories:

```c
typedef enum {
    APP_ERROR_NONE = 0,
    APP_ERROR_INVALID_ARGUMENT,
    APP_ERROR_NOT_FOUND,
    APP_ERROR_CONFLICT,
    APP_ERROR_STORAGE_UNAVAILABLE,
    APP_ERROR_STORAGE_FULL,
    APP_ERROR_STORAGE_CORRUPT,
    APP_ERROR_MACRO_SYNTAX,
    APP_ERROR_MACRO_LIMIT,
    APP_ERROR_USB_NOT_READY,
    APP_ERROR_EXECUTOR_BUSY,
    APP_ERROR_EXECUTION_CANCELLED,
    APP_ERROR_AUTH_REQUIRED,
    APP_ERROR_AUTH_FAILED,
    APP_ERROR_RATE_LIMITED,
    APP_ERROR_INTERNAL
} app_error_code_t;
```

Do not collapse distinct expected errors into `ESP_FAIL`.

### 3.4 Define persistent models

Create bounded C model types for:

- macro set;
- macro;
- procedure;
- procedure step;
- progress;
- settings;
- transaction manifest;
- package metadata.

Create matching TypeScript interfaces.

Do not place arbitrarily sized strings directly on small task stacks. Use
explicit ownership and bounded allocation.

Example TypeScript core:

```ts
export type MacroScope = "set" | "global";

export interface Macro {
  schemaVersion: 1;
  id: string;
  revision: number;
  scope: MacroScope;
  setId?: string;
  name: string;
  source: string;
  favorite: boolean;
  keyPressMs: number;
  interKeyMs: number;
}

export type ProcedureStep =
  | {
      id: string;
      type: "macro";
      title: string;
      macroId: string;
      required: boolean;
      autoCompleteOnSuccess: boolean;
    }
  | {
      id: string;
      type: "instruction" | "checkpoint";
      title: string;
      body: string;
      required: boolean;
    };
```

**Phase completion gate**

- C and TypeScript models cover every required field.
- Invalid IDs and revisions are rejected.
- Limits are centralized.
- Serialization round trips are tested.
- All quality checks pass with zero warnings.

## 4. Phase 3 — Macro parser and compiler

### 4.1 Implement lexer/parser

Create:

```text
firmware/components/macro_parser/include/macro_parser.h
firmware/components/macro_parser/macro_parser.c
firmware/components/macro_parser/macro_keymap_us.c
firmware/components/macro_parser/test/
```

The parser MUST:

- accept only the supported source character set;
- normalize CRLF;
- handle `{{` and `}}`;
- parse every named key;
- parse chords;
- parse bounded delays;
- reject unknown or malformed syntax;
- track byte offset, line, and column;
- consume the whole input;
- produce no executable plan on failure.

Suggested API:

```c
typedef enum {
    MACRO_ACTION_KEY,
    MACRO_ACTION_CHORD,
    MACRO_ACTION_DELAY
} macro_action_type_t;

typedef struct {
    macro_action_type_t type;
    uint8_t modifiers;
    uint8_t usage;
    uint32_t delay_ms;
} macro_action_t;

typedef struct {
    macro_action_t *actions;
    size_t action_count;
    uint32_t estimated_duration_ms;
} macro_plan_t;

app_error_code_t macro_compile(
    const char *source,
    size_t source_length,
    const macro_compile_options_t *options,
    macro_plan_t *out_plan,
    macro_parse_error_t *out_error);
```

`out_plan` MUST remain empty on failure.

### 4.2 Implement US key mapping

Create a complete table for printable ASCII to HID usage plus modifiers.

Do not use locale-dependent C library conversion. Test every supported
character.

### 4.3 Implement duration accounting

The compiler calculates a bounded estimated duration using:

- key press duration;
- inter-key delay;
- explicit delay actions;
- overflow-safe arithmetic.

Reject plans exceeding the maximum total duration.

### 4.4 Add parser tests and fuzzing

Cover all requirements from `docs/SPEC.md`.

At minimum, test:

```text
Hello, world!{ENTER}
{{literal braces}}
{CTRL+ALT+T}{DELAY:500}cd /tmp{ENTER}
```

Reject:

```text
{WAIT:500}
{CTRL+}
{CTRL+SHIFT}
{CTRL+A+B}
{DELAY:0}
{DELAY:10001}
{
}
é
```

Add property/fuzz tests that prove the parser never writes outside bounds and
never returns a partial plan on error.

**Phase completion gate**

- Every ASCII mapping test passes.
- Exact location tests pass.
- Boundary tests pass.
- Fuzz/property test corpus runs cleanly.
- Parser code has zero warnings and no suppressions.

## 5. Phase 4 — LittleFS and transactional storage

### 5.1 Integrate separate LittleFS mounts

Create:

```text
firmware/components/storage/storage_mount.c
firmware/components/storage/storage_paths.c
firmware/components/storage/include/storage.h
```

Mount:

```text
/web
/data
```

Set `format_if_mount_failed = false` or the component-equivalent behavior.

Verify that web and user-data labels match `partitions.csv`.

### 5.2 Implement safe path construction

Every path builder accepts validated typed IDs and writes into a bounded caller
buffer.

Example:

```c
app_error_code_t storage_make_macro_path(
    const app_uuid_t *set_id,
    const app_uuid_t *macro_id,
    char *buffer,
    size_t buffer_size);
```

Reject traversal, separators, and malformed IDs before path construction.

### 5.3 Implement atomic single-file writes

Create a helper that:

- creates a unique temporary path;
- writes all bytes with a loop;
- checks short writes;
- flushes;
- calls `fsync` when supported;
- checks close;
- reopens and validates;
- renames transactionally;
- preserves old committed content on failure.

Do not make the helper report success until every required step succeeds.

### 5.4 Implement transaction manifests

Create a transaction engine for:

- set import;
- set replacement;
- set duplication;
- set deletion;
- full restore;
- schema migration.

The manifest records operation type, phase, source, staging path, destination,
backup/trash path, and revisions.

Startup recovery MUST be deterministic and idempotent.

### 5.5 Implement repositories

Implement storage repositories for:

- set index and ordering;
- set metadata;
- set macro ordering and objects;
- global macro ordering and objects;
- procedures;
- progress;
- settings backup metadata;
- quarantine inventory.

All update/delete methods require expected revision.

### 5.6 Implement corruption and quarantine behavior

An invalid file MUST:

- produce a stable error;
- be recorded with source path and parse reason;
- be preserved;
- not be replaced with empty/default data;
- be shown by diagnostics.

### 5.7 Implement storage tests and fault injection

Add test seams for:

- short write;
- ENOSPC;
- failed flush/close;
- failed rename;
- power interruption after each transaction phase;
- corrupt JSON;
- missing index;
- orphan resource;
- mount failure.

**Phase completion gate**

- CRUD and ordering tests pass.
- Stale updates return conflict.
- Every injected failure preserves old or new committed state.
- No test causes automatic formatting.
- Quarantine retains original evidence.
- All storage code passes strict quality checks.

## 6. Phase 5 — USB HID and macro executor

### 6.1 Add USB descriptors and TinyUSB initialization

Create:

```text
firmware/components/usb_keyboard/usb_descriptors.c
firmware/components/usb_keyboard/usb_keyboard.c
firmware/components/usb_keyboard/include/usb_keyboard.h
```

Define:

- VID/PID policy for development;
- manufacturer string;
- product string;
- stable non-secret serial;
- HID report descriptor;
- boot keyboard interface where supported.

Do not copy an example descriptor without verifying endpoint, interface, and
string behavior for this product.

### 6.2 Implement USB state tracking

Expose a thread-safe state machine:

```c
typedef enum {
    USB_KEYBOARD_UNINITIALIZED,
    USB_KEYBOARD_DISCONNECTED,
    USB_KEYBOARD_ENUMERATING,
    USB_KEYBOARD_READY,
    USB_KEYBOARD_SUSPENDED,
    USB_KEYBOARD_ERROR
} usb_keyboard_state_t;
```

Publish state changes to the application without blocking TinyUSB callbacks.

### 6.3 Implement report methods

Required methods:

```c
app_error_code_t usb_keyboard_press(
    uint8_t modifiers,
    uint8_t usage);

app_error_code_t usb_keyboard_release_all(void);

usb_keyboard_state_t usb_keyboard_get_state(void);
```

Every report submission must have a bounded completion/ready policy. Never wait
forever.

### 6.4 Implement the executor task

Create:

```text
firmware/components/macro_executor/macro_executor.c
firmware/components/macro_executor/include/macro_executor.h
```

Use one task and one owned immutable plan.

Suggested command:

```c
typedef struct {
    app_uuid_t execution_id;
    app_uuid_t set_id;
    app_uuid_t macro_id;
    uint32_t macro_revision;
    macro_plan_t plan;
} macro_execution_request_t;
```

The submit function atomically rejects busy state. It does not queue another
user execution.

### 6.5 Implement cancellation

Cancellation MUST interrupt:

- text/key loops;
- explicit delays;
- waiting for USB readiness where applicable;
- physical confirmation.

Use short bounded waits or task notifications so a 10-second delay is still
immediately cancellable.

### 6.6 Enforce release-all

Use one terminal cleanup path that is reached by every completion and failure
branch.

Pseudocode:

```c
static void finish_execution(execution_state_t terminal_state,
                             app_error_code_t error)
{
    const app_error_code_t release_result = usb_keyboard_release_all();
    executor_clear_pressed_state();
    executor_publish_terminal_state(terminal_state, error, release_result);
}
```

Do not overwrite the original terminal error with the release error. Report both
where needed.

### 6.7 Add USB and executor tests

Test:

- exact report sequence;
- shifted characters;
- chords;
- busy rejection;
- cancel during delay;
- disconnect;
- suspend timeout;
- report failure;
- watchdog timeout;
- release-all on every terminal branch.

**Phase completion gate**

- Linux recognizes the device as a keyboard.
- Harmless text types correctly in a test field.
- Every terminal-path test observes release-all attempt.
- The cancel button stops a long delay promptly.
- No executor warning or linter finding remains.

## 7. Phase 6 — Device controls and startup orchestration

### 7.1 Implement Kconfig board settings

Add configurable GPIOs and active levels for:

- confirmation button;
- cancel button;
- status LED;
- reset gesture.

Do not hardcode board-specific GPIO values in subsystem code.

### 7.2 Implement debounced controls

Use bounded debounce and explicit press events. Define short press, long press,
and boot-held behavior without ambiguous overlap.

### 7.3 Implement status indication

Map application states to documented LED patterns. The indicator task MUST not
block executor, USB, Wi-Fi, or HTTP tasks.

### 7.4 Implement startup state machine

`app_main` should orchestrate explicit stages:

```text
boot
NVS initialization
configuration validation
webfs mount
userdata mount/recovery
USB initialization
controls initialization
Wi-Fi initialization
HTTP server initialization
ready/degraded/fatal state
```

Each stage records success or an explicit error. Do not continue into a state
that violates a later subsystem's preconditions.

**Phase completion gate**

- Every startup failure has a documented state and log.
- No invalid stage is reported ready.
- Physical controls remain responsive.
- Startup code passes all quality checks.

## 8. Phase 7 — SoftAP, credentials, authentication, and sessions

### 8.1 Implement configuration storage

Create NVS keys with schema versioning for device configuration only.

Use a random salt and a documented password derivation function. Never store
plaintext passwords.

### 8.2 Implement first-boot provisioning

Generate:

- protected AP credentials;
- one-time setup code;
- initial device name and SSID.

Print bootstrap data only in development/provisioning context. Redact it from
normal later logs.

Do not start an open AP.

### 8.3 Implement SoftAP

Create `wifi_ap` with explicit event handling and bounded client count.

Expose:

- starting;
- ready;
- client count;
- stopped;
- error.

Do not enable NAT or internet routing.

### 8.4 Implement login throttling

Use bounded state and monotonic time. Return `429` with retry information after
the threshold.

### 8.5 Implement sessions and CSRF

Create random RAM-only sessions with:

- idle timeout;
- bounded table;
- logout invalidation;
- reboot invalidation;
- CSRF token;
- constant-time token comparison.

Cookie MUST be `HttpOnly`, `SameSite=Strict`, and `Path=/`. It cannot be marked
`Secure` on the version 0.1 HTTP-only SoftAP.

### 8.6 Add authentication tests

Test:

- setup code;
- successful login;
- wrong password;
- throttling;
- expiry;
- logout;
- reboot;
- session-table exhaustion;
- missing/invalid CSRF;
- invalid Origin and Host;
- log redaction.

**Phase completion gate**

- No unauthenticated mutation succeeds.
- No macro execution succeeds without auth and CSRF.
- Credentials never appear in ordinary logs or diagnostics.
- No open AP fallback exists.

## 9. Phase 8 — HTTP server and API

### 9.1 Implement the static file server

Create a bounded static handler for `/web`.

Requirements:

- no path traversal;
- allowlist or strict normalized paths;
- correct MIME;
- gzip negotiation;
- chunked/bounded file reads;
- immutable caching for hashed assets;
- no-cache/revalidation for `index.html`;
- no access to `/data`.

### 9.2 Implement JSON helpers

Create bounded request readers and response writers.

Do not allocate based solely on an untrusted Content-Length. Enforce route
limits before allocation.

### 9.3 Implement middleware-style checks

Centralize:

- authentication;
- CSRF;
- Host/Origin;
- content type;
- body limit;
- error envelope;
- request ID where useful.

Avoid duplicating inconsistent security checks per route.

### 9.4 Implement status and limits APIs

Return exact subsystem state, version metadata, limits, and redacted diagnostics.

### 9.5 Implement set APIs

Implement list/create/read/update/delete/duplicate/select/export/import.

All mutable operations use expected revision.

### 9.6 Implement macro APIs

Implement set and global macro CRUD, validation, and ordering.

Validation returns the compiled summary but never executes.

### 9.7 Implement procedure APIs

Implement CRUD, ordering, progress, skip/reset semantics, and reference
validation.

### 9.8 Implement execution APIs

`POST /api/v1/executions` accepts:

```json
{
  "setId": "uuid",
  "macroId": "uuid",
  "macroRevision": 3,
  "sourceContext": {
    "procedureId": "uuid",
    "stepId": "uuid"
  }
}
```

It loads server-side source; the client does not submit arbitrary executable
source for execution.

Return `202` only after the executor owns a valid plan.

Implement polling through `GET /api/v1/executions/current`.

### 9.9 Implement settings, reset, backup, restore, and diagnostics

Destructive routes require reauthentication or typed confirmation where
specified. Factory reset requires physical confirmation or boot gesture.

### 9.10 Add API tests

Test all status codes, limits, stale revisions, partial-body reads, malformed
JSON, missing references, storage errors, USB errors, busy state, and redaction.

**Phase completion gate**

- Every documented route has tests.
- Every failure uses the error envelope.
- No route reports completion prematurely.
- Static handler cannot read user data.
- API code has zero warnings and no suppressions.

## 10. Phase 9 — Web application foundation

### 10.1 Create typed API client

Implement:

- shared success/error envelopes;
- session and CSRF management;
- request timeout;
- JSON content-type validation;
- typed domain errors;
- no hardcoded `192.168.4.1` API base;
- same-origin relative URLs.

Do not treat a non-2xx response as a successful empty response.

### 10.2 Add application state boundaries

Keep server state and ephemeral UI state separate. A small query/cache library
MAY be used if its size and offline behavior are acceptable and it is pinned.
Otherwise, implement focused hooks.

Do not add a large state framework without need.

### 10.3 Implement layout and navigation

Create:

- operational header;
- active-set selector;
- USB/storage state indicator;
- bottom navigation;
- global error presentation;
- offline/reconnect presentation;
- accessible dialogs and sheets.

### 10.4 Implement reusable controls

Required components include:

- set card;
- macro card;
- procedure card;
- step card;
- status badge;
- confirm dialog;
- typed-name delete dialog;
- reorder list with drag handle plus Move Up/Down/First/Last alternatives;
- macro source editor;
- directive insertion sheet;
- progress panel;
- error detail panel.

**Phase completion gate**

- TypeScript strict mode passes.
- ESLint, Prettier, and Stylelint report zero warnings.
- Keyboard and touch navigation work.
- No remote asset request occurs.

## 11. Phase 10 — Required web screens and workflows

### 11.1 First-run setup and login

Implement exact setup, password, validation, restart, login, throttling, and
session-expired behavior.

### 11.2 Set selector

Default to asking for a set on login.

Show:

- recents;
- all sets;
- search;
- device metadata;
- create/import/manage actions.

### 11.3 Procedures home and execution

Implement:

- procedure list;
- progress;
- current step;
- previous/next navigation;
- resend;
- skip confirmation;
- reset progress;
- no automatic next execution.

### 11.4 Instruction and checkpoint screens

Show clear manual steps and explicit completion. A checkpoint must ask the user
to confirm the expected result.

### 11.5 Procedure editor and ordering

Support:

- add/edit/delete steps;
- macro reference selection;
- manual instruction body;
- drag reorder;
- accessible movement commands;
- stale revision conflict.

### 11.6 Macro library and editor

Support:

- search;
- favorites;
- create/edit/duplicate/delete;
- set/global scope;
- ordering;
- live validation;
- source bytes;
- action count;
- estimated duration;
- directive insertion;
- unsupported-character location.

The Save button stays disabled for invalid source.

### 11.7 Send confirmation and execution progress

Confirmation shows:

- active set;
- macro;
- decoded actions;
- duration;
- USB state;
- procedure context;
- physical confirmation requirement.

Progress shows:

- action index;
- total;
- current action summary;
- large cancel button;
- completed/cancelled/failed result.

Never show success merely because the server returned `202`.

### 11.8 Set management

Implement:

- create;
- duplicate;
- rename;
- reorder;
- export;
- import as new;
- replace;
- delete;
- explicit conflict handling;
- storage-full errors.

### 11.9 Settings and diagnostics

Implement all specified settings, backup/restore, restart/reset, storage health,
redacted diagnostics, and clear fatal/degraded states.

**Phase completion gate**

- Every screen in `docs/SPEC.md` exists.
- Component and end-to-end tests cover primary workflows.
- The UI always shows the active set before sending.
- Send is disabled when USB is unavailable.
- All frontend quality checks pass with zero warnings.

## 12. Phase 11 — Import, export, backup, and recovery hardening

### 12.1 Define package schemas

Create versioned JSON schemas for:

- one macro set;
- all-data backup;
- diagnostic report.

Commit schemas under a real repository path and reference only those committed
files.

### 12.2 Implement export

Resolve shared macro dependencies into the set package without including device
secrets. Use bounded streaming if the package can approach the upload limit.

### 12.3 Implement import validation

Validate before mutation:

- format/version;
- package size;
- counts;
- IDs;
- revisions;
- strings;
- macro syntax;
- procedure references;
- keyboard layout;
- duplicate IDs;
- available storage;
- path safety.

### 12.4 Implement transactional replace

Stage the entire replacement, validate readback, move current data to backup,
activate staged data, update index, then clean backup.

Add interruption tests after each phase.

### 12.5 Implement backup and restore

Full backup includes all sets, global macros, procedures, and optional progress,
but excludes all secrets.

Restore is all-or-nothing.

**Phase completion gate**

- Import never leaves a partial active set.
- Replace rollback tests pass.
- Exports contain no secrets.
- Cross-device import produces an independent usable set.

## 13. Phase 12 — Security and failure review

Perform an explicit review for:

- unauthenticated routes;
- CSRF gaps;
- source execution bypass;
- path traversal;
- ID-to-path injection;
- integer overflow;
- unbounded allocation;
- task-stack overuse;
- race conditions;
- stale references;
- use-after-free in execution-plan ownership;
- ignored ESP-IDF errors;
- partial writes;
- unsafe retries;
- modifier-key retention;
- accidental open AP;
- secret logging;
- destructive automatic recovery;
- framework or dependency lint scope leakage.

Create regression tests for every finding. Do not merely document known defects.

**Completion gate**

- Every review finding is fixed and tested.
- There are no accepted first-party warnings.
- There are no silent fallback paths.
- All security invariants in `docs/SPEC.md` have direct tests or justified
  inspection evidence.

## 14. Phase 13 — Hardware-in-the-loop validation

### 14.1 USB hosts

Validate at least Linux and ChromeOS when available. Add Windows validation when
available.

Record:

- enumeration;
- reconnect;
- suspend/resume;
- typing;
- chords;
- cancellation;
- disconnect during execution.

### 14.2 Chromebook workflow dry run

Build representative sets for at least:

- HP Chromebook 11 G6 EE;
- one second Chromebook model or a generic test set.

Use harmless commands or a text editor during testing. Confirm set switching and
procedure ordering prevent cross-model confusion.

Do not ship model-specific commands as authoritative until independently
verified.

### 14.3 Persistence and power interruption

Test:

- power cycle after edits;
- power interruption during import;
- firmware slot switch;
- AP credential change;
- procedure progress;
- full user-data partition;
- corrupt object;
- mount failure.

### 14.4 Physical controls

Measure cancellation latency during a maximum delay and during rapid typing.
Verify credential reset and factory reset cannot be triggered accidentally.

**Completion gate**

- Hardware results are recorded in committed test documentation.
- Any failure becomes a tracked fix and regression test.
- User data survives the firmware update path.
- Release-all behavior is observed on the host.

## 15. Phase 14 — Release preparation

### 15.1 Final dependency pinning

Verify and commit:

- ESP-IDF exact tag documentation;
- managed-component lock;
- Node major pin;
- npm lockfile;
- tool versions or reproducible setup;
- build identifiers.

### 15.2 Size budgets

Record:

- firmware slot size and headroom;
- webfs image size and headroom;
- user-data minimum free-space requirements;
- static RAM;
- peak heap during import;
- task stack high-water marks.

Fail the build when budgets are exceeded.

### 15.3 Final quality gate

Run from a clean checkout:

```bash
./scripts/check-all.sh
```

Then perform a clean production build and flash.

The release is blocked by:

- any first-party warning;
- any lint or formatting finding;
- any failed test;
- any missing required screen;
- any untested storage transaction;
- any unsafe fallback;
- any secret leak;
- any known key-release failure;
- any mismatch with `docs/SPEC.md`.

### 15.4 Documentation synchronization

Update:

- build and flash instructions;
- provisioning instructions;
- user guide;
- macro language reference;
- API reference;
- import/export schema;
- recovery guide;
- LED/button behavior;
- test report;
- release notes.

Do not reference missing companion files.

## 16. Final definition of done

The project is done for version 0.1 only when every acceptance criterion in
Section 25 of `docs/SPEC.md` is demonstrated and every applicable task in this
document is checked off with evidence.

A task is not complete because code exists. It is complete only when:

- behavior is correct;
- failure behavior is correct;
- tests cover success and failure;
- first-party lint and compiler output is clean;
- no warning is suppressed;
- no third-party code was modified unnecessarily;
- documentation matches the implementation;
- the result is committed and reproducible from a clean checkout.
