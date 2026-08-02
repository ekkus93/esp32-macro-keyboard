# SPEC → test traceability

Generated from `docs/SPEC.md` and `tests/host/test_*.c`. Regenerate with
`scripts/generate-spec-traceability.py`.

## Why this exists

The host suite has 293 test functions and passes. That number says nothing about
whether the specification is covered, because the tests were written **after**
the code they test, in the same pass — so they encode what the implementation
does rather than what the specification requires.

That is not hypothetical. `POST /api/v1/sets/{setId}/select` required an
`expectedRevision` field that the handler parsed and never used. Six tests
asserted that requirement because the handler had it. Not one asked what §12.3
actually says, which is nothing about a revision on selection. All six passed
for months. Hardware found it in a minute by sending `{}`.

Consensus among tests derived from the same source is worth nothing. This
document exists so a requirement can be checked against a test, in that
direction.

## How to read `Status`

- **referenced** — at least one test cites this section. That is a *weak* signal:
  it means someone had the section in mind, not that this particular sentence is
  covered. Verify before trusting it.
- **UNMAPPED** — no test anywhere cites this section. Certainly not deliberately
  covered.

Neither value is a coverage measurement. This is a worklist, not a score.

## Totals

| | Statements | Unmapped |
| --- | --- | --- |
| MUST NOT | 34 | 27 |
| MUST | 72 | 60 |
| **Total** | **106** | **87** |

## Prohibitions (`MUST NOT`) — do these first

A prohibition has no happy path, so nothing covers it by accident. These are the
cheapest place to find real gaps.

| Section | SPEC line | Requirement | Status | Test(s) referencing this section |
| --- | --- | --- | --- | --- |
| §1.1 | L39 | product** and MUST NOT be reintroduced without a deliberate amendment: | **UNMAPPED** | — |
| §2 | L60 | The words **MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and | **UNMAPPED** | — |
| §4 | L91 | Version 0.1 MUST NOT attempt to provide: | **UNMAPPED** | — |
| §5.2 | L143 | The hardware MUST NOT require any button, jumper, or other component to be added | **UNMAPPED** | — |
| §5.3 | L165 | file, and test it. It MUST NOT silently fall back to another filesystem or USB | **UNMAPPED** | — |
| §6 | L228 | and Vite output are generated or third-party content and MUST NOT be linted as | **UNMAPPED** | — |
| §7.1 | L250 | through. Firmware MUST preserve it exactly and MUST NOT reorder, sort, or | referenced | web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §7.1 | L253 | The user MUST explicitly select the active set. Firmware MUST NOT infer or | referenced | web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §8.1 | L284 | The device MUST NOT fall back to an open AP. | **UNMAPPED** | — |
| §8.4 | L330 | rather than demanding confirmation unconditionally. That wait MUST NOT | **UNMAPPED** | — |
| §8.4 | L341 | The next macro in the list MUST NOT execute automatically. Advancing is a | **UNMAPPED** | — |
| §8.7 | L394 | It MUST NOT contain: | **UNMAPPED** | — |
| §9.2 | L465 | application MUST NOT fetch remote resources. | **UNMAPPED** | — |
| §10.7 | L578 | They MUST NOT be duplicated as inconsistent magic numbers. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.5 | L652 | There is one macro-executor task. HTTP handlers MUST NOT type directly. | **UNMAPPED** | — |
| §13.2 | L787 | Firmware MUST NOT automatically format either filesystem. | **UNMAPPED** | — |
| §13.2 | L790 | Web-assets failure MUST NOT expose an unauthenticated fallback UI. | **UNMAPPED** | — |
| §13.3 | L804 | This is the whole tree: two paths and one object type. There MUST NOT be a | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file) |
| §13.5 | L847 | not atomic across sets**, and MUST NOT pretend to be: each set file is written | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.5 | L852 | Restore MUST NOT perform the whole rewrite synchronously on the HTTP server task. | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.6 | L879 | Deleting a corrupt file MUST NOT be reported as successful recovery. | referenced | storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space |
| §13.7 | L888 | resource metadata. The server MUST NOT silently overwrite a newer edit. | **UNMAPPED** | — |
| §14 | L905 | Passwords MUST NOT be stored in plaintext. Use a per-password random salt and a | **UNMAPPED** | — |
| §15 | L925 | AP startup failure is a visible fatal network state. The firmware MUST NOT | **UNMAPPED** | — |
| §16.5 | L991 | The console MUST NOT expose credentials or secret material even so, because | **UNMAPPED** | — |
| §17 | L1094 | 'GET /api/v1/backup' MUST NOT let one damaged object make the repository | **UNMAPPED** | — |
| §17 | L1114 | MUST NOT report '200' for a run that failed to write some of them. | **UNMAPPED** | — |
| §19 | L1176 | The device MUST NOT require any button, and MUST NOT require hardware to be | **UNMAPPED** | — |
| §20.1 | L1218 | The project MUST NOT: | **UNMAPPED** | — |
| §21.1 | L1269 | The defect MUST be fixed at its source. It MUST NOT be hidden, suppressed, | **UNMAPPED** | — |
| §21.3 | L1303 | The project MUST NOT modify ESP-IDF, managed components, npm dependencies, or | **UNMAPPED** | — |
| §21.4 | L1313 | First-party source and project configuration MUST NOT use warning suppression as | **UNMAPPED** | — |
| §26 | L1596 | Deferred features MUST NOT be partially or silently enabled in version 0.1. | **UNMAPPED** | — |
| §27 | L1619 | MUST NOT be assumed to exist. Implement the pages from this specification until | **UNMAPPED** | — |

