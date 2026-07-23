# ESP32 Macro Keyboard — Product and System Specification

**Document status:** Authoritative implementation specification
**Product version:** 0.1
**Target hardware:** ESP32-S3 with native USB device wiring
**Firmware framework:** ESP-IDF v5.5.5, exact release tag
**Last updated:** 2026-07-22

## 1. Purpose

The ESP32 Macro Keyboard is a self-contained USB keyboard automation appliance.

The device connects to a target computer through the ESP32-S3 native USB
peripheral and enumerates as a standard USB HID keyboard. At the same time, the
device creates a password-protected Wi-Fi access point and serves a local web
application. An authenticated user can select an ordered set of macros, follow a
guided procedure, and explicitly send the current macro as keyboard input to the
target computer.

The primary motivating workflow is converting multiple Chromebook makes and
models from ChromeOS to Debian. Each Chromebook model can have a distinct macro
set containing the commands and manual instructions required for that model.
The user explicitly selects the set for the current machine and proceeds through
the ordered steps.

This specification is normative. `docs/TODO.md` defines the implementation
sequence for this specification.

## 2. Normative language

The words **MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and
**MAY** have their usual requirements meaning.

When implementation behavior conflicts with this document, this document wins
unless it is deliberately amended in the same change.

## 3. Product goals

The product MUST:

1. Enumerate as a standards-compliant USB HID keyboard on common desktop
   operating systems without a custom host driver.
2. Provide a local, mobile-first web application over an ESP32-S3 SoftAP.
3. Let the user create, edit, duplicate, reorder, delete, import, and export
   macro sets.
4. Let each macro set contain ordered procedures and a macro library.
5. Let procedures contain both executable macro steps and non-executable manual
   instruction steps.
6. Require an explicit user action before every macro execution.
7. Show the active macro set, USB state, execution state, and errors clearly.
8. Stop safely and release every key after completion, cancellation, USB loss,
   timeout, or internal failure.
9. Preserve user data across resets and firmware updates.
10. Reject malformed or unsafe state rather than silently substituting defaults.
11. Treat every first-party compiler, type-checker, formatter, or linter warning
    and error as a defect.
12. Operate without internet access after firmware and web assets are installed.

## 4. Non-goals for version 0.1

Version 0.1 MUST NOT attempt to provide:

- arbitrary Unicode typing;
- automatic host operating-system detection;
- automatic Chromebook make, model, or board detection;
- automatic execution of the next procedure step;
- unattended command chains triggered by boot, Wi-Fi connection, or USB
  connection;
- USB host functionality;
- Bluetooth HID;
- cloud accounts, cloud synchronization, or internet routing;
- station-mode Wi-Fi;
- macro-set merge conflict resolution;
- server-side JavaScript, React Server Components, or Node.js on the device;
- TLS termination on the isolated SoftAP;
- automatic filesystem formatting after a mount or integrity failure.

These features require a later specification revision.

## 5. Authoritative platform and dependency policy

### 5.1 ESP-IDF

The firmware MUST build against the exact signed ESP-IDF tag:

```text
v5.5.5
```

The build MUST reject an unrecognized ESP-IDF version. Development documentation
and CI MUST clone ESP-IDF recursively from the exact tag rather than a moving
release branch.

Reference:

- <https://github.com/espressif/esp-idf/releases/tag/v5.5.5>
- <https://docs.espressif.com/projects/esp-idf/en/v5.5.5/>

### 5.2 Target

```text
IDF_TARGET=esp32s3
```

The hardware MUST expose the ESP32-S3 native USB D+ and D- signals. A board with
separate native-USB and USB-to-UART connectors is strongly preferred during
development.

### 5.3 Managed components

The initial managed-component targets are:

- `espressif/esp_tinyusb` for TinyUSB integration and USB HID support;
- `joltwallet/littlefs` for LittleFS VFS and image tooling.

All dependency resolutions MUST be pinned by committed manifest and lock files.
No production build may depend on an unbounded floating version. If a selected
component version is incompatible with ESP-IDF v5.5.5, the implementation MUST
select a compatible non-yanked release, document the decision, update the lock
file, and test it. It MUST NOT silently fall back to another filesystem or USB
stack.

LittleFS component reference:

- <https://components.espressif.com/components/joltwallet/littlefs>

### 5.4 Frontend toolchain

The frontend MUST use:

- TypeScript;
- React;
- Tailwind CSS;
- Vite;
- the browser Fetch API.

The Node.js major version MUST be pinned in the repository. JavaScript package
versions MUST be locked with a committed lockfile. Production assets MUST be
static files and MUST contain no CDN, remote-font, remote-icon, analytics, or
internet dependency.

## 6. Repository architecture

The intended repository layout is:

