# Execution Feature

The current frontend polls `/api/v1/executions/current`, displays action progress,
moves to the result state on terminal status, and sends authenticated cancellation
requests to `/api/v1/executions/current/cancel`.

Preview, send confirmation, physical-confirmation waiting, and result screens are
present as UI states.

The frontend must never synthesize completion after a request failure.
