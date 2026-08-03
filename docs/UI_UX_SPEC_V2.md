# ESP32 Macro Keyboard — UI/UX Specification v2

**Document status:** Authoritative UI/UX companion and amendment to `docs/SPEC_V2.md`  
**Product version:** 0.2 rebuild  
**Last updated:** 2026-08-03

## 0. Authority

This document defines the v2 React user interface, screen behavior, navigation,
startup decisions, and user workflows.

`docs/SPEC_V2.md` remains authoritative for product architecture, firmware
behavior, repository schema, storage, HTTP APIs, security, limits, and quality
gates. This document is authoritative for UI/UX details and supersedes the
higher-level web-application workflow text in §14 of `docs/SPEC_V2.md` wherever
the two differ.

No v1 specification, retired TODO, or current v1 React behavior may be used to
infer v2 UI requirements.

The mockups discussed with the product owner are design references. The
requirements in this document are normative even when a mockup contains an
accidental label, date, control, or behavior that conflicts with this text.

---

## 1. UX principles

1. The **Macros page is the primary operating console**.
2. The normal action—sending an existing macro—MUST keep the user on the Macros
   page.
3. Package and macro editing occur in React's in-memory working copy and MUST NOT
   produce a device write after every edit.
4. Persistence is explicit through **Save snapshot**.
5. Repository snapshots live on the ESP32; repository state MUST NOT be cached in
   browser persistent storage.
6. A new phone or browser uses the same device repository. There is no per-phone
   repository, package selection, recent-package history, or user profile.
7. The UI MUST distinguish device state, working-copy state, and send state.
8. Destructive actions require clear confirmation and exact consequences.
9. Safety controls—especially cancellation and release-all—remain reachable
   during an active send.
10. The application is mobile-first, works offline, and remains usable on
    tablets and desktop browsers.

---

## 2. Application shell

### 2.1 Operational header

Authenticated operational screens SHOULD show:

- device name;
- active package name, or `No active package`;
- USB state;
- working-copy state (`Saved` or `Unsaved changes`);
- access to package switching and settings.

USB readiness MUST be visually prominent. Quick Send controls MUST be disabled
unless USB is `ready`.

### 2.2 Primary navigation

The authenticated bottom navigation is:

```text
Macros | Packages | Snapshots | Settings
```

The active destination MUST be exposed through `aria-current="page"` and a
non-color-only visual treatment.

### 2.3 Routing

Hash routing SHOULD be used. A route may encode the selected package or macro
for navigation, but the repository remains the source of truth and URLs MUST NOT
cause firmware package or macro lookups.

---

## 3. Startup state machine

The first screen is determined by device provisioning, authentication, and
whether React still has an in-memory working copy.

### 3.1 Unconfigured device

An unprovisioned device opens **First-Run Setup**.

The setup workflow is:

1. identify the device and enter the one-time setup code;
2. set device name;
3. set access-point SSID and passphrase;
4. set the administrator password;
5. optionally require serial confirmation before typing;
6. review and apply;
7. restart and instruct the user to reconnect;
8. proceed to Sign In.

Repository creation is not part of device provisioning.

### 3.2 Configured device without an active session

A configured device with no valid session opens **Sign In**.

After successful authentication, React performs the repository startup process
in §3.4. It MUST NOT insert a separate welcome tour merely because this is the
first sign-in from a particular Android phone or browser.

### 3.3 Already authenticated with a live working copy

If the tab still holds a valid in-memory repository, returning to the application
restores the current screen and working-copy state.

An active or recently completed send MUST be recovered and shown inline.

### 3.4 Authenticated without a live working copy

A refreshed or reopened application briefly shows:

```text
Opening your repository…
Loading the newest snapshot from the device.
```

React then:

1. verifies the session;
2. lists stored blobs;
3. selects the newest blob by numeric ID;
4. downloads it;
5. gzip-decompresses it;
6. validates the complete repository;
7. restores `activePackageId`;
8. checks `GET /api/v1/send` for a non-terminal or recent send.

Normal destination rules:

- valid repository with a valid active package → that package's **Macros page**;
- valid repository with `activePackageId: null` → **Package chooser**;
- no blobs → **Create your first repository**;
- newest blob unreadable or invalid → **Snapshot chooser** with the error.

React MUST NOT silently fall back to an older blob. The user chooses an older
snapshot explicitly after the newest one fails.

### 3.5 New phone or browser

A first sign-in from a new Android phone follows §3.2 and §3.4. The device's
newest repository snapshot is loaded; nothing is restored from the phone.

---

## 4. Required screens and surfaces

1. First-Run Setup
2. Setup Complete / Reconnect
3. Sign In
4. Repository Loading
5. Create Your First Repository
6. Snapshot Chooser / Snapshot Error Recovery
7. Package Chooser
8. Macros page with Quick Send
9. Macro Editor
10. Optional Preview and Send
11. Package Management
12. Snapshot Management
13. Repository Import and Export
14. Settings
15. Diagnostics
16. Dialogs and inline states for delete, replace, failure, cancellation, and
    unsaved changes

