# ESP32 Macro Keyboard — Specification v2

**Document status:** Authoritative implementation specification  
**Product version:** 0.2 rebuild  
**Target hardware:** ESP32-S3R8, native USB wiring, octal PSRAM  
**Firmware framework:** ESP-IDF v5.5.5, exact release tag  
**Last updated:** 2026-08-03

## 0. Authority and scope

This document is the authoritative product, firmware, data, storage, API,
security, and validation specification for the ESP32 Macro Keyboard rebuild.

[`docs/UI_UX_SPEC_V2.md`](UI_UX_SPEC_V2.md) is the authoritative companion for
React screens, navigation, startup presentation, responsive behavior,
accessibility, and detailed user workflows. The two documents form one v2
specification set and MUST be implemented together. Neither document may be used
to contradict the other.

`docs/SPEC.md` and every earlier specification are retired. They may be read only
as historical records in git history. They MUST NOT be used to infer product
requirements, implementation behavior, API contracts, data structures, or test
expectations.

The organizing decision for v2 is that the ESP32 does not own the application
data model. Firmware owns keyboard execution, device configuration, and opaque
file persistence. React owns packages, macros, ordering, validation,
serialization, compression, import, export, and the in-memory repository working
copy.

Package selection is not repository content. React interprets the selection, and
firmware persists only an opaque last-selected-package UUID in device settings.
Changing the selected package does not change repository data and does not require
a snapshot.

Future changes to either authoritative v2 specification require the product
owner's explicit permission. Implementation work MUST NOT silently amend the
specification to match existing code.

---

## 1. Purpose

The ESP32 Macro Keyboard is a local USB keyboard automation appliance.

It connects to a target computer through the ESP32-S3 native USB peripheral and
enumerates as a standard HID keyboard. It also runs a protected Wi-Fi access
point and serves a static React application. The user chooses a macro in that
application and explicitly sends it. The ESP32 compiles the supplied macro source
and emits the resulting USB keyboard reports.

The product is generic. It has no knowledge of the target computer, its operating
system, or the purpose of a macro.

### 1.1 Primary responsibilities

The ESP32 has two primary product responsibilities:

1. Act as a standards-compliant USB HID keyboard and type one macro explicitly
   supplied by React.
2. Persist opaque, compressed repository snapshots so package and macro data can
   survive browser closure and device power loss.

The ESP32 also provides the supporting services required to perform those jobs:
Wi-Fi, authentication, provisioning, device settings, static asset serving,
health reporting, diagnostics, and lifecycle management.

### 1.2 Division of labor

Firmware owns:

- USB HID enumeration and reports;
- the C macro parser and compiler used immediately before execution;
- one bounded executor and cancellation path;
- opaque blob storage and retrieval;
- Wi-Fi access-point and optional station operation;
- authentication and first-run provisioning;
- device configuration and UI preferences in NVS;
- static web-asset serving;
- status, limits, and diagnostics.

React owns:

- the repository schema;
- packages and macros;
- package and macro ordering;
- interpreting and changing the selected package;
- live macro-language validation;
- the in-memory working copy and dirty-state comparison;
- JSON serialization and validation;
- gzip compression and decompression;
- repository import and export;
- deliberate snapshot creation, loading, downloading, and deletion;
- choosing which stored snapshot to load;
- sending macro source and timing to firmware.

Firmware MUST NOT parse, decompress, validate, index, reorder, merge, or otherwise
interpret repository contents. It MUST NOT contain a package repository, macro
repository, package index, per-package files, per-macro files, or package and
macro CRUD routes.

The persisted `lastSelectedPackageId` device setting is an opaque UUID value.
Firmware validates only its wire and storage representation. It does not verify
that the UUID identifies a package in any blob.

---

## 2. Normative language

**MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and **MAY** have
their usual requirements meaning.

Where implemented behavior conflicts with this specification set, the
specification wins.

---

## 3. Product goals

The product MUST:

1. Enumerate as a standards-compliant USB HID keyboard on common desktop
   operating systems without a custom driver.
2. Provide a local, mobile-first application over its own protected access point.
3. Persist complete repository snapshots across power loss and firmware updates.
4. Require an explicit user action before every macro execution.
5. Support one-tap Quick Send from the Macros page without mandatory navigation
   to a separate confirmation screen.
6. Compile the complete macro before emitting its first keyboard report.
7. Release every key after completion, cancellation, USB loss, timeout, or an
   internal failure.
8. Operate with no added buttons or hardware beyond a stock supported devkit and
   USB cable.
9. Operate without internet access.
10. Report failures explicitly rather than silently substituting defaults.
11. Preserve unsaved repository work in memory whenever technically possible and
    clearly identify when it remains unsaved.
12. Treat every first-party compiler, type-checker, formatter, linter, or static
    analyzer warning as a defect.

## 4. Non-goals for v0.2

Version 0.2 does not provide:

- arbitrary Unicode typing;
- target-computer or operating-system detection;
- automatic macro execution on boot, USB connection, Wi-Fi connection, or after
  another macro;
- procedures, workflow steps, checkpoints, or progress tracking;
- a global or shared macro library;
- USB host support;
- Bluetooth HID;
- cloud accounts, cloud synchronization, or internet routing;
- TLS termination on the isolated access point;
- server-side JavaScript, React Server Components, or Node.js on the device;
- repository merge or multi-user conflict resolution;
- repository autosave;
- automatic snapshot creation or deletion;
- automatic filesystem formatting after a mount or integrity failure;
- firmware-side repository schema validation;
- firmware-side compression or decompression;
- firmware-side repository checksums, hashes, or CRC records.

---

## 5. Platform and toolchain

### 5.1 Firmware

- ESP-IDF exact signed tag `v5.5.5`.
- Target `esp32s3`.
- The build MUST reject an unrecognized ESP-IDF version.
- Dependencies MUST be pinned by committed manifest and lock files.

### 5.2 Web application

- Node.js exactly `v24.18.0`.
- TypeScript, React, Tailwind CSS, Vite, and the browser Fetch API.
- Package versions MUST be exact and locked in the committed npm lock file.
- Production assets MUST be static and contain no CDN, remote-font, remote-icon,
  analytics, or other internet dependency.

### 5.3 Hardware

The reference module is the ESP32-S3R8 with 8 MB embedded octal PSRAM. The build
MUST enable `CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`, and
`CONFIG_SPIRAM_USE_MALLOC`. A quad-PSRAM build is not interchangeable with the
reference hardware.

FreeRTOS task stacks MUST remain in internal SRAM.

The board MUST expose the native USB D+/D- signals. No external button, jumper,
display, or other added component is required.

