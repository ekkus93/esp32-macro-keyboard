# ESP32 Macro Keyboard — UI/UX Specification v2

**Document status:** Authoritative UI/UX companion to `docs/SPEC_V2.md`  
**Product version:** 0.2 rebuild  
**Last updated:** 2026-08-03

## 0. Authority

This document defines the v2 React user interface, screen behavior, navigation,
startup decisions, and user workflows.

`docs/SPEC_V2.md` remains authoritative for product architecture, firmware
behavior, repository schema, storage, HTTP APIs, security, limits, and quality
gates. This document is authoritative for UI/UX details. Where the high-level
web-application workflow text in `docs/SPEC_V2.md` differs from this document,
this document controls until the parent specification is synchronized.

No v1 specification, retired TODO, or current v1 React behavior may be used to
infer v2 UI requirements.

Mockups are design references. The requirements in this document are normative
even when a mockup contains an accidental label, value, control, or behavior
that conflicts with this text.

---

## 1. UX principles

1. The **Macros page is the primary operating console**.
2. Sending an existing macro normally keeps the user on the Macros page.
3. Package and macro editing occurs in React's in-memory working copy and does
   not produce a device write after every edit.
4. Repository persistence is explicit through **Save snapshot**.
5. Creating and deleting snapshots are manual user actions.
6. Repository snapshots live on the ESP32. Repository data is not cached in
   browser persistent storage.
7. A new phone or browser uses the same device repository. There is no per-phone
   repository, package history, or user profile.
8. Package selection is a device UI preference, not repository content. Merely
   changing the selected package does not make the repository dirty.
9. The UI distinguishes device state, repository working-copy state, and send
   state.
10. Destructive actions require clear confirmation and exact consequences.
11. Cancellation and release-all controls remain reachable during an active
    send.
12. The application is mobile-first, works without internet access, and remains
    usable on tablets and desktop browsers.

---

## 2. Application shell

### 2.1 Operational header

Authenticated operational screens show:

- device name;
- selected package name, or `No package selected`;
- USB state;
- repository state: `Saved` or `Unsaved changes`;
- **Save snapshot** when the repository is dirty;
- access to package switching and settings.

USB readiness is visually prominent. Quick Send controls are disabled unless USB
is `ready`.

The unsaved indicator remains visible on all authenticated operational screens.
It is cleared only after a complete snapshot upload succeeds with `201 Created`
or after the user deliberately discards or replaces the working copy.

### 2.2 Primary navigation

The authenticated bottom navigation is:

```text
Macros | Packages | Snapshots | Settings
```

The active destination uses `aria-current="page"` and a non-color-only visual
treatment.

### 2.3 Routing

Hash routing SHOULD be used. A route may encode a package or macro selection for
navigation, but the in-memory repository remains the source of truth. URLs do not
cause firmware package or macro lookups.

---

## 3. Startup state machine

The first screen is determined by device provisioning, authentication, and
whether React still has an in-memory working copy.

### 3.1 Unconfigured device

An unprovisioned device opens **First-Run Setup**.

The setup workflow is:

1. identify the device and enter the one-time setup code;
2. set the device name;
3. set the access-point SSID and passphrase;
4. set the administrator password;
5. optionally require serial confirmation before typing;
6. review and apply;
7. restart and instruct the user to reconnect;
8. proceed to Sign In.

Repository creation is not part of device provisioning.

### 3.2 Configured device without an active session

A configured device with no valid session opens **Sign In**.

After successful authentication, React performs the repository startup process
in §3.4. It does not insert a welcome tour merely because this is the first
sign-in from a particular Android phone or browser.

### 3.3 Already authenticated with a live working copy

When the tab still holds a valid in-memory repository, returning to the
application restores the current route, drafts, dirty state, and send state.

### 3.4 Authenticated without a live working copy

A refreshed or reopened application briefly shows:

```text
Opening your repository…
Loading the newest snapshot from the device.
```

React then:

1. verifies the session;
2. loads device UI settings, including `lastSelectedPackageId`;
3. lists stored blobs;
4. selects the newest blob by numeric ID;
5. downloads it;
6. gzip-decompresses it;
7. validates the complete repository;
8. checks `GET /api/v1/send` for a non-terminal or recent send;
9. resolves the selected package using §3.6.

Normal destination rules:

- valid repository and resolvable package selection → that package's **Macros
  page**;
- valid repository with packages but no resolvable selection → **Package
  chooser**;
