# Macros Feature

Phase 17.5 provides a server-backed active-set macro library and editor.

Implemented behavior:

- list and load set-owned macros from the firmware API;
- create and update with exact expected revisions;
- UTF-8 byte counts for names and source;
- debounced compile-only server validation;
- exact action count, estimated duration, line, column, and byte offset;
- named-key, chord, delay, and literal-brace insertion controls;
- Save disabled unless the current draft has a matching successful validation;
- explicit stale-revision conflict UI that never overwrites the local draft.

Delete, duplicate, reorder, global-macro management, and direct execution remain
separate management or execution slices. Macro syntax and limits remain owned by
the firmware contracts and `docs/MACRO_LANGUAGE.md`; the frontend does not
implement a second parser.