Send progress and terminal results are normally inline surfaces on the Macros
page, not mandatory standalone pages.

---

## 5. Macros page

### 5.1 Purpose

The Macros page is the default destination for a valid active package and the
most frequently used screen.

It shows:

- active package and a **Change** action;
- macro count;
- **Add macro**;
- ordered macro rows or cards;
- macro name;
- short source preview;
- **Edit**;
- primary **Send** control;
- overflow actions such as Preview and send, Duplicate, Move, and Delete;
- reorder affordances;
- working-copy state;
- inline send status and cancellation.

### 5.2 Quick Send

The primary **Send** button starts the selected macro directly from the list.
The user MUST remain on the Macros page.

A Send press is the explicit user action required before keyboard output. The
UI calls `POST /api/v1/send` with source and timing and then polls
`GET /api/v1/send`.

The application MUST NOT require navigation to a confirmation page for ordinary
Quick Send.

### 5.3 Optional preview

A full preview remains available through the macro overflow menu and MAY be the
default under an optional user preference such as **Always preview before
sending**.

The preview shows:

- package name;
- macro name;
- readable source or decoded action summary;
- key-press and inter-key timing;
- action count and estimated duration when available;
- current USB state;
- explicit **Send now** and **Cancel** actions.

### 5.4 Inline send states

When Quick Send starts:

1. the selected row changes to `Sending…` or the current progress;
2. all other Send controls are disabled because only one send may be active;
3. a compact page-level status surface identifies the macro and progress;
4. **Cancel and release all keys** remains available;
5. serial-confirmation state is shown inline when required.

Terminal behavior:

- completed → show `Sent` with an acknowledgement for approximately three to
  five seconds, then restore the ordinary Send control;
- cancelled → show a persistent cancelled acknowledgement until dismissed or
  another send begins;
- failed or timed out → show the exact error and Retry when safe;
- release failure → report it separately and prominently.

The acknowledgement MUST identify the macro that was sent. A transient page-level
message MAY accompany the row state.

### 5.5 Recovery after reload

If the page reloads during a send, React obtains the state from
`GET /api/v1/send` and restores the inline status and cancellation control.

---

## 6. Package workflows

### 6.1 Package chooser

The Package chooser appears when the repository has packages but no valid active
package, or when the user selects **Change**.

It shows package names, macro counts, search, and an **Open** action. Selecting a
package updates `activePackageId` only in the working copy and opens its Macros
page.

### 6.2 Package management

Users can create, rename, duplicate, reorder, and delete packages. These are
local working-copy operations.

Deleting the active package requires the UI to explain how active selection will
change. Destructive confirmation MUST identify the package by name.

---

## 7. Macro editing

The editor includes:

- name;
- key-press duration;
- inter-key delay;
- macro source;
- directive insertion controls;
- UTF-8 byte counts;
- live TypeScript validation;
- exact error location and **Go to error**;
- action count and estimated duration when valid;
- Save changes and Cancel.

Saving updates the in-memory package. It does not call a firmware macro route and
does not create a repository snapshot automatically.

The UI MUST show `Unsaved changes` after the working copy diverges from the
loaded or last-saved snapshot.

---

## 8. Snapshot workflows

### 8.1 Automatic startup loading

The newest blob is loaded automatically during normal authenticated startup.
The Snapshot chooser is not shown on every visit.

### 8.2 Save snapshot

**Save snapshot** validates, serializes, gzip-compresses, size-checks, and uploads
the complete repository. A successful `201` marks the working copy saved.

A failed save leaves the working copy intact and dirty.

### 8.3 Snapshot management

Snapshot Management shows:

- blob ID;
- stored size;
- loaded snapshot indicator;
- storage usage;
- Load;
- Download;
- Delete;
- Save current snapshot;
- retention preference.

The UI MUST NOT display device-generated dates because none exist. A browser may
show transient local presentation dates only when clearly labeled as browser
information and never as device ordering metadata.

Loading a snapshot while the working copy is dirty requires a warning with:

- Cancel;
- Export current working copy;
- Discard changes and load.

### 8.4 Unreadable snapshot

Decompression or schema failure leaves the snapshot in storage, reports the
specific failure, and lets the user download it or choose another snapshot.

### 8.5 Replace

Normal saves add new snapshots. Explicit replacement is an advanced destructive
operation implemented as delete followed by add. The UI MUST state that the old
blob remains deleted if the subsequent upload fails.

---

## 9. Import and export

Export downloads the current working copy as `.emk-repository.json.gz`.

Import:

1. selects a local gzip file;
2. decompresses and validates it;
3. shows package and macro counts plus active-package summary;
4. replaces only the in-memory working copy after confirmation.

