# Firmware Main Component

`app_main.c` is the production ESP-IDF entry point. It starts the trusted physical
UART console first and fails closed if that prerequisite is unavailable; only then
does it call `app_core_start()`. If application startup fails, the already-running
console remains available for local diagnostics while the stable application error
is logged.

Subsystem initialization, rollback, and runtime ownership do not belong in this
directory. They are implemented in the first-party components under
`firmware/components/`.

The component is compiled with the repository's strict first-party warning policy.
Do not add ignored return values, diagnostic suppression, or fallback startup paths
here.
