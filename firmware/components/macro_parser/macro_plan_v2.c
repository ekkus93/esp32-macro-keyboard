#include "macro_parser.h"

#include <stdlib.h>

void macro_plan_v2_free(macro_plan_t *plan) {
    if (plan == NULL) {
        return;
    }
    free(plan->actions);
    plan->actions = NULL;
    plan->action_count = 0U;
    plan->estimated_duration_ms = 0U;
}