Import MUST NOT automatically upload a snapshot.

---

## 10. Settings

Settings include:

- device name;
- serial-confirmation policy;
- administrator password change;
- access-point and optional station-network configuration;
- snapshot retention preference;
- Quick Send versus Always Preview sending preference;
- restart;
- reset settings;
- factory reset;
- Diagnostics access.

Active package is repository state, not a device setting.

---

## 11. Portrait-phone requirement

### 11.1 Policy

On phone-sized touch devices, the operational UI is portrait-only.

A browser tab cannot be trusted to enforce `screen.orientation.lock()`, so the
application MUST enforce the product behavior through responsive UI rather than
relying solely on an orientation API.

### 11.2 Landscape behavior on phones

When a phone enters landscape orientation, the application replaces ordinary
screen content with a full-viewport orientation surface:

```text
Rotate your phone
ESP32 Macro Keyboard is designed for portrait mode.
```

Returning to portrait restores the exact prior route and working-copy state.
The orientation change MUST NOT reload the application, clear the repository, or
restart a send.

### 11.3 Active-send safety

If a send is awaiting confirmation or running while the phone is landscape, the
orientation surface MUST also show:

- macro name;
- current send state or progress;
- **Cancel and release all keys**.

The portrait requirement MUST NOT make emergency cancellation inaccessible.

### 11.4 Device classification

The landscape block applies only to phone-like displays, using tested responsive
criteria that combine orientation, coarse pointer/touch capability, and a short
viewport dimension. It MUST NOT block ordinary landscape use on tablets,
laptops, or desktop monitors.

The implementation SHOULD include `orientation: "portrait-primary"` in the web
app manifest as progressive enhancement, but MUST NOT depend on manifest or
fullscreen orientation locking for correctness.

### 11.5 Testing

Real-browser tests MUST cover:

- portrait phone shows the application;
- landscape phone shows the rotate surface;
- returning to portrait restores the same route and draft;
- active-send progress and cancellation remain available in landscape;
- tablet and desktop landscape remain usable;
- orientation changes do not create duplicate sends or duplicate completion
  callbacks.

---

## 12. Responsive design

- Minimum supported CSS viewport width: 320 px.
- Touch targets MUST be at least 44 by 44 CSS pixels.
- Phones use a single-column layout.
- Tablets and desktops may use wider cards, split panes, or denser management
  layouts while preserving the same workflow.
- Bottom navigation remains reachable without covering the final actionable
  content.
- Safe-area insets MUST be respected on devices with display cutouts or gesture
  navigation.

---

## 13. Accessibility

- All controls are keyboard accessible.
- Focus order follows visual and workflow order.
- Dialogs trap focus and restore it to their invoking control.
- Status and errors use appropriate live regions without repeatedly announcing
  polling updates.
- Color is never the only indicator of USB, dirty, validation, or send state.
- Source editors expose labels, help text, and validation locations.
- Reordering has accessible Move first, Move up, Move down, and Move last actions
  even when drag and drop is available.
- Reduced-motion preferences disable nonessential animation.

---

## 14. Error and offline states

The UI MUST explicitly handle:

- device unreachable;
- session expired;
- unsupported Compression Streams;
- no snapshots;
- invalid or unreadable newest snapshot;
- no active package;
- USB not ready;
- send already active;
- firmware parser rejection;
- cancellation;
- send timeout;
- release-all failure;
- snapshot too large;
- insufficient device storage;
- snapshot deletion failure;
- factory reset and reconnect.

Errors MUST preserve the working copy whenever technically possible.

---

## 15. UI acceptance criteria

1. An unconfigured device opens First-Run Setup.
2. A configured device without a session opens Sign In.
3. A successful sign-in from a new phone automatically loads the newest valid
   snapshot and opens the active package's Macros page.
4. An authenticated refresh shows a brief repository-loading state, not Sign In.
5. No snapshots opens Create Your First Repository.
6. An invalid newest snapshot opens the Snapshot chooser and does not silently
   load an older snapshot.
7. The Macros page provides one-tap Quick Send for each macro.
8. Quick Send does not navigate away from the Macros page.
9. Inline progress, cancellation, completion, failure, timeout, and release-error
   states are visible.
10. A terminal completion acknowledgement identifies the macro and returns the
    row to Send after a short interval.
11. Optional Preview and send remains available.
12. Package and macro edits do not call firmware CRUD routes.
13. Dirty working-copy state is always visible after an edit.
14. Save snapshot is the normal persistence operation.
15. Repository data is absent from browser persistent storage.
16. Phone landscape shows the portrait-required surface.
17. Active-send cancellation remains accessible from that surface.
18. Tablets and desktops remain usable in landscape.
19. All accessibility requirements in §13 pass automated and manual checks.
20. This document matches the implemented v2 React behavior.
