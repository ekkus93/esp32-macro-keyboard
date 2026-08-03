# ESP32 Macro Keyboard — Specification v2

**Document status:** Draft for review. Not yet authoritative.
**Product version:** 0.2 (rebuild)
**Target hardware:** ESP32-S3R8, native USB wiring, octal PSRAM
**Firmware framework:** ESP-IDF v5.5.5, exact release tag
**Created:** 2026-08-03

## 0. About this document

`docs/SPEC.md` describes a device that owns a data model. This describes one
that does not. The difference is large enough that amending the old document
kept producing contradictions, so it is being replaced rather than patched.

**Provenance.** Every requirement here comes from one of three places, and
anything that comes from nowhere is a defect in this document:

1. **Decisions Phil made in conversation on 2026-08-02 and 2026-08-03.** These
   are quoted where the wording matters.
2. **Measurements taken on the bench device**, cited with their numbers.
3. **Carried over from `docs/SPEC.md`** where the text is settled and still
   true — the macro language, the HID safety invariant, the toolchain pins.
   Carried text is marked **[carried]** the first time it appears in a section.

Items marked **[unattributed]** were in the old specification with no recorded
source and no measurement behind them. They are reproduced so nothing is lost
silently, and each is a candidate for deletion.

**This document is frozen once accepted.** It may not be modified without Phil's
explicit, per-change permission. That rule exists because an acceptance
criterion in the previous planning documents turned out to have been invented by
the assistant and then cited back as a requirement; see
`docs/SPEC_CHANGE_AUDIT_2026-08-03.md`.

---

## 1. Purpose

The ESP32 Macro Keyboard is a USB keyboard that types what it is told to type.

It connects to a target computer through the ESP32-S3 native USB peripheral and
enumerates as a standard HID keyboard. It also runs a Wi-Fi access point and
serves a web application. The user works in that application, picks a macro, and
explicitly sends it; the device types it at the target computer.

The product is **generic**. It has no knowledge of what the target computer is,
what operating system it runs, or what the macros are for.

### 1.1 The division of labour

This is the organising decision of v2. Phil, 2026-08-03:

> "I think that we're trying to do way too much in the esp32 and that's really
> unnecessary. Its main job is that it needs to emulate a usb keyboard and send
> macros as a usb keyboard to the computer. The second job is to save a copy of
> all of the macro data so it persists after the esp32 is turned off. Everything
> else will be handled on the client side with React."

**The device does two things:**

1. Emulates a USB HID keyboard and types macro source it is given.
2. Stores a blob of user data so it survives power loss, and gives it back.

**The web application does everything else:** the data model, creating and
editing packages and macros, ordering, selection, validation feedback, import
and export to files.

The device does not parse the blob. It has no concept of a package, a macro
list, an active selection, or an index. Firmware MUST NOT be written to depend
on the blob's internal structure.

---

## 2. Normative language

**MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT** and **MAY**
have their usual requirements meaning.

Where implementation behaviour conflicts with this document, this document wins
unless it is deliberately amended in the same change — and amending it requires
permission (§0).

---

## 3. Product goals

The product MUST:

1. Enumerate as a standards-compliant USB HID keyboard on common desktop
   operating systems without a custom host driver.
2. Provide a local, mobile-first web application over its own access point.
3. Persist the user's data across power loss and firmware updates.
4. Require an explicit user action before every macro execution.
5. Release every key after completion, cancellation, USB loss, timeout, or
   internal failure.
6. Operate with no buttons and no hardware added to a stock devkit.
7. Operate with no internet access.
8. Treat every first-party compiler, type-checker, formatter or linter warning
   as a defect.

## 4. Non-goals for v0.2

- arbitrary Unicode typing;
- any awareness of the target computer;
- automatic execution of anything, on boot, on connection, or in sequence;
- USB host, Bluetooth HID, cloud accounts, internet routing;
- TLS on the access point;
- merge or conflict resolution between two edits of the repository;
- server-side rendering, Node.js, or JavaScript execution on the device;
- automatic filesystem formatting after a mount or integrity failure.

---

## 5. Platform

### 5.1 Toolchain **[carried]**

- ESP-IDF, exact signed tag **`v5.5.5`**. The build MUST reject any other
  version.
