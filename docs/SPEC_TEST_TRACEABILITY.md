# SPEC → test traceability

Generated from `docs/SPEC.md` and `tests/host/test_*.c`. Regenerate with
`scripts/generate-spec-traceability.py`. Do not edit by hand.

## Why this exists

The host suite has hundreds of test functions and passes. That number says
nothing about whether the specification is covered, because the tests were
written **after** the code they test, in the same pass — so they encode what the
implementation does rather than what the specification requires.

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
- **gate-enforced** — no test cites it, but a `scripts/check-*.sh` that runs on
  every `check-all.sh` does. Some prohibitions are properties of the tree, not
  behaviours of a function: "MUST NOT fetch remote resources" and "MUST NOT use
  warning suppression" cannot be unit-tested, and a script that fails the build
  is the stronger enforcement. Listing them as unmapped understated coverage;
  calling them tests would overstate it.
- **UNMAPPED** — nothing anywhere cites this section. Certainly not deliberately
  covered.

None of these is a coverage measurement. This is a worklist, not a score.

## Totals

| | Statements | Unmapped |
| --- | --- | --- |
| MUST NOT | 41 | 13 |
| MUST | 74 | 45 |
| **Total** | **115** | **58** |

## Prohibitions (`MUST NOT`) — do these first

A prohibition has no happy path, so nothing covers it by accident. These are the
cheapest place to find real gaps.

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
| §1.1 | L39 | product** and MUST NOT be reintroduced without a deliberate amendment: | **UNMAPPED** | — |
| §2 | L60 | The words **MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and | **UNMAPPED** | — |
| §4 | L91 | Version 0.1 MUST NOT attempt to provide: | **UNMAPPED** | — |
| §5.2 | L143 | The hardware MUST NOT require any button, jumper, or other component to be added | **UNMAPPED** | — |
| §5.3 | L165 | file, and test it. It MUST NOT silently fall back to another filesystem or USB | **UNMAPPED** | — |
| §6 | L228 | and Vite output are generated or third-party content and MUST NOT be linted as | **UNMAPPED** | — |
| §7.1 | L250 | through. Firmware MUST preserve it exactly and MUST NOT reorder, sort, or | referenced | storage_sets → measured_user_data_tracks_set_files<br>web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §7.1 | L253 | The user MUST explicitly select the active set. Firmware MUST NOT infer or | referenced | storage_sets → measured_user_data_tracks_set_files<br>web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §8.1 | L284 | The device MUST NOT fall back to an open AP. | referenced | wifi_ap → minimum_credentials_and_existing_event_loop<br>wifi_ap → operation_validation |
| §8.4 | L330 | rather than demanding confirmation unconditionally. That wait MUST NOT | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.4 | L341 | The next macro in the list MUST NOT execute automatically. Advancing is a | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.7 | L394 | It MUST NOT contain: | referenced | storage_package_export → deterministic_export_and_filtering |
| §9.2 | L465 | application MUST NOT fetch remote resources. | gate-enforced | verify-no-remote-assets.sh (gate script) |
| §10.7 | L578 | They MUST NOT be duplicated as inconsistent magic numbers. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.5 | L652 | There is one macro-executor task. HTTP handlers MUST NOT type directly. | **UNMAPPED** | — |
| §13.2 | L787 | Firmware MUST NOT automatically format either filesystem. | gate-enforced | check-mount-policy.sh (gate script) |
| §13.2 | L790 | Web-assets failure MUST NOT expose an unauthenticated fallback UI. | gate-enforced | check-mount-policy.sh (gate script) |
| §13.3 | L804 | This is the whole tree: two paths and one object type. There MUST NOT be a | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file)<br>storage_sets → duplicate_index_is_discarded_and_output_cleared<br>web_api_repository_handlers → (file) |
| §13.5 | L847 | not atomic across sets**, and MUST NOT pretend to be: each set file is written | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.5 | L852 | Restore MUST NOT perform the whole rewrite synchronously on the HTTP server task. | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.6 | L879 | Deleting a corrupt file MUST NOT be reported as successful recovery. | referenced | storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space<br>web_api_core → route_parsing |
| §13.7 | L888 | resource metadata. The server MUST NOT silently overwrite a newer edit. | referenced | web_api_repository_handlers → session_json_redaction |
| §14 | L906 | The administrator password MUST NOT be stored in plaintext, nor in any form from | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §14 | L914 | them is confinement rather than hashing: firmware MUST NOT emit either | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §14 | L916 | report. A caller that needs an SSID MUST NOT be handed a copy of the whole | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.1 | L946 | AP startup failure is a visible fatal network state. The firmware MUST NOT | **UNMAPPED** | — |
| §15.2 | L958 | firmware MUST NOT keep a list, and MUST NOT scan for, rank, or join any network | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L970 | availability a fatal-if-absent property, so it MUST NOT be made to wait on, or | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L975 | ignored: the device continues as access-point only. Firmware MUST NOT treat it | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L976 | as a startup failure, MUST NOT retry it in a way that delays or blocks the rest | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L977 | of startup, and MUST NOT discard the stored credentials because one join | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §16.5 | L1040 | The console MUST NOT expose credentials or secret material even so, because | gate-enforced | check-credential-logging.sh (gate script) |
| §17 | L1143 | `GET /api/v1/backup` MUST NOT let one damaged object make the repository | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1163 | MUST NOT report `200` for a run that failed to write some of them. | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §19 | L1225 | The device MUST NOT require any button, and MUST NOT require hardware to be | **UNMAPPED** | — |
| §20.1 | L1267 | The project MUST NOT: | **UNMAPPED** | — |
| §21.1 | L1318 | The defect MUST be fixed at its source. It MUST NOT be hidden, suppressed, | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §21.3 | L1352 | The project MUST NOT modify ESP-IDF, managed components, npm dependencies, or | **UNMAPPED** | — |
| §21.4 | L1362 | First-party source and project configuration MUST NOT use warning suppression as | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §26 | L1645 | Deferred features MUST NOT be partially or silently enabled in version 0.1. | **UNMAPPED** | — |
| §27 | L1668 | MUST NOT be assumed to exist. Implement the pages from this specification until | **UNMAPPED** | — |

