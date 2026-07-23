# Firmware Main Component

`app_main.c` is the production ESP-IDF entry point. It calls `app_core_start()` and
logs the stable application error when startup fails.

Subsystem initialization, rollback, and runtime ownership do not belong in this
directory. They are implemented in the first-party components under
`firmware/components/`.

The component is compiled with the repository's strict first-party warning policy.
Do not add ignored return values, diagnostic suppression, or fallback startup paths
here.
