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

Phase 3 will build the opaque byte-oriented snapshot store on top of these generic
primitives. Until that work is implemented and tested, do not describe blob
listing, upload, download, deletion, startup scanning, temporary-file cleanup, or
power-loss recovery as complete.