### 5.4 No wall clock

The device has no trusted wall clock, RTC synchronization, or SNTP service.
Firmware MUST NOT create, require, or report wall-clock timestamps.

A browser MAY display or export a date obtained from its own clock, but dates are
not part of repository schema version 1 and MUST NOT be used as device ordering,
identity, or concurrency tokens.

### 5.5 Partitions

The reference partition layout is:

```text
nvs        NVS         24 KiB
nvs_keys   NVS keys     4 KiB   encrypted
otadata    OTA data     8 KiB
phy_init   PHY          4 KiB
ota_0      app        2.5 MiB
ota_1      app        2.5 MiB
webfs      LittleFS     1 MiB   static web application assets
userdata   LittleFS   512 KiB   compressed repository blobs
coredump   coredump    64 KiB
```

Exact sizes are defined by the committed partition table and MUST be validated
against the selected flash device.

---

## 6. Terminology

### 6.1 Repository

A **repository** is decompressed JSON application data owned by React. It contains
only ordered packages and their ordered macros.

Package selection, UI preferences, credentials, sessions, and device settings are
not repository content.

### 6.2 Repository blob

A **blob** is the gzip-compressed UTF-8 serialization of one complete repository
snapshot. Firmware stores it as opaque bytes.

A blob is not a firmware object model. The device does not know whether its bytes
contain valid gzip, JSON, packages, or macros.

### 6.3 Package

A **package** is a name and an ordered list of macros. Array order is the user's
order. There is no intermediate hierarchy and no shared macro library.

### 6.4 Selected package

The **selected package** is the package currently shown by React. Its ID is stored
separately as the device-wide `lastSelectedPackageId` preference.

Selecting a package does not modify repository JSON and does not make the working
copy dirty.

### 6.5 Macro

A **macro** is a name, macro source, and execution timing. React stores it inside
exactly one package. Firmware receives only its source and timing when the user
sends it.

### 6.6 Snapshot

A **snapshot** is one deliberately created repository blob. Snapshot creation,
loading, downloading, and deletion are explicit user actions. A configured
retention target is advisory only.

### 6.7 Send

A **send** is the single current or most recent attempt to compile and emit one
macro over USB. Only one non-terminal send may exist at a time.

---

## 7. USB HID and macro execution

### 7.1 USB identity

The device enumerates with the project's USB identity:

```text
VID:PID       303a:4001
manufacturer  ESP32 Macro Keyboard Project
product       ESP32 Macro Keyboard
serial        ESP32S3-MACRO-01
```

It MUST NOT ship with Espressif example product strings.

### 7.2 USB state

USB state is one of:

```text
uninitialized
disconnected
enumerating
ready
suspended
error
```

A send MUST NOT start unless USB is `ready`.

### 7.3 Report safety invariant

After every key or chord action, firmware MUST emit a release-all report.

On completion, cancellation, USB disconnect, prolonged USB suspension, executor
timeout, parser invariant failure, task failure, queue failure, or internal
error, firmware MUST attempt a release-all report and move the send to a terminal
state. The executor MUST clear its internal pressed-key state even when the
transport cannot deliver the report.

### 7.4 Concurrency

There is exactly one executor task. HTTP handlers MUST NOT type directly.

A second `POST /api/v1/send` while a send is awaiting confirmation or running
returns `409`. It is not queued.

Cancellation MUST remain responsive during ordinary typing and delay actions.

### 7.5 Character support

The macro language uses the US English keyboard layout and supports:

- printable ASCII `0x20` through `0x7E`;
- line feed, mapped to Enter;
- tab, mapped to Tab;
- the directives below.

CRLF is normalized to LF. Other Unicode input is rejected with an exact source
position.

### 7.6 Escaping

```text
{{  ->  {
}}  ->  }
```

An unmatched brace is an error.

### 7.7 Key directives

```text
{ENTER} {TAB} {ESC} {BACKSPACE} {DELETE}
{INSERT} {HOME} {END} {PAGEUP} {PAGEDOWN}
{UP} {DOWN} {LEFT} {RIGHT}
{SPACE}
{F1} through {F12}
```

### 7.8 Chords

Modifiers are `CTRL`, `ALT`, `SHIFT`, and `GUI`.

```text
{CTRL+L}
{CTRL+SHIFT+T}
{ALT+F4}
{GUI+R}
```

A chord contains one non-modifier key plus one or more unique modifiers.
Duplicate modifiers, modifier-only chords, multiple ordinary keys, and unknown
names are errors.

### 7.9 Delay

```text
{DELAY:500}
```

The value is an integer number of milliseconds from 1 through 10,000 inclusive.

### 7.10 Grammar and compilation

- Directive spelling is uppercase and canonical.
- Whitespace inside a directive is prohibited.
- Unknown directives are errors.
- The parser MUST consume the entire source.
- Parsing and compilation MUST complete before execution starts.
- A partial parse MUST NOT execute.
- Errors MUST include an error code, byte offset, line, column, and readable
  message.

### 7.11 Execution limits

```text
package name UTF-8 bytes         64
macro name UTF-8 bytes           64
macro source UTF-8 bytes       4096
compiled actions               4096
delay per directive         10,000 ms
key press duration           10,000 ms maximum
inter-key delay             10,000 ms maximum
estimated total duration      300,000 ms
absolute executor deadline    310,000 ms
```

Defaults are 8 ms key press duration and 15 ms inter-key delay.

The absolute deadline includes a 10-second safety margin beyond the maximum
accepted estimate. A send reaching the absolute deadline fails, attempts
release-all, and becomes terminal.

Limits MUST be centralized, exposed by `GET /api/v1/limits`, and tested at their
boundaries.

### 7.12 Physical confirmation

When physical confirmation is disabled, an accepted send enters `running`.

When physical confirmation is enabled, an accepted send enters
`awaiting_confirmation` for at most 60 seconds. The serial-console `confirm`
command starts it. Expiry produces `timed_out` and types nothing.

### 7.13 Two parsers, one language

React validates the macro language in TypeScript for immediate editing feedback.
Firmware compiles the source again in C immediately before execution. The C
compiler remains authoritative for what is emitted over USB.

A checked-in conformance corpus MUST contain valid and invalid macro sources,
expected compiled actions, and expected errors. Both the C and TypeScript test
suites MUST execute the same corpus. Parser drift MUST fail CI.

---

## 8. React repository schema

### 8.1 Encoding

The repository is strict UTF-8 JSON. Before compression, React serializes it with
`JSON.stringify` using the exact field names in this section. Whitespace is not
significant, but newly saved snapshots SHOULD use compact JSON.

