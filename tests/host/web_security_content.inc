static void test_content_boundaries(void)
{
    TEST_CHECK(web_accept_encoding_gzip("gzip"));
    TEST_CHECK(web_accept_encoding_gzip("GZIP"));
    TEST_CHECK(web_accept_encoding_gzip("br, gzip"));
    TEST_CHECK(web_accept_encoding_gzip("br,\tgzip ; q=1"));
    TEST_CHECK(web_accept_encoding_gzip("gzip;q=1"));
    TEST_CHECK(web_accept_encoding_gzip("gzip;q=0.1"));
    TEST_CHECK(!web_accept_encoding_gzip("gzip;q=0"));
    TEST_CHECK(!web_accept_encoding_gzip("gzip;Q=0.0"));
    TEST_CHECK(!web_accept_encoding_gzip("gzip;q=0.000"));
    TEST_CHECK(!web_accept_encoding_gzip("xgzip"));
    TEST_CHECK(!web_accept_encoding_gzip("gzipx"));
    TEST_CHECK(!web_accept_encoding_gzip("br"));
    TEST_CHECK(!web_accept_encoding_gzip(""));
    TEST_CHECK(!web_accept_encoding_gzip(NULL));

    TEST_CHECK_EQ_STRING("text/html; charset=utf-8", web_content_type("index.html"));
    TEST_CHECK_EQ_STRING("text/javascript; charset=utf-8", web_content_type("app.js"));
    TEST_CHECK_EQ_STRING("text/css; charset=utf-8", web_content_type("app.css"));
    TEST_CHECK_EQ_STRING("image/svg+xml", web_content_type("icon.svg"));
    TEST_CHECK_EQ_STRING("application/json", web_content_type("data.json"));
    TEST_CHECK_EQ_STRING("image/png", web_content_type("icon.png"));
    TEST_CHECK_EQ_STRING("application/octet-stream", web_content_type("app.js.txt"));
    TEST_CHECK_EQ_STRING("application/octet-stream", web_content_type("APP.JS"));
    TEST_CHECK_EQ_STRING("application/octet-stream", web_content_type("no-extension"));
    TEST_CHECK_EQ_STRING("application/octet-stream", web_content_type(NULL));
}