```text
.
├── docs/
│   ├── SPEC.md
│   ├── TODO.md
│   └── mockups/
├── firmware/
│   ├── CMakeLists.txt
│   ├── dependencies.lock
│   ├── partitions.csv
│   ├── sdkconfig.defaults
│   ├── main/
│   ├── components/
│   │   ├── app_core/
│   │   ├── auth/
│   │   ├── device_controls/
│   │   ├── macro_executor/
│   │   ├── macro_model/
│   │   ├── macro_parser/
│   │   ├── storage/
│   │   ├── usb_keyboard/
│   │   ├── web_server/
│   │   └── wifi_ap/
│   ├── managed_components/
│   ├── webfs/
│   └── test_app/
├── webapp/
│   ├── src/
│   ├── public/
│   ├── package.json
│   └── package-lock.json
├── scripts/
├── tests/
└── .github/workflows/
```

`managed_components/`, `node_modules/`, build trees, generated LittleFS images,
and Vite output are generated or third-party content and MUST NOT be linted as
first-party source.

## 7. Core terminology and hierarchy

### 7.1 Macro set

A **macro set** is the active workspace for one device model or purpose.

Examples:

- HP Chromebook 11 G6 EE;
- Dell Chromebook 11 3120;
- Acer Chromebook C720;
- Generic ChromeOS recovery.

The user MUST explicitly select the active set. Firmware MUST NOT infer or
automatically switch the active set.

### 7.2 Procedure

A **procedure** is an ordered workflow within a macro set. A procedure contains
one or more steps and stores independent progress.

Example:

```text
HP Chromebook 11 G6 EE
└── ChromeOS to Debian 13
    ├── Enter Developer Mode
    ├── Open Crosh
    ├── Enter shell
    ├── Download MrChromebox utility
    ├── Run firmware utility
    └── Install Debian
```

### 7.3 Procedure step

Version 0.1 supports:

- `macro`: references a set-owned or shared macro;
- `instruction`: displays manual instructions and requires explicit completion;
- `checkpoint`: asks the user to confirm an expected result before continuing.

A delay is a macro action, not a standalone procedure step.

### 7.4 Macro

A **macro** is source text compiled into a bounded sequence of keyboard actions.
Macros may be set-owned or shared globally.

### 7.5 Active execution

An **active execution** is the one macro currently being emitted over USB. Only
one execution may exist at a time.

## 8. End-to-end user workflows

### 8.1 First boot and setup

1. Firmware initializes NVS, mounts the read-only web-assets LittleFS partition,
   mounts the user-data LittleFS partition, initializes USB HID, and starts the
   SoftAP.
2. If no valid device configuration exists, firmware generates a random AP
   passphrase and a one-time setup code using the hardware random-number
   generator.
3. Development firmware prints the bootstrap credentials to the UART console.
   A production device requires an equivalent physical delivery mechanism, such
   as a label or QR code.
4. The device MUST NOT fall back to an open AP.
5. The user connects to the AP and opens the local web application.
6. The user supplies the setup code, sets the device name, AP passphrase, and web
   administrator password, and selects startup behavior.
7. The configuration is validated and committed transactionally.
8. The device restarts the AP when network credentials change.

A fixed development credential MAY exist only behind an explicit development
Kconfig option that is disabled in release builds.

### 8.2 Login

1. The user opens the device page.
2. The login page shows only minimal device and USB readiness information.
3. The user supplies the web password.
4. Failed authentication is rate-limited.
5. Successful authentication creates an in-memory session and an `HttpOnly`,
   `SameSite=Strict`, `Path=/` cookie.
6. Mutating requests require a per-session CSRF token and a valid same-origin
   request.

### 8.3 Select macro set

The default startup behavior is **always ask which macro set to use**.

1. The set selector lists recent and all sets.
2. Each card shows name, manufacturer, model, optional board identifier,
   procedure count, macro count, and last-used time.
3. The user explicitly selects a set.
4. The active set is visible in the application header on every operational
   page.
5. Switching sets saves progress and cancels any pending, not-yet-started
   confirmation request.
6. Switching is prohibited while a macro is actively typing.

A setting MAY change startup behavior to open the last selected set.

### 8.4 Run a procedure

1. The user selects a procedure in the active set.
2. The application opens at the first incomplete step.
3. The current step is expanded and future steps remain visible.
4. For a macro step, the user selects **Send**.
5. The application displays a decoded preview, duration estimate, active set,
   macro name, and current USB state.
6. The user focuses the target computer and explicitly selects **Send Now**.
7. If physical confirmation mode is enabled, the request waits for the device
   confirmation button and expires after a bounded timeout.
8. Firmware starts execution only if USB is ready and the executor is idle.
9. Progress is displayed until the executor reports completed, cancelled, or
   failed.
10. A successfully completed macro step may be marked complete automatically.
11. The next step becomes current but MUST NOT execute automatically.
12. Manual instruction and checkpoint steps require explicit completion.