- valid empty repository → **Create your first package**;
- no blobs → **Create your first repository**;
- newest blob unreadable or invalid → **Snapshot chooser** with the error.

React does not silently fall back to an older blob. The user chooses an older
snapshot explicitly after the newest one fails.

### 3.5 New phone or browser

A first sign-in from a new Android phone follows §3.2 and §3.4. The newest device
snapshot and device-wide package selection are used; nothing is restored from
the phone.

### 3.6 Selected-package resolution

Package selection is not stored in repository snapshots.

The authenticated device settings contain an opaque
`lastSelectedPackageId: UUID | null`. Firmware stores and returns that value but
does not interpret it as repository data.

After a repository is loaded, React resolves the initial package as follows:

1. when `lastSelectedPackageId` identifies a package in the loaded repository,
   open that package;
2. otherwise, when the repository contains exactly one package, open it and
   update `lastSelectedPackageId`;
3. otherwise show the Package chooser.

Selecting another package updates the device UI preference and the current React
route. It does not alter repository JSON, mark the working copy dirty, or require
a snapshot.

---

## 4. Required screens and surfaces

1. First-Run Setup
2. Setup Complete / Reconnect
3. Sign In
4. Repository Loading
5. Create Your First Repository / First Package
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
16. Portrait-required phone surface
17. Dialogs and inline states for delete, replace, failure, cancellation, session
    expiry, and unsaved changes

Send progress and terminal results are normally inline surfaces on the Macros
page rather than mandatory standalone pages.

---

## 5. Macros page

### 5.1 Purpose

The Macros page is the default destination for a resolved package and the most
frequently used screen.

It shows:

- selected package and a **Change** action;
- macro count;
- **Add macro**;
- ordered macro rows or cards;
- macro name;
- source visibility state;
- **Edit**;
- primary **Send** control;
- overflow actions such as Preview and send, Duplicate, Move, and Delete;
- reorder affordances;
- repository dirty state;
- inline send status and cancellation.

### 5.2 Macro-source privacy

Macro source is sensitive user content and may contain passwords, tokens, or
private commands.

The Macros page hides source previews by default. A hidden row may show `Source
hidden` or an equivalent non-revealing placeholder. The user may reveal one row
temporarily or enable the device-wide **Show macro source previews** preference.

Revealing source does not change repository data. Source is shown normally in the
Macro Editor and Optional Preview and Send screen.

Macro source must not be copied into logs, diagnostics, browser telemetry,
notification text, or send acknowledgements.

### 5.3 Quick Send

The primary **Send** button starts the selected macro directly from the list. The
user remains on the Macros page.

A Send press is the explicit user action required before keyboard output. React
calls `POST /api/v1/send` with source and timing and then polls
`GET /api/v1/send`.

The application does not require navigation to a confirmation page for ordinary
Quick Send. Quick Send is the default device setting.

### 5.4 Optional preview

A full preview remains available through the macro overflow menu. The device-wide
**Always preview before sending** preference may make it the default behavior.

The preview shows:

- package name;
- macro name;
- readable source or decoded action summary;
- key-press and inter-key timing;
- action count and estimated duration when available;
- current USB state;
- explicit **Send now** and **Cancel** actions.

### 5.5 Inline send states

When Quick Send starts:

1. the selected row changes to `Sending…` or current progress;
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

The acknowledgement identifies the macro but never includes macro source.

### 5.6 Recovery after reload

When the page reloads during a send, React obtains state from
`GET /api/v1/send` and restores inline status and cancellation controls.

---

## 6. Package workflows

### 6.1 Package chooser

The Package chooser appears when the loaded repository has packages but the
selected package cannot be resolved, or when the user selects **Change**.

It shows package names, macro counts, search, and an **Open** action. Selecting a
package updates the device-wide `lastSelectedPackageId` preference and opens its
Macros page. It does not alter the repository or dirty state.

### 6.2 Package management

Users can create, rename, duplicate, reorder, and delete packages. These are
repository working-copy operations and mark the repository dirty.

Deleting the selected package requires the UI to identify it by name and explain
that another package must be selected. After deletion, React resolves the
selection using §3.6 and updates `lastSelectedPackageId` as needed.

---

## 7. Repository editing and unsaved changes

### 7.1 Macro editor

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

### 7.2 Dirty-state rules

The repository becomes dirty after any repository-content change, including:

- creating, editing, deleting, duplicating, moving, or reordering a package;
- creating, editing, deleting, duplicating, moving, or reordering a macro;
- importing a repository into the working copy.

The repository does not become dirty after:

- selecting a package;
- changing Quick Send, retention-target, or source-preview preferences;
- sending or cancelling a macro;
- viewing, downloading, or deleting a stored snapshot.

### 7.3 Leaving with unsaved changes

When the repository is dirty:

- the application header continuously shows **Unsaved changes** and **Save
  snapshot**;
- browser reload, tab close, or navigation away registers a `beforeunload`
  warning where the browser supports it;
- Sign Out, loading another snapshot, replacing the working copy through import,
  reset settings, and factory reset require an in-app warning;
- the warning offers the context-appropriate choices: Cancel, Export working
  copy, Save snapshot, or Discard changes;
- the UI never claims that a closed dirty working copy can be recovered.

A session expiry does not discard the in-memory working copy. React presents a
reauthentication surface and resumes the same working copy after successful
login whenever the page remains open.

---

## 8. First repository workflow

When no snapshots exist, the application opens **Create your first repository**.

The normal sequence is:

1. create a new valid empty repository in memory;
2. ask for the first package name;
3. create that package;
4. store its ID as `lastSelectedPackageId`;
5. open its empty Macros page;
6. let the user add macros;
7. continuously show **Unsaved changes** until **Save snapshot** succeeds.

The UI explicitly states that the repository exists only in this browser tab
until a snapshot has been saved successfully.

---

## 9. Snapshot workflows

### 9.1 Automatic startup loading

Normal authenticated startup attempts to load the newest blob automatically. The
Snapshot chooser is not shown merely because several blobs exist.

### 9.2 Save snapshot

**Save snapshot** validates, serializes, gzip-compresses, size-checks, and uploads
the complete repository. A successful `201 Created` marks the working copy saved.

A failed save leaves the working copy intact and dirty.

Snapshot creation is always initiated by the user. The application does not
autosave and does not create snapshots on a timer, after each edit, on package
selection, or after a macro send.

### 9.3 Snapshot management

Snapshot Management shows:

- blob ID;
- stored size;
- loaded snapshot indicator;
- storage usage;
- configured retention target;
- Load;
- Download;
- Delete;
- Save current snapshot.

The UI does not display device-generated dates because none exist. A browser may
show transient local presentation dates only when clearly labeled as browser
information and never as device ordering metadata.

### 9.4 Manual loading

The user may open Snapshots at any time and load a different stored snapshot.
Loading changes only the in-memory working copy. It does not delete, overwrite,
or modify any stored snapshot.

Loading while the working copy is dirty requires a warning with:

- Cancel;
- Export current working copy;
- Save snapshot;
- Discard changes and load.

After loading, selected-package resolution follows §3.6.

### 9.5 Manual deletion and retention target

Snapshot deletion is always an explicit user action with confirmation. The device
and React application never automatically delete snapshots.

The device-wide `snapshotRetentionTarget` defaults to `5`. It is advisory: when
the stored count exceeds the target, the Snapshots tab and management page show a
non-blocking cleanup indicator. The user chooses which snapshots, if any, to
delete.

Saving a sixth snapshot succeeds when space permits. It does not automatically
delete the oldest snapshot.

### 9.6 Unreadable snapshot

A decompression or schema failure leaves the snapshot in storage, reports the
specific failure, and lets the user download it or deliberately choose another
snapshot.

### 9.7 Replace

Normal saves add snapshots. Explicit replacement is an advanced destructive
operation implemented as delete followed by add. The UI states that the old blob
remains deleted if the subsequent upload fails.

---

## 10. Import and export

Export downloads the current working copy as `.emk-repository.json.gz`.

Import:

1. selects a local gzip file;
2. decompresses and validates it;
3. shows package and macro counts;
4. replaces only the in-memory working copy after confirmation;
5. resolves the package selection using §3.6.

Import does not automatically upload a snapshot and marks the repository dirty.

---

## 11. Settings

User-visible settings include:

- device name;
- serial-confirmation policy;
- administrator password change;
- access-point and optional station-network configuration;
- sending behavior: Quick Send or Always Preview;
- snapshot retention target, default `5` and advisory only;
- Show macro source previews, default off;
- restart;
- reset settings;
- factory reset;
- Diagnostics access.

`lastSelectedPackageId` is persisted as device UI state but is not normally shown
as an editable text setting.

