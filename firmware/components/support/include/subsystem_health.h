#ifndef SUBSYSTEM_HEALTH_H
#define SUBSYSTEM_HEALTH_H

/* Shared health-state vocabulary for the Phase 19 diagnostics aggregation (FIX1
 * handoff §7.1). Each subsystem keeps its own health struct shaped for its own
 * primary/cleanup errors and resource-ownership fields (see app_operation_result.h,
 * device_controls_health_t, macro_execution_status_t.error/release_error); this
 * header only standardizes the resulting state enum so every subsystem reports it
 * the same way. A subsystem must never report HEALTHY while its own cleanup is
 * incomplete or a resource remains inconsistently owned. */
typedef enum {
    SUBSYSTEM_HEALTH_HEALTHY = 0,
    SUBSYSTEM_HEALTH_DEGRADED,
    SUBSYSTEM_HEALTH_UNAVAILABLE,
    SUBSYSTEM_HEALTH_RECOVERING,
    SUBSYSTEM_HEALTH_FAILED,
} subsystem_health_state_t;

const char *subsystem_health_state_string(subsystem_health_state_t state);

#endif