The root `schemaVersion` is the only schema-version field. Packages and macros do
not repeat it.

Version 0.2 reads and writes only schema version `1`. An unsupported version MUST
produce an explicit error and MUST NOT replace the current in-memory working
copy.

No v2 implementation was released under the earlier draft root shape. The schema
version 1 root defined here is therefore authoritative and contains no
`activePackageId` field.

### 8.2 Root object

```json
{
  "format": "esp32-macro-keyboard-repository",
  "schemaVersion": 1,
  "packages": []
}
```

Required root fields:

| Field | Type | Rule |
| --- | --- | --- |
| `format` | string | Exactly `esp32-macro-keyboard-repository` |
| `schemaVersion` | integer | Exactly `1` |
| `packages` | array | Ordered package list |

No other root fields are permitted in schema version 1.

An empty repository is valid and has `packages: []`.

### 8.3 Package object

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "name": "Build server login",
  "macros": []
}
```

Required package fields:

| Field | Type | Rule |
| --- | --- | --- |
| `id` | UUID string | Canonical lowercase UUID v4 |
| `name` | string | Non-empty after trimming; at most 64 UTF-8 bytes |
| `macros` | array | Ordered macro list |

No other package fields are permitted in schema version 1.

Package names need not be unique. Package IDs MUST be unique within the
repository.

### 8.4 Macro object

```json
{
  "id": "6ba7b810-9dad-41d1-80b4-00c04fd430c8",
  "name": "Start the build",
  "source": "make -j8{ENTER}",
  "keyPressMs": 8,
  "interKeyMs": 15
}
```

Required macro fields:

| Field | Type | Rule |
| --- | --- | --- |
| `id` | UUID string | Canonical lowercase UUID v4 |
| `name` | string | Non-empty after trimming; at most 64 UTF-8 bytes |
| `source` | string | At most 4096 UTF-8 bytes and valid under §7 |
| `keyPressMs` | integer | 0 through 10,000 inclusive |
| `interKeyMs` | integer | 0 through 10,000 inclusive |

No other macro fields are permitted in schema version 1.

A macro ID MUST be unique across the entire repository, not merely within its
package. A macro has no package ID because its containing package already defines
ownership.

Macro names need not be unique.

### 8.5 Repository invariants

Before React accepts a repository as a working copy or uploads it as a snapshot,
it MUST validate all of the following:

1. The JSON root, package objects, and macro objects contain exactly the fields
   defined above.
2. Every type and bound is correct.
3. Every package ID is a valid, unique canonical UUID v4.
4. Every macro ID is a valid, globally unique canonical UUID v4.
5. Every macro source passes the TypeScript implementation of §7.
6. Array order is preserved exactly.
7. No non-finite number, sparse array, prototype-bearing object, or value that
   cannot round-trip through JSON is admitted.

IDs are generated in the browser with `crypto.randomUUID()`.

Validation failure MUST leave the existing working copy unchanged and identify
the failing field or macro to the user.

### 8.6 Working copy and dirty state

After authentication, React loads one chosen blob, decompresses it, validates it,
and holds the repository as an in-memory working copy.

Repository data MUST NOT be persisted in `localStorage`, `sessionStorage`,
IndexedDB, Cache Storage, or a service worker cache. Firmware blobs are the
persistent store. Browser storage MAY hold unrelated presentation data, but MUST
NOT contain package IDs, macro IDs, package names, macro names, macro source,
repository JSON, or compressed repository bytes.

The repository becomes dirty after any package or macro create, edit, duplicate,
delete, move, reorder, or import operation. Selecting a package, sending a macro,
changing a UI preference, viewing a snapshot, downloading a snapshot, or deleting
an already stored snapshot does not dirty the repository.

The application MUST continuously expose `Saved` or `Unsaved changes`. A dirty
working copy is marked saved only after a complete snapshot upload succeeds with
`201 Created`, or after the user deliberately discards or replaces the working
copy.

A failed save MUST NOT discard or reset the working copy.

### 8.7 Unsaved-change protection

While the working copy is dirty:

- the UI exposes **Save snapshot** prominently;
- reload and tab-close warnings are registered where supported;
- sign-out, loading another snapshot, import replacement, reset settings, and
  factory reset require an in-app warning;
- context-appropriate choices include Cancel, Export working copy, Save snapshot,
  and Discard changes;
- the UI MUST NOT claim that a closed dirty working copy can be recovered.

A session expiry does not discard an in-memory working copy while the page remains
open. React reauthenticates and resumes the same working copy.

### 8.8 Import and export

Repository export writes the same gzip representation used for device storage.
The recommended filename suffix is:

```text
.emk-repository.json.gz
```

The MIME type is `application/gzip`.

Import:

1. reads the selected file as bytes;
2. gzip-decompresses it;
3. decodes UTF-8;
4. parses JSON;
5. validates the complete schema and macro language;
6. shows package and macro counts before replacing the in-memory working copy;
7. marks the resulting working copy dirty.

Import does not automatically upload. The user explicitly saves a snapshot after
reviewing the imported repository.

Exports MUST NOT contain access-point credentials, station credentials,
administrator password material, sessions, setup codes, device keys, UI
preferences, selected-package state, or diagnostics.

---

## 9. Compression

React is the only component that compresses or decompresses repository data.

The required format is **gzip**, produced and consumed with the browser-native
`CompressionStream("gzip")` and `DecompressionStream("gzip")` APIs.

React MUST feature-detect both APIs during startup. An unsupported browser MUST
receive an explicit compatibility error before repository editing is enabled.
The application MUST NOT silently store uncompressed JSON as a fallback.

Firmware MUST NOT:

- instantiate a compressor or decompressor;
- inspect gzip headers;
- verify gzip's internal checksum;
- add a repository CRC, hash, or digest;
- derive metadata from decompressed contents.

A decompression error is handled by React. It reports that the selected blob is
unreadable and lets the user choose another stored version. The device does not
automatically delete or replace the unreadable blob.

---

## 10. Blob storage

### 10.1 Layout

```text
/data/
└── repository/
    ├── 00000000000000000001.gz
    ├── 00000000000000000002.gz
    └── 00000000000000000003.gz
