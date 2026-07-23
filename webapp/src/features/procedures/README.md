# Procedures Feature

Procedure list, execution, instruction, and editor screens currently model the
required navigation and explicit per-step workflow.

Persisted procedure CRUD, step ordering, manual checkpoint completion, progress,
skip/reset behavior, and macro-reference validation are not connected to firmware
APIs yet. The current representative Chromebook procedure is sample UI data, not a
stored procedure.

Procedure steps must remain explicit. Completing one step must never automatically
send the next macro.
