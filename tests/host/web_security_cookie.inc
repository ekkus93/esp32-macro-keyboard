#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"
#include "storage.h"
#include "test_assert.h"
#include "web_content.h"
#include "web_cookie.h"
#include "web_static_path.h"

#define TOKEN "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
#define TOKEN_UPPER "0123456789ABCDEF0123456789abcdef0123456789abcdef0123456789abcdef"

static void expect_cookie_result(const char *cookie, app_error_code_t expected) {
    char output[AUTH_TOKEN_HEX_BYTES];
    memset(output, 'x', sizeof(output));
    output[sizeof(output) - 1U] = '\0';
    TEST_CHECK_EQ_INT(expected, web_cookie_extract_session(cookie, output, sizeof(output)));
    if (expected == APP_ERROR_NONE) {
        TEST_CHECK_EQ_STRING(TOKEN, output);
    } else {
        TEST_CHECK_EQ_STRING("", output);
    }
}

static void test_cookie_boundaries(void) {
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
    expect_cookie_result(
        "MKSESSION=g123456789abcdef0123456789abcdef0123456789abcdef0123456789abcde",
        APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=" TOKEN "; MKSESSION=" TOKEN, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("MKSESSION=bad; MKSESSION=" TOKEN, APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result("a=1; b=2", APP_ERROR_AUTH_REQUIRED);
    expect_cookie_result(NULL, APP_ERROR_INVALID_ARGUMENT);

    char output[AUTH_TOKEN_HEX_BYTES];
    memset(output, 'x', sizeof(output));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      web_cookie_extract_session("MKSESSION=" TOKEN, NULL, sizeof(output)));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      web_cookie_extract_session("MKSESSION=" TOKEN, output, 0U));
    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        web_cookie_extract_session("MKSESSION=" TOKEN, output, AUTH_TOKEN_HEX_BYTES - 1U));
    TEST_CHECK_EQ_STRING("", output);
}

/* web_cookie_build_session_header() is the pure half of send_login_accepted()'s
 * (web_server_login.c) Set-Cookie construction -- see that function's header
 * comment. Covering it here, and round-tripping through
 * web_cookie_extract_session() above, is the host-testable proof for the
 * cookie string formatting SPEC_V2 13.5/V2-051 requires; login_handler()
 * itself stays untestable on host (it needs a live httpd_req_t). */
static void test_cookie_build_session_header(void) {
    char header[160U];

    memset(header, 'x', sizeof(header));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_cookie_build_session_header(TOKEN, header, sizeof(header)));
    TEST_CHECK_EQ_STRING("MKSESSION=" TOKEN "; HttpOnly; SameSite=Strict; Path=/", header);

    /* Round-trips through the extraction side above. */
    char extracted[AUTH_TOKEN_HEX_BYTES];
    memset(extracted, 'x', sizeof(extracted));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_cookie_extract_session(header, extracted, sizeof(extracted)));
    TEST_CHECK_EQ_STRING(TOKEN, extracted);

    memset(header, 'x', sizeof(header));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_cookie_build_session_header(NULL, header, sizeof(header)));
    TEST_CHECK_EQ_STRING("", header);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_cookie_build_session_header(TOKEN, NULL, sizeof(header)));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_cookie_build_session_header(TOKEN, header, 0U));

    memset(header, 'x', sizeof(header));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_cookie_build_session_header(TOKEN_UPPER, header, sizeof(header)));
    TEST_CHECK_EQ_STRING("", header);
    memset(header, 'x', sizeof(header));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_cookie_build_session_header("short", header, sizeof(header)));
    TEST_CHECK_EQ_STRING("", header);

    /* A buffer that fits the token but not the full cookie attributes fails
     * cleanly rather than truncating -- login_handler's fixed 160-byte stack
     * buffer relies on this. */
    char tiny[16U];
    memset(tiny, 'x', sizeof(tiny));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_cookie_build_session_header(TOKEN, tiny, sizeof(tiny)));
    TEST_CHECK_EQ_STRING("", tiny);
}
