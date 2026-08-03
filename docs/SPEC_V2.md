# ESP32 Macro Keyboard — Specification v2

**Document status:** Authoritative implementation specification
**Product version:** 0.2 rebuild
**Target hardware:** ESP32-S3R8, native USB wiring, octal PSRAM
**Firmware framework:** ESP-IDF v5.5.5, exact release tag
**Last updated:** 2026-08-03

## 0. Authority and scope

This document is the sole authoritative product and system specification for the
ESP32 Macro Keyboard rebuild.

`docs/SPEC.md` and every earlier specification are retired. They may be read only
as historical records in git history. They MUST NOT be used to infer product
requirements, implementation behavior, API contracts, data structures, or test
expectations.

The organizing decision for v2 is that the ESP32 does not own the application
data model. The firmware owns keyboard execution and opaque file persistence.
The React application owns packages, macros, ordering, selection, validation,
serialization, compression, import, and export.

Future changes to this document require the product owner's explicit permission.
Implementation work MUST NOT silently amend the specification to match existing
code.

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

1. Act as a standards-compliant USB HID keyboard and type a macro supplied by the
   React application.
2. Persist opaque, compressed repository snapshots so the user's application
   state survives power loss.

The ESP32 also provides the supporting services required to perform those jobs:
Wi-Fi, authentication, provisioning, device settings, static asset serving,
health reporting, diagnostics, and lifecycle management.

### 1.2 Division of labor

The firmware owns:

- USB HID enumeration and reports;
- the C macro parser and compiler used immediately before execution;
- one bounded executor and cancellation path;
- opaque blob storage and retrieval;
- Wi-Fi access-point and optional station operation;
- authentication and first-run provisioning;
- small device configuration in NVS;
- static web-asset serving;
- status, limits, and diagnostics.

The React application owns:

- the repository schema;
- packages and macros;
- package and macro ordering;
- active package selection;
- live macro-language validation;
- the in-memory working copy;
- JSON serialization and validation;
- gzip compression and decompression;
- repository import and export;
- snapshot retention decisions;
- choosing which stored snapshot to load;
- sending macro source and timing to the ESP32.

Firmware MUST NOT parse, decompress, validate, index, reorder, merge, or otherwise
interpret repository contents. It MUST NOT contain a package repository, macro
repository, active-package record, package index, per-package files, per-macro
files, or package and macro CRUD routes.

---

## 2. Normative language

**MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and **MAY** have
their usual requirements meaning.

Where implemented behavior conflicts with this document, this document wins.

---

## 3. Product goals

The product MUST:

1. Enumerate as a standards-compliant USB HID keyboard on common desktop
   operating systems without a custom driver.
2. Provide a local, mobile-first application over its own protected access point.
3. Persist complete application-state snapshots across power loss and firmware
   updates.
4. Require an explicit user action before every macro execution.
5. Compile the complete macro before emitting its first keyboard report.
6. Release every key after completion, cancellation, USB loss, timeout, or an
   internal failure.
7. Operate with no added buttons or hardware beyond a stock supported devkit and
   USB cable.
8. Operate without internet access.
9. Report failures explicitly rather than silently substituting defaults.
10. Treat every first-party compiler, type-checker, formatter, linter, or static
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
- automatic filesystem formatting after a mount or integrity failure;
- firmware-side repository schema validation;
- firmware-side compression or decompression;
- firmware-side repository checksums or CRC records.

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
- Production assets MUST be static and contain no CDN, remote-font,
  remote-icon, analytics, or other internet dependency.

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
not part of the persisted v1 repository schema and MUST NOT be used as device
ordering or concurrency tokens.

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

A **repository** is the decompressed JSON application state owned by React. It
contains ordered packages, ordered macros, and the active package selection.

### 6.2 Repository blob

A **blob** is the gzip-compressed UTF-8 serialization of one complete repository
snapshot. The ESP32 stores it as opaque bytes.

A blob is not a firmware object model. The device does not know whether its bytes
contain valid gzip, JSON, packages, or macros.

### 6.3 Package

A **package** is a name and an ordered list of macros. Array order is the user's
order. There is no intermediate hierarchy and no shared macro library.

### 6.4 Macro

A **macro** is a name, macro source, and execution timing. React stores it inside
exactly one package. Firmware receives only its source and timing when the user
sends it.

### 6.5 Send

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