The user may revisit, resend, skip with confirmation, or reset procedure
progress.

### 8.5 Direct macro execution

The user may open the active set's macro library and send a macro outside a
procedure. The same preview, confirmation, executor, progress, cancellation, and
failure rules apply.

### 8.6 Manage sets

The user may:

- create an empty set;
- duplicate an existing set without copying progress by default;
- rename a set;
- edit metadata;
- reorder sets;
- export one set;
- import a set as new or replace an existing set;
- delete a set;
- export all application data;
- restore a complete backup.

Merge import is not supported in version 0.1.

### 8.7 Delete set

Deletion MUST:

1. be rejected while the set has an active execution;
2. show exactly what will be removed;
3. require a deliberate confirmation, including typed set name for destructive
   deletion;
4. rename the set directory into `/data/trash/` as one transaction;
5. update the set index transactionally;
6. retain shared/global macros;
7. return the UI to set selection when the active set is deleted.

Permanent trash cleanup occurs only after successful transaction recovery or an
explicit cleanup operation.

### 8.8 Import and export

A set export MUST be a single versioned JSON package containing:

- package format identifier and version;
- set metadata;
- set-owned macros;
- procedures and ordered steps;
- resolved copies of referenced shared macros;
- keyboard-layout requirements;
- optional procedure progress;
- integrity metadata.

It MUST NOT contain:

- AP credentials;
- password verifiers;
- session tokens;
- setup codes;
- device keys;
- other device secrets.

Import MUST validate the entire package, all limits, references, syntax, schema,
and available space before modifying active data. The supported conflict choices
are:

- import as a new independent set;
- replace an existing set transactionally;
- cancel.

## 9. Web application information architecture

The application MUST be mobile-first and usable from a desktop browser.

Required screens:

1. First-run setup
2. Login
3. Choose macro set
4. Procedures home
5. Procedure execution
6. Manual instruction/checkpoint step
7. Edit procedure and reorder steps
8. Macro library
9. Macro editor
10. Send confirmation
11. Execution progress and cancel
12. Completion, cancellation, and failure results
13. Manage macro sets
14. Create or duplicate macro set
15. Import macro set
16. Export macro set
17. Delete macro set confirmation
18. Settings
19. Storage diagnostics

The persistent operational header MUST show:

- device name;
- active macro set;
- USB state;
- access to set switching;
- access to settings.

The primary bottom navigation after set selection SHOULD be:

```text
Procedures | Macros | Settings
```

### 9.1 Routing

The application SHOULD use hash routing:

```text
/#/sets
/#/procedures
/#/macros
/#/settings
```

Hash routing avoids server-side SPA fallback complexity.

### 9.2 Offline assets

All application assets MUST be bundled into the web-assets filesystem. The
application MUST NOT fetch remote resources.

### 9.3 Compression and caching

Vite output MUST use content-hashed filenames. JavaScript, CSS, SVG, and other
compressible assets SHOULD have pre-generated gzip variants.

The server MUST:

- stream files in bounded chunks;
- set correct content types;
- set `Content-Encoding: gzip` when serving a gzip variant;
- cache hashed assets as immutable;
- serve `index.html` with revalidation or no-cache behavior;
- reject path traversal;
- never expose files under the user-data mount through the static-file handler.

## 10. Macro language

### 10.1 Character support

Version 0.1 supports the US English keyboard layout and:

- printable ASCII characters `0x20` through `0x7E`;
- line feed, mapped to Enter;
- tab, mapped to Tab;
- directives described below.

Carriage-return/line-feed input is normalized to line feed. Other Unicode input
is rejected with an exact source position.

### 10.2 Escaping

Literal braces are represented as:

```text
{{  -> {
}}  -> }
```

An unmatched brace is an error.

### 10.3 Key directives

Supported named keys:

```text
{ENTER} {TAB} {ESC} {BACKSPACE} {DELETE}
{INSERT} {HOME} {END} {PAGEUP} {PAGEDOWN}
{UP} {DOWN} {LEFT} {RIGHT}
{SPACE}
{F1} through {F12}
```

### 10.4 Chords

Supported modifiers:

```text
CTRL ALT SHIFT GUI
```

Example chords:

```text
{CTRL+L}
{CTRL+SHIFT+T}
{ALT+F4}
{GUI+R}
```

Version 0.1 permits one non-modifier key plus one or more unique modifiers in a
chord. Duplicate modifiers, modifier-only chords, multiple ordinary keys, and
unknown names are validation errors.

### 10.5 Delay

```text
{DELAY:500}
```

The value is an integer number of milliseconds in the inclusive range 1 through
10,000.

### 10.6 Grammar rules

- Directive spelling is uppercase and canonical.
- Whitespace inside a directive is prohibited.
- Unknown directives are errors.
- The parser MUST consume the entire source.
- Parsing and compilation MUST complete before execution begins.
- Validation errors MUST include byte offset, line, column, error code, and a
  human-readable message.