- Target `esp32s3`.
- Node.js exactly **`v24.18.0`** for the web application.
- Dependencies MUST be pinned by committed manifest and lock files. No caret or
  tilde ranges.

### 5.2 Hardware **[carried]**

The reference module is the **ESP32-S3R8**: 8 MB embedded **octal** PSRAM. The
build MUST enable `CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT` and
`CONFIG_SPIRAM_USE_MALLOC`. Octal is not interchangeable with quad — a build
configured for quad does not boot on this module.

FreeRTOS task stacks MUST come from internal SRAM; PSRAM cannot host them.

The hardware MUST expose the native USB D+/D− signals. It MUST NOT require any
button, jumper or added component beyond a stock devkit.

### 5.3 The device has no clock

There is no RTC synchronisation, no SNTP, and no internet (§4). The device
cannot know the date or time.

Firmware MUST NOT record, report, or require a wall-clock timestamp. Anything
needing a date — "when was this last edited" — is the web application's, from
the browser's clock.

*Verified 2026-08-03: no `sntp`, `settimeofday`, `esp_netif_sntp` or
`time(NULL)` call exists anywhere in the firmware.*

### 5.4 Partitions **[carried]**

```text
nvs        NVS         24 KiB
nvs_keys   NVS keys     4 KiB   encrypted
otadata    OTA data     8 KiB
phy_init   PHY          4 KiB
ota_0      app        2.5 MiB
ota_1      app        2.5 MiB
webfs      LittleFS     1 MiB   web application assets
userdata   LittleFS   512 KiB   the repository blobs
coredump   coredump    64 KiB
```

---

## 6. USB HID keyboard

### 6.1 Identity **[carried]**

The device enumerates with the project's own USB identity. It MUST NOT ship
Espressif's example vendor and product strings.

```text
VID:PID       303a:4001
manufacturer  ESP32 Macro Keyboard Project
product       ESP32 Macro Keyboard
serial        ESP32S3-MACRO-01
```

### 6.2 State model

USB state is one of: not connected, connected, ready, suspended, error. The web
application MUST be able to read it (§10).

Execution MUST NOT start unless USB is ready.

### 6.3 Report safety invariant **[carried]**

After every key or chord action, firmware MUST emit a release-all report.

On completion, cancellation, USB disconnect, USB suspend beyond the allowed
timeout, executor timeout, parser invariant failure, task failure, queue
failure, or internal error, firmware MUST attempt a release-all report and move
the execution to a terminal state. The executor MUST clear its internal
pressed-key state even when the transport cannot deliver the report.

### 6.4 Concurrency

There is exactly one executor task. HTTP handlers MUST NOT type directly.

---

## 7. Macro language **[carried]**

This section is unchanged from v1 and is the contract between the device's
compiler and the web application's validator (§7.8).

### 7.1 Character support

US English layout, and:

- printable ASCII `0x20`–`0x7E`;
- line feed, mapped to Enter;
- tab, mapped to Tab;
- the directives below.

CRLF is normalised to LF. Other Unicode input is rejected with an exact source
position.

### 7.2 Escaping

```text
{{  ->  {
}}  ->  }
```

An unmatched brace is an error.

### 7.3 Key directives

```text
{ENTER} {TAB} {ESC} {BACKSPACE} {DELETE}
{INSERT} {HOME} {END} {PAGEUP} {PAGEDOWN}
{UP} {DOWN} {LEFT} {RIGHT}
{SPACE}
{F1} through {F12}
```

### 7.4 Chords

Modifiers: `CTRL` `ALT` `SHIFT` `GUI`.

```text
{CTRL+L}   {CTRL+SHIFT+T}   {ALT+F4}   {GUI+R}
```

One non-modifier key plus one or more unique modifiers. Duplicate modifiers,
modifier-only chords, multiple ordinary keys and unknown names are errors.

### 7.5 Delay

```text
{DELAY:500}
```

An integer number of milliseconds, 1 to 10,000 inclusive.

### 7.6 Grammar rules

- Directive spelling is uppercase and canonical.
- Whitespace inside a directive is prohibited.
- Unknown directives are errors.
- The parser MUST consume the entire source.
- Parsing and compilation MUST complete before execution begins.
- Errors MUST include byte offset, line, column, error code and a
  human-readable message.
