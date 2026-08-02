# ESP32 Macro Keyboard — Product and System Specification

**Document status:** Authoritative implementation specification
**Product version:** 0.1
**Target hardware:** ESP32-S3R8 with native USB device wiring and octal PSRAM
**Firmware framework:** ESP-IDF v5.5.5, exact release tag
**Last updated:** 2026-08-02

## 1. Purpose

The ESP32 Macro Keyboard is a self-contained USB keyboard automation appliance.

The device connects to a target computer through the ESP32-S3 native USB
peripheral and enumerates as a standard USB HID keyboard. At the same time, the
device creates a password-protected Wi-Fi access point and serves a local web
application. An authenticated user selects a macro set, picks a macro from that
set's ordered list, and explicitly sends it as keyboard input to the target
computer.

The product is **generic**. It types text and key combinations at a computer on
request. It has no knowledge of what that computer is, what operating system it
runs, or what the macros are for.

Converting Chromebooks from ChromeOS to Debian is one thing a user might keep
macros for. It is an example, not a requirement, and no part of the firmware or
the web application may be built around it. Earlier revisions of this document
named it as the product's purpose and grew per-set `manufacturer`, `model`, and
`board` fields, "guided conversion procedures", and a shared macro library out of
that mistake. All of it is removed.

This specification is normative. `docs/TODO.md` defines the implementation
sequence for this specification.

### 1.1 Scope of this revision

This device is a microcontroller with 512 KiB of user storage. The 2026-08-02
revision removed features that were specified without ever being requested and
that the storage budget does not support. The following are **not part of this
product** and MUST NOT be reintroduced without a deliberate amendment:

- procedures, instruction steps, and checkpoint steps;
- per-procedure progress tracking;
- global or shared macros; a macro belongs to exactly one set;
- any field, screen, or code path specific to Chromebooks, ChromeOS, Debian, or
  any other particular target machine;
- buttons of any kind, and any hardware added to the board;
- quarantine or archival of damaged files;
- staging, trash, and transaction directories.

`docs/HANDOFF_2026-08-02_SIMPLIFICATION.md` records the reasoning and the
measurements behind these removals.

The implementation has not finished catching up with this revision: procedure,
progress, and transaction code is still present in the firmware and the web
application. This document states the required end state; `docs/TODO.md` tracks
the work to reach it. Where the two disagree, this document wins.

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
4. Let each macro set hold an ordered list of macros, and let the user reorder
   that list.
5. Operate with no buttons and no hardware added to the board: a bare devkit and
   a USB cable are a complete product.
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
- any awareness of what the target computer is: no host operating-system
  detection, no hardware detection, and no behavior conditional on either;
- automatic execution of the next macro in a set;
- guided procedures, instruction steps, checkpoint steps, or progress tracking
  (see §1.1);
- global or shared macros (see §1.1);
- unattended command chains triggered by boot, Wi-Fi connection, or USB
  connection;
- USB host functionality;
- Bluetooth HID;
- cloud accounts, cloud synchronization, or internet routing;
- station-mode Wi-Fi as a product feature (a development-only station-mode
  command exists on the physical serial console; see §26);
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

The hardware MUST NOT require any button, jumper, or other component to be added
to the board beyond what a stock devkit provides. See §19.

The reference module is the ESP32-S3R8, which carries 8 MB of embedded **octal**
SPI PSRAM. The build MUST enable it (`CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`,
`CONFIG_SPIRAM_USE_MALLOC`). Octal is not interchangeable with quad here: a build
configured for quad mode on this module does not boot. PSRAM raises the free heap
from roughly 200 KiB to roughly 8 MB, which is what makes whole-package backup
and restore bodies affordable. FreeRTOS task stacks MUST still come from internal
SRAM; PSRAM cannot host them.

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

A **macro set** is a name and an ordered list of macros. That is the entire
structure; there is no level between a set and a macro.

A set is the active workspace for whatever the user groups together. The device
attaches no meaning to that grouping:

```text
Build server login
├── Focus terminal
├── Type username
├── Type password
└── Start the build
```

Set order is user-controlled and meaningful: it is the order the user works
through. Firmware MUST preserve it exactly and MUST NOT reorder, sort, or
renumber on its own.

The user MUST explicitly select the active set. Firmware MUST NOT infer or
automatically switch the active set.

