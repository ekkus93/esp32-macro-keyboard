# Execution Feature

The current frontend polls `/api/v1/executions/current`, displays action progress,
moves to the result state on terminal status, and sends authenticated cancellation
requests to `/api/v1/executions/current/cancel`.

Preview, send confirmation, physical-confirmation waiting, and result screens are
present as UI states. Macro lookup, server-side compilation, execution submission,
and procedure-step completion are not connected yet.

The frontend must never synthesize completion after a request failure and must not
automatically execute the next procedure step.