- A partially parsed macro MUST NOT execute.

### 7.7 Limits

These bound what the device will execute. They are not a data model.

```text
macro source bytes            4096
compiled actions              4096
delay per directive        10,000 ms
estimated total duration       300 s
```

The v1 limits on package counts, macros per package and file sizes are **gone**:
the device has no packages to count. What replaces them is a single ceiling on
the uploaded blob (§8.4).

### 7.8 Two implementations, one contract

The device compiles macro source in C in order to type it. The web application
validates macro source in TypeScript in order to give the user errors while
editing (§1.1). Both answer to this section.

They can drift, and the failure is bad: the application accepts a macro, the
device refuses it at execute time, and the user was told it was fine right up
until it typed nothing.

A **shared conformance corpus** MUST exist: one checked-in set of macro sources
with their expected compiled output and expected errors, exercised by both the C
host tests and the web application's tests. Drift MUST fail CI.

---

## 8. Storage

### 8.1 The repository is an opaque blob

Phil, 2026-08-03: *"Just store it in a blob to make things simple. We can save a
couple of older versions. It should be compressed anyway so it shouldn't take up
that much space."*

The device stores the user's data as a sequence of bytes it does not interpret.
It has no index, no per-package files and no metadata file. The blob is the only
user-data state.

### 8.2 Layout

```text
/data/
└── repository/
    ├── 000000007.bin      newest; the current repository
    ├── 000000006.bin
    ├── 000000005.bin
    ├── 000000004.bin
    └── 000000003.bin      oldest kept
```

The highest number is current. Firmware MUST keep the newest **five** and unlink
anything older after a successful write.

There MUST NOT be an index file, a metadata file, a per-package file, or a
`staging/`, `trash/`, `transactions/` or `quarantine/` directory.

*Measured 2026-08-03 on the bench device: a repository of 14 packages and 21
macros is 6,136 bytes of JSON, 1,332 bytes compressed. Five versions at that
size cost 6,660 bytes — 1.3% of the partition.*

### 8.3 Compression is the client's

The web application compresses the repository before uploading and decompresses
after downloading. The device stores the bytes it is given.

The device MUST NOT compress, decompress, or otherwise interpret a blob. It
therefore needs no compressor, and cannot disagree with the application about
the format.

### 8.4 Writing

A write is:

1. write `<n+1>.bin.tmp`;
2. `fsync`;
3. `rename()` over `<n+1>.bin`;
4. `fsync` the directory;
5. unlink anything older than the newest five.

The rename is the commit point. An interruption at any step leaves the previous
blob intact and current — **this is the rollback**, and it needs no second copy,
no marker and no boot repair.

Firmware MUST NOT ignore a short write, or an error from `fclose`, `fflush`,
`fsync`, `rename` or `unlink`.

Boot MUST remove any stray `.tmp` file.

Firmware MUST enforce a byte ceiling on an uploaded blob and reject an
over-budget upload with `507` (§10). The ceiling MUST leave room for five
versions plus one in-progress `.tmp` within the 512 KiB partition. That ceiling
is the only property of a blob the device can check.

### 8.5 Integrity

Firmware MUST record a CRC-32 of each blob's bytes when it is written, and MUST
verify it on read.

On mismatch, firmware MUST report the failure and the version number through the
API, and MUST NOT serve the blob, substitute an empty repository, or silently
fall back to an older version.

Firmware MUST NOT delete a blob that fails its checksum. A package file was one
of fifty and its loss was bounded; the blob is everything, and a damaged one may
still be largely recoverable by the application. The device reports; the user
decides.

*The v1 rule that a corrupt file is deleted does not carry over, and this is the
reason.*

### 8.6 Older versions

Choosing an older version is a **user action**. Firmware MUST NOT select one on
its own initiative, at boot or at any other time.

### 8.7 Mount policy **[carried]**

A mount failure MUST NOT format the partition. Automatic formatting MUST be
disabled at every filesystem registration. A failed mount is a reported,
visible, degraded state.

### 8.8 What the device cannot check