### 7.2 Macro

A **macro** is source text compiled into a bounded sequence of keyboard actions.

Every macro belongs to exactly one set. There is no shared or global macro
library, and a macro carries no `scope` field. If the user wants the same macro
in two sets, they duplicate the set or copy the text; on this device a duplicated
macro costs a few hundred bytes, while a second macro library cost 16 KiB of
empty directory metadata to save perhaps 3 KiB of duplication.

### 7.3 Active execution

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
2. Each card shows the set name, its macro count, and its last-used time.
3. The user explicitly selects a set.
4. The active set is visible in the application header on every operational
   page.
5. Switching sets cancels any pending, not-yet-started confirmation request.
6. Switching is prohibited while a macro is actively typing.

A setting MAY change startup behavior to open the last selected set.

### 8.4 Send a macro

1. The application shows the active set's macros in their stored order.
2. The user selects **Send** on a macro.
3. The application displays a decoded preview, duration estimate, active set,
   macro name, and current USB state.
4. The user focuses the target computer and explicitly selects **Send Now**.
5. If physical confirmation mode is enabled - it is off by default - the
   request waits for confirmation and expires after a bounded timeout.
   Confirmation is supplied by the `confirm` serial-console command; no button
   is required, and every confirmation-gated route MUST honour the setting
   rather than demanding confirmation unconditionally. That wait MUST NOT
   run on the HTTP server task: `esp_http_server` serves every socket from a
   single task, so waiting there makes the whole device unreachable for the
   duration of the window. Confirmation-gated requests are handed to a worker
   task (`web_server_async.c`), leaving the server responsive throughout.
   Because one confirmation command cannot disambiguate two pending requests, at
   most one such request is accepted at a time; a second concurrent one is
   rejected with `409` rather than queued.
6. Firmware starts execution only if USB is ready and the executor is idle.
7. Execution progress is displayed until the executor reports completed,
   cancelled, or failed.
8. The next macro in the list MUST NOT execute automatically. Advancing is a
   display convenience only; every send is a separate explicit user action.

The user may resend any macro at any time. The device stores no notion of a
macro being "done": there is no completion state, no skip state, and no progress
record. Which macros the user has already run is the user's business.

### 8.5 Manage sets

The user may:

- create an empty set;
- duplicate an existing set;
- rename a set;
- reorder sets;
- reorder the macros within a set;
- export one set;
- import a set as new or replace an existing set;
- delete a set;
- export all application data;
- restore a complete backup.

Merge import is not supported in version 0.1.

### 8.6 Delete set

Deletion MUST:

1. be rejected while the set has an active execution;
2. show exactly what will be removed;
3. require a deliberate confirmation, including typed set name for destructive
   deletion;
4. remove the set file and update the set index;
5. return the UI to set selection when the active set is deleted.

Deletion is permanent. There is no trash directory and no undelete: the device
does not have the storage to keep a copy of everything the user has thrown away
(see §13). The confirmation in step 3 is the safeguard.

### 8.7 Import and export

A set export MUST be a single versioned JSON package containing:

- package format identifier and version;
- the set name and identity;
- the set's macros, in order;
- keyboard-layout requirements;
- integrity metadata.

A set package is self-contained by construction: every macro it needs is inside
it, because a macro cannot live anywhere but in a set. Import therefore has no
external dependency to verify.

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
- replace an existing set;
- cancel.

Because a set is a single file, replacement is a single atomic `rename()` (§13.4)
and needs no staging area or transaction manifest.

## 9. Web application information architecture

The application MUST be mobile-first and usable from a desktop browser.

Required screens:

1. First-run setup
2. Login
3. Choose macro set
4. Macro list for the active set, in order, with reordering
5. Macro editor
6. Send confirmation
7. Execution progress and cancel
8. Completion, cancellation, and failure results
9. Manage macro sets
10. Create or duplicate macro set
11. Import macro set
12. Export macro set
13. Delete macro set confirmation
14. Settings
15. Storage diagnostics

The persistent operational header MUST show:

- device name;
- active macro set;
- USB state;
- access to set switching;
- access to settings.

The primary bottom navigation after set selection SHOULD be:

```text
Macros | Sets | Settings
```

### 9.1 Routing

The application SHOULD use hash routing:

```text
/#/sets
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
macro sets                       50
set file bytes               32 KiB
total user data bytes       480 KiB
import package bytes        512 KiB
```

Limits MUST be centralized, visible through the API, and tested at boundaries.
They MUST NOT be duplicated as inconsistent magic numbers.

The nominal per-set and total limits are far below the arithmetic product of the
per-object limits, and that is deliberate: 50 sets of 100 macros of 4096 bytes is
20 MB against a 512 KiB partition. Firmware MUST enforce the storage limits by
measuring actual serialized size before committing a write, not by trusting the
count limits, and MUST reject an over-budget write with `507` (§17) rather than
filling the filesystem. The web application SHOULD surface remaining space and
gate the user before the request is made.

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

Objects carry no field that the product does not use. A field that exists only
because it might be useful later is a defect on this device, not future-proofing.

### 12.1 Macro set

A set is a name and an ordered list of macros. Required fields:

```json
{
  "schema_version": 1,
  "id": "uuid",
  "revision": 1,
  "name": "Build server login",
  "macros": []
}
```

`macros` is the ordered list defined in §12.2. Array order **is** the user's
order; there is no separate order file and no `sort_order` field on a macro.

Earlier revisions specified `description`, `manufacturer`, `model`, `board`, and
`keyboard_layout` on a set. They are removed. `manufacturer`, `model`, and
`board` existed only because the specification mistook one user's Chromebook
workflow for the product (§1); `description` duplicates the name; and the layout
is a device-wide property in version 0.1 (§10.1), not a per-set one.

### 12.2 Macro

Required fields:

```json
{
  "schema_version": 1,
  "id": "uuid",
  "revision": 1,
  "name": "Start the build",
  "source": "make -j8{ENTER}",
  "key_press_ms": 8,
  "inter_key_ms": 15
}
```

A macro is stored inline in its set's `macros` array. It carries no `scope` and
no `set_id`: there is exactly one place a macro can be, and the file it is in
identifies the set. API responses and export packages MAY carry the owning set ID
as an envelope field, but it is not part of the object.

### 12.3 Set index

The index is the order of the sets themselves, plus which set is active:

```json
{
  "schema_version": 1,
  "revision": 1,
  "active_set_id": "uuid",
  "set_ids": ["uuid", "uuid"]
}
```

A set ID in the index with no corresponding set file, or a set file not named in
the index, is a corruption of the index and is handled under §13.6. Firmware MUST
NOT silently reconstruct the index from a directory listing, because doing so
discards the user's set order — the one thing the index exists to hold.

### 12.4 Objects that do not exist

There is no procedure object, no step object, and no progress object. There is
no `completed`, `skipped`, `current_step`, or `auto_complete_on_success` field
anywhere in the data model. See §1.1.

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

The current layout gives the web assets 1 MiB and user data **512 KiB**. Two OTA
application slots of 2.5 MiB each consume most of the 8 MiB part, so user storage
is not expandable without giving up OTA. Every storage decision in this section
follows from that 512 KiB figure.

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

The `userdata` partition is **512 KiB**. The layout MUST be flat: one index file
and one file per set.

```text
/data/
├── index.json              schema version, active set, set order
└── sets/
    └── <set-id>.json       set name and its ordered macros
```

This is the whole tree: two paths and one object type. There MUST NOT be a
per-set directory, a per-object file, a separate order file, a global or shared
macro store, a `staging/` directory, a `trash/` directory, a `transactions/`
directory, or a `quarantine/` directory.

The reason is measured, not stylistic. LittleFS on this device uses 4096-byte
blocks and represents each directory as a metadata pair, so **every directory
costs 8 KiB** whether or not it holds anything, while a file under roughly 512
bytes is inlined into its parent's metadata for free. A device observed on
2026-08-01 reported 98,304 bytes used — 24 blocks, all of it directory metadata
for 12 directories — while holding 1,370 bytes of actual user data. A layout with
a directory per set spends the partition on empty structure before the user has
stored anything.

One file per set rather than one file for everything: a write duplicates only the
set being edited instead of the whole repository, which is what makes roughly
480 KiB usable rather than roughly 252 KiB.

### 13.4 Atomic file update

Every update MUST:

1. serialize into a bounded buffer;
2. validate the serialized bytes;
3. reject the write if it would exceed the storage budget (§10.7), returning
   `507`;
