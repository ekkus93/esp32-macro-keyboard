#include "subsystem_health.h"

const char *subsystem_health_state_string(subsystem_health_state_t state) {
    switch (state) {
    case SUBSYSTEM_HEALTH_HEALTHY:
        return "healthy";
    case SUBSYSTEM_HEALTH_DEGRADED:
        return "degraded";
    case SUBSYSTEM_HEALTH_UNAVAILABLE:
        return "unavailable";
    case SUBSYSTEM_HEALTH_RECOVERING:
        return "recovering";
    case SUBSYSTEM_HEALTH_FAILED:
        return "failed";
    default:
        return "unknown";
    }
}