- No partially parsed macro may execute.

### 10.7 Limits

Default hard limits:

```text
macro name UTF-8 bytes          64
macro source bytes            4096
compiled actions              4096
delay per directive        10,000 ms
estimated total duration        300 s
macros per set                  100
procedures per set               50
steps per procedure             200
macro sets                       50
import package bytes        512 KiB
```

Limits MUST be centralized, visible through the API, and tested at boundaries.
They MUST NOT be duplicated as inconsistent magic numbers.

## 11. USB HID keyboard subsystem

### 11.1 Device behavior

The ESP32-S3 MUST enumerate as a USB HID keyboard using the native USB device
peripheral and TinyUSB integration.

USB descriptors MUST use project-owned manufacturer, product, and serial strings.
The serial string SHOULD be derived from a stable device identifier without
revealing secrets.

Version 0.1 SHOULD implement a standard boot-keyboard-compatible report with
modifier bits and keyboard usages.

### 11.2 State model

The USB subsystem exposes:

```text
uninitialized
disconnected
enumerating
ready
suspended
error
```

Send requests are accepted only in `ready`.

### 11.3 Report safety invariant

After every normal key or chord action, firmware MUST emit a release-all report.

On:

- completion;
- cancellation;
- USB disconnect;
- USB suspend that exceeds the allowed timeout;
- executor timeout;
- parser invariant failure;
- task failure;
- queue failure;
- internal error;

firmware MUST attempt a release-all report and transition the execution to a
terminal state. The executor MUST also clear its internal pressed-key state even
when the transport cannot deliver the report.

### 11.4 Timing

Defaults:

```text
key press duration      8 ms
inter-key delay        15 ms
physical confirm       20 s timeout
execution watchdog      estimated duration plus bounded margin
```

Timing values are bounded configuration, not arbitrary user-controlled sleeps.

### 11.5 Concurrency

There is one macro-executor task. HTTP handlers MUST NOT type directly.

A send request:

1. authenticates and validates authorization;
2. verifies the revision and active set;
3. loads and validates the macro;
4. compiles it into an immutable in-memory execution plan;
5. verifies USB readiness and executor idleness;
6. atomically transfers ownership of the plan to the executor;
7. returns `202 Accepted` with an execution ID.

A second send request while busy returns `409 Conflict`; it is not silently
queued.

Cancellation MUST use a thread-safe flag, task notification, or equivalent
bounded mechanism and MUST remain responsive during delay actions.

## 12. Data model

All persistent objects MUST contain:

- `schema_version`;
- stable ID;
- revision number;
- creation timestamp or monotonic metadata where available;
- update timestamp or monotonic metadata where available.

Wall-clock timestamps are advisory because the device may not have a trusted
real-time clock. Revisions and IDs are authoritative.

IDs SHOULD be random UUID version 4 strings created from the hardware random
number generator.

### 12.1 Macro set

Required fields:

```json
{
  "schema_version": 1,
  "id": "uuid",
  "revision": 1,
  "name": "HP Chromebook 11 G6 EE",
  "description": "ChromeOS, MrChromebox, and Debian workflows",
  "manufacturer": "HP",
  "model": "Chromebook 11 G6 EE",
  "board": "SNAPPY",
  "keyboard_layout": "en-US",
  "sort_order": 10
}
```

### 12.2 Macro

Required fields:

```json
{
  "schema_version": 1,
  "id": "uuid",
  "revision": 1,
  "scope": "set",
  "set_id": "uuid",
  "name": "Download MrChromebox utility",
  "source": "cd; curl -LO mrchromebox.tech/firmware-util.sh{ENTER}",
  "favorite": false,
  "key_press_ms": 8,
  "inter_key_ms": 15
}
```

For a shared macro, `scope` is `global` and `set_id` is absent.

### 12.3 Procedure

Required fields:

```json
{
  "schema_version": 1,
  "id": "uuid",
  "revision": 1,
  "set_id": "uuid",
  "name": "ChromeOS to Debian 13",
  "description": "Guided conversion procedure",
  "steps": [
    {
      "id": "uuid",
      "type": "instruction",
      "title": "Enter Developer Mode",
      "body": "Follow the device-specific recovery sequence.",
      "required": true
    },
    {
      "id": "uuid",
      "type": "macro",
      "title": "Open Crosh",
      "macro_id": "uuid",
      "required": true,
      "auto_complete_on_success": true
    }
  ]
}
```

### 12.4 Progress

Progress is keyed by user-visible procedure ID and set ID and stores completed,
skipped, and current step IDs plus the procedure revision it applies to.
Procedure edits MUST reconcile progress explicitly; they MUST NOT silently mark
new steps complete.

## 13. Persistent storage