## Requirements (`MUST`)

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
| §3 | L68 | The product MUST: | **UNMAPPED** | — |
| §5.1 | L118 | The firmware MUST build against the exact signed ESP-IDF tag: | **UNMAPPED** | — |
| §5.1 | L124 | The build MUST reject an unrecognized ESP-IDF version. Development documentation | **UNMAPPED** | — |
| §5.1 | L125 | and CI MUST clone ESP-IDF recursively from the exact tag rather than a moving | **UNMAPPED** | — |
| §5.2 | L139 | The hardware MUST expose the ESP32-S3 native USB D+ and D- signals. A board with | **UNMAPPED** | — |
| §5.2 | L147 | SPI PSRAM. The build MUST enable it (`CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`, | **UNMAPPED** | — |
| §5.2 | L151 | and restore bodies affordable. FreeRTOS task stacks MUST still come from internal | **UNMAPPED** | — |
| §5.3 | L161 | All dependency resolutions MUST be pinned by committed manifest and lock files. | **UNMAPPED** | — |
| §5.3 | L163 | component version is incompatible with ESP-IDF v5.5.5, the implementation MUST | **UNMAPPED** | — |
| §5.4 | L174 | The frontend MUST use: | **UNMAPPED** | — |
| §5.4 | L182 | The Node.js major version MUST be pinned in the repository. JavaScript package | **UNMAPPED** | — |
| §5.4 | L183 | versions MUST be locked with a committed lockfile. Production assets MUST be | **UNMAPPED** | — |
| §5.4 | L184 | static files and MUST contain no CDN, remote-font, remote-icon, analytics, or | **UNMAPPED** | — |
| §8.4 | L329 | is required, and every confirmation-gated route MUST honour the setting | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.6 | L367 | Deletion MUST: | referenced | storage_sets → create_leaves_no_staging_artifacts<br>storage_sets → crud_ordering_revisions_and_cleanup |
| §8.7 | L382 | A set export MUST be a single versioned JSON package containing: | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L403 | Import MUST validate the entire package, all limits, references, syntax, schema, | referenced | storage_package_export → deterministic_export_and_filtering |
| §9 | L416 | The application MUST be mobile-first and usable from a desktop browser. | **UNMAPPED** | — |
| §9 | L436 | The persistent operational header MUST show: | **UNMAPPED** | — |
| §9.2 | L464 | All application assets MUST be bundled into the web-assets filesystem. The | gate-enforced | verify-no-remote-assets.sh (gate script) |
| §9.3 | L469 | Vite output MUST use content-hashed filenames. JavaScript, CSS, SVG, and other | **UNMAPPED** | — |
| §9.3 | L472 | The server MUST: | **UNMAPPED** | — |
| §10.6 | L554 | The parser MUST consume the entire source. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L555 | Parsing and compilation MUST complete before execution begins. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L556 | Validation errors MUST include byte offset, line, column, error code, and a | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.7 | L577 | Limits MUST be centralized, visible through the API, and tested at boundaries. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L582 | MB against a 512 KiB partition. Firmware MUST enforce the storage limits by | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L584 | count limits, and MUST reject an over-budget write with `507` (§17) rather than | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.1 | L592 | The ESP32-S3 MUST enumerate as a USB HID keyboard using the native USB device | **UNMAPPED** | — |
| §11.1 | L595 | USB descriptors MUST use project-owned manufacturer, product, and serial strings. | **UNMAPPED** | — |
| §11.3 | L619 | After every normal key or chord action, firmware MUST emit a release-all report. | **UNMAPPED** | — |
| §11.3 | L633 | firmware MUST attempt a release-all report and transition the execution to a | **UNMAPPED** | — |
| §11.3 | L634 | terminal state. The executor MUST also clear its internal pressed-key state even | **UNMAPPED** | — |
| §11.5 | L667 | Cancellation MUST use a thread-safe flag, task notification, or equivalent | **UNMAPPED** | — |
| §11.5 | L668 | bounded mechanism and MUST remain responsive during delay actions. | **UNMAPPED** | — |
| §12 | L672 | All persistent objects MUST contain: | **UNMAPPED** | — |
| §12.3 | L747 | the index, is a corruption of the index and is handled under §13.6. Firmware MUST | referenced | provisioning_settings → (file)<br>storage_active_set_delete → (file)<br>storage_sets → delete_is_permanent_and_leaves_no_trash<br>web_api_json → settings_update_matrix<br>web_api_repository_handlers → set_routes |
| §13.1 | L771 | Exact sizes are defined in `firmware/partitions.csv` and MUST be validated | **UNMAPPED** | — |
| §13.3 | L794 | The `userdata` partition is **512 KiB**. The layout MUST be flat: one index file | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file)<br>storage_sets → duplicate_index_is_discarded_and_output_cleared<br>web_api_repository_handlers → (file) |
| §13.4 | L824 | Every update MUST: | referenced | storage_atomic → create_enforces_operation_sequence<br>storage_atomic_recovery → (file)<br>storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>storage_parent_sync → (file)<br>storage_parent_sync → rename_failure_on_create_leaves_nothing |
| §13.6 | L878 | The error MUST name the object and MUST be surfaced through the API and the UI. | referenced | storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space<br>web_api_core → route_parsing |
| §14 | L921 | A stored record whose length does not match the current layout MUST be rejected | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L974 | A station join that fails, times out, or is refused MUST be logged and otherwise | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §16.2 | L994 | Every mutating request MUST provide a valid CSRF token tied to the session. | **UNMAPPED** | — |
| §16.3 | L1001 | policy. The implementation MUST avoid unbounded per-IP state. | **UNMAPPED** | — |
| §16.4 | L1005 | The HTTP server MUST enforce: | **UNMAPPED** | — |
| §16.5 | L1024 | the device's own SoftAP or, in development builds, a joined network - MUST | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1025 | carry a valid RAM-only session, and every mutation MUST additionally carry a | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1027 | failures MUST be rate-limited. No network-reachable route may mutate device | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1046 | third parties it MUST be excluded from the shipped image, since a shipped | gate-enforced | check-credential-logging.sh (gate script) |
| §17 | L1140 | it, but external behavior and resource boundaries MUST remain equivalent and be | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1148 | A partial backup MUST be self-describing, so it can never be mistaken for a | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1156 | I/O, storage unavailable, timeout) MUST still fail the export, because | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1162 | partial success MUST enumerate which sets were restored and which were not; it | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §19 | L1227 | product. GPIO assignment for the one remaining output MUST be configurable | **UNMAPPED** | — |
| §19 | L1243 | Cancellation MUST remain available during execution and delay actions, over | **UNMAPPED** | — |
| §19 | L1256 | Indicator semantics MUST be documented and testable. Failure LEDs do not replace | **UNMAPPED** | — |
| §20.1 | L1263 | Every operation MUST return, log, or expose an explicit success or failure. | **UNMAPPED** | — |
| §20.2 | L1280 | Logs MUST: | **UNMAPPED** | — |
| §20.3 | L1308 | A downloadable diagnostic report MUST redact secrets and macro source by | referenced | storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space |
| §21.3 | L1338 | The quality gate MUST exclude: | **UNMAPPED** | — |
| §21.3 | L1355 | If a diagnostic originates exclusively in a third-party header, the tool MUST be | **UNMAPPED** | — |
| §21.5 | L1418 | MUST run the authoritative local quality gate. CI MUST call the same command. | **UNMAPPED** | — |
| §21.5 | L1419 | The script MUST fail on the first failed phase or aggregate failures while still | **UNMAPPED** | — |
| §21.5 | L1420 | returning nonzero; it MUST never mask failures. | **UNMAPPED** | — |
| §23 | L1466 | The firmware build MUST fail when the expected web assets are absent, stale | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1469 | The build MUST record: | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1479 | Release builds MUST be reproducible from committed sources and lockfiles. | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §24.1 | L1485 | Tests MUST cover: | **UNMAPPED** | — |
| §24.2 | L1503 | Tests MUST cover: | **UNMAPPED** | — |
| §24.3 | L1522 | Tests MUST cover: | **UNMAPPED** | — |
| §24.4 | L1538 | Tests MUST cover: | **UNMAPPED** | — |
| §24.5 | L1556 | Tests MUST cover: | **UNMAPPED** | — |
| §24.6 | L1575 | At minimum, acceptance testing MUST include: | **UNMAPPED** | — |