```

Each final file is one opaque blob. The fixed-width decimal filename is the blob
ID.

There is no repository index file, checksum file, metadata file, package file,
macro file, backup directory, staging directory, transaction directory, trash
directory, or quarantine directory.

Firmware MAY keep the next numeric blob ID in NVS as a convenience. At startup it
MUST also scan existing filenames and ensure the next ID is greater than every
stored ID, so an erased or stale counter cannot overwrite a blob.

### 10.2 Listing

Listing is generated from the repository directory. For each valid final
filename, firmware reports:

- blob ID;
- stored byte size.

The list is sorted by numeric ID, newest first. It contains no date, package
count, macro count, checksum, title, or active/current flag.

The device has no concept of a current blob. React chooses which one to load.

Stray names that do not match the final filename grammar MUST be reported in
diagnostics and MUST NOT appear as valid blobs.

### 10.3 Adding a blob

Adding a blob is atomic:

1. allocate the next blob ID;
2. stream the request body to `<id>.gz.tmp` in bounded chunks;
3. reject a short write or any write, flush, close, or synchronization failure;
4. synchronize the temporary file;
5. rename it to `<id>.gz`;
6. synchronize the directory when supported;
7. return `201` with the assigned ID and stored size.

The rename is the commit point. An interrupted add leaves existing blobs intact.
A `.tmp` file is not a blob and is removed during boot recovery.

Firmware checks only byte count and available space. It does not inspect the
payload.

### 10.4 Loading a blob

Loading streams the selected file's bytes exactly as stored with:

```text
Content-Type: application/gzip
```

Firmware MUST NOT decompress, transform, repair, or substitute another blob. A
read failure is returned explicitly.

### 10.5 Deleting a blob

Deletion removes exactly the blob named by the request. It MUST NOT delete any
other version and MUST NOT select a replacement.

The last blob may be deleted. A device with no blobs is a valid empty-storage
state; React may create a new empty repository in memory.

Deletion is always initiated by an explicit user action. Firmware and React MUST
NOT automatically delete snapshots.

### 10.6 No replace operation

There is no `PUT` and no atomic blob-replace operation.

When a user explicitly requests replacement of a particular blob, React performs
two independent operations:

1. delete the selected blob;
2. add the replacement as a new blob with a new ID.

If deletion succeeds and the subsequent add fails, the deleted blob remains
deleted. The UI MUST make this non-atomic consequence clear before beginning.
Normal snapshot saving does not replace a blob; it adds a new version.

### 10.7 Capacity and limits

The device exposes:

- total user-data bytes;
- used bytes;
- remaining bytes;
- maximum accepted blob bytes.

The maximum accepted blob size is **131,072 bytes (128 KiB)**. The userdata
partition and filesystem configuration MUST be proven to hold two maximum-sized
final blobs plus one maximum-sized temporary write, including filesystem
overhead. Failure of that real image and hardware proof requires reducing the
centralized limit before release; it does not permit weakening atomicity.

An upload over the declared body limit returns `413`. An upload within that limit
but impossible with current free space returns `507`. Either failure leaves all
final blobs unchanged.

### 10.8 Manual retention target

The device-wide `snapshotRetentionTarget` defaults to `5`. It is an advisory UI
preference, not a deletion rule.

When the blob count exceeds the target, React shows a non-blocking cleanup
indicator. Saving another snapshot still succeeds when space permits. The user
chooses which blobs, if any, to delete.

A target of `0` disables the cleanup indicator. Valid values are `0` through
`100` inclusive.

### 10.9 Mount and recovery policy

A mount failure MUST NOT format the partition. It produces a visible degraded or
failed storage state.

Boot recovery removes only `*.tmp` files created by interrupted blob adds. It
MUST NOT delete a final `.gz` blob because its contents are unreadable to React.

---

## 11. Device configuration

### 11.1 NVS contents

NVS stores small device configuration only:

- device name;
- access-point SSID and passphrase;
- at most one station SSID and passphrase;
- administrator password verifier, salt, algorithm version, and work factor;
- whether physical confirmation is required;
- provisioned flag and credential-record version;
- optional next-blob-ID counter;
- `sendMode`, default `quick`;
- `snapshotRetentionTarget`, default `5`;
- `showMacroSourcePreviews`, default `false`;
- `lastSelectedPackageId`, default `null`.

Repository JSON and blobs MUST NOT be stored in NVS.

UI preferences are device-wide. They are shared by every authenticated browser
and do not make the repository dirty.

### 11.2 Preference values

```text
sendMode                    quick | preview
snapshotRetentionTarget    integer 0..100
showMacroSourcePreviews     boolean
lastSelectedPackageId       canonical lowercase UUID v4 | null
```

`quick` means the primary Send control starts a send from the Macros page.
`preview` means the primary Send control first opens the optional Preview and
Send screen.

Macro source previews are hidden by default because source may contain passwords,
tokens, or private commands.

### 11.3 Credential handling

The administrator password MUST be 12 through 128 UTF-8 bytes and MUST NOT be
recoverable.

Use PBKDF2-HMAC-SHA-256 with a per-password cryptographically random salt and
constant-time verifier comparison. The iteration count is a versioned stored
parameter selected from committed ESP32-S3R8 benchmark evidence so derivation
takes at least 250 ms and no more than 500 ms under normal reference-hardware
conditions. The implementation MUST freeze one exact iteration count before the
v0.2 acceptance gate; it MUST NOT guess or silently vary the count at runtime.

Wi-Fi passphrases are necessarily recoverable by firmware but MUST NOT appear in
logs, API responses, blobs, exports, or diagnostics.

A stored configuration record with the wrong fixed length, unsupported version,
invalid enum, invalid UUID representation, or invalid credential parameters is
corrupt and MUST NOT be parsed on a best-effort basis.

### 11.4 Reset semantics

**Reset settings** restores non-credential behavior to these defaults:

```text
deviceName                  ESP32 Macro Keyboard
requireSerialConfirmation   false
sendMode                    quick
snapshotRetentionTarget     5
showMacroSourcePreviews     false
lastSelectedPackageId       null
station configuration       removed
```

Reset settings preserves:

- access-point SSID and passphrase;
- administrator password verifier;
- provisioning state;
- repository blobs;
- firmware and web assets.

**Factory reset** erases device configuration, credentials, sessions, and all
repository blobs, then reboots unprovisioned. It preserves firmware and web
assets.

---

## 12. Network, authentication, and setup

### 12.1 Wi-Fi

The device always runs its own protected access point and MAY also join one
explicitly configured station network.

The access point starts first and unconditionally. A station join failure is
logged and reported but MUST NOT prevent access-point operation, erase stored
credentials, or cause an unbounded retry loop.

At most one station network is remembered. Firmware MUST NOT scan for, rank, or
join a network the user did not explicitly configure.

SSID values are 1 through 32 UTF-8 bytes. WPA2 passphrases are 8 through 63 UTF-8
bytes. Empty credential strings are invalid.

### 12.2 Authentication and sessions

Successful login creates a bounded RAM-only session and an `HttpOnly`,
`SameSite=Strict`, `Path=/` cookie. The isolated HTTP appliance profile cannot
require the cookie `Secure` attribute because the device does not terminate TLS.

Session policy is:

```text
maximum concurrent sessions      8
session token entropy             32 random bytes
idle lifetime                     86,400 seconds
absolute lifetime                 604,800 seconds
```

Tokens are generated from the ESP32 cryptographic random source and encoded with
an unambiguous URL-safe representation. Raw tokens and token-derived secrets MUST
NOT be logged or stored in NVS.

Activity on an authenticated request refreshes the idle deadline but never the
absolute deadline. When a successful login would exceed eight sessions, firmware
invalidates the least-recently-used session before creating the new one.

Failed login policy is per source address:

- five failed attempts within a rolling 60-second window trigger lockout;
- lockout lasts 300 seconds;
- a successful login clears that source's failure window;
- rate-limit state is bounded in RAM and is not persisted.

CORS is disabled. There is no separate CSRF token. There is no Host/Origin check
in the development appliance profile. A product distributed to third parties
MUST revisit DNS-rebinding protection before release.

### 12.3 First-run setup

An unprovisioned device exposes only setup state and setup submission, plus the
static assets required for that UI.

Setup requires an eight-digit decimal one-time code shown on the serial console.
The code is regenerated on every unprovisioned boot, is valid until successful
setup or reboot, and is never returned over HTTP.

Setup sets:

- device name;
- access-point SSID and passphrase;
- administrator password;
- physical-confirmation preference.

Setup MUST preserve configuration fields it does not modify. It MUST NOT rebuild
a partial record and silently discard unrelated valid settings.

### 12.4 Serial console

The serial console is a trusted development surface. Possession of the board is
the authorization for `confirm`, `cancel`, and network-configuration commands.
It MUST NOT disclose credentials or secret material.

Before distribution to third parties, the interactive development console MUST
be excluded or redesigned for the shipped product.

---

## 13. HTTP API

### 13.1 General rules

All routes are under `/api/v1`.

Authenticated binary and JSON requests have bounded bodies. The maximum JSON
request body is **8192 bytes**. JSON routes accept only `application/json`; blob
uploads accept only `application/gzip`.

Malformed paths, unsupported methods, wrong content types, oversized bodies,
missing fields, unknown fields, invalid enum values, and wrong field types receive
explicit 4xx responses.

No route accepts a user-controlled filesystem path. Blob IDs are parsed as
bounded decimal identifiers and converted to fixed internal paths.

There is no optimistic concurrency token, revision field, `If-Match`, or checksum
round trip. Last successful settings update wins. Repository snapshots are
immutable blobs.

### 13.2 Error envelope

Every JSON error response uses:

```json
{
  "error": {
    "code": "invalid_field",
    "message": "Device name exceeds 32 UTF-8 bytes.",
    "field": "deviceName"
  }
}
```

`code` is a stable machine-readable identifier. `message` is human-readable.
`field` is omitted when no single request field caused the error.

Parser errors additionally include:

```json
{
  "error": {
    "code": "macro_parse_error",
    "message": "Unknown directive.",
    "field": "source",
    "byteOffset": 12,
    "line": 1,
    "column": 13
  }
}
```

### 13.3 Route table

```text
POST    /api/v1/setup

