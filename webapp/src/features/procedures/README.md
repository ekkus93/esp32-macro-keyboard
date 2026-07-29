# Procedures Feature

Phase 17.6 connects the active-set procedure library and procedure workflow to
the firmware API.

The library loads bounded procedure summaries and each procedure's independent
progress. A missing progress resource is represented explicitly as **not
started**. Other failures remain visible; invalid or cross-resource data is not
replaced with sample content.

The workflow loads the full procedure and its revision-bound progress, exposes
the current step while keeping every future step visible, and supports review
with previous/next navigation. Instruction completion, checkpoint confirmation,
skip confirmation, and reset use the dedicated progress endpoints. Stale
progress must be reset explicitly and is never silently reconciled.

Macro steps navigate to the Phase 17.7 confirmation boundary with procedure,
step, and macro identifiers. Phase 17.6 never submits an execution, and
completing or skipping a step never automatically sends the next macro.