### 13.1 Partitioning

The reference build assumes at least 8 MiB flash and reserves:

- NVS;
- OTA metadata;
- application slot A;
- application slot B;
- read-only web-assets LittleFS;
- mutable user-data LittleFS;
- core dump storage.

Exact sizes are defined in `firmware/partitions.csv` and MUST be validated
against the selected module flash size. OTA-ready layout is required even though
the web-based OTA user experience is deferred.

### 13.2 Mount policy

The web-assets filesystem and user-data filesystem are separate mounts.

- Web assets are treated as immutable at runtime.
- User data is mutable.
- A mount failure is a visible fatal or degraded-storage state.
- Firmware MUST NOT automatically format either filesystem.
- Formatting user data is allowed only through an explicit physical or
  authenticated factory-reset/repair operation.
- Web-assets failure MUST NOT expose an unauthenticated fallback UI.

### 13.3 Logical user-data layout

```text
/data/
├── schema.json
├── set-index.json
├── sets/
│   └── <set-id>/
│       ├── set.json
│       ├── macro-order.json
│       ├── macros/
│       ├── procedures/
│       └── progress/
├── global/
│   ├── macro-order.json
│   └── macros/
├── staging/
├── trash/
└── quarantine/
```

### 13.4 Atomic file update

Every update MUST:

1. serialize into a bounded buffer or bounded stream;
2. validate the serialized object;
3. write to a unique file in the same filesystem;
4. flush, synchronize when supported, and close;
5. reopen and validate the temporary file;
6. atomically rename the old file to a backup or transaction location;
7. atomically rename the temporary file to its final path;
8. update indexes transactionally;
9. remove backup state only after the complete operation succeeds.

A failed write MUST leave the prior committed object intact.

### 13.5 Transactions

Multi-file operations such as set import, replacement, duplication, and deletion
MUST use staging plus an explicit transaction manifest. Startup recovery MUST
either complete or roll back an interrupted transaction deterministically.

No code may:

- ignore a short write;
- ignore `fclose`, `fflush`, `fsync`, `rename`, or unlink errors;
- treat a missing file as an empty valid object unless absence is part of the
  defined initial state;
- replace malformed data with a default object;
- format the filesystem to recover from an ordinary parsing error.

### 13.6 Corruption handling

An invalid object is moved to quarantine only after its original path and error
are recorded. The UI exposes the issue. Quarantine does not count as successful
recovery of the affected object.

The device MUST preserve evidence needed to diagnose corruption.

### 13.7 Optimistic concurrency

Mutable API resources use a revision number. Update and delete requests include
the expected revision. A stale revision returns `409 Conflict` with the current
resource metadata. The server MUST NOT silently overwrite a newer edit.

## 14. NVS configuration

NVS stores only small device configuration, including:

- device name;
- AP SSID and credential material;
- password verifier and salts;
- startup-set behavior;
- execution policy;
- bounded timing defaults;
- credential-reset state;
- schema version.

Macro sets, procedures, macros, progress, imports, and web assets do not belong
in NVS.

Passwords MUST NOT be stored in plaintext. Use a per-password random salt and a
documented password-based key derivation function available through the
ESP-IDF/mbedTLS environment. Constant-time comparison is required.

## 15. Wi-Fi access point

Version 0.1 operates as a SoftAP only.

Defaults:

```text
IPv4 address       192.168.4.1
internet routing   disabled
open AP fallback   prohibited
max clients        bounded and configurable
```

The SSID is derived from the configured device name plus a short non-secret
device suffix.

AP startup failure is a visible fatal network state. The firmware MUST NOT
silently continue as though the web application were available.

A captive-portal helper MAY redirect common connectivity-check requests to the
local application, but `192.168.4.1` remains the authoritative address.

## 16. Authentication and request security

### 16.1 Sessions

- Session identifiers are cryptographically random.
- Sessions exist in RAM only.
- Reboot invalidates every session.
- Default idle timeout is 30 minutes.
- Logout invalidates the session immediately.
- The session table is bounded.
- Session-table exhaustion returns an explicit error.

### 16.2 CSRF and origin checks

Every mutating request MUST provide a valid CSRF token tied to the session.
Requests with unexpected `Origin` or `Host` values are rejected. CORS is disabled
except for explicitly documented development builds.

### 16.3 Login throttling

Authentication failures are rate-limited by a bounded global and client-aware
policy. The implementation MUST avoid unbounded per-IP state.

### 16.4 Request limits

The HTTP server MUST enforce:

- route-specific body limits;
- header count and size limits where configurable;
- JSON nesting and collection limits;
- bounded parsing memory;
- request timeout;
- upload size limit;
- filename and ID validation;
- correct content type.

Malformed or oversized requests receive explicit 4xx responses.

## 17. HTTP API

All API routes are under:

```text
/api/v1
```

JSON responses use a consistent envelope.