Cancellation MUST remain responsive during ordinary typing and during delay
actions.

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
macro name UTF-8 bytes          64
macro source UTF-8 bytes      4096
compiled actions              4096
delay per directive        10,000 ms
key press duration          10,000 ms maximum
inter-key delay            10,000 ms maximum
estimated total duration       300 s
```

Defaults are 8 ms key press duration and 15 ms inter-key delay.

Limits MUST be centralized, exposed by `GET /api/v1/limits`, and tested at their
boundaries.

### 7.12 Two parsers, one language

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

### 8.2 Root object

```json
{
  "format": "esp32-macro-keyboard-repository",
  "schemaVersion": 1,
  "activePackageId": "550e8400-e29b-41d4-a716-446655440000",
  "packages": []
}
```

Required root fields:

| Field | Type | Rule |
| --- | --- | --- |
| `format` | string | Exactly `esp32-macro-keyboard-repository` |
| `schemaVersion` | integer | Exactly `1` |
| `activePackageId` | UUID string or `null` | When non-null, identifies one package in `packages` |
| `packages` | array | Ordered package list |

No other root fields are permitted in schema version 1.

An empty repository is valid and has `activePackageId: null` and `packages: []`.
A repository with packages MAY still have `activePackageId: null` while the user
is at the package selector.

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
5. `activePackageId` is null or identifies an existing package.
6. Every macro source passes the TypeScript implementation of §7.
7. Array order is preserved exactly.
8. No non-finite number, sparse array, prototype-bearing object, or value that
   cannot round-trip through JSON is admitted.

IDs are generated in the browser with `crypto.randomUUID()`.

Validation failure MUST leave the existing working copy unchanged and identify
the failing field or macro to the user.

### 8.6 Working copy and saves

After login, React loads one chosen blob, decompresses it, validates it, and
holds the repository as an in-memory working copy.

Repository data MUST NOT be persisted in `localStorage`, `sessionStorage`,
IndexedDB, Cache Storage, or a service worker cache. The ESP32 blobs are the
persistent store. Browser storage MAY hold unrelated presentation preferences,
but MUST NOT contain package IDs, macro IDs, names, sources, repository JSON, or
compressed repository bytes.

Editing changes the in-memory working copy. A **Save snapshot** action:

1. validates the complete repository;
2. serializes it as UTF-8 JSON;
3. gzip-compresses it in React;
4. checks the compressed size against device limits;
5. uploads it with `POST /api/v1/blob`;
6. treats the save as complete only after the device returns `201`.

A failed save MUST NOT discard or reset the working copy.

### 8.7 Snapshot retention

The device never chooses a snapshot and never deletes one automatically.

React SHOULD retain the five newest successful snapshots by default. Retention
cleanup occurs only after a new snapshot has been stored successfully. The user
may keep more or fewer snapshots, subject to available storage.

If storage is insufficient, React MUST show the stored blobs and let the user
delete one or more deliberately. Firmware MUST NOT silently delete an older blob
to make space.

### 8.8 Import and export

Repository export writes the same gzip bytes used for device storage. The
recommended filename suffix is:

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
6. shows a summary before replacing the in-memory working copy.

Import does not automatically upload. The user explicitly saves a snapshot after
reviewing the imported repository.

Exports MUST NOT contain AP credentials, station credentials, administrator
password material, sessions, setup codes, device keys, or diagnostics.

---

## 9. Compression

React is the only component that compresses or decompresses repository data.

The required format is **gzip**, produced and consumed with the browser-native
`CompressionStream("gzip")` and `DecompressionStream("gzip")` APIs.

Reasons:

- gzip is a standard portable file format;
- the required browser APIs are available in current major browsers;
- no compression library needs to be shipped in the web partition;
- the result can be downloaded, inspected, and decompressed by ordinary desktop
  tools;
- the ESP32 remains unaware of both the compression algorithm and repository
  schema.

React MUST feature-detect both APIs during startup. An unsupported browser MUST
receive an explicit compatibility error before repository editing is enabled.
The application MUST NOT silently store uncompressed JSON as a fallback.

The firmware MUST NOT:

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

Firmware MUST NOT decompress, transform, repair, or substitute another blob.
A read failure is returned explicitly.

### 10.5 Deleting a blob

Deletion removes exactly the blob named by the request. It MUST NOT delete any
other version and MUST NOT select a replacement.

The last blob may be deleted. A device with no blobs is a valid empty-storage
state; React may create a new empty repository and save it.

### 10.6 No replace operation

There is no `PUT` and no atomic replace operation.

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

The maximum blob size is a centralized firmware constant chosen so the partition
can hold at least two maximum-sized final blobs plus one maximum-sized temporary
write, including filesystem overhead.

An upload over the declared body limit returns `413`. An upload within that limit
but impossible with current free space returns `507`. Either failure leaves all
final blobs unchanged.

### 10.8 Mount and recovery policy

A mount failure MUST NOT format the partition. It produces a visible degraded or
failed storage state.

Boot recovery removes only `*.tmp` files created by interrupted blob adds. It
MUST NOT delete a final `.gz` blob because its contents are unreadable to React.

---

## 11. Device configuration

NVS stores small device configuration only:

- device name;
- access-point SSID and passphrase;
- at most one station SSID and passphrase;
- administrator password verifier and salt;
- whether physical confirmation is required;
- provisioned flag and credential version;
- the optional next-blob-ID counter.

Repository JSON and blobs MUST NOT be stored in NVS.

The administrator password MUST NOT be recoverable. Use a per-password random
salt, an ESP-IDF/mbedTLS-compatible password derivation function, and constant-
time comparison.

Wi-Fi passphrases are necessarily recoverable by firmware but MUST NOT appear in
logs, API responses, blobs, exports, or diagnostics.

A stored configuration record with the wrong fixed length or invalid version is
corrupt and MUST NOT be parsed on a best-effort basis.

---

## 12. Network, authentication, and setup

### 12.1 Wi-Fi

The device always runs its own protected access point and MAY also join one
explicitly configured station network.

The access point starts first and unconditionally. A station join failure is
logged and ignored; it MUST NOT prevent access-point operation, erase stored
credentials, or cause an unbounded retry loop.

At most one station network is remembered. Firmware MUST NOT scan for, rank, or
join a network the user did not explicitly configure.

### 12.2 Authentication

Successful login creates a bounded RAM-only session and an `HttpOnly`,
`SameSite=Strict`, `Path=/` cookie. The cookie is the complete browser
credential.

Failed authentication is rate-limited. CORS is disabled.

There is no separate CSRF token. There is no Host/Origin check in the development
appliance profile. A product distributed to third parties MUST revisit DNS
rebinding protection before release.

### 12.3 First-run setup

An unprovisioned device exposes only setup state and setup submission, plus the
static assets required for that UI.

Setup requires a one-time code shown on the serial console and sets the device
name, access-point credentials, and administrator password.

Setup MUST preserve configuration fields it does not modify. It MUST NOT rebuild
a partial record and silently discard station credentials or unrelated settings.

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

Authenticated binary and JSON requests have bounded bodies. JSON routes accept
only `application/json`; blob uploads accept only `application/gzip`.
Malformed paths, unsupported methods, wrong content types, oversized bodies, and
unknown fields receive explicit 4xx responses.

No route accepts a user-controlled filesystem path. Blob IDs are parsed as
bounded decimal identifiers and converted to fixed internal paths.

There is no optimistic concurrency token, revision field, `If-Match`, or checksum
round trip. Last successful write wins.

### 13.2 Route table

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

### 13.3 Blob API

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

### 13.4 Send API

`POST /api/v1/send` is the only operation that starts keyboard output.

Request:

```json
{
  "source": "make -j8{ENTER}",
  "keyPressMs": 8,
  "interKeyMs": 15
}
```

The device validates bounds and compiles the complete source before accepting the
send. Invalid source returns `422` with the exact parser location. Nothing is
typed after a compile failure.

On acceptance, the device returns `202`:

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "state": "running",
  "actionCount": 9,
  "estimatedDurationMs": 214
}
```