POST    /api/v1/auth/login
POST    /api/v1/auth/logout
GET     /api/v1/auth/session

GET     /api/v1/status
GET     /api/v1/limits

GET     /api/v1/blob
POST    /api/v1/blob
GET     /api/v1/blob/{blob_id}
DELETE  /api/v1/blob/{blob_id}

POST    /api/v1/send
GET     /api/v1/send
DELETE  /api/v1/send

GET     /api/v1/settings
PUT     /api/v1/settings
POST    /api/v1/settings/change-password

POST    /api/v1/device/restart
POST    /api/v1/device/reset-settings
POST    /api/v1/device/factory-reset

GET     /api/v1/diagnostics
```

There are no package routes, macro routes, validation routes, repository restore
routes, or plural `executions` resource.

### 13.4 Setup API

Request:

```json
{
  "setupCode": "12345678",
  "deviceName": "Desk Macro Keyboard",
  "apSsid": "MacroKeyboard",
  "apPassphrase": "example-passphrase",
  "adminPassword": "example-admin-password",
  "requireSerialConfirmation": false
}
```

On success firmware commits the complete configuration transactionally and
returns `202`:

```json
{
  "accepted": true,
  "restartRequired": true,
  "connectionWillClose": true,
  "reprovisioningRequired": false
}
```

The response never returns a setup code or credential. Setup on an already
provisioned device returns `409`.

### 13.5 Authentication API

Login request:

```json
{
  "adminPassword": "example-admin-password"
}
```

Successful login returns `200`, sets the session cookie, and returns:

```json
{
  "authenticated": true,
  "idleExpiresInSeconds": 86400,
  "absoluteExpiresInSeconds": 604800
}
```

`GET /api/v1/auth/session` returns the same sanitized object for a valid session
and `401` otherwise.

`POST /api/v1/auth/logout` invalidates the current session, clears the cookie,
and returns `204 No Content`.

### 13.6 Status API

`GET /api/v1/status` returns exactly this top-level shape:

```json
{
  "provisioned": true,
  "deviceName": "Desk Macro Keyboard",
  "firmwareVersion": "0.2.0",
  "buildId": "git-abcdef0",
  "uptimeMs": 123456,
  "usb": {
    "state": "ready"
  },
  "accessPoint": {
    "state": "running",
    "ssid": "MacroKeyboard",
    "clientCount": 1
  },
  "station": {
    "configured": false,
    "state": "disabled",
    "ssid": null,
    "ipv4": null
  },
  "storage": {
    "state": "ready",
    "totalBytes": 524288,
    "usedBytes": 4096,
    "remainingBytes": 520192,
    "blobCount": 3
  },
  "send": {
    "present": false,
    "state": null
  }
}
```

When a send exists, `send.state` contains one state from §13.10. Status does not
contain macro source, macro name, repository data, credentials, session data, or
wall-clock time.

### 13.7 Limits API

`GET /api/v1/limits` returns:

```json
{
  "packageNameMaxBytes": 64,
  "macroNameMaxBytes": 64,
  "macroSourceMaxBytes": 4096,
  "compiledActionsMax": 4096,
  "delayDirectiveMaxMs": 10000,
  "keyPressMaxMs": 10000,
  "interKeyMaxMs": 10000,
  "estimatedDurationMaxMs": 300000,
  "executorAbsoluteDeadlineMs": 310000,
  "jsonBodyMaxBytes": 8192,
  "blobMaxBytes": 131072,
  "adminPasswordMinBytes": 12,
  "adminPasswordMaxBytes": 128,
  "snapshotRetentionTargetMax": 100
}
```

### 13.8 Blob API

#### List

```http
GET /api/v1/blob
```

```json
{
  "blobs": [
    { "id": "3", "sizeBytes": 1332 },
    { "id": "2", "sizeBytes": 1298 }
  ],
  "usedBytes": 2630,
  "remainingBytes": 492000
}
```

#### Add

```http
POST /api/v1/blob
Content-Type: application/gzip
```

The body is the raw compressed repository bytes.

```json
{
  "id": "4",
  "sizeBytes": 1350
}
```

#### Load

```http
GET /api/v1/blob/4
```

The response body is raw `application/gzip` bytes.

#### Delete

```http
DELETE /api/v1/blob/4
```

A successful delete returns `204 No Content`.

### 13.9 Settings API

`GET /api/v1/settings` returns sanitized device settings:

```json
{
  "deviceName": "Desk Macro Keyboard",
  "requireSerialConfirmation": false,
  "sendMode": "quick",
  "snapshotRetentionTarget": 5,
  "showMacroSourcePreviews": false,
  "lastSelectedPackageId": null,
  "apSsid": "MacroKeyboard",
  "stationConfigured": false,
  "stationSsid": null
}
```

No password or password-presence hint is returned except
`stationConfigured`.

`PUT /api/v1/settings` is a strict partial update. Omitted fields preserve their
current values. Accepted fields are:

```json
{
  "deviceName": "Desk Macro Keyboard",
  "requireSerialConfirmation": false,
  "sendMode": "quick",
  "snapshotRetentionTarget": 5,
  "showMacroSourcePreviews": false,
  "lastSelectedPackageId": null,
  "accessPoint": {
    "ssid": "MacroKeyboard",
    "passphrase": "new-example-passphrase"
  },
  "station": {
    "ssid": "OfficeWiFi",
    "passphrase": "station-example-passphrase"
  }
}
```

Rules:

- unknown fields are rejected;
- an empty update object is rejected;
- `accessPoint`, when present, contains both `ssid` and `passphrase`;
- `station`, when present, is either a complete credential object or `null`;
- `station: null` removes the configured station network;
- omitted credential objects preserve existing credentials;
- empty credential strings are rejected;
- `lastSelectedPackageId` is a canonical lowercase UUID v4 or `null`;
- firmware does not verify that the UUID exists in repository data.

Success returns `200`:

```json
{
  "settings": {
    "deviceName": "Desk Macro Keyboard",
    "requireSerialConfirmation": false,
    "sendMode": "quick",
    "snapshotRetentionTarget": 5,
    "showMacroSourcePreviews": false,
    "lastSelectedPackageId": null,
    "apSsid": "MacroKeyboard",
    "stationConfigured": true,
    "stationSsid": "OfficeWiFi"
  },
  "restartRequired": false,
  "reconnectRequired": false
}
```

Changing access-point credentials sets both flags to `true`. Other fields do not
require a restart unless their implementation cannot safely apply them live; any
such case MUST be specified and contract-tested rather than inferred by React.

Password-change request:

```json
{
  "currentPassword": "old-example-password",
  "newPassword": "new-example-password"
}
```

A successful password change returns `204`, invalidates all sessions including
the current one, and clears the current cookie. An incorrect current password
returns `403`.

### 13.10 Send API

`POST /api/v1/send` is the only operation that starts keyboard output.

Request:

```json
{
  "source": "make -j8{ENTER}",
  "keyPressMs": 8,
  "interKeyMs": 15
}
```

Firmware validates bounds and compiles the complete source before accepting the
send. Invalid source returns `422` with the exact parser location. Nothing is
typed after a compile failure.

On acceptance, firmware returns `202`:

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "state": "running",
  "actionCount": 9,
  "estimatedDurationMs": 214
}
```