Changing UI preferences does not dirty the repository and does not create a
snapshot.

---

## 12. Portrait-phone requirement

### 12.1 Policy

On phone-sized touch devices, the operational UI is portrait-only.

A browser tab cannot be trusted to enforce `screen.orientation.lock()`, so the
application enforces the product behavior through responsive UI rather than
relying solely on an orientation API.

### 12.2 Landscape behavior on phones

When a phone enters landscape orientation, the application replaces ordinary
screen content with a full-viewport orientation surface:

```text
Rotate your phone
ESP32 Macro Keyboard is designed for portrait mode.
```

Returning to portrait restores the exact prior route, draft, working copy, and
dirty state. Orientation changes do not reload the application, clear the
repository, or restart a send.

### 12.3 Active-send safety

When a send is awaiting confirmation or running while the phone is landscape,
the orientation surface also shows:

- macro name;
- current send state or progress;
- **Cancel and release all keys**.

The portrait requirement never makes emergency cancellation inaccessible.

### 12.4 Device classification

The landscape block applies only to phone-like displays, using tested responsive
criteria that combine landscape orientation, coarse pointer capability, and a
short viewport dimension. The initial implementation target is equivalent to:

```css
@media (orientation: landscape) and (pointer: coarse) and (max-height: 600px)
```

Implementation tests may refine the exact query to handle browser chrome,
foldables, and known tablets without broadening the block to ordinary tablet,
laptop, or desktop landscape use.

The web app manifest SHOULD include `orientation: "portrait-primary"` as
progressive enhancement, but correctness does not depend on manifest or
fullscreen orientation locking.

### 12.5 Testing

Real-browser tests cover:

- portrait phone shows the application;
- landscape phone shows the rotate surface;
- returning to portrait restores the same route, draft, and dirty state;
- active-send progress and cancellation remain available in landscape;
- tablet and desktop landscape remain usable;
- orientation changes do not create duplicate sends or callbacks.

---

## 13. Responsive design

- Minimum supported CSS viewport width: 320 px.
- Touch targets are at least 44 by 44 CSS pixels.
- Phones use a single-column layout.
- Tablets and desktops may use wider cards, split panes, or denser management
  layouts while preserving the same workflow.
- Bottom navigation remains reachable without covering final actionable content.
- Safe-area insets are respected on devices with display cutouts or gesture
  navigation.

---

## 14. Accessibility

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
- Hidden macro source is not exposed accidentally through accessible names or
  live-region announcements.

---

## 15. Error and offline states

The UI explicitly handles:

- device unreachable;
- session expired without discarding a live working copy;
- unsupported Compression Streams;
- no snapshots;
- invalid or unreadable newest snapshot;
- no selected package;
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

Errors preserve the working copy whenever technically possible.

---

## 16. UI acceptance criteria

1. An unconfigured device opens First-Run Setup.
2. A configured device without a session opens Sign In.
3. A successful sign-in from a new phone attempts the newest snapshot and opens
   the resolved package's Macros page.
4. An authenticated refresh shows a brief repository-loading state rather than
   Sign In.
5. No snapshots opens Create Your First Repository.
6. An invalid newest snapshot opens recovery and does not silently load an older
   snapshot.
7. Selecting another package does not alter repository JSON or dirty state.
8. The Macros page provides one-press Quick Send for each macro.
9. Quick Send does not navigate away from the Macros page.
10. Inline progress, cancellation, completion, failure, timeout, and release
    errors are visible.
11. A terminal completion acknowledgement identifies the macro, reveals no
    source, and returns the row to Send after a short interval.
12. Optional Preview and send remains available.
13. Macro source is hidden on the Macros page by default.
14. Package and macro edits do not call firmware CRUD routes.
15. Dirty working-copy state remains visible until a snapshot save succeeds or
    changes are deliberately discarded.
16. Reload, close, sign-out, import replacement, and snapshot replacement paths
    protect dirty work as specified.
17. Snapshot creation and deletion are manual.
18. Exceeding the default retention target of five produces an advisory cleanup
    indicator and no automatic deletion.
19. Repository data is absent from browser persistent storage.
20. Phone landscape shows the portrait-required surface.
21. Active-send cancellation remains accessible from that surface.
22. Tablets and desktops remain usable in landscape.
23. Accessibility requirements in §14 pass automated and manual checks.
24. This document matches the implemented v2 React behavior.
