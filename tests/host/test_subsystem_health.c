#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "subsystem_health.h"
#include "test_assert.h"

static void test_every_state_has_a_distinct_name(void) {
    static const subsystem_health_state_t states[] = {
        SUBSYSTEM_HEALTH_HEALTHY,    SUBSYSTEM_HEALTH_DEGRADED, SUBSYSTEM_HEALTH_UNAVAILABLE,
        SUBSYSTEM_HEALTH_RECOVERING, SUBSYSTEM_HEALTH_FAILED,
    };
    for (size_t index = 0U; index < sizeof(states) / sizeof(states[0]); ++index) {
        const char *name = subsystem_health_state_string(states[index]);
        TEST_CHECK(name != NULL);
        TEST_CHECK(strcmp(name, "unknown") != 0);
        for (size_t other = 0U; other < index; ++other) {
            TEST_CHECK(strcmp(name, subsystem_health_state_string(states[other])) != 0);
        }
    }
}

static void test_healthy_string(void) {
    TEST_CHECK_EQ_STRING("healthy", subsystem_health_state_string(SUBSYSTEM_HEALTH_HEALTHY));
}

static void test_unknown_value_is_safe(void) {
    /* Out-of-range values must not crash and must fall through to "unknown". */
    TEST_CHECK_EQ_STRING("unknown", subsystem_health_state_string((subsystem_health_state_t)99));
}

int main(void) {
    test_every_state_has_a_distinct_name();
    test_healthy_string();
    test_unknown_value_is_safe();
    puts("subsystem health tests passed");
    return EXIT_SUCCESS;
}
