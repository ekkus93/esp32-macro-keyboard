# Storage Component

This component currently provides generic storage infrastructure retained for the
v2 rebuild:

- separate LittleFS mounts for web assets and user data;
- fail-visible mounting without format-on-failure;
- bounded filesystem wrappers;
- generic atomic file replacement primitives;
- storage mount state, health, and partition-capacity reporting.

The component does not own packages, macros, repository JSON, revisions, indexes,
backups, restores, imports, exports, or active-package state. Those retired v1
responsibilities must not return.

V2-030 now provides the opaque snapshot-store foundation: `/data/repository/`,
fixed-width numeric `.gz` names, startup scanning, invalid-name accounting,
newest-first numeric ordering, and next-ID reconciliation against the optional NVS
counter and existing final files.

Later Phase 3 tasks still own atomic upload, list/download/delete APIs, temporary-file
cleanup, capacity proof, and physical power-loss validation. Do not describe those
behaviors as complete until their individual TODO items and evidence are closed.