The device cannot tell whether a blob's contents are meaningful. A blob whose
checksum is good but whose JSON is malformed is the application's problem to
detect and report.

Product goal "reject malformed state rather than substituting defaults" applies
to the application for repository content, and to the device for macro source
(§7) and for everything in NVS (§9).

---

## 9. Device configuration (NVS)

NVS stores small device configuration only:

- device name;
- access-point SSID and passphrase;
- station SSID and passphrase, when a network has been joined;
- administrator password verifier and salt;
- execution policy and timing defaults;
- whether physical confirmation is required;
- provisioned flag and credential version;
- a record revision, used for its own concurrency.

Repository data MUST NOT be stored in NVS.

The administrator password MUST NOT be stored in plaintext, nor in any form from
which it can be recovered. Use a per-password random salt and a documented
password-based key derivation function available in the ESP-IDF/mbedTLS
environment. Comparison MUST be constant-time.

Wi-Fi passphrases are necessarily recoverable, because the radio must be given
the passphrase itself on every join. They are stored as-is and protected by
confinement instead: firmware MUST NOT emit either passphrase in a log line, an
API response, a blob, or a diagnostic report. A caller that needs an SSID MUST
NOT be handed the whole configuration record to pick it out of.

All configuration lives in one fixed-size record with a fixed field layout. A
stored record whose length does not match the current layout MUST be rejected as
corrupt rather than parsed on a best-effort basis.

---

## 10. Network and API

### 10.1 Wi-Fi

The device always runs its own access point, and MAY additionally join one
existing network as a station. The access point is the guaranteed control path
and MUST NOT depend on any external network.

Defaults: WPA2, no open fallback, DHCP server, `192.168.4.1`.

At boot the access point starts **first and unconditionally**; the station join
is attempted only afterwards. A join that fails, times out, or is refused MUST
be logged and otherwise ignored — the device continues as access-point only.
Firmware MUST NOT treat it as a startup failure, MUST NOT retry in a way that
delays startup, and MUST NOT discard stored credentials because one attempt
failed.

At most one network is remembered. Storing one replaces the previous. Firmware
MUST NOT scan for, rank, or join any network it was not explicitly given.

Station credentials are set from the serial console and persist across a power
cycle. *Measured 2026-08-03: the bench device rejoins its network unaided about
12 s after a reboot.*

### 10.2 Authentication

The session cookie is the entire credential. It is issued on successful login
and MUST be `HttpOnly`, `SameSite=Strict`, `Path=/`.

There is **no CSRF token**: `SameSite=Strict` means a browser will not attach
the cookie to a cross-site request at all, which is the attack a CSRF token
exists to stop.

There is **no `Host`/`Origin` check**. That defended against DNS rebinding, and
was removed deliberately for a device whose only network is an isolated access
point on a bench. Authenticated routes remain safe because such a page cannot
obtain the cookie; unauthenticated routes become reachable that way. **A product
shipped to third parties should reinstate it.**

Sessions are RAM-only, bounded in number, and expire. Failed authentication is
rate-limited. CORS is disabled.

### 10.3 First-run setup **[carried]**

An unprovisioned device serves only the setup route. Setup requires a one-time
code shown on the serial console, and sets the device name, access-point
credentials and administrator password.

Setup MUST NOT discard configuration it does not set. *This is stated because it
was a real defect: a fresh record was built with a designated initialiser and
silently dropped the stored station network.*

### 10.4 Concurrency

**There is none, and none is offered.** No revision numbers, no checksums on the
wire, no `If-Match`. A client never sends a token.

Phil, 2026-08-03: *"Get rid of revision. I don't think it adds any value."* and
*"Why does the client need to send a checksum? … That's overkill."*

Last write wins. Two browsers open on the same repository, both edited, both
saved: the second overwrites the first and nothing reports it. Writes remain
atomic (§8.4), so neither is half-applied; what is given up is *detecting* that
two clients wrote. This is a deliberate trade for a single-user appliance and
would not be acceptable on a shared one.

The device MAY report a blob's checksum, and the application MAY use it to
notice its own copy is stale. Firmware MUST NOT require it to be sent back.

### 10.5 Routes

All routes are under `/api/v1`.