Success:

```json
{
  "ok": true,
  "data": {}
}
```

Failure:

```json
{
  "ok": false,
  "error": {
    "code": "MACRO_SYNTAX_ERROR",
    "message": "Unknown directive",
    "details": {
      "line": 2,
      "column": 8
    }
  }
}
```

Required route groups:

```text
POST   /api/v1/setup
POST   /api/v1/auth/login
POST   /api/v1/auth/logout
GET    /api/v1/auth/session

GET    /api/v1/status
GET    /api/v1/limits

GET    /api/v1/sets
POST   /api/v1/sets
GET    /api/v1/sets/{set_id}
PUT    /api/v1/sets/{set_id}
DELETE /api/v1/sets/{set_id}
POST   /api/v1/sets/{set_id}/duplicate
POST   /api/v1/sets/{set_id}/select
GET    /api/v1/sets/{set_id}/export
POST   /api/v1/sets/import

GET    /api/v1/sets/{set_id}/macros
POST   /api/v1/sets/{set_id}/macros
GET    /api/v1/sets/{set_id}/macros/{macro_id}
PUT    /api/v1/sets/{set_id}/macros/{macro_id}
DELETE /api/v1/sets/{set_id}/macros/{macro_id}
POST   /api/v1/sets/{set_id}/macros/{macro_id}/validate
POST   /api/v1/sets/{set_id}/macros/reorder

GET    /api/v1/global/macros
POST   /api/v1/global/macros
PUT    /api/v1/global/macros/{macro_id}
DELETE /api/v1/global/macros/{macro_id}

GET    /api/v1/sets/{set_id}/procedures
POST   /api/v1/sets/{set_id}/procedures
GET    /api/v1/sets/{set_id}/procedures/{procedure_id}
PUT    /api/v1/sets/{set_id}/procedures/{procedure_id}
DELETE /api/v1/sets/{set_id}/procedures/{procedure_id}
POST   /api/v1/sets/{set_id}/procedures/reorder
PUT    /api/v1/sets/{set_id}/procedures/{procedure_id}/progress
DELETE /api/v1/sets/{set_id}/procedures/{procedure_id}/progress

POST   /api/v1/executions
GET    /api/v1/executions/current
POST   /api/v1/executions/{execution_id}/confirm
POST   /api/v1/executions/{execution_id}/cancel

GET    /api/v1/settings
PUT    /api/v1/settings
POST   /api/v1/settings/change-password
POST   /api/v1/device/restart
POST   /api/v1/device/reset-settings
POST   /api/v1/device/factory-reset

GET    /api/v1/diagnostics/storage
POST   /api/v1/diagnostics/storage/check
GET    /api/v1/backup
POST   /api/v1/restore
```

The API implementation MAY consolidate routes where memory constraints justify
it, but external behavior and resource boundaries MUST remain equivalent and be
documented.

Important status codes:

```text
200 OK                    completed synchronous request
201 Created               resource created
202 Accepted              execution accepted
400 Bad Request           malformed request
401 Unauthorized          login required or invalid
403 Forbidden             CSRF, origin, or policy failure
404 Not Found             resource absent
409 Conflict              busy, stale revision, or duplicate conflict
413 Content Too Large     body or package over limit
415 Unsupported Media     wrong content type
422 Unprocessable Content valid JSON but invalid resource
429 Too Many Requests     rate limit
500 Internal Error        unexpected internal failure
503 Service Unavailable   USB or storage unavailable
507 Insufficient Storage  LittleFS lacks required space
```

Acceptance of an execution is not completion. The UI reports success only after
the executor reaches `completed`.

## 18. Execution state machine

Required states:

```text
created
awaiting_physical_confirmation
running
cancelling
completed
cancelled
failed
expired
```

Terminal states are immutable.

Each execution record contains:

- execution ID;
- set ID;
- macro ID;
- macro revision;
- start policy;
- state;
- action index and total;
- characters or actions emitted;
- created/start/end monotonic timestamps;
- terminal error code and message;
- whether release-all was attempted;
- whether release-all was acknowledged while USB was ready.

Only a bounded current execution summary is retained in RAM. Persistent execution
history is deferred.

## 19. Physical controls and indicators

GPIO assignment MUST be configurable through Kconfig and a board profile.

Required logical controls:

- cancel button;
- confirmation button, which MAY share the cancel button through a deliberate
  short-press/long-press design;
- credential-reset/factory-reset boot gesture;
- status indicator.

The cancel action MUST remain available during execution and delay actions.

Suggested indicator meanings:

```text
steady ready color        USB and web service ready
slow pulse                waiting for physical confirmation
rapid activity pulse      typing
warning pattern           recoverable storage/network issue
error pattern             fatal initialization failure
```

Indicator semantics MUST be documented and testable. Failure LEDs do not replace
API and UART diagnostics.