If physical confirmation is enabled, the initial state is
`awaiting_confirmation`. The serial-console `confirm` command starts it. The
request expires after a bounded timeout. A second POST while one is awaiting
confirmation or running returns `409`.

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

### 13.5 React completion callback

A callback URL is not part of the wire protocol. A browser does not expose a
reliable local HTTP callback listener to the ESP32.

The React API layer SHOULD expose a helper shaped conceptually as:

```text
sendMacro(request, { onStatus, onComplete })
```

The helper:

1. calls `POST /api/v1/send`;
2. polls `GET /api/v1/send` at a bounded interval no slower than once per second;
3. invokes `onStatus` when the state changes;
4. invokes `onComplete` exactly once after a terminal state is observed.

If the page closes, the JavaScript callback is not guaranteed. The firmware send
continues independently, and reopening the application can recover its state
with `GET /api/v1/send`.

### 13.6 Status codes

```text
200  successful read or update
201  blob created
202  send accepted or cancellation requested
204  blob deleted
400  malformed request
401  login required or invalid credentials
403  policy failure
404  resource or send absent
409  send already active or confirmation slot busy
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

The React application is served from the `webfs` partition and works without
internet access.

Required screens:

1. First-run setup
2. Login
3. Snapshot chooser
4. Package chooser
5. Macro list
6. Macro editor with live TypeScript validation
7. Send preview and explicit confirmation
8. Send progress and cancel
9. Send result
10. Package management
11. Repository import and export
12. Snapshot management and deletion
13. Settings
14. Diagnostics

The operational header SHOULD show the device name, active package, USB state,
and access to snapshot and device settings.

Hash routing SHOULD be used so static-file serving does not require server-side
SPA fallback rules.

### 14.1 Startup

After authentication:

- if no blobs exist, React creates an empty in-memory repository;
- if one blob exists, React may offer to load it directly;
- if several blobs exist, React shows the newest first and lets the user choose;
- a decompression or validation failure keeps the chooser available and does not
  cause automatic fallback or deletion.

### 14.2 Editing

Package and macro CRUD, duplication, ordering, and active selection occur only in
the in-memory repository.

React MUST NOT call the ESP32 after every edit. Device writes occur only when the
user saves a complete snapshot.

### 14.3 Explicit send

Every send requires a user action on a preview showing:

- package name;
- macro name;
- decoded source or action summary;
- estimated duration;
- current USB state.

The application sends source and timing, not package ID, macro ID, revision, or
blob ID.

The next macro MUST NOT execute automatically.

### 14.4 Static assets

All production assets are local. Vite output uses content-hashed filenames.
Compressible static assets SHOULD have pre-generated gzip variants. The static
server sets correct content types, streams in bounded chunks, caches hashed
assets as immutable, serves `index.html` with revalidation, rejects path
traversal, and never exposes the userdata mount.

---

## 15. Physical controls

No button is required.

Confirmation and cancellation are available through the serial-console
`confirm` and `cancel` commands. Network cancellation through
`DELETE /api/v1/send` remains available during typing and delays.

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
- macro source.

Macro source may itself contain credentials and is always treated as sensitive
user content.

### 16.3 Diagnostics

Diagnostics report:

- firmware version and build ID;
- reset reason and uptime;
- free and minimum-free heap;
- task stack high-water marks;
- USB state;
- Wi-Fi state;
- webfs and userdata capacity;
- number of stored blobs;
- current or most recent send state;
- per-subsystem health;
- invalid filenames and temporary files found during storage inspection.

Diagnostics do not parse blobs and do not report package or macro information.

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

Checks MUST prevent reintroduction of firmware package/macro persistence and
package/macro API routes.

---

## 18. Testing

### 18.1 Native C tests

Host tests with fakes cover:

- macro parsing and compilation;
- the shared conformance corpus;
- executor state, cancellation, timeout, and release-all behavior;
- opaque blob list, add, load, delete, capacity, and boot cleanup;
- authentication and session policy;
- HTTP routing, methods, content types, body limits, and path rejection;
- startup sequencing;
- Wi-Fi and provisioning.

Tests for the retired package repository, package JSON parser, package imports,
package exports, package restore, macro CRUD, package ordering, and active-package
firmware state are removed with those features.

### 18.2 Web tests

Vitest and real-browser tests cover:

- strict repository schema validation;
- UUID uniqueness and reference invariants;
- TypeScript macro validation and conformance corpus;
- gzip round trips through browser Compression Streams;
- import failure leaving the working copy unchanged;
- package and macro editing and ordering;
- snapshot add, load, list, and delete workflows;
- save failure retaining the working copy;
- send polling and `onComplete` callback behavior;
- explicit-send confirmation;
- offline assets.

### 18.3 Hardware-in-the-loop

Against an attached ESP32-S3 and host HID reports:

- printable text arrives exactly;
- a chord sets modifier and usage concurrently;
- every send ends with an all-zero report;
- cancellation works through the API and console during typing and delay;
- invalid source types nothing;
- one compressed blob is added, power is removed, and the same bytes load after
  reboot;
- multiple blobs list in numeric order;
- deleting one blob leaves every other blob byte-identical;
- interrupted add leaves no final partial blob and boot removes the temporary;
- a full partition returns `507` without altering existing blobs;
- factory reset, settings reset, and reprovisioning behave as specified.

### 18.4 Host matrix

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
6. Firmware contains no package repository, macro repository, package index,
   active package, package/macro CRUD route, repository parser, compressor, or
   decompressor.
7. The documented schema round-trips through JSON and gzip without changing
   order or values.
8. A saved blob survives power loss and loads byte-identically.
9. Blob list, add, load, and delete behave exactly as §13.3.
10. An interrupted add leaves existing blobs intact and exposes no partial final
    blob.
11. Firmware stores no explicit checksum, CRC, or digest for repository blobs.
12. React reports decompression and schema errors and lets the user choose a
    different blob.
13. An over-limit or out-of-space add fails without modifying final blobs.
14. `POST /api/v1/send` accepts source and timing and no package or macro ID.
15. Invalid source returns an exact parser location and emits no key report.
16. Every terminal send path releases all keys, verified from HID reports.
17. `GET /api/v1/send` reports terminal completion and the React helper invokes
    its completion callback exactly once.
18. Cancellation works during typing and delay through the API and console.
19. No button or added hardware is required.
20. A mount failure does not format storage.
21. The web application works without internet access and fits its partition.
22. No credentials, repository data, or macro source appear in logs or
    diagnostics.
23. The shared conformance corpus passes in C and TypeScript.
24. The tests in §18 pass.
25. This document matches implemented behavior.

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
