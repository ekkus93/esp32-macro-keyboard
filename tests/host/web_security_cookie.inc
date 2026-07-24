#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"
#include "storage.h"
#include "test_assert.h"
#include "web_content.h"
#include "web_cookie.h"
#include "web_origin.h"
#include "web_static_path.h"

#define TOKEN "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#define TOKEN_UPPER "0123456789ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef"

static void expect_cookie_result(const char *cookie, app_error_code_t expected)
{
    char output[AUTH_TOKEN_HEX_BYTES];
    memset(output, 'x', sizeof(output));
    output[sizeof(output) - 1U] = '\0';
    TEST_CHECK_EQ_INT(expected,
                      web_cookie_extract_session(cookie, output, sizeof(output)));
    if (expected == APP_ERROR_NONE) {
        TEST_CHECK_EQ_STRING(TOKEN, output);
    } else {
        TEST_CHECK_EQ_STRING("", output);
    }
}

static void test_cookie_boundaries(void)
{
    expect_cookie_result("MKSESSION=" TOKEN "; a=1", APP_ERROR_NONE);
    expect_cookie_result("a=1; MKSESSION=" TOKEN "; b=2", APP_ERROR_NONE);
    expect_cookie_result("a=1;\tMKSESSION=" TOKEN, APP_ERROR_NONE);
    expect_cookie_result("XMKSESSION=" TOKEN, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSIONX=" TOKEN, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("mkSession=" TOKEN, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=", APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=abc", APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=" TOKEN "0", APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=" TOKEN_UPPER, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=g123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde",
                         APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=" TOKEN "; MKSESSION=" TOKEN,
                         APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=bad; MKSESSION=" TOKEN, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("a=1; b=2", APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result(NULL, APP_ERROR_INVALID_ARGUMENT);

    char output[AUTH_TOKEN_HEX_BYTES];
    memset(output, 'x', sizeof(output));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      web_cookie_extract_session("MKSESSION=" TOKEN, NULL, sizeof(output)));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      web_cookie_extract_session("MKSESSION=" TOKEN, output, 0U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      web_cookie_extract_session("MKSESSION=" TOKEN,
                                                 output,
                                                 AUTH_TOKEN_HEX_BYTES - 1U));
    TEST_CHECK_EQ_STRING("", output);
}
