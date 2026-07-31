#!/usr/bin/env python3
from pathlib import Path


def replace_if_needed(text: str, old: str, new: str, description: str) -> str:
    if new in text:
        return text
    if old not in text:
        raise SystemExit(f"{description} not found")
    return text.replace(old, new, 1)


todo_path = Path(
    "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md"
)
todo = todo_path.read_text(encoding="utf-8")
todo = replace_if_needed(
    todo,
    """### 18.4 Implement full backup and restore

- [ ] backup all sets, global macros, procedures, and optional progress;
- [ ] exclude credentials and sessions;
- [ ] restore all-or-nothing;
- [ ] require physical/admin confirmation;
- [ ] test storage full during staging;
- [ ] test power loss after every phase.
""",
    """### 18.4 Implement full backup and restore

- [x] backup all sets, global macros, procedures, and optional progress;
- [x] exclude credentials and sessions;
- [x] restore all-or-nothing;
- [x] require physical/admin confirmation;
- [x] test storage full during staging;
- [x] test power loss after every phase.

Implemented: full backup serializes every logical repository object from one
locked snapshot, remains bounded by `APP_IMPORT_PACKAGE_MAX_BYTES`, and excludes
provisioning, credentials, sessions, CSRF material, encryption keys, quarantine,
schema markers, and transaction evidence by construction. Restore validates the
complete package before mutation, stages and validates a full replacement tree,
and activates only `set-index.json`, `sets/`, and `global/` through a durable
six-phase restore transaction. Startup resolves restore manifests before ordinary
set transactions. Host tests cover every durable phase, partial renames,
idempotent recovery, contradictory evidence, deterministic `STORAGE_FULL` during
staging, and preservation of the complete old repository. The API requires an
authenticated administrator session, CSRF, same-origin policy, and physical
confirmation. The frontend adds strict file validation, the exact typed phrase
`RESTORE FULL BACKUP`, visible device-confirmation state, and a mandatory reload
after success.
""",
    "Phase 18.4 TODO block",
)
todo = replace_if_needed(
    todo,
    """**Implemented:** every enabled management control performs a real request or
navigation. Set create, edit, duplicate, keyboard-accessible reorder, and
guarded delete use live revisioned APIs. Settings, storage health, redacted
quarantine evidence, restart, settings reset, and factory reset use strict
response guards and visible physical-confirmation waits. Import, replace,
export, backup, and restore are honest disabled boundaries because their
all-or-nothing package services remain Phase 18; the frontend never simulates
success or sends an unsupported mutation.
""",
    """**Implemented:** every enabled management control performs a real request or
navigation. Set create, edit, duplicate, keyboard-accessible reorder, and
guarded delete use live revisioned APIs. Settings, storage health, redacted
quarantine evidence, restart, settings reset, and factory reset use strict
response guards and visible physical-confirmation waits. Deterministic set
export, transactional replacement, full backup, and all-or-nothing restore now
use the completed Phase 18 services. Import-as-new remains an honest disabled
boundary until its identity-rewrite transaction is implemented; the frontend
never simulates success or sends an unsupported mutation.
""",
    "Phase 17.9 implementation paragraph",
)
todo_path.write_text(todo, encoding="utf-8")

progress_path = Path(
    "docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md"
)
progress = progress_path.read_text(encoding="utf-8")
progress = replace_if_needed(
    progress,
    "| 18 | Import / export / backup / restore | in progress (§18.1–18.2 complete; §18.3–18.5 remain) |",
    "| 18 | Import / export / backup / restore | in progress (§18.1–18.4 complete; §18.5 remains) |",
    "Phase 18 progress row",
)
progress = replace_if_needed(
    progress,
    """  Host Tests run `30455828432` passed static checks, unit tests, coverage,
  sanitizers, and native tests. Browser Tests run `30455823494` built the
  production bundle and passed the full Chrome workflow.
""",
    """  Host Tests run `30455828432` passed static checks, unit tests, coverage,
  sanitizers, and native tests. Browser Tests run `30455823494` built the
  production bundle and passed the full Chrome workflow. Phase 18 later enabled
  deterministic set export, transactional replacement, full backup, and
  all-or-nothing restore; import-as-new remains the only package UI boundary.
""",
    "Phase 17 progress paragraph",
)
entry = """
- Phase 18.4 full backup and restore — complete through `991cabe`, with API
  documentation synchronized in `25edd78`. Backup snapshots every set, local and
  global macro, procedure, ordering record, and optional progress under one
  repository lock; serialized output is bounded, deterministic, self-validated,
  and excludes all credential/session/provisioning/encryption stores. Restore
  prevalidates the entire backup, writes a durable manifest, materializes and
  validates a complete staged repository, then activates only the logical
  repository roots all-or-nothing. Startup restore recovery runs before ordinary
  transactions and fails closed on multiple or mixed manifests. The six-phase
  real-filesystem matrix covers every interruption point and partial rename;
  linker-level fault injection proves `APP_ERROR_STORAGE_FULL` during staged
  `set.json` write rolls back with the old repository and schema marker intact and
  no transaction, staging, or trash evidence left. Authenticated/CSRF-protected
  API routes require physical confirmation for restore, and the frontend requires
  strict 512 KiB package validation plus `RESTORE FULL BACKUP` before posting and
  reloading. Permanent Host Tests run `30611750909` passed all five jobs on
  `991cabefcee46ae82fbebb67ee326480b626d1d5`.

"""
if entry.strip() not in progress:
    marker = "## Completed tasks (commit evidence)\n"
    if marker not in progress:
        raise SystemExit("completed-task marker not found")
    progress = progress.replace(marker, marker + entry, 1)
progress_path.write_text(progress, encoding="utf-8")