4. write to `<target>.tmp` in the same directory;
5. flush, synchronize when supported, and close;
6. reopen and validate the temporary file;
7. `rename()` it over the target path.

POSIX `rename()` is atomic, so an interruption at any point leaves either the
complete old file or the complete new one. Nothing else is needed and nothing
else is permitted: there is no `.bak` file, no backup copy, and no second
generation of any object retained on the device.

Boot recovery is, in its entirety: delete any `*.tmp` file found under `/data`.
An interrupted write is indistinguishable from one that never started, which is
the correct outcome.

### 13.5 Multi-file operations

Restore is the only operation that writes more than one file. It is therefore
**not atomic across sets**, and MUST NOT pretend to be: each set file is written
atomically by §13.4 and the response reports per-set success or failure. A
partial restore leaves each individual set either fully old or fully new, and
tells the client exactly which are which.

Restore MUST NOT perform the whole rewrite synchronously on the HTTP server task.
`esp_http_server` serves every socket from one task, so a multi-second write loop
there starves the idle task and trips the task watchdog. Restore is handled on a
worker task (§8.4, `web_server_async.c`).

No code may:

- ignore a short write;
- ignore `fclose`, `fflush`, `fsync`, `rename`, or unlink errors;
- treat a missing file as an empty valid object unless absence is part of the
  defined initial state;
- replace malformed data with a default object;
- format the filesystem to recover from an ordinary parsing error.

### 13.6 Corruption handling

A file that fails to parse or validate is **deleted, and the failure is reported
to the caller** with the path and the error. It is not archived, not renamed into
a holding area, and not left in place to fail again on every subsequent read.

This is a deliberate trade against the general principle of preserving evidence.
On a 512 KiB partition, keeping a copy of every damaged object is a denial of
service against the user's own data, and the earlier quarantine mechanism cost
over a thousand lines to store what the user cannot act on anyway. The device
reports what was lost; it does not hoard it.

The error MUST name the object and MUST be surfaced through the API and the UI.
Deleting a corrupt file MUST NOT be reported as successful recovery.

`GET /api/v1/backup` (§17) is the mechanism for preserving data ahead of a
problem, and it is required to work even when individual objects are damaged.

### 13.7 Optimistic concurrency

Mutable API resources use a revision number. Update and delete requests include
the expected revision. A stale revision returns `409 Conflict` with the current
resource metadata. The server MUST NOT silently overwrite a newer edit.

## 14. NVS configuration

NVS stores only small device configuration, including:

- device name;
- AP SSID and credential material;
- station SSID and passphrase, when a network has been joined (§15.2);
- password verifier and salts;
- startup-set behavior;
- execution policy;
- bounded timing defaults;
- credential-reset state;
- schema version.

Macro sets, macros, imports, and web assets do not belong in NVS.

The administrator password MUST NOT be stored in plaintext, nor in any form from
which the password can be recovered. Use a per-password random salt and a
documented password-based key derivation function available through the
ESP-IDF/mbedTLS environment. Constant-time comparison is required.

Wi-Fi passphrases — both the AP's own and a stored station passphrase — are
necessarily recoverable, because the radio must be handed the passphrase itself
on every join. They are therefore stored as-is, and the guarantee that protects
them is confinement rather than hashing: firmware MUST NOT emit either
passphrase in a log line, an API response, a backup archive, or a diagnostics
report. A caller that needs an SSID MUST NOT be handed a copy of the whole
configuration record to pick it out of, because that record also carries the
password verifier, its salt, and the AP passphrase.

All configuration lives in a single fixed-size record with a fixed field layout.
A stored record whose length does not match the current layout MUST be rejected
as corrupt rather than parsed on a best-effort basis: a short read of a record
whose fields have moved yields plausible-looking garbage, and silently accepting
it is worse than refusing to start.

## 15. Wi-Fi

The device always operates its own access point (§15.1). It MAY additionally
join one existing network as a station (§15.2). The access point is the
guaranteed control path and does not depend on any external network.

### 15.1 Access point

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

### 15.2 Station mode

Station credentials are optional. With none stored, the device is access-point
only, and that is the defined initial state rather than a fault.

At most one network is remembered. Storing a network replaces the previous one;
firmware MUST NOT keep a list, and MUST NOT scan for, rank, or join any network
it was not explicitly given.

