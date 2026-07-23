#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"
#include "test_assert.h"
#include "web_content.h"
#include "web_cookie.h"
#include "web_origin.h"
#include "web_static_path.h"

#define TOKEN "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

static void test_cookie(void)
{
    char output[AUTH_TOKEN_HEX_BYTES];
    TEST_CHECK(web_cookie_extract_session("MKSESSION=" TOKEN, output, sizeof(output)) ==
               APP_ERROR_NONE);
    TEST_CHECK_EQ_STRING(TOKEN, output);
    TEST_CHECK(web_cookie_extract_session("a=1; MKSESSION=" TOKEN "; b=2",
                                          output,
                                          sizeof(output)) == APP_ERROR_NONE);
    TEST_CHECK_EQ_STRING(TOKEN, output);
    TEST_CHECK(web_cookie_extract_session("XMKSESSION=" TOKEN,
                                          output,
                                          sizeof(output)) == APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK_EQ_STRING("", output);
    TEST_CHECK(web_cookie_extract_session("MKSESSION=" TOKEN "; MKSESSION=" TOKEN,
                                          output,
                                          sizeof(output)) == APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK_EQ_STRING("", output);
    TEST_CHECK(web_cookie_extract_session("MKSESSION=ABCDEF", output, sizeof(output)) ==
               APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK(web_cookie_extract_session(NULL, output, sizeof(output)) ==
               APP_ERROR_INVALID_ARGUMENT);
}

static void test_origin(void)
{
    TEST_CHECK(web_origin_matches_host("http://192.168.4.1", "192.168.4.1"));
    TEST_CHECK(web_origin_matches_host("http://192.168.4.1:8080", "192.168.4.1:8080"));
    TEST_CHECK(!web_origin_matches_host("https://192.168.4.1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1/", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://user@192.168.4.1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1?q=1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://HOST", "host"));
    TEST_CHECK(!web_origin_matches_host(NULL, "host"));
}

static void test_static_path(void)
{
    char output[32U];
    TEST_CHECK(web_static_uri_normalize("/", output, sizeof(output)));
    TEST_CHECK_EQ_STRING("/", output);
    TEST_CHECK(web_static_uri_normalize("/assets/app.js?v=1", output, sizeof(output)));
    TEST_CHECK_EQ_STRING("/assets/app.js", output);
    TEST_CHECK(!web_static_uri_normalize("/api/v1/status", output, sizeof(output)));
    TEST_CHECK(!web_static_uri_normalize("/api", output, sizeof(output)));
    TEST_CHECK(!web_static_uri_normalize("/../data", output, sizeof(output)));
    TEST_CHECK(!web_static_uri_normalize("/a..b", output, sizeof(output)));
    TEST_CHECK(!web_static_uri_normalize("/%2e%2e/data", output, sizeof(output)));
    TEST_CHECK(!web_static_uri_normalize("/a\\b", output, sizeof(output)));
    TEST_CHECK(!web_static_uri_normalize("/a b", output, sizeof(output)));
    char exact[5U];
    TEST_CHECK(web_static_uri_normalize("/abc", exact, sizeof(exact)));
    char small[4U];
    TEST_CHECK(!web_static_uri_normalize("/abc", small, sizeof(small)));
    TEST_CHECK_EQ_STRING("", small);
}

static void test_content(void)
{
    TEST_CHECK(web_accept_encoding_gzip("gzip"));
    TEST_CHECK(web_accept_encoding_gzip("br, gzip"));
    TEST_CHECK(web_accept_encoding_gzip("gzip;q=1"));
    TEST_CHECK(!web_accept_encoding_gzip("gzip;q=0"));
    TEST_CHECK(!web_accept_encoding_gzip("gzip;q=0.0"));
    TEST_CHECK(!web_accept_encoding_gzip("xgzip"));
    TEST_CHECK(!web_accept_encoding_gzip(NULL));
    TEST_CHECK_EQ_STRING("text/html; charset=utf-8", web_content_type("index.html"));
    TEST_CHECK_EQ_STRING("text/javascript; charset=utf-8", web_content_type("app.js"));
    TEST_CHECK_EQ_STRING("text/css; charset=utf-8", web_content_type("app.css"));
    TEST_CHECK_EQ_STRING("image/svg+xml", web_content_type("icon.svg"));
    TEST_CHECK_EQ_STRING("application/json", web_content_type("data.json"));
    TEST_CHECK_EQ_STRING("image/png", web_content_type("icon.png"));
    TEST_CHECK_EQ_STRING("application/octet-stream", web_content_type("app.js.txt"));
}

int main(void)
{
    test_cookie();
    test_origin();
    test_static_path();
    test_content();
    puts("web security tests passed");
    return EXIT_SUCCESS;
}
