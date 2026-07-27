#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth_core.h"
#include "fake_call_log.h"
#include "test_assert.h"

/* The fixture defines the fakes and helpers the suites below depend on, so it
 * must be included first; the blank line keeps it in its own include block so
 * include sorting cannot reorder it after the alphabetized suites. */
#include "auth_test_fixture.inc"

#include "auth_additional_password_tests.inc"
#include "auth_additional_rate_tests.inc"
#include "auth_additional_session_tests.inc"
#include "auth_existing_tests.inc"

int main(void) {
    auth_run_existing_tests();
    test_null_inputs_and_known_vector();
    test_password_verification_boundaries();
    test_password_verify_result_matrix();
    test_session_token_formats_and_failures();
    test_complete_rate_limit_matrix();
    puts("authentication tests passed");
    return EXIT_SUCCESS;
}