Credentials are set through the serial console (`wifi-connect <ssid>
<password>`). The console verifies the credentials by joining the network before
storing them, so a typo is reported rather than persisted. They are written to
the same provisioning record described in §14 that every other durable setting
uses — there is no second store and no separate file — and so they survive a
power cycle. An empty SSID clears the stored network.

At boot the access point is started first and unconditionally; the station join
is attempted only afterwards. This ordering is required. §15.1 makes access-point
availability a fatal-if-absent property, so it MUST NOT be made to wait on, or
depend on, an external network being present, in range, or still accepting the
stored passphrase.

A station join that fails, times out, or is refused MUST be logged and otherwise
ignored: the device continues as access-point only. Firmware MUST NOT treat it
as a startup failure, MUST NOT retry it in a way that delays or blocks the rest
of startup, and MUST NOT discard the stored credentials because one join
attempt failed — the network may simply be down.

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

### 16.5 Trust boundaries

The device has two distinct control surfaces, and they are deliberately held
to different standards.

**The network surface is untrusted.** Anything arriving over Wi-Fi - whether
the device's own SoftAP or, in development builds, a joined network - MUST
carry a valid RAM-only session, and every mutation MUST additionally carry a
matching CSRF token and accepted `Host` and `Origin` headers. Authentication
failures MUST be rate-limited. No network-reachable route may mutate device
state, read settings, or start an execution without satisfying all of these.
This is the boundary that protects the user from anyone else on the network.

**The physical serial surface is trusted.** Commands issued on the UART0
console (`firmware/components/serial_console`) require no session, no CSRF
token, and no physical confirmation: possession of the board and
access to its UART port *is* the authorization. This is a deliberate scope
decision, not an oversight. Reaching that port means holding the hardware,
at which point the device is already fully controllable - it can be
reflashed outright - so authenticating that surface would add friction
without adding protection.

The console MUST NOT expose credentials or secret material even so, because
the failure mode there is disclosure rather than control:
`scripts/check-credential-logging.sh` enforces that for all firmware
sources, console included.

The console is a development and bring-up interface. Before any release to
third parties it MUST be excluded from the shipped image, since a shipped
device's physical surface belongs to its user rather than its developer, and
`wifi-connect` would then let anyone with momentary physical access redirect
the device onto a network of their choosing. Until then it is present in
every build, and that is a documented product limitation rather than a
defect.

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

There are no procedure routes, no progress routes, and no `/api/v1/global/macros`
routes. `GET /api/v1/diagnostics/quarantine` existed in an earlier revision and
is removed (§13.6). Every macro is reached through its set.

The API implementation MAY consolidate routes where memory constraints justify
it, but external behavior and resource boundaries MUST remain equivalent and be
documented.

`GET /api/v1/backup` MUST NOT let one damaged object make the repository
unbackupable — a backup is most needed exactly when storage is damaged. Objects
that are individually unusable are omitted from the package rather than failing
the export.

A partial backup MUST be self-describing, so it can never be mistaken for a
complete one. It carries an optional top-level `skipped` object recording the
true total and enumerating up to `STORAGE_PACKAGE_SKIP_REPORT_MAX` of the
dropped objects. The field is written only when something was actually skipped:
a complete backup is byte-identical to one produced before this behavior
existed, and remains readable by anything predating the field.

Skipping applies only to per-object faults. A device-level failure (allocation,
I/O, storage unavailable, timeout) MUST still fail the export, because
continuing past it would silently drop objects that are perfectly good and
present a truncated backup as a whole one. A failed export names the object it
stopped on.

`POST /api/v1/restore` reports per-set outcomes (§13.5). A response reporting
partial success MUST enumerate which sets were restored and which were not; it
MUST NOT report `200` for a run that failed to write some of them.

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

The device MUST NOT require any button, and MUST NOT require hardware to be
added to the board. A stock ESP32-S3 devkit and a USB cable are a complete
product. GPIO assignment for the one remaining output MUST be configurable
through Kconfig and a board profile.

Required logical controls:

- a status indicator.

Earlier revisions of this document specified cancel and confirmation buttons and
a reset boot gesture; that was a mistake. No board used by this project exposes
them (the cancel button was assigned GPIO4, which a bare devkit does not break
out), the reset gesture was never implemented, and making six API routes demand
a press rendered the device unusable while concealing a crash in the code the
press was gating.

