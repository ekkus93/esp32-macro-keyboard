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
| MUST NOT | 41 | 7 |
| MUST | 74 | 37 |
| **Total** | **115** | **44** |

## Prohibitions (`MUST NOT`) — do these first

A prohibition has no happy path, so nothing covers it by accident. These are the
cheapest place to find real gaps.

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
| §1.1 | L39 | product** and MUST NOT be reintroduced without a deliberate amendment: | gate-enforced | check-removed-features.sh (gate script) |
| §2 | L62 | The words **MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and | **UNMAPPED** | — |
| §4 | L93 | Version 0.1 MUST NOT attempt to provide: | **UNMAPPED** | — |
| §5.2 | L154 | The hardware MUST NOT require any button, jumper, or other component to be added | referenced | device_controls → runtime_failures |
| §5.3 | L176 | file, and test it. It MUST NOT silently fall back to another filesystem or USB | **UNMAPPED** | — |
| §6 | L239 | and Vite output are generated or third-party content and MUST NOT be linted as | **UNMAPPED** | — |
| §7.1 | L261 | through. Firmware MUST preserve it exactly and MUST NOT reorder, sort, or | referenced | storage_sets → measured_user_data_tracks_set_files<br>web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §7.1 | L264 | The user MUST explicitly select the active set. Firmware MUST NOT infer or | referenced | storage_sets → measured_user_data_tracks_set_files<br>web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §8.1 | L295 | The device MUST NOT fall back to an open AP. | referenced | wifi_ap → minimum_credentials_and_existing_event_loop<br>wifi_ap → operation_validation |
| §8.4 | L341 | rather than demanding confirmation unconditionally. That wait MUST NOT | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.4 | L352 | The next macro in the list MUST NOT execute automatically. Advancing is a | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.7 | L405 | It MUST NOT contain: | referenced | storage_package_export → deterministic_export_and_filtering |
| §9.2 | L476 | application MUST NOT fetch remote resources. | gate-enforced | verify-no-remote-assets.sh (gate script) |
| §10.7 | L589 | They MUST NOT be duplicated as inconsistent magic numbers. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.5 | L663 | There is one macro-executor task. HTTP handlers MUST NOT type directly. | gate-enforced | check-layer-boundaries.sh (gate script) |
| §13.2 | L798 | Firmware MUST NOT automatically format either filesystem. | referenced | web_server_adapter_json_static → static_file_selection<br>check-layer-boundaries.sh (gate script)<br>check-mount-policy.sh (gate script) |
| §13.2 | L801 | Web-assets failure MUST NOT expose an unauthenticated fallback UI. | referenced | web_server_adapter_json_static → static_file_selection<br>check-layer-boundaries.sh (gate script)<br>check-mount-policy.sh (gate script) |
| §13.3 | L815 | This is the whole tree: two paths and one object type. There MUST NOT be a | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file)<br>storage_sets → duplicate_index_is_discarded_and_output_cleared<br>web_api_repository_handlers → (file) |
| §13.5 | L858 | not atomic across sets**, and MUST NOT pretend to be: each set file is written | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.5 | L863 | Restore MUST NOT perform the whole rewrite synchronously on the HTTP server task. | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.6 | L890 | Deleting a corrupt file MUST NOT be reported as successful recovery. | referenced | storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space<br>web_api_core → route_parsing |
| §13.7 | L899 | resource metadata. The server MUST NOT silently overwrite a newer edit. | referenced | web_api_repository_handlers → session_json_redaction |
| §14 | L917 | The administrator password MUST NOT be stored in plaintext, nor in any form from | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §14 | L925 | them is confinement rather than hashing: firmware MUST NOT emit either | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §14 | L927 | report. A caller that needs an SSID MUST NOT be handed a copy of the whole | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.1 | L957 | AP startup failure is a visible fatal network state. The firmware MUST NOT | referenced | app_core → normal_failure_matrix<br>wifi_ap → events_and_client_saturation |
| §15.2 | L969 | firmware MUST NOT keep a list, and MUST NOT scan for, rank, or join any network | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L981 | availability a fatal-if-absent property, so it MUST NOT be made to wait on, or | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L986 | ignored: the device continues as access-point only. Firmware MUST NOT treat it | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L987 | as a startup failure, MUST NOT retry it in a way that delays or blocks the rest | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L988 | of startup, and MUST NOT discard the stored credentials because one join | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §16.5 | L1051 | The console MUST NOT expose credentials or secret material even so, because | gate-enforced | check-credential-logging.sh (gate script) |
| §17 | L1154 | `GET /api/v1/backup` MUST NOT let one damaged object make the repository | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1174 | MUST NOT report `200` for a run that failed to write some of them. | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §19 | L1236 | The device MUST NOT require any button, and MUST NOT require hardware to be | referenced | device_controls → runtime_failures |
| §20.1 | L1278 | The project MUST NOT: | **UNMAPPED** | — |
| §21.1 | L1329 | The defect MUST be fixed at its source. It MUST NOT be hidden, suppressed, | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §21.3 | L1363 | The project MUST NOT modify ESP-IDF, managed components, npm dependencies, or | **UNMAPPED** | — |
| §21.4 | L1373 | First-party source and project configuration MUST NOT use warning suppression as | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §26 | L1656 | Deferred features MUST NOT be partially or silently enabled in version 0.1. | gate-enforced | check-removed-features.sh (gate script) |
| §27 | L1679 | MUST NOT be assumed to exist. Implement the pages from this specification until | **UNMAPPED** | — |

