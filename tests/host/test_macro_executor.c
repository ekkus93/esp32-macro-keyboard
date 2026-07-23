#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_call_log.h"
#include "macro_executor_engine.h"
#include "macro_limits.h"
#include "test_assert.h"

#include "executor_test_fixture.h"
#include "executor_test_fixture.inc"
#include "executor_validation_tests.inc"
#include "executor_execution_tests.inc"
#include "executor_terminal_tests.inc"

int main(void)
{
    executor_run_validation_tests();
    executor_run_execution_tests();
    executor_run_terminal_tests();
    puts("macro executor tests passed");
    return EXIT_SUCCESS;
}