## Requirements (`MUST`)

| Section | SPEC line | Requirement | Status | Test(s) referencing this section |
| --- | --- | --- | --- | --- |
| §3 | L68 | The product MUST: | **UNMAPPED** | — |
| §5.1 | L118 | The firmware MUST build against the exact signed ESP-IDF tag: | **UNMAPPED** | — |
| §5.1 | L124 | The build MUST reject an unrecognized ESP-IDF version. Development documentation | **UNMAPPED** | — |
| §5.1 | L125 | and CI MUST clone ESP-IDF recursively from the exact tag rather than a moving | **UNMAPPED** | — |
| §5.2 | L139 | The hardware MUST expose the ESP32-S3 native USB D+ and D- signals. A board with | **UNMAPPED** | — |
| §5.2 | L147 | SPI PSRAM. The build MUST enable it ('CONFIG_SPIRAM', 'CONFIG_SPIRAM_MODE_OCT', | **UNMAPPED** | — |
| §5.2 | L151 | and restore bodies affordable. FreeRTOS task stacks MUST still come from internal | **UNMAPPED** | — |
| §5.3 | L161 | All dependency resolutions MUST be pinned by committed manifest and lock files. | **UNMAPPED** | — |
| §5.3 | L163 | component version is incompatible with ESP-IDF v5.5.5, the implementation MUST | **UNMAPPED** | — |
| §5.4 | L174 | The frontend MUST use: | **UNMAPPED** | — |
| §5.4 | L182 | The Node.js major version MUST be pinned in the repository. JavaScript package | **UNMAPPED** | — |
| §5.4 | L183 | versions MUST be locked with a committed lockfile. Production assets MUST be | **UNMAPPED** | — |
| §5.4 | L184 | static files and MUST contain no CDN, remote-font, remote-icon, analytics, or | **UNMAPPED** | — |
| §8.4 | L329 | is required, and every confirmation-gated route MUST honour the setting | **UNMAPPED** | — |
| §8.6 | L367 | Deletion MUST: | referenced | storage_sets → create_leaves_no_staging_artifacts<br>storage_sets → crud_ordering_revisions_and_cleanup |
| §8.7 | L382 | A set export MUST be a single versioned JSON package containing: | **UNMAPPED** | — |
| §8.7 | L403 | Import MUST validate the entire package, all limits, references, syntax, schema, | **UNMAPPED** | — |
| §9 | L416 | The application MUST be mobile-first and usable from a desktop browser. | **UNMAPPED** | — |
| §9 | L436 | The persistent operational header MUST show: | **UNMAPPED** | — |
| §9.2 | L464 | All application assets MUST be bundled into the web-assets filesystem. The | **UNMAPPED** | — |
| §9.3 | L469 | Vite output MUST use content-hashed filenames. JavaScript, CSS, SVG, and other | **UNMAPPED** | — |
| §9.3 | L472 | The server MUST: | **UNMAPPED** | — |
| §10.6 | L554 | The parser MUST consume the entire source. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L555 | Parsing and compilation MUST complete before execution begins. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L556 | Validation errors MUST include byte offset, line, column, error code, and a | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.7 | L577 | Limits MUST be centralized, visible through the API, and tested at boundaries. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L582 | MB against a 512 KiB partition. Firmware MUST enforce the storage limits by | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L584 | count limits, and MUST reject an over-budget write with '507' (§17) rather than | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.1 | L592 | The ESP32-S3 MUST enumerate as a USB HID keyboard using the native USB device | **UNMAPPED** | — |
| §11.1 | L595 | USB descriptors MUST use project-owned manufacturer, product, and serial strings. | **UNMAPPED** | — |
| §11.3 | L619 | After every normal key or chord action, firmware MUST emit a release-all report. | **UNMAPPED** | — |
| §11.3 | L633 | firmware MUST attempt a release-all report and transition the execution to a | **UNMAPPED** | — |
| §11.3 | L634 | terminal state. The executor MUST also clear its internal pressed-key state even | **UNMAPPED** | — |
| §11.5 | L667 | Cancellation MUST use a thread-safe flag, task notification, or equivalent | **UNMAPPED** | — |
| §11.5 | L668 | bounded mechanism and MUST remain responsive during delay actions. | **UNMAPPED** | — |
| §12 | L672 | All persistent objects MUST contain: | **UNMAPPED** | — |
| §12.3 | L747 | the index, is a corruption of the index and is handled under §13.6. Firmware MUST | referenced | provisioning_settings → (file)<br>storage_active_set_delete → (file)<br>storage_sets → delete_is_permanent_and_leaves_no_trash |
| §13.1 | L771 | Exact sizes are defined in 'firmware/partitions.csv' and MUST be validated | **UNMAPPED** | — |
| §13.3 | L794 | The 'userdata' partition is **512 KiB**. The layout MUST be flat: one index file | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file) |
| §13.4 | L824 | Every update MUST: | referenced | storage_atomic → create_enforces_operation_sequence<br>storage_atomic_recovery → (file)<br>storage_atomic_recovery → stray_temporary_is_removed_at_boot |
| §13.6 | L878 | The error MUST name the object and MUST be surfaced through the API and the UI. | referenced | storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space |
| §16.2 | L945 | Every mutating request MUST provide a valid CSRF token tied to the session. | **UNMAPPED** | — |
| §16.3 | L952 | policy. The implementation MUST avoid unbounded per-IP state. | **UNMAPPED** | — |
| §16.4 | L956 | The HTTP server MUST enforce: | **UNMAPPED** | — |
| §16.5 | L975 | the device's own SoftAP or, in development builds, a joined network - MUST | **UNMAPPED** | — |
| §16.5 | L976 | carry a valid RAM-only session, and every mutation MUST additionally carry a | **UNMAPPED** | — |
| §16.5 | L978 | failures MUST be rate-limited. No network-reachable route may mutate device | **UNMAPPED** | — |
| §16.5 | L997 | third parties it MUST be excluded from the shipped image, since a shipped | **UNMAPPED** | — |
| §17 | L1091 | it, but external behavior and resource boundaries MUST remain equivalent and be | **UNMAPPED** | — |
| §17 | L1099 | A partial backup MUST be self-describing, so it can never be mistaken for a | **UNMAPPED** | — |
| §17 | L1107 | I/O, storage unavailable, timeout) MUST still fail the export, because | **UNMAPPED** | — |
| §17 | L1113 | partial success MUST enumerate which sets were restored and which were not; it | **UNMAPPED** | — |
| §19 | L1178 | product. GPIO assignment for the one remaining output MUST be configurable | **UNMAPPED** | — |
| §19 | L1194 | Cancellation MUST remain available during execution and delay actions, over | **UNMAPPED** | — |
| §19 | L1207 | Indicator semantics MUST be documented and testable. Failure LEDs do not replace | **UNMAPPED** | — |
| §20.1 | L1214 | Every operation MUST return, log, or expose an explicit success or failure. | **UNMAPPED** | — |
| §20.2 | L1231 | Logs MUST: | **UNMAPPED** | — |
| §20.3 | L1259 | A downloadable diagnostic report MUST redact secrets and macro source by | referenced | storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space |
| §21.3 | L1289 | The quality gate MUST exclude: | **UNMAPPED** | — |
| §21.3 | L1306 | If a diagnostic originates exclusively in a third-party header, the tool MUST be | **UNMAPPED** | — |
| §21.5 | L1369 | MUST run the authoritative local quality gate. CI MUST call the same command. | **UNMAPPED** | — |
| §21.5 | L1370 | The script MUST fail on the first failed phase or aggregate failures while still | **UNMAPPED** | — |
| §21.5 | L1371 | returning nonzero; it MUST never mask failures. | **UNMAPPED** | — |
| §23 | L1417 | The firmware build MUST fail when the expected web assets are absent, stale | **UNMAPPED** | — |
| §23 | L1420 | The build MUST record: | **UNMAPPED** | — |
| §23 | L1430 | Release builds MUST be reproducible from committed sources and lockfiles. | **UNMAPPED** | — |
| §24.1 | L1436 | Tests MUST cover: | **UNMAPPED** | — |
| §24.2 | L1454 | Tests MUST cover: | **UNMAPPED** | — |
| §24.3 | L1473 | Tests MUST cover: | **UNMAPPED** | — |
| §24.4 | L1489 | Tests MUST cover: | **UNMAPPED** | — |
| §24.5 | L1507 | Tests MUST cover: | **UNMAPPED** | — |
| §24.6 | L1526 | At minimum, acceptance testing MUST include: | **UNMAPPED** | — |