## Requirements (`MUST`)

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
| §3 | L70 | The product MUST: | **UNMAPPED** | — |
| §5.1 | L129 | The firmware MUST build against the exact signed ESP-IDF tag: | **UNMAPPED** | — |
| §5.1 | L135 | The build MUST reject an unrecognized ESP-IDF version. Development documentation | **UNMAPPED** | — |
| §5.1 | L136 | and CI MUST clone ESP-IDF recursively from the exact tag rather than a moving | **UNMAPPED** | — |
| §5.2 | L150 | The hardware MUST expose the ESP32-S3 native USB D+ and D- signals. A board with | referenced | device_controls → runtime_failures |
| §5.2 | L158 | SPI PSRAM. The build MUST enable it (`CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`, | referenced | device_controls → runtime_failures |
| §5.2 | L162 | and restore bodies affordable. FreeRTOS task stacks MUST still come from internal | referenced | device_controls → runtime_failures |
| §5.3 | L172 | All dependency resolutions MUST be pinned by committed manifest and lock files. | **UNMAPPED** | — |
| §5.3 | L174 | component version is incompatible with ESP-IDF v5.5.5, the implementation MUST | **UNMAPPED** | — |
| §5.4 | L185 | The frontend MUST use: | **UNMAPPED** | — |
| §5.4 | L193 | The Node.js major version MUST be pinned in the repository. JavaScript package | **UNMAPPED** | — |
| §5.4 | L194 | versions MUST be locked with a committed lockfile. Production assets MUST be | **UNMAPPED** | — |
| §5.4 | L195 | static files and MUST contain no CDN, remote-font, remote-icon, analytics, or | **UNMAPPED** | — |
| §8.4 | L340 | is required, and every confirmation-gated route MUST honour the setting | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.6 | L378 | Deletion MUST: | referenced | storage_sets → create_leaves_no_staging_artifacts<br>storage_sets → crud_ordering_revisions_and_cleanup |
| §8.7 | L393 | A set export MUST be a single versioned JSON package containing: | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L414 | Import MUST validate the entire package, all limits, references, syntax, schema, | referenced | storage_package_export → deterministic_export_and_filtering |
| §9 | L427 | The application MUST be mobile-first and usable from a desktop browser. | **UNMAPPED** | — |
| §9 | L447 | The persistent operational header MUST show: | **UNMAPPED** | — |
| §9.2 | L475 | All application assets MUST be bundled into the web-assets filesystem. The | gate-enforced | verify-no-remote-assets.sh (gate script) |
| §9.3 | L480 | Vite output MUST use content-hashed filenames. JavaScript, CSS, SVG, and other | **UNMAPPED** | — |
| §9.3 | L483 | The server MUST: | **UNMAPPED** | — |
| §10.6 | L565 | The parser MUST consume the entire source. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L566 | Parsing and compilation MUST complete before execution begins. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L567 | Validation errors MUST include byte offset, line, column, error code, and a | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.7 | L588 | Limits MUST be centralized, visible through the API, and tested at boundaries. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L593 | MB against a 512 KiB partition. Firmware MUST enforce the storage limits by | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L595 | count limits, and MUST reject an over-budget write with `507` (§17) rather than | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.1 | L603 | The ESP32-S3 MUST enumerate as a USB HID keyboard using the native USB device | **UNMAPPED** | — |
| §11.1 | L606 | USB descriptors MUST use project-owned manufacturer, product, and serial strings. | **UNMAPPED** | — |
| §11.3 | L630 | After every normal key or chord action, firmware MUST emit a release-all report. | **UNMAPPED** | — |
| §11.3 | L644 | firmware MUST attempt a release-all report and transition the execution to a | **UNMAPPED** | — |
| §11.3 | L645 | terminal state. The executor MUST also clear its internal pressed-key state even | **UNMAPPED** | — |
| §11.5 | L678 | Cancellation MUST use a thread-safe flag, task notification, or equivalent | gate-enforced | check-layer-boundaries.sh (gate script) |
| §11.5 | L679 | bounded mechanism and MUST remain responsive during delay actions. | gate-enforced | check-layer-boundaries.sh (gate script) |
| §12 | L683 | All persistent objects MUST contain: | **UNMAPPED** | — |
| §12.3 | L758 | the index, is a corruption of the index and is handled under §13.6. Firmware MUST | referenced | provisioning_settings → (file)<br>storage_active_set_delete → (file)<br>storage_sets → delete_is_permanent_and_leaves_no_trash<br>web_api_json → settings_update_matrix<br>web_api_repository_handlers → set_routes |
| §13.1 | L782 | Exact sizes are defined in `firmware/partitions.csv` and MUST be validated | **UNMAPPED** | — |
| §13.3 | L805 | The `userdata` partition is **512 KiB**. The layout MUST be flat: one index file | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file)<br>storage_sets → duplicate_index_is_discarded_and_output_cleared<br>web_api_repository_handlers → (file) |
| §13.4 | L835 | Every update MUST: | referenced | storage_atomic → create_enforces_operation_sequence<br>storage_atomic_recovery → (file)<br>storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>storage_parent_sync → (file)<br>storage_parent_sync → rename_failure_on_create_leaves_nothing |
| §13.6 | L889 | The error MUST name the object and MUST be surfaced through the API and the UI. | referenced | storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space<br>web_api_core → route_parsing |
| §14 | L932 | A stored record whose length does not match the current layout MUST be rejected | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L985 | A station join that fails, times out, or is refused MUST be logged and otherwise | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one |
| §16.2 | L1005 | Every mutating request MUST provide a valid CSRF token tied to the session. | **UNMAPPED** | — |
| §16.3 | L1012 | policy. The implementation MUST avoid unbounded per-IP state. | **UNMAPPED** | — |
| §16.4 | L1016 | The HTTP server MUST enforce: | **UNMAPPED** | — |
| §16.5 | L1035 | the device's own SoftAP or, in development builds, a joined network - MUST | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1036 | carry a valid RAM-only session, and every mutation MUST additionally carry a | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1038 | failures MUST be rate-limited. No network-reachable route may mutate device | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1057 | third parties it MUST be excluded from the shipped image, since a shipped | gate-enforced | check-credential-logging.sh (gate script) |
| §17 | L1151 | it, but external behavior and resource boundaries MUST remain equivalent and be | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1159 | A partial backup MUST be self-describing, so it can never be mistaken for a | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1167 | I/O, storage unavailable, timeout) MUST still fail the export, because | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1173 | partial success MUST enumerate which sets were restored and which were not; it | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §19 | L1238 | product. GPIO assignment for the one remaining output MUST be configurable | referenced | device_controls → runtime_failures |
| §19 | L1254 | Cancellation MUST remain available during execution and delay actions, over | referenced | device_controls → runtime_failures |
| §19 | L1267 | Indicator semantics MUST be documented and testable. Failure LEDs do not replace | referenced | device_controls → runtime_failures |
| §20.1 | L1274 | Every operation MUST return, log, or expose an explicit success or failure. | **UNMAPPED** | — |
| §20.2 | L1291 | Logs MUST: | **UNMAPPED** | — |
| §20.3 | L1319 | A downloadable diagnostic report MUST redact secrets and macro source by | referenced | storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space |
| §21.3 | L1349 | The quality gate MUST exclude: | **UNMAPPED** | — |
| §21.3 | L1366 | If a diagnostic originates exclusively in a third-party header, the tool MUST be | **UNMAPPED** | — |
| §21.5 | L1429 | MUST run the authoritative local quality gate. CI MUST call the same command. | **UNMAPPED** | — |
| §21.5 | L1430 | The script MUST fail on the first failed phase or aggregate failures while still | **UNMAPPED** | — |
| §21.5 | L1431 | returning nonzero; it MUST never mask failures. | **UNMAPPED** | — |
| §23 | L1477 | The firmware build MUST fail when the expected web assets are absent, stale | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1480 | The build MUST record: | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1490 | Release builds MUST be reproducible from committed sources and lockfiles. | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §24.1 | L1496 | Tests MUST cover: | **UNMAPPED** | — |
| §24.2 | L1514 | Tests MUST cover: | **UNMAPPED** | — |
| §24.3 | L1533 | Tests MUST cover: | **UNMAPPED** | — |
| §24.4 | L1549 | Tests MUST cover: | **UNMAPPED** | — |
| §24.5 | L1567 | Tests MUST cover: | **UNMAPPED** | — |
| §24.6 | L1586 | At minimum, acceptance testing MUST include: | **UNMAPPED** | — |