```text
POST   /api/v1/setup
POST   /api/v1/auth/login
POST   /api/v1/auth/logout
GET    /api/v1/auth/session

GET    /api/v1/status
GET    /api/v1/limits

GET    /api/v1/repository
PUT    /api/v1/repository
GET    /api/v1/repository/versions
GET    /api/v1/repository/{version}

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

GET    /api/v1/diagnostics
```

There are no package routes, no macro routes, no import, export, backup or
restore routes, and no validate route. The device cannot address anything inside
the blob, so those routes would have nothing to operate on.

### 10.6 Executing a macro

`POST /api/v1/executions` carries the macro **source** and its timing, not an
identifier. Phil, 2026-08-03: *"React just sends the macro source. It just needs
to send it what will be sent to the computer as a usb keyboard. The esp32 app
doesn't need the id."*

```json
{ "source": "make -j8{ENTER}", "keyPressMs": 8, "interKeyMs": 15 }
```

The device compiles what it is handed and types it. Source it cannot compile is
rejected with `422` and the parse error's offset (§7.6).

If physical confirmation is enabled — off by default — the request waits for the
`confirm` console command and expires after a bounded timeout. That wait MUST
NOT run on the HTTP server task: `esp_http_server` serves every socket from one
task, so waiting there makes the device unreachable for the duration. At most
one confirmation-gated request is accepted at a time; a second is refused with
`409` rather than queued, because one confirmation cannot disambiguate two.

Execution starts only if USB is ready and the executor is idle.

### 10.7 Request limits

Bounded body size, bounded JSON depth, explicit content-type checks, no
user-controlled filesystem paths, no path traversal in static file serving.
Malformed or oversized requests receive explicit 4xx responses.

### 10.8 Status codes

```text
200  OK
201  created
202  execution accepted
400  malformed request
401  login required or invalid
403  policy failure
404  absent
409  busy, or a second confirmation-gated request
413  body over limit
415  wrong content type
422  invalid macro source, or invalid field
429  rate limited
500  internal error
503  subsystem unavailable
507  blob over the storage ceiling
```

---

## 11. Physical controls

The device MUST NOT require any button, and MUST NOT require hardware added to
the board. A stock devkit and a USB cable are a complete product.

Required logical controls: a status indicator.

Confirmation and cancellation are serial-console commands (`confirm`, `cancel`).
Cancellation MUST remain available during typing and during delays, over both
the API and the console.

*v1 specified cancel and confirm buttons and a reset boot gesture. No board this
project uses breaks out the GPIO they were assigned, the reset gesture was never
implemented, and gating six routes on a press made the device unusable. They are
console commands instead.*

### 11.1 The serial console is a trusted surface

Console commands require no session and no confirmation: possession of the board
*is* the authorization. Reaching the UART port means holding hardware that can
be reflashed outright, so authenticating it would add friction without
protection.

The console MUST NOT expose credentials or secret material, because the failure
mode there is disclosure rather than control.

**The console is a development interface.** Before any release to third parties
it MUST be excluded from the shipped image: a shipped device's physical surface
belongs to its user, and `wifi-connect` would let anyone with momentary physical
access redirect it. Until then it is present in every build, and that is a
documented product limitation rather than a defect.

---

## 12. Web application

Served from the `webfs` partition. React, TypeScript, built to static assets.

The application MUST work with no internet access. Every asset MUST be local —
no remote `//` URLs in the built output.

It owns the data model (§1.1): packages, macros, ordering, the active selection,
validation feedback, and import/export to files on the user's computer.

It MUST NOT persist repository data in `localStorage`, `sessionStorage`,
IndexedDB, or a service worker cache. The device is the store; the browser holds
a working copy for the session.

Required screens: setup, login, repository browsing and editing, macro editing
with live validation, send-and-confirm, execution progress, settings,
diagnostics.

---

## 13. Error handling

### 13.1 No silent failures **[carried]**

Every operation MUST return, log, or expose an explicit success or failure.

No code may swallow an `esp_err_t`, discard an error result, return success
after partial completion, log an error and continue in an invalid state,
substitute empty data after a parse failure, retry forever silently, silently
downgrade authentication, storage or USB behaviour, or use a dangerous fallback
to keep running.

### 13.2 Logging

