# Storage Component

This component currently implements:

- separate non-formatting LittleFS mounts for web assets and user data
- typed and bounded path construction
- unique temporary files and verified atomic replacement
- durable transaction manifests with conservative recovery behavior
- a trash directory
- corrupt objects deleted with the failure reported (SPEC 13.6)
- transactional macro-set create, read, list, update, and delete
- optimistic revision conflicts
- host tests for set persistence, interrupted operations, and corruption handling

Missing metadata on an initialized store is treated as corruption; it is not
silently replaced with an empty index.

Macro repositories, procedures, progress, full import/export,
transactional replacement, backup, restore, and operation-complete power-loss
recovery are still open work. Do not describe those paths as implemented until
their code and tests are committed.
