# Macros Feature

The macro library and editor screens currently provide presentation scaffolding,
local source editing, and a byte-count display.

Persisted macro listing, create/update/delete, ordering, duplication, live parser
validation, favorites, global macros, and direct execution are not connected to
firmware APIs yet. Buttons without backend behavior must remain visibly incomplete
rather than reporting success.

Macro syntax and limits must come from the firmware contracts and
`docs/MACRO_LANGUAGE.md`; the frontend must not invent a second grammar.
