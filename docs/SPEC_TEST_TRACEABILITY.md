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
| MUST NOT | 73 | 13 |
| MUST | 186 | 19 |
| **Total** | **259** | **32** |

## Prohibitions (`MUST NOT`) — do these first

A prohibition has no happy path, so nothing covers it by accident. These are the
cheapest place to find real gaps.

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
| §1.1 | L41 | procedures, instruction steps, and checkpoint steps | gate-enforced | check-removed-features.sh (gate script) |
| §1.1 | L42 | per-procedure progress tracking | gate-enforced | check-removed-features.sh (gate script) |
| §1.1 | L43 | global or shared macros; a macro belongs to exactly one set | gate-enforced | check-removed-features.sh (gate script) |
| §1.1 | L44 | any field, screen, or code path specific to Chromebooks, ChromeOS, Debian, or any other particular target machine | gate-enforced | check-removed-features.sh (gate script) |
| §1.1 | L46 | buttons of any kind, and any hardware added to the board | gate-enforced | check-removed-features.sh (gate script) |
| §1.1 | L47 | quarantine or archival of damaged files | gate-enforced | check-removed-features.sh (gate script) |
| §1.1 | L48 | staging, trash, and transaction directories | gate-enforced | check-removed-features.sh (gate script) |
| §2 | L62 | The words **MUST**, **MUST NOT**, **REQUIRED**, **SHOULD**, **SHOULD NOT**, and | **UNMAPPED** | — |
| §4 | L95 | arbitrary Unicode typing | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L96 | any awareness of what the target computer is: no host operating-system detection, no hardware detection, and no behavior conditional on either | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L98 | automatic execution of the next macro in a set | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L99 | guided procedures, instruction steps, checkpoint steps, or progress tracking (see §1.1) | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L101 | global or shared macros (see §1.1) | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L102 | unattended command chains triggered by boot, Wi-Fi connection, or USB connection | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L104 | USB host functionality | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L105 | Bluetooth HID | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L106 | cloud accounts, cloud synchronization, or internet routing | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L107 | ~~station-mode Wi-Fi as a product feature~~ — **amended 2026-08-02.** Station mode was a non-goal, on the reasoning that the SoftAP is the whole product surface and joining a network adds attack surface for no user benefit. It was then explicitly requested: set the network from the serial console, have it persist across a power cycle, and rejoin unaided at boot. Persisting and rejoining are product behaviour, not a development-only console command, so keeping this bullet while §15.2 describes the behaviour would leave the specification contradicting itself. The constraints that made it a non-goal are preserved in §15.2 instead: the access point starts first and unconditionally, a join failure is non-fatal, and at most one network is ever remembered | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L118 | macro-set merge conflict resolution | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L119 | server-side JavaScript, React Server Components, or Node.js on the device | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L120 | TLS termination on the isolated SoftAP | gate-enforced | check-removed-features.sh (gate script) |
| §4 | L121 | automatic filesystem formatting after a mount or integrity failure | gate-enforced | check-removed-features.sh (gate script) |
| §5.2 | L154 | The hardware MUST NOT require any button, jumper, or other component to be added | referenced | device_controls → runtime_failures |
| §5.3 | L176 | file, and test it. It MUST NOT silently fall back to another filesystem or USB | **UNMAPPED** | — |
| §6 | L239 | and Vite output are generated or third-party content and MUST NOT be linted as | **UNMAPPED** | — |
| §7.1 | L261 | through. Firmware MUST preserve it exactly and MUST NOT reorder, sort, or | referenced | storage_sets → measured_user_data_tracks_set_files<br>web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §7.1 | L264 | The user MUST explicitly select the active set. Firmware MUST NOT infer or | referenced | storage_sets → measured_user_data_tracks_set_files<br>web_api_core → route_parsing<br>web_api_repository_handlers → set_delete_and_persistent_readback |
| §8.1 | L295 | The device MUST NOT fall back to an open AP. | referenced | wifi_ap → minimum_credentials_and_existing_event_loop<br>wifi_ap → operation_validation |
| §8.4 | L341 | rather than demanding confirmation unconditionally. That wait MUST NOT | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.4 | L352 | The next macro in the list MUST NOT execute automatically. Advancing is a | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.7 | L407 | AP credentials | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L408 | password verifiers | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L409 | session tokens | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L410 | setup codes | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L411 | device keys | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L412 | other device secrets | referenced | storage_package_export → deterministic_export_and_filtering |
| §9.2 | L476 | application MUST NOT fetch remote resources. | gate-enforced | verify-no-remote-assets.sh (gate script) |
| §10.7 | L589 | They MUST NOT be duplicated as inconsistent magic numbers. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.5 | L663 | There is one macro-executor task. HTTP handlers MUST NOT type directly. | gate-enforced | check-layer-boundaries.sh (gate script) |
| §13.2 | L798 | Firmware MUST NOT automatically format either filesystem. | referenced | web_server_adapter_json_static → static_file_selection<br>check-layer-boundaries.sh (gate script)<br>check-mount-policy.sh (gate script) |
| §13.2 | L801 | Web-assets failure MUST NOT expose an unauthenticated fallback UI. | referenced | web_server_adapter_json_static → static_file_selection<br>check-layer-boundaries.sh (gate script)<br>check-mount-policy.sh (gate script) |
| §13.3 | L815 | This is the whole tree: two paths and one object type. There MUST NOT be a | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file)<br>storage_sets → duplicate_index_is_discarded_and_output_cleared<br>web_api_repository_handlers → (file) |
| §13.5 | L858 | not atomic across sets**, and MUST NOT pretend to be: each set file is written | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.5 | L863 | Restore MUST NOT perform the whole rewrite synchronously on the HTTP server task. | referenced | storage_package_restore → (file)<br>web_api_admin_boundary → restore_failure_is_visible |
| §13.6 | L890 | Deleting a corrupt file MUST NOT be reported as successful recovery. | referenced | management-screens → shows live redacted storage data<br>storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space<br>web_api_core → route_parsing |
| §13.7 | L899 | resource metadata. The server MUST NOT silently overwrite a newer edit. | referenced | web_api_repository_handlers → session_json_redaction |
| §14 | L917 | The administrator password MUST NOT be stored in plaintext, nor in any form from | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §14 | L925 | them is confinement rather than hashing: firmware MUST NOT emit either | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §14 | L927 | report. A caller that needs an SSID MUST NOT be handed a copy of the whole | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.1 | L957 | AP startup failure is a visible fatal network state. The firmware MUST NOT | referenced | app_core → normal_failure_matrix<br>wifi_ap → events_and_client_saturation |
| §15.2 | L969 | firmware MUST NOT keep a list, and MUST NOT scan for, rank, or join any network | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one<br>web_setup → success_requires_code_and_confirmation |
| §15.2 | L981 | availability a fatal-if-absent property, so it MUST NOT be made to wait on, or | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one<br>web_setup → success_requires_code_and_confirmation |
| §15.2 | L986 | ignored: the device continues as access-point only. Firmware MUST NOT treat it | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one<br>web_setup → success_requires_code_and_confirmation |
| §15.2 | L987 | as a startup failure, MUST NOT retry it in a way that delays or blocks the rest | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one<br>web_setup → success_requires_code_and_confirmation |
| §15.2 | L988 | of startup, and MUST NOT discard the stored credentials because one join | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one<br>web_setup → success_requires_code_and_confirmation |
| §16.5 | L1068 | The console MUST NOT expose credentials or secret material even so, because | gate-enforced | check-credential-logging.sh (gate script) |
| §16.6 | L1097 | forgets a password MUST NOT have to choose between recovering the device and | **UNMAPPED** | — |
| §17 | L1203 | `GET /api/v1/backup` MUST NOT let one damaged object make the repository | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1223 | MUST NOT report `200` for a run that failed to write some of them. | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §19 | L1285 | The device MUST NOT require any button, and MUST NOT require hardware to be | referenced | device_controls → runtime_failures |
| §20.1 | L1329 | swallow an `esp_err_t` | **UNMAPPED** | — |
| §20.1 | L1330 | cast away or discard an error result | **UNMAPPED** | — |
| §20.1 | L1331 | return success after partial completion | **UNMAPPED** | — |
| §20.1 | L1332 | log an error and then continue in an invalid state | **UNMAPPED** | — |
| §20.1 | L1333 | substitute empty data after parse failure | **UNMAPPED** | — |
| §20.1 | L1334 | silently retry forever | **UNMAPPED** | — |
| §20.1 | L1335 | silently downgrade authentication, storage, USB, or validation behavior | **UNMAPPED** | — |
| §20.1 | L1336 | use a dangerous fallback merely to keep the application running | **UNMAPPED** | — |
| §21.1 | L1378 | The defect MUST be fixed at its source. It MUST NOT be hidden, suppressed, | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §21.3 | L1412 | The project MUST NOT modify ESP-IDF, managed components, npm dependencies, or | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §21.4 | L1422 | First-party source and project configuration MUST NOT use warning suppression as | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §26 | L1705 | Deferred features MUST NOT be partially or silently enabled in version 0.1. | gate-enforced | check-removed-features.sh (gate script) |
| §27 | L1728 | MUST NOT be assumed to exist. Implement the pages from this specification until | **UNMAPPED** | — |

