# Settings Feature

The settings and storage-diagnostics screens currently provide presentation
scaffolding for startup selection, physical confirmation, storage state, and
quarantine.

Settings retrieval and mutation, password changes, network configuration, restart,
credential reset, factory reset, backup/restore, and live storage diagnostics are
not connected to firmware APIs yet.

Security-sensitive settings must fail closed and surface errors. Destructive
operations require the confirmations defined in `docs/SPEC.md`.
