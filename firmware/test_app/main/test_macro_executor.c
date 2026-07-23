#include <stdlib.h>
#include <string.h>

#include "macro_executor.h"
#include "unity.h"

TEST_CASE("executor initializes idle and rejects unavailable USB", "[device][executor]") {
    TEST_ASSERT_EQUAL(APP_ERROR_NONE, macro_executor_init());
    const macro_execution_status_t initial = macro_executor_get_status();
    TEST_ASSERT_EQUAL(EXECUTION_IDLE, initial.state);
    TEST_ASSERT_EQUAL(APP_ERROR_NOT_FOUND, macro_executor_cancel());

    macro_execution_request_t request = {0};
    static const char execution_id[] = "123e4567-e89b-42d3-a456-426614174000";
    static const char set_id[] = "123e4567-e89b-42d3-b456-426614174001";
    static const char macro_id[] = "123e4567-e89b-42d3-8456-426614174002";
    memcpy(request.execution_id.value, execution_id, sizeof(execution_id));
    memcpy(request.set_id.value, set_id, sizeof(set_id));
    memcpy(request.macro_id.value, macro_id, sizeof(macro_id));
    request.macro_revision = 1U;
    request.key_press_ms = 8U;
    request.inter_key_ms = 15U;
    request.plan.actions = calloc(1U, sizeof(*request.plan.actions));
    TEST_ASSERT_NOT_NULL(request.plan.actions);
    request.plan.actions[0] = (macro_action_t){
        .type = MACRO_ACTION_KEY,
        .usage = 4U,
    };
    request.plan.action_count = 1U;
    request.plan.estimated_duration_ms = 23U;

    TEST_ASSERT_EQUAL(APP_ERROR_USB_NOT_READY, macro_executor_submit(&request));
    TEST_ASSERT_NOT_NULL(request.plan.actions);
    free(request.plan.actions);
}