Logs MUST NOT contain passwords, passphrases, session tokens, setup codes, key
material, or macro source. Macro source is user content and may contain
anything the user types, including credentials.

### 13.3 Diagnostics

The device reports: firmware version and build id, reset reason, uptime, free
and minimum-free heap, task stack high-water marks, USB state, Wi-Fi state,
partition usage, the current repository version and its checksum status, and
per-subsystem health.

---

## 14. Quality gates **[carried]**

`./scripts/check-all.sh` is the authoritative local gate and CI MUST call the
same command. It MUST fail on the first failed phase and MUST never mask
failures.

- No `|| true`, no redirected errors, no warning suppression, no first-party
  lint or analyser exclusions.
- clang-tidy runs with `WarningsAsErrors: '*'`; ESLint and stylelint with
  `--max-warnings=0`.
- A defect MUST be fixed at its source, not suppressed. Approved exceptions are
  registered in a tracked document.
- The project MUST NOT modify ESP-IDF, managed components, or npm dependencies
  in place.

---

## 15. Testing

### 15.1 Host tests

Native C tests with fakes for every hardware backend: macro parsing and
compilation, execution state machine, blob storage, authentication, HTTP
routing and policy, startup sequencing, Wi-Fi and provisioning.

### 15.2 Web application tests

Unit and component tests, plus a real-browser pass against a fixture device.

### 15.3 Conformance corpus

§7.8. One corpus, both parsers, drift fails CI.

### 15.4 Hardware-in-the-loop

Against an attached device, reading back the kernel's HID reports — the bytes on
the wire, not text captured from an editor:

- printable text arrives exactly;
- a chord sets the modifier bit concurrently with the usage code;
- every run ends with an all-zero report;
- cancellation over both the API and the console, during typing and during a
  delay;
- a repository saved, the device power-cycled, and the repository returned
  byte-identical;
- an interrupted save leaves the previous version current;
- factory reset, credential reset, and re-provisioning.

### 15.5 Host matrix

Linux, and ChromeOS and Windows when the machines are available.

---

## 16. Acceptance criteria for v0.2

1. A clean checkout builds with ESP-IDF v5.5.5 for `esp32s3`.
2. `./scripts/check-all.sh` exits 0 with zero first-party warnings.
3. The device enumerates as a USB keyboard with the project's own identity.
4. The device starts a protected access point with no open fallback.
5. A repository saved from the web application survives a power cycle and comes
   back byte-identical.
6. Five versions are retained; the user can load an older one; the device never
   selects one by itself.
7. Power loss during a save leaves the previous version current and complete,
   and boot removes any stray `.tmp`.
8. A blob that fails its checksum is reported and not served, and not deleted.
9. An over-budget upload is refused with `507` and does not disturb the stored
   versions.
10. The user can execute a macro only through an explicit action, and invalid
    source is refused with an exact position.
11. Every terminal execution path releases all keys, verified from HID reports.
12. Cancellation works during typing and during delays, over the API and the
    console.
13. The device requires no button and no added hardware.
14. A mount failure does not format storage.
15. The web application works with no internet access and fits its partition.
16. No credentials or macro source appear in logs or diagnostics.
17. The conformance corpus passes against both parsers.
18. The tests in §15 pass.
19. This document matches implemented behaviour.

---

## 17. Carried forward without a recorded source

These were in v1 with no attribution and no measurement. They are reproduced so
nothing is lost silently. Each is a candidate for deletion — **[unattributed]**:

- **Estimated total duration 300 s** (§7.7). No source for the number.
- **Compiled actions 4096** (§7.7). Plausibly derived from the source byte
  limit, but not recorded.
- **`192.168.4.1`** as the access-point address (§10.1). This is the ESP-IDF
  default, so it is probably inherited rather than chosen.
- **Session table bounds and expiry values** (§10.2), which v1 required without
  giving numbers.
- **The status indicator** (§11). Required as a "logical control" with no
  behaviour specified anywhere.
- **Device name** as a stored setting (§9). Nothing reads it.

## 18. Deferred

Additional keyboard layouts; platform-specific Unicode entry; mDNS; web-based
OTA; encrypted blobs; signed blobs; merge; execution history; an OLED display;
Bluetooth HID; multiple users.