## Requirements (`MUST`)

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
| §3 | L70 | The product MUST: | **UNMAPPED** | — |
| §5.1 | L129 | The firmware MUST build against the exact signed ESP-IDF tag: | gate-enforced | verify-toolchain.sh (gate script) |
| §5.1 | L135 | The build MUST reject an unrecognized ESP-IDF version. Development documentation | gate-enforced | verify-toolchain.sh (gate script) |
| §5.1 | L136 | and CI MUST clone ESP-IDF recursively from the exact tag rather than a moving | gate-enforced | verify-toolchain.sh (gate script) |
| §5.2 | L150 | The hardware MUST expose the ESP32-S3 native USB D+ and D- signals. A board with | referenced | device_controls → runtime_failures |
| §5.2 | L158 | SPI PSRAM. The build MUST enable it (`CONFIG_SPIRAM`, `CONFIG_SPIRAM_MODE_OCT`, | referenced | device_controls → runtime_failures |
| §5.2 | L162 | and restore bodies affordable. FreeRTOS task stacks MUST still come from internal | referenced | device_controls → runtime_failures |
| §5.3 | L172 | All dependency resolutions MUST be pinned by committed manifest and lock files. | **UNMAPPED** | — |
| §5.3 | L174 | component version is incompatible with ESP-IDF v5.5.5, the implementation MUST | **UNMAPPED** | — |
| §5.4 | L187 | TypeScript | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L188 | React | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L189 | Tailwind CSS | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L190 | Vite | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L191 | the browser Fetch API | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L193 | The Node.js major version MUST be pinned in the repository. JavaScript package | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L194 | versions MUST be locked with a committed lockfile. Production assets MUST be | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §5.4 | L195 | static files and MUST contain no CDN, remote-font, remote-icon, analytics, or | gate-enforced | verify-no-remote-assets.sh (gate script)<br>verify-toolchain.sh (gate script) |
| §8.4 | L340 | is required, and every confirmation-gated route MUST honour the setting | referenced | executor_terminal_tests → terminal_publish_failure_leaves_executor_unavailable |
| §8.6 | L378 | Deletion MUST: | referenced | storage_sets → create_leaves_no_staging_artifacts<br>storage_sets → crud_ordering_revisions_and_cleanup |
| §8.7 | L395 | package format identifier and version | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L396 | the set name and identity | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L397 | the set's macros, in order | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L398 | keyboard-layout requirements | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L399 | integrity metadata | referenced | storage_package_export → deterministic_export_and_filtering |
| §8.7 | L414 | Import MUST validate the entire package, all limits, references, syntax, schema, | referenced | storage_package_export → deterministic_export_and_filtering |
| §9 | L427 | The application MUST be mobile-first and usable from a desktop browser. | referenced | run-browser-tests → (file)<br>spec-screens → (file)<br>spec-screens → SPEC 9 required screens |
| §9 | L449 | device name | referenced | run-browser-tests → (file)<br>spec-screens → (file)<br>spec-screens → SPEC 9 required screens |
| §9 | L450 | active macro set | referenced | run-browser-tests → (file)<br>spec-screens → (file)<br>spec-screens → SPEC 9 required screens |
| §9 | L451 | USB state | referenced | run-browser-tests → (file)<br>spec-screens → (file)<br>spec-screens → SPEC 9 required screens |
| §9 | L452 | access to set switching | referenced | run-browser-tests → (file)<br>spec-screens → (file)<br>spec-screens → SPEC 9 required screens |
| §9 | L453 | access to settings | referenced | run-browser-tests → (file)<br>spec-screens → (file)<br>spec-screens → SPEC 9 required screens |
| §9.2 | L475 | All application assets MUST be bundled into the web-assets filesystem. The | gate-enforced | verify-no-remote-assets.sh (gate script) |
| §9.3 | L480 | Vite output MUST use content-hashed filenames. JavaScript, CSS, SVG, and other | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L485 | stream files in bounded chunks | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L486 | set correct content types | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L487 | set `Content-Encoding: gzip` when serving a gzip variant | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L488 | cache hashed assets as immutable | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L489 | serve `index.html` with revalidation or no-cache behavior | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L490 | reject path traversal | referenced | web_server_adapter_json_static → json_envelopes |
| §9.3 | L491 | never expose files under the user-data mount through the static-file handler | referenced | web_server_adapter_json_static → json_envelopes |
| §10.6 | L565 | The parser MUST consume the entire source. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L566 | Parsing and compilation MUST complete before execution begins. | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.6 | L567 | Validation errors MUST include byte offset, line, column, error code, and a | referenced | macro_parser → error_locations_and_directive_boundaries |
| §10.7 | L588 | Limits MUST be centralized, visible through the API, and tested at boundaries. | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L593 | MB against a 512 KiB partition. Firmware MUST enforce the storage limits by | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §10.7 | L595 | count limits, and MUST reject an over-budget write with `507` (§17) rather than | referenced | storage_macros → missing_set_and_revision_overflow<br>web_api_admin_boundary → (file)<br>web_api_admin_boundary → backup_failure_without_detail_stays_plain |
| §11.1 | L603 | The ESP32-S3 MUST enumerate as a USB HID keyboard using the native USB device | gate-enforced | check-usb-identity.sh (gate script) |
| §11.1 | L606 | USB descriptors MUST use project-owned manufacturer, product, and serial strings. | gate-enforced | check-usb-identity.sh (gate script) |
| §11.3 | L630 | After every normal key or chord action, firmware MUST emit a release-all report. | referenced | executor_terminal_tests → (file)<br>usb_keyboard → report_failure |
| §11.3 | L644 | firmware MUST attempt a release-all report and transition the execution to a | referenced | executor_terminal_tests → (file)<br>usb_keyboard → report_failure |
| §11.3 | L645 | terminal state. The executor MUST also clear its internal pressed-key state even | referenced | executor_terminal_tests → (file)<br>usb_keyboard → report_failure |
| §11.5 | L678 | Cancellation MUST use a thread-safe flag, task notification, or equivalent | gate-enforced | check-layer-boundaries.sh (gate script) |
| §11.5 | L679 | bounded mechanism and MUST remain responsive during delay actions. | gate-enforced | check-layer-boundaries.sh (gate script) |
| §12 | L685 | `schema_version` | referenced | acceptance_reset → (file)<br>storage_object_json → macro_rejects_noncanonical_json |
| §12 | L686 | stable ID | referenced | acceptance_reset → (file)<br>storage_object_json → macro_rejects_noncanonical_json |
| §12 | L687 | revision number | referenced | acceptance_reset → (file)<br>storage_object_json → macro_rejects_noncanonical_json |
| §12 | L688 | creation timestamp or monotonic metadata where available | referenced | acceptance_reset → (file)<br>storage_object_json → macro_rejects_noncanonical_json |
| §12 | L689 | update timestamp or monotonic metadata where available | referenced | acceptance_reset → (file)<br>storage_object_json → macro_rejects_noncanonical_json |
| §12.3 | L758 | the index, is a corruption of the index and is handled under §13.6. Firmware MUST | referenced | app-sets → persists real settings updates<br>provisioning_settings → (file)<br>storage_active_set_delete → (file)<br>storage_sets → delete_is_permanent_and_leaves_no_trash<br>web_api_json → settings_update_matrix<br>web_api_repository_handlers → set_routes |
| §13.1 | L782 | Exact sizes are defined in `firmware/partitions.csv` and MUST be validated | gate-enforced | check-partitions.sh (gate script) |
| §13.3 | L805 | The `userdata` partition is **512 KiB**. The layout MUST be flat: one index file | referenced | storage_mount → unmount_continues_after_one_failure<br>storage_package_restore → (file)<br>storage_sets → (file)<br>storage_sets → duplicate_index_is_discarded_and_output_cleared<br>web_api_repository_handlers → (file) |
| §13.4 | L835 | Every update MUST: | referenced | storage_atomic → create_enforces_operation_sequence<br>storage_atomic_recovery → (file)<br>storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>storage_parent_sync → (file)<br>storage_parent_sync → rename_failure_on_create_leaves_nothing |
| §13.6 | L889 | The error MUST name the object and MUST be surfaced through the API and the UI. | referenced | management-screens → shows live redacted storage data<br>storage_macros → oversized_set_file_is_refused<br>storage_macros → set_local_crud_duplicate_and_order<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space<br>web_api_core → route_parsing |
| §14 | L932 | A stored record whose length does not match the current layout MUST be rejected | referenced | provisioning → corrupt_persisted_records<br>provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → oversized_credentials_are_refused_without_side_effects<br>provisioning → station_credentials_survive_a_power_cycle<br>provisioning → storing_a_network_replaces_the_previous_one |
| §15.2 | L985 | A station join that fails, times out, or is refused MUST be logged and otherwise | referenced | provisioning → load_error_and_uninitialized_calls<br>provisioning → no_stored_network_is_the_initial_state<br>provisioning → storing_a_network_disturbs_nothing_else<br>provisioning → storing_a_network_replaces_the_previous_one<br>web_setup → success_requires_code_and_confirmation |
| §16.2 | L1006 | issued on successful login, and it MUST be `HttpOnly` and `SameSite=Strict`. | referenced | network_security → (file)<br>web_request_policy → success_and_generated_request_id |
| §16.3 | L1030 | policy. The implementation MUST avoid unbounded per-IP state. | referenced | auth_additional_rate_tests → (file) |
| §16.4 | L1036 | route-specific body limits | referenced | web_request_policy → failure_statuses |
| §16.4 | L1037 | header count and size limits where configurable | referenced | web_request_policy → failure_statuses |
| §16.4 | L1038 | JSON nesting and collection limits | referenced | web_request_policy → failure_statuses |
| §16.4 | L1039 | bounded parsing memory | referenced | web_request_policy → failure_statuses |
| §16.4 | L1040 | request timeout | referenced | web_request_policy → failure_statuses |
| §16.4 | L1041 | upload size limit | referenced | web_request_policy → failure_statuses |
| §16.4 | L1042 | filename and ID validation | referenced | web_request_policy → failure_statuses |
| §16.4 | L1043 | correct content type | referenced | web_request_policy → failure_statuses |
| §16.5 | L1053 | the device's own SoftAP or, in development builds, a joined network - MUST | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1054 | carry a valid RAM-only session cookie (§16.2). Authentication failures MUST be | gate-enforced | check-credential-logging.sh (gate script) |
| §16.5 | L1074 | third parties it MUST be excluded from the shipped image, since a shipped | gate-enforced | check-credential-logging.sh (gate script) |
| §16.6 | L1089 | It MUST clear the administrator password verifier and its salt, and the AP SSID | **UNMAPPED** | — |
| §16.6 | L1090 | and passphrase, and it MUST mark the device unprovisioned so first-run setup | **UNMAPPED** | — |
| §16.6 | L1093 | It MUST preserve everything the user did not lose: the device name, the settings | **UNMAPPED** | — |
| §16.6 | L1100 | Each reset increments a credential version, so a device MUST refuse the | **UNMAPPED** | — |
| §17 | L1200 | it, but external behavior and resource boundaries MUST remain equivalent and be | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1208 | A partial backup MUST be self-describing, so it can never be mistaken for a | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1216 | I/O, storage unavailable, timeout) MUST still fail the export, because | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §17 | L1222 | partial success MUST enumerate which sets were restored and which were not; it | referenced | storage_package_backup → backup_output_passes_secret_sentinel_scanner |
| §19 | L1287 | product. GPIO assignment for the one remaining output MUST be configurable | referenced | device_controls → runtime_failures |
| §19 | L1303 | Cancellation MUST remain available during execution and delay actions, over | referenced | device_controls → runtime_failures |
| §19 | L1316 | Indicator semantics MUST be documented and testable. Failure LEDs do not replace | referenced | device_controls → runtime_failures |
| §20.1 | L1323 | Every operation MUST return, log, or expose an explicit success or failure. | **UNMAPPED** | — |
| §20.2 | L1342 | use component tags | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §20.2 | L1343 | identify state transitions | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §20.2 | L1344 | include object or execution IDs where safe | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §20.2 | L1345 | avoid passwords, tokens, raw cookie values, setup codes, and macro text that may contain secrets | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §20.2 | L1347 | use bounded formatting | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §20.2 | L1348 | distinguish user error, recoverable system error, and fatal invariant failure | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §20.3 | L1368 | A downloadable diagnostic report MUST redact secrets and macro source by | referenced | storage_atomic_recovery → stray_temporary_is_removed_at_boot<br>web_api_admin_boundary → storage_snapshot_publishes_remaining_space |
| §21.3 | L1398 | The quality gate MUST exclude: | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §21.3 | L1415 | If a diagnostic originates exclusively in a third-party header, the tool MUST be | gate-enforced | check-static-analysis-policy.sh (gate script) |
| §21.5 | L1478 | MUST run the authoritative local quality gate. CI MUST call the same command. | gate-enforced | check-all.sh (gate script) |
| §21.5 | L1479 | The script MUST fail on the first failed phase or aggregate failures while still | gate-enforced | check-all.sh (gate script) |
| §21.5 | L1480 | returning nonzero; it MUST never mask failures. | gate-enforced | check-all.sh (gate script) |
| §23 | L1526 | The firmware build MUST fail when the expected web assets are absent, stale | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1531 | Git commit | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1532 | dirty/clean state | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1533 | ESP-IDF version | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1534 | managed-component lock hash | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1535 | frontend lockfile hash | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1536 | build type | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1537 | build timestamp where reproducibility policy permits | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §23 | L1539 | Release builds MUST be reproducible from committed sources and lockfiles. | gate-enforced | build-webfs-image.sh (gate script)<br>check-release-budgets.sh (gate script)<br>check-scripts.sh (gate script)<br>generate-flash-manifest.sh (gate script) |
| §24.1 | L1547 | every supported ASCII character | referenced | macro_parser → fuzz_corpus |
| §24.1 | L1548 | shifted punctuation | referenced | macro_parser → fuzz_corpus |
| §24.1 | L1549 | every named key | referenced | macro_parser → printable_ascii |
| §24.1 | L1550 | every allowed modifier combination | referenced | macro_parser → delay_boundaries |
| §24.1 | L1551 | brace escaping | referenced | macro_parser → error_locations_and_directive_boundaries |
| §24.1 | L1552 | newline and tab normalization | referenced | macro_parser → named_keys_and_modifiers |
| §24.1 | L1553 | unknown directives | referenced | macro_parser → case_whitespace_and_line_endings |
| §24.1 | L1554 | malformed chords | referenced | macro_parser → delay_boundaries |
| §24.1 | L1555 | delay boundaries | referenced | macro_parser → timing_boundaries |
| §24.1 | L1556 | source and action limits | referenced | macro_parser → null_empty_and_output_arguments |
| §24.1 | L1557 | exact error offsets | referenced | macro_parser → case_whitespace_and_line_endings |
| §24.1 | L1558 | property/fuzz inputs | referenced | macro_parser → output_plan_reuse_contract |
| §24.1 | L1559 | cancellation-safe compiled plans | referenced | macro_parser → braces_and_character_policy |
| §24.2 | L1565 | create/read/update/delete | referenced | storage_sets → argument_validation |
| §24.2 | L1566 | stale revisions | referenced | storage_sets → repository_deinit_is_a_safe_noop |
| §24.2 | L1567 | short writes | referenced | storage_atomic → create_and_replace |
| §24.2 | L1568 | full filesystem, and rejection of an over-budget write with `507` | referenced | storage_macros → missing_set_and_revision_overflow |
| §24.2 | L1569 | interruption between writing `.tmp` and `rename()`, in both orders | referenced | storage_atomic → short_io_is_completed |
| §24.2 | L1570 | boot cleanup of stray `.tmp` files | referenced | storage_atomic_recovery → (file) |
| §24.2 | L1571 | corrupt JSON, including that the corrupt file is deleted and the failure reported | referenced | storage_sets → set_limit_and_stable_order |
| §24.2 | L1573 | an index naming a set file that is absent, and a set file the index omits | referenced | storage_sets → delete_is_permanent_and_leaves_no_trash |
| §24.2 | L1574 | macro order preserved exactly across write, reboot, export, and restore | referenced | storage_sets → measured_user_data_tracks_set_files |
| §24.2 | L1575 | import as new | referenced | storage_package_import → invalid_arguments_and_collision_do_not_mutate |
| §24.2 | L1576 | replace import | referenced | storage_package_replace → invalid_and_conflict_inputs_do_not_mutate |
| §24.2 | L1577 | partial restore reporting per-set outcomes | referenced | storage_package_restore → concurrency_restore_excludes_mutation |
| §24.2 | L1578 | no-format mount failure | referenced | storage_mount → web_mount_failure |
| §24.3 | L1584 | descriptor enumeration | **UNMAPPED** | — |
| §24.3 | L1585 | ASCII-to-HID mapping | referenced | macro_parser → fuzz_corpus |
| §24.3 | L1586 | press and release sequence | referenced | executor_execution_tests → (file) |
| §24.3 | L1587 | chords | referenced | executor_execution_tests → (file) |
| §24.3 | L1588 | delays | referenced | executor_execution_tests → (file) |
| §24.3 | L1589 | busy rejection | referenced | executor_validation_tests → engine_and_request_validation |
| §24.3 | L1590 | cancel during text | referenced | executor_execution_tests → action_order_delay_and_status_progress |
| §24.3 | L1591 | cancel during delay | referenced | executor_execution_tests → action_order_delay_and_status_progress |
| §24.3 | L1592 | disconnect and suspend | referenced | usb_keyboard → press_reports_and_waits |
| §24.3 | L1593 | timeout | referenced | executor_execution_tests → timestamps_and_current_action_track_execution |
| §24.3 | L1594 | final release-all on every terminal path | referenced | executor_terminal_tests → (file) |
| §24.4 | L1600 | authentication and logout | referenced | auth_existing_tests → derive_failures_zero_outputs |
| §24.4 | L1601 | rate limiting | referenced | auth_existing_tests → session_expiry_and_capacity |
| §24.4 | L1602 | session expiry | referenced | auth_existing_tests → sessions |
| §24.4 | L1603 | session cookie required on every route | **UNMAPPED** | — |
| §24.4 | L1604 | host/origin validation | **UNMAPPED** | — |
| §24.4 | L1605 | body and upload limits | referenced | web_request_policy → failure_statuses |
| §24.4 | L1606 | invalid content type | referenced | web_request_policy → failure_statuses |
| §24.4 | L1607 | path traversal | referenced | web_server_adapter_json_static → json_envelopes |
| §24.4 | L1608 | stale revisions | referenced | web_api_repository_handlers → session_json_redaction |
| §24.4 | L1609 | busy execution | referenced | web_request_policy → get_does_not_require_csrf |
| §24.4 | L1610 | redaction | referenced | app_core → residual_ownership_queries_trigger_cleanup |
| §24.4 | L1611 | import validation | referenced | storage_package_import → (file) |
| §24.4 | L1612 | explicit status codes | referenced | web_request_policy → get_does_not_require_csrf |
| §24.5 | L1618 | every required screen | referenced | spec-screens → ${screen.ordinal}. ${screen.heading} renders at #${screen.hash}<br>spec-screens → 1. first-run setup is shown for an unprovisioned device<br>spec-screens → 10. create and duplicate set are reachable from set management<br>spec-screens → 2. login is shown for a provisioned device with no session<br>spec-screens → SPEC 9 required screens |
| §24.5 | L1619 | active-set visibility | referenced | app-sets → shows live metadata and filters by search |
| §24.5 | L1620 | set switching | referenced | app-sets → shows live metadata and filters by search |
| §24.5 | L1621 | set and macro ordering, including that a reorder round-trips through the API | referenced | set-management → set management |
| §24.5 | L1622 | live validation | referenced | set-management → traps modal focus, closes with Escape, and restores focus |
| §24.5 | L1623 | send preview | referenced | execution-confirmation → disables Send with a visible USB explanation |
| §24.5 | L1624 | disabled Send when USB is unavailable | referenced | execution-confirmation → loads a persisted macro without executing |
| §24.5 | L1625 | progress polling | referenced | app-execution → execution workflow |
| §24.5 | L1626 | cancellation | referenced | app-execution → stops polling after unmount |
| §24.5 | L1627 | import/export/delete confirmations | referenced | set-management → creates a set only after UTF-8 validation succeeds |
| §24.5 | L1628 | stale-edit conflict UI | referenced | app-sets → selects a set with the settings revision and updates the header |
| §24.5 | L1629 | storage error UI | referenced | management-screens → shows live redacted storage data |
| §24.5 | L1630 | keyboard and touch accessibility | referenced | set-management → offers keyboard reorder alternatives and commits exact order |
| §24.5 | L1631 | responsive mobile layout | referenced | run-browser-tests → (file) |
| §24.6 | L1637 | Linux host | **UNMAPPED** | — |
| §24.6 | L1638 | ChromeOS host when available | **UNMAPPED** | — |
| §24.6 | L1639 | Windows host when available | **UNMAPPED** | — |
| §24.6 | L1640 | power-cycle persistence | referenced | acceptance_reset → (file) |
| §24.6 | L1641 | repeated USB reconnects | **UNMAPPED** | — |
| §24.6 | L1642 | repeated AP reconnects | **UNMAPPED** | — |
| §24.6 | L1643 | a full set of macros sent in order against a harmless text target | **UNMAPPED** | — |
| §24.6 | L1644 | cancellation over both the API and the `cancel` console command | **UNMAPPED** | — |
| §24.6 | L1645 | credential reset | referenced | acceptance_reset → (file) |
| §24.6 | L1646 | factory reset | referenced | acceptance_reset → (file) |
| §24.6 | L1647 | user-data preservation across firmware slot switch | **UNMAPPED** | — |