## 20. Error handling and observability

### 20.1 No silent failures

Every operation MUST return, log, or expose an explicit success or failure.
Expected failures use stable error codes. Unexpected failures include source
context in logs without leaking secrets.

The project MUST NOT:

- swallow an `esp_err_t`;
- cast away or discard an error result;
- return success after partial completion;
- log an error and then continue in an invalid state;
- substitute empty data after parse failure;
- silently retry forever;
- silently downgrade authentication, storage, USB, or validation behavior;
- use a dangerous fallback merely to keep the application running.

### 20.2 Logging

Logs MUST:

- use component tags;
- identify state transitions;
- include object or execution IDs where safe;
- avoid passwords, tokens, raw cookie values, setup codes, and macro text that
  may contain secrets;
- use bounded formatting;
- distinguish user error, recoverable system error, and fatal invariant failure.

### 20.3 Diagnostics

The diagnostics page and API expose:

- firmware version and build identifier;
- ESP-IDF version;
- component versions;
- USB state;
- SoftAP state and client count;
- web-assets and user-data mount state;
- capacity, used space, and free space;
- object counts;
- quarantine, trash, staging, and pending transaction counts;
- last storage integrity result;
- executor state;
- reset reason.

A downloadable diagnostic report MUST redact secrets and macro source by
default.

## 21. First-party code-quality policy

### 21.1 Defect rule

Every warning or error produced by a configured compiler, type checker,
formatter, static analyzer, or linter for first-party source is a bug.

The defect MUST be fixed at its source. It MUST NOT be hidden, suppressed,
downgraded, excluded, redirected away, or ignored.

### 21.2 First-party scope

First-party scope includes:

```text
firmware/main/**
firmware/components/**
firmware/test_app/**
webapp/src/**
webapp/tests/**
scripts/**
tests/**
first-party CMake, JSON, YAML, CSS, Markdown, and configuration files
```

### 21.3 Third-party scope

The quality gate MUST exclude:

```text
$IDF_PATH/**
firmware/managed_components/**
third-party or vendored component directories
node_modules/**
build/**
dist/**
coverage/**
generated filesystem images
generated source explicitly identified as generated
```

The project MUST NOT modify ESP-IDF, managed components, npm dependencies, or
other third-party code merely to satisfy first-party linting.

If a diagnostic originates exclusively in a third-party header, the tool MUST be
scoped correctly to first-party code, such as through header filters or system
include classification. This is not permission to suppress a diagnostic in
first-party code.

### 21.4 Prohibited suppression

First-party source and project configuration MUST NOT use warning suppression as
a substitute for a fix, including:

```text
NOLINT or NOLINTNEXTLINE
eslint-disable comments
@ts-ignore
@ts-nocheck
#pragma GCC diagnostic ignored
-Wno-* for a first-party defect
-Wno-error=*
eslint --quiet
ignored linter exit status
|| true on a quality command
stderr redirection to hide diagnostics
excluding a failing first-party file
lowering a rule severity to make CI pass
```

A future exception requires an explicit amendment to this specification,
documented rationale, narrow scope, and a regression test. No implementation
agent may invent an exception.

### 21.5 Required checks

Firmware first-party checks:

- ESP-IDF build with warnings as errors for first-party component targets;
- `clang-format --dry-run --Werror`;
- `clang-tidy` with a first-party header filter;
- configured C/C++ static analysis;
- unit and integration tests.

Frontend checks:

- TypeScript strict type checking with no emit;
- ESLint with `--max-warnings=0`;
- Prettier check;
- Stylelint with zero warnings;
- unit/component tests;
- production Vite build.

Other first-party checks:

- ShellCheck and shfmt for shell;
- Ruff check and format check for Python, if Python exists;
- CMake formatting/linting;
- markdownlint;
- YAML and JSON validation/formatting.

A single command:

```text
./scripts/check-all.sh
```

MUST run the authoritative local quality gate. CI MUST call the same command.
The script MUST fail on the first failed phase or aggregate failures while still
returning nonzero; it MUST never mask failures.

## 22. Security invariants

1. No open AP fallback.
2. No unauthenticated macro execution.
3. No macro execution from a GET request.
4. No automatic execution triggered by connection or boot.
5. No plaintext password storage.
6. No credential or session material in exports or logs.
7. No static-file path traversal.
8. No user-controlled filesystem path.
9. No unbounded request, JSON, object, queue, or session allocation.
10. No partially validated import activation.
11. No execution of a partially parsed macro.
12. No stale-revision overwrite.
13. No retained modifier key after any terminal path.
14. No automatic formatting after mount failure.
15. No silent recovery that destroys evidence or user data.

## 23. Build and packaging pipeline

The build pipeline is:

```text
install pinned frontend dependencies
run frontend checks and tests
build Vite production assets
generate gzip variants
copy production assets into firmware/webfs
generate web-assets LittleFS image
run firmware checks and tests
build ESP-IDF application for esp32s3
generate flash manifest
```

The firmware build MUST fail when the expected web assets are absent, stale
relative to the frontend source, or over the partition budget.

The build MUST record:

- Git commit;
- dirty/clean state;
- ESP-IDF version;
- managed-component lock hash;
- frontend lockfile hash;
- build type;
- build timestamp where reproducibility policy permits.

Release builds MUST be reproducible from committed sources and lockfiles.

## 24. Testing requirements

### 24.1 Macro parser

Tests MUST cover:

- every supported ASCII character;
- shifted punctuation;
- every named key;
- every allowed modifier combination;
- brace escaping;
- newline and tab normalization;
- unknown directives;
- malformed chords;
- delay boundaries;
- source and action limits;
- exact error offsets;
- property/fuzz inputs;
- cancellation-safe compiled plans.

### 24.2 Storage

Tests MUST cover:

- create/read/update/delete;
- stale revisions;
- short writes;
- full filesystem;
- interrupted transaction stages;
- corrupt JSON;
- missing references;
- orphan files;
- quarantine;
- trash recovery;
- import as new;
- replace import rollback;
- no-format mount failure;
- migration success and rollback.

### 24.3 USB and executor

Tests MUST cover:

- descriptor enumeration;
- ASCII-to-HID mapping;
- press and release sequence;
- chords;
- delays;
- busy rejection;
- cancel during text;
- cancel during delay;
- disconnect and suspend;
- timeout;
- final release-all on every terminal path.

### 24.4 HTTP and security

Tests MUST cover:

- authentication and logout;
- rate limiting;
- session expiry;
- CSRF;
- host/origin validation;
- body and upload limits;
- invalid content type;
- path traversal;
- stale revisions;
- busy execution;
- redaction;
- import validation;
- explicit status codes.

### 24.5 Frontend

Tests MUST cover:

- every required screen;
- active-set visibility;
- set switching;
- macro/procedure ordering;
- live validation;
- send preview;
- disabled Send when USB is unavailable;
- progress polling;
- cancellation;
- import/export/delete confirmations;
- stale-edit conflict UI;
- storage error UI;
- keyboard and touch accessibility;
- responsive mobile layout.

### 24.6 Hardware acceptance

At minimum, acceptance testing MUST include:

- Linux host;
- ChromeOS host when available;
- Windows host when available;
- power-cycle persistence;
- repeated USB reconnects;
- repeated AP reconnects;
- full conversion procedure dry run using harmless text targets;
- physical cancel response;
- credential reset;
- factory reset;
- user-data preservation across firmware slot switch.

## 25. Version 0.1 acceptance criteria

Version 0.1 is complete only when:

1. A clean checkout builds with ESP-IDF v5.5.5 for `esp32s3`.
2. `./scripts/check-all.sh` exits successfully with zero first-party warnings and
   zero first-party lint errors.
3. The device enumerates as a USB keyboard.
4. The device starts a protected SoftAP without an open fallback.
5. An authenticated user can create, select, rename, duplicate, export, import,
   replace, and delete macro sets.
6. The user can reorder sets, procedures, macros, and procedure steps.
7. The user can create and edit valid macros and receives exact errors for
   invalid source.
8. The user can execute a macro only through explicit confirmation.
9. Procedure progress advances without automatically executing the next step.
10. Cancellation works during text and delays.
11. Every tested terminal execution path releases all keys.
12. Power loss during representative storage transactions preserves either the
    old or new committed state.
13. A mount failure does not format storage.
14. Import failures do not partially modify active data.
15. The web application works without internet access and fits its partition.
16. No credentials or macro source leak through normal logs or diagnostic
    exports.
17. The tests in Section 24 pass.
18. Documentation matches implemented behavior.

## 26. Deferred features

Potential later work includes:

- additional keyboard layouts;
- platform-specific Unicode entry;
- station mode;
- mDNS;
- web OTA management;
- encrypted backup packages;
- merge import;
- global macro conflict UI;
- OLED display;
- more physical macro buttons;
- Bluetooth HID;
- execution history;
- signed macro-set packages;
- role-based users;
- USB composite CDC plus HID;
- automated host-side HID conformance testing.

Deferred features MUST NOT be partially or silently enabled in version 0.1.

## 27. Handoff manifest

The authoritative design handoff currently consists of:

- `docs/SPEC.md`;
- `docs/TODO.md`;
- `docs/README.md`;
- `docs/mockups/README.md`.

Individual mockup SVG and PNG files are not currently part of the handoff and
MUST NOT be assumed to exist. Implement the pages from this specification until
those assets are added deliberately.

No implementation document may reference an assistant-created review report,
response file, schema, template, or companion document unless that exact file is
also committed at the referenced repository path.