Those functions are serial-console commands instead: `confirm` supplies
physical confirmation when it is enabled, and `cancel` stops a running macro.
Cancellation MUST remain available during execution and delay actions, over
both the API (`POST /api/v1/executions/current/cancel`) and the console.

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
- stray temporary files removed at boot (§13.4);
- objects deleted as corrupt since boot, with their paths and errors (§13.6);
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
3a. No network-reachable route mutates state, reads settings, or starts an
    execution without a valid session, a matching CSRF token, and accepted
    `Host` and `Origin` headers. (The physical UART console is outside this
    invariant by design; see §16.5.)
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
15. No silent recovery that destroys user data. Deleting an unreadable object is
    permitted and required (§13.6), but only when the object, its path, and its
    error are reported to the caller and exposed in diagnostics. Discarding data
    without saying so is a defect.
16. No second on-device copy of user data. There is no trash, no quarantine, and
    no backup file; the only transient duplicate is the `.tmp` file of a write in
    progress (§13.4).

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
- full filesystem, and rejection of an over-budget write with `507`;
- interruption between writing `.tmp` and `rename()`, in both orders;
- boot cleanup of stray `.tmp` files;
- corrupt JSON, including that the corrupt file is deleted and the failure
  reported;
- an index naming a set file that is absent, and a set file the index omits;
- macro order preserved exactly across write, reboot, export, and restore;
- import as new;
- replace import;
- partial restore reporting per-set outcomes;
- no-format mount failure.

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
- set and macro ordering, including that a reorder round-trips through the API;
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
- a full set of macros sent in order against a harmless text target;
- cancellation over both the API and the `cancel` console command;
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
6. The user can reorder sets and can reorder the macros within a set, and that
   order survives reboot, export, and restore.
7. The user can create and edit valid macros and receives exact errors for
   invalid source.
8. The user can execute a macro only through explicit confirmation.
9. The device requires no button and no added hardware to perform any product
   function.
10. Cancellation works during text and delays.
11. Every tested terminal execution path releases all keys.
12. Power loss during a representative write preserves either the old or the new
    committed state, and boot removes any stray `.tmp` file.
13. A mount failure does not format storage.
14. Import failures do not partially modify active data.
15. The web application works without internet access and fits its partition.
16. No credentials or macro source leak through normal logs or diagnostic
    exports.
17. The tests in Section 24 pass.
18. Documentation matches implemented behavior.

## 26. Deferred features

Station mode was previously listed here. It is now implemented, but only as
a development/debugging facility reached over the physical serial console
(`firmware/components/serial_console`), not as a product feature: the
device still creates its own protected SoftAP for normal operation, station
credentials are not persisted across reboots, and no product screen exposes
joining an existing network. Promoting it to a product feature would still
need its own specification pass.

Potential later work includes:

- additional keyboard layouts;
- platform-specific Unicode entry;
- mDNS;
- web OTA management;
- encrypted backup packages;
- merge import;
- OLED display;
- Bluetooth HID;
- execution history;
- signed macro-set packages;
- role-based users;
- USB composite CDC plus HID;
- automated host-side HID conformance testing.

Deferred features MUST NOT be partially or silently enabled in version 0.1.

The items in §1.1 — procedures, instruction and checkpoint steps, progress
tracking, global macros, target-machine-specific behavior, buttons, added
hardware, quarantine — are **not** deferred. They are rejected. Reintroducing any
of them requires an amendment to §1.1 that states what changed about the storage
budget or the product to justify it.

The general rule behind §1.1: this is a generic device that types what it is told
to type. A feature that only makes sense for one user's particular task belongs
in that user's macro text, not in the firmware.

## 27. Handoff manifest

The authoritative design handoff currently consists of:

- `docs/SPEC.md`;
- `docs/TODO.md`;
- `docs/README.md`;
- `docs/HANDOFF_2026-08-02_SIMPLIFICATION.md`;
- `docs/mockups/README.md`.

Individual mockup SVG and PNG files are not currently part of the handoff and
MUST NOT be assumed to exist. Implement the pages from this specification until
those assets are added deliberately.

No implementation document may reference an assistant-created review report,
response file, schema, template, or companion document unless that exact file is
also committed at the referenced repository path.