If physical confirmation is enabled, the initial state is
`awaiting_confirmation`. A second POST while one send is awaiting confirmation or
running returns `409`.

`GET /api/v1/send` returns the current or most recent send:

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "state": "completed",
  "actionIndex": 9,
  "actionCount": 9,
  "estimatedDurationMs": 214,
  "cancellationRequested": false,
  "error": "",
  "releaseError": ""
}
```

Send states are:

```text
awaiting_confirmation
running
completed
cancelled
failed
timed_out
```

When no send exists since boot, GET returns `404`.

`DELETE /api/v1/send` requests cancellation of the current non-terminal send. It
returns `202` when the request was recorded. Cancellation is idempotent while the
same send remains non-terminal.

### 13.11 React completion helper

A callback URL is not part of the wire protocol. A browser does not expose a
reliable local HTTP callback listener to firmware.

The React API layer SHOULD expose a helper shaped conceptually as:

```text
sendMacro(request, { onStatus, onComplete })
```

The helper:

1. calls `POST /api/v1/send`;
2. polls `GET /api/v1/send` at a bounded interval no slower than once per second;
3. invokes `onStatus` when state changes;
4. invokes `onComplete` exactly once after a terminal state is observed.

If the page closes, the JavaScript callback is not guaranteed. Firmware continues
the send independently, and reopening the application recovers its state with
`GET /api/v1/send`.

### 13.12 Device actions

`POST /api/v1/device/restart` returns `202`:

```json
{
  "accepted": true,
  "connectionWillClose": true,
  "reprovisioningRequired": false
}
```

`POST /api/v1/device/reset-settings` requires:

```json
{
  "confirmation": "RESET SETTINGS"
}
```

It applies §11.4, invalidates all sessions, schedules a reboot, and returns `202`:

```json
{
  "accepted": true,
  "connectionWillClose": true,
  "reprovisioningRequired": false,
  "repositoryBlobsPreserved": true
}
```

`POST /api/v1/device/factory-reset` requires:

```json
{
  "adminPassword": "example-admin-password",
  "confirmation": "FACTORY RESET"
}
```

It applies §11.4 and returns `202`:

```json
{
  "accepted": true,
  "connectionWillClose": true,
  "reprovisioningRequired": true,
  "repositoryBlobsPreserved": false
}
```

The destructive operation MUST NOT begin until the complete request, password,
and confirmation phrase have been validated.

### 13.13 Diagnostics API

`GET /api/v1/diagnostics` returns a fixed schema:

```json
{
  "firmwareVersion": "0.2.0",
  "buildId": "git-abcdef0",
  "resetReason": "power_on",
  "uptimeMs": 123456,
  "memory": {
    "freeHeapBytes": 200000,
    "minimumFreeHeapBytes": 180000,
    "largestFreeBlockBytes": 120000
  },
  "usb": {
    "state": "ready"
  },
  "wifi": {
    "accessPointState": "running",
    "stationState": "disabled"
  },
  "storage": {
    "state": "ready",
    "webfsTotalBytes": 1048576,
    "webfsUsedBytes": 500000,
    "userdataTotalBytes": 524288,
    "userdataUsedBytes": 4096,
    "blobCount": 3,
    "invalidNames": [],
    "temporaryFiles": []
  },
  "send": {
    "present": false,
    "state": null
  },
  "subsystems": []
}
```

The committed TypeScript and C contract definitions may add fields only through
an explicit specification update. Diagnostics MUST NOT include credentials,
session values, repository bytes, package or macro metadata, or macro source.

### 13.14 Status codes

```text
200  successful read or update
201  blob created
202  setup, send, cancellation, restart, or reset accepted
204  logout, password change, or blob deletion completed
400  malformed request
401  login required or invalid session
403  credential or policy failure
404  resource or send absent
409  already provisioned, send active, or state conflict
413  request body over declared limit
415  wrong content type
422  invalid field or macro source
429  rate limited
500  internal error
503  subsystem unavailable
507  insufficient blob storage
```

---

## 14. Web application

Detailed UI behavior is defined by
[`docs/UI_UX_SPEC_V2.md`](UI_UX_SPEC_V2.md). The requirements below are system
integration requirements.

### 14.1 Startup

After authentication and without a live in-memory working copy, React:

1. loads sanitized device settings;
2. lists stored blobs;
3. attempts to load the newest blob by numeric ID;
4. decompresses and validates it;
5. resolves `lastSelectedPackageId` against the loaded repository;
6. checks the current or most recent send.

Normal startup does not show the snapshot chooser merely because several blobs
exist.

If the newest blob is unreadable or invalid, React reports that exact failure and
shows snapshot recovery. It MUST NOT silently load an older blob or delete the
failed blob.

If `lastSelectedPackageId` identifies a package, React opens it. If the repository
contains exactly one package, React opens it and updates the device setting. When
several packages exist and no selection resolves, React shows the Package
Chooser.

### 14.2 First repository

When no blobs exist, React creates a valid empty repository in memory, asks for
the first package name, opens its empty Macros page, and continuously identifies
the working copy as unsaved until the user successfully creates a snapshot.

### 14.3 Editing and saving

Package and macro CRUD, duplication, ordering, and import occur only in the
in-memory repository.

React MUST NOT call firmware after every edit. Device repository writes occur
only when the user explicitly selects **Save snapshot**.

A Save snapshot action:

1. validates the complete repository;
2. serializes it as UTF-8 JSON;
3. gzip-compresses it in React;
4. checks compressed size against device limits;
5. uploads it with `POST /api/v1/blob`;
6. treats the working copy as saved only after `201 Created`.

There is no autosave and no automatic snapshot deletion.

### 14.4 Manual snapshot management

The Snapshots page is always available to an authenticated user. It supports:

- listing blobs;
- manually loading any selected blob;
- downloading a blob;
- manually deleting a blob;
- explicitly saving the current working copy as a new blob.

Loading a different snapshot replaces only the in-memory working copy. It does
not alter stored blobs. A dirty working copy requires an explicit save, export,
discard, or cancel decision first.

### 14.5 Quick Send

The Macros page is the primary operating console.

With `sendMode: quick`, pressing the primary Send control is the explicit user
action and immediately calls `POST /api/v1/send`. The user remains on the Macros
page while React displays inline progress, cancellation, completion, failure,
timeout, and release-error states.

With `sendMode: preview`, the primary Send control opens the Preview and Send
screen first. Preview remains available as an optional macro action in either
mode.

The application sends source and timing, not package ID, macro ID, revision, or
blob ID. The next macro MUST NOT execute automatically.

### 14.6 Macro-source privacy

Macro source is hidden on the Macros page by default. The user may reveal one row
temporarily or enable the device-wide preview preference.

Macro source is shown in the editor and optional preview screen but MUST NOT be
included in logs, diagnostics, send acknowledgements, notification text, or
hidden accessible names.

### 14.7 Portrait phones

Phone-sized touch displays are operationally portrait-only. In phone landscape,
React shows the portrait-required surface defined in the UI/UX specification.

An active send's progress and **Cancel and release all keys** control MUST remain
accessible from that surface. Tablets, laptops, and desktop displays remain
usable in landscape.

### 14.8 Static assets

All production assets are local. Vite output uses content-hashed filenames.
Compressible static assets SHOULD have pre-generated gzip variants. The static
server sets correct content types, streams in bounded chunks, caches hashed
assets as immutable, serves `index.html` with revalidation, rejects path
traversal, and never exposes the userdata mount.

---

## 15. Physical controls

No button is required.

Confirmation and cancellation are available through the serial-console `confirm`
and `cancel` commands. Network cancellation through `DELETE /api/v1/send`
remains available during typing and delays.

A logical status indicator MAY be provided by logs or the web UI. No unspecified
GPIO indicator is an acceptance requirement.

---

## 16. Error handling, logging, and diagnostics

### 16.1 No silent failures

Every operation returns, logs, or exposes an explicit result.

Code MUST NOT swallow an `esp_err_t`, discard an error, report success after a
partial operation, silently substitute empty data, retry forever without status,
silently weaken authentication, format storage as a fallback, or continue in an
invalid state merely to remain available.

### 16.2 Logging

Logs MUST NOT contain:

- passwords or passphrases;
- session tokens;
- setup codes;
- key material;
- repository bytes;
- decompressed repository JSON;
- package names or macro names;
- macro source;
- `lastSelectedPackageId`.

Macro source may itself contain credentials and is always treated as sensitive
user content.

### 16.3 Diagnostics

Diagnostics follow the fixed contract in §13.13 and report operational health,
not user repository content.

Invalid filenames and interrupted-add temporary files are reported without
reading blob contents.

---

## 17. Quality gates

`./scripts/check-all.sh` is the authoritative local gate and CI MUST invoke the
same command.

It MUST fail on the first failed phase and MUST never mask failures.

- No `|| true` failure hiding.
- No redirected analyzer errors.
- No warning suppression or first-party lint exclusions without an explicitly
  tracked exception.
- clang-tidy runs with warnings as errors.
- ESLint and stylelint run with zero allowed warnings.
- Defects are fixed at their source rather than suppressed.
- ESP-IDF, managed components, and npm dependencies are never modified in place.

Checks MUST prevent reintroduction of firmware package/macro persistence,
package/macro API routes, automatic snapshot deletion, repository
`activePackageId`, and mandatory standalone send-flow navigation.

---

## 18. Testing

### 18.1 Native C tests

Host tests with fakes cover:

- macro parsing and compilation;
- the shared conformance corpus;
- executor state, cancellation, confirmation timeout, absolute deadline, and
  release-all behavior;
- opaque blob list, add, load, delete, capacity, and boot cleanup;
- manual-only deletion and absence of retention cleanup in firmware;
- settings defaults, validation, partial update, NVS persistence, and reset
  semantics;
- opaque `lastSelectedPackageId` storage;
- authentication, session expiry, LRU eviction, and rate limiting;
- exact HTTP routes, methods, request and response contracts, content types, body
  limits, and error envelopes;
- startup sequencing;
- Wi-Fi and provisioning.

Tests for the retired package repository, package JSON parser, package imports,
package exports, package restore, macro CRUD, package ordering, and firmware-owned
active-package state are removed with those features.

### 18.2 Web tests

Vitest and real-browser tests cover:

- strict repository schema validation with no `activePackageId` root field;
- UUID uniqueness and exact-field invariants;
- TypeScript macro validation and the shared conformance corpus;
- gzip round trips through browser Compression Streams;
- import failure leaving the working copy unchanged;
- package and macro editing, ordering, and dirty-state transitions;
- package selection without dirtying the repository;
- first-repository unsaved warning;
- startup automatic newest-snapshot loading;
- invalid newest snapshot recovery without fallback;
- manual snapshot load, add, download, and delete workflows;
- advisory retention indicators without automatic deletion;
- save failure retaining the working copy;
- Quick Send remaining on the Macros page;
- optional preview mode;
- send polling, cancellation, and exactly-once completion callbacks;
- source-preview privacy defaults;
- reauthentication without discarding a live working copy;
- portrait-phone blocking with active-send cancellation available;
- tablet and desktop landscape behavior;
- offline assets and fixed API contract parsing.

### 18.3 Hardware-in-the-loop

Against an attached ESP32-S3 and host HID reports:

- printable text arrives exactly;
- a chord sets modifier and usage concurrently;
- every send ends with an all-zero report;
- cancellation works through the API and console during typing and delay;
- physical-confirmation timeout types nothing;
- the absolute executor deadline releases all keys;
- invalid source types nothing;
- one compressed blob is added, power is removed, and the same bytes load after
  reboot;
- multiple blobs list in numeric order;
- deleting one blob leaves every other blob byte-identical;
- no retention preference causes automatic deletion;
- interrupted add leaves no final partial blob and boot removes the temporary;
- two 128 KiB final blobs plus one 128 KiB temporary write fit and commit, or the
  centralized limit is reduced before acceptance;
- a full partition returns `507` without altering existing blobs;
- device settings and selected-package preference survive reboot;
- reset settings preserves blobs and credentials as specified;
- factory reset erases blobs and reprovisions as specified.

### 18.4 Password-derivation benchmark

Committed reference-hardware evidence records candidate PBKDF2 iteration counts,
median duration, worst observed duration, firmware build ID, and device identity.
The selected exact iteration count satisfies §11.3 and is covered by regression
and watchdog tests.

### 18.5 Host matrix

Linux is required. ChromeOS and Windows are required when test machines are
available. Current Chrome, Firefox, Safari, and Chromium-based mobile browsers
SHOULD be represented in web compatibility testing where practical.

---

## 19. Acceptance criteria for v0.2

1. A clean checkout builds with ESP-IDF v5.5.5 for `esp32s3`.
2. `./scripts/check-all.sh` exits zero with no first-party warnings.
3. The device enumerates with the project USB identity.
4. The protected access point starts with no open fallback.
5. React owns the only package and macro data model in production code.
6. Repository schema version 1 contains exactly `format`, `schemaVersion`, and
   `packages` at the root.
7. Firmware contains no package repository, macro repository, package index,
   package/macro CRUD route, repository parser, compressor, or decompressor.
8. `lastSelectedPackageId` is device UI state, is absent from snapshots, and
   package switching does not dirty the repository.
9. The documented schema round-trips through JSON and gzip without changing
   order or values.
10. A saved blob survives power loss and loads byte-identically.
11. Blob list, add, load, and delete behave exactly as §13.8.
12. Snapshot creation and deletion occur only after explicit user actions.
13. A retention target never causes automatic deletion.
14. An interrupted add leaves existing blobs intact and exposes no partial final
    blob.
15. Firmware stores no explicit checksum, CRC, hash, or digest for repository
    blobs.
16. React reports decompression and schema errors and lets the user deliberately
    choose a different blob without silent fallback.
17. An over-limit or out-of-space add fails without modifying final blobs.
18. `POST /api/v1/send` accepts source and timing and no package or macro ID.
19. Quick Send remains on the Macros page and optional preview mode works.
20. Invalid source returns an exact parser location and emits no key report.
21. Every terminal send path releases all keys, verified from HID reports.
22. `GET /api/v1/send` reports terminal completion and the React helper invokes
    its completion callback exactly once.
23. Cancellation works during typing and delay through the API and console.
24. Physical-confirmation timeout types nothing.
25. The absolute executor deadline is 310,000 ms and releases all keys.
26. Device settings use the exact sanitized contracts and defaults in this
    document.
27. Quick Send defaults on, retention target defaults to five, source previews
    default off, and package selection defaults to null.
28. Session, login-rate-limit, password-length, JSON-body, and blob limits match
    this document.
29. The exact PBKDF2 iteration count is frozen from committed ESP32-S3R8 evidence
    meeting the 250–500 ms target.
30. Reset settings preserves repository blobs and credentials; factory reset
    erases blobs and returns the device to unprovisioned state.
31. Unsaved repository changes remain visibly identified until a snapshot saves
    or the user deliberately discards them.
32. No repository data or macro source appears in browser persistent storage,
    logs, notifications, or diagnostics.
33. Phone landscape shows the portrait-required surface while active-send
    cancellation remains available.
34. A mount failure does not format storage.
35. The web application works without internet access and fits its partition.
36. The shared conformance corpus passes in C and TypeScript.
37. The tests in §18 pass.
38. This document and `docs/UI_UX_SPEC_V2.md` match implemented behavior.

---

## 20. Deferred

- additional keyboard layouts;
- platform-specific Unicode entry;
- mDNS;
- web-based OTA;
- encrypted or signed repository blobs;
- repository merge;
- multi-user editing;
- execution history beyond the current or most recent send;
- Server-Sent Events or WebSocket progress instead of polling;
- OLED or other display;
- Bluetooth HID.
