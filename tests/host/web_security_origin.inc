static void test_origin_boundaries(void)
{
    TEST_CHECK(web_origin_matches_host("http://192.168.4.1", "192.168.4.1"));
    TEST_CHECK(web_origin_matches_host("http://192.168.4.1:8080", "192.168.4.1:8080"));
    TEST_CHECK(web_origin_matches_host("http://[fe80::1]:8080", "[fe80::1]:8080"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1", "192.168.4.2"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1:80", "192.168.4.1:8080"));
    TEST_CHECK(!web_origin_matches_host("https://192.168.4.1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("HTTP://192.168.4.1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1/", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://user@192.168.4.1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1/path", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1?q=1", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1#x", "192.168.4.1"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1 ", "192.168.4.1 "));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1\t", "192.168.4.1\t"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1\r", "192.168.4.1\r"));
    TEST_CHECK(!web_origin_matches_host("http://192.168.4.1\n", "192.168.4.1\n"));
    TEST_CHECK(!web_origin_matches_host("http://HOST", "host"));
    TEST_CHECK(!web_origin_matches_host("http://host", "HOST"));
    TEST_CHECK(!web_origin_matches_host("http://", ""));
    TEST_CHECK(!web_origin_matches_host(NULL, "host"));
    TEST_CHECK(!web_origin_matches_host("http://host", NULL));
}
