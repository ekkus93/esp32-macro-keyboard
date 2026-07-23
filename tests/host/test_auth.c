#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_core.h"
#include "fake_call_log.h"
#include "test_assert.h"

#include "auth_test_fixture.inc"
#include "auth_existing_tests.inc"
#include "auth_additional_password_tests.inc"
#include "auth_additional_session_tests.inc"
#include "auth_additional_rate_tests.inc"

int main(void)
{
    auth_run_existing_tests();
    test_null_inputs_and_known_vector();
    test_password_verification_boundaries();
    test_session_token_formats_and_failures();
    test_complete_rate_limit_matrix();
    puts("authentication tests passed");
    return EXIT_SUCCESS;
}
