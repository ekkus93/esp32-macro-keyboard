static void expect_path_failure(const char *uri)
{
    char output[64U];
    memset(output, 'x', sizeof(output));
    output[sizeof(output) - 1U] = '\0';
    TEST_CHECK(!web_static_uri_normalize(uri, output, sizeof(output)));
    TEST_CHECK_EQ_STRING("", output);
}

static void test_static_path_boundaries(void)
{
    char output[64U];
    TEST_CHECK(web_static_uri_normalize("/", output, sizeof(output)));
    TEST_CHECK_EQ_STRING("/", output);
    TEST_CHECK(web_static_uri_normalize("/assets/app.js?v=1", output, sizeof(output)));
    TEST_CHECK_EQ_STRING("/assets/app.js", output);
    TEST_CHECK(web_static_uri_normalize("/a-b_c.1/file", output, sizeof(output)));
    TEST_CHECK_EQ_STRING("/a-b_c.1/file", output);

    expect_path_failure("/api");
    expect_path_failure("/api?v=1");
    expect_path_failure("/api/v1/status");
    expect_path_failure("/api//status");
    expect_path_failure("/../data");
    expect_path_failure("/a..b");
    expect_path_failure("/...");
    expect_path_failure("/%2e%2e/data");
    expect_path_failure("/%00");
    expect_path_failure("/a\\b");
    expect_path_failure("/a b");
    expect_path_failure("/a\tb");
    expect_path_failure("/a\rb");
    expect_path_failure("/a\nb");
    expect_path_failure("/a#fragment");
    expect_path_failure("");
    expect_path_failure(NULL);

    char exact[5U];
    TEST_CHECK(web_static_uri_normalize("/abc", exact, sizeof(exact)));
    TEST_CHECK_EQ_STRING("/abc", exact);
    char small[4U];
    memset(small, 'x', sizeof(small));
    TEST_CHECK(!web_static_uri_normalize("/abc", small, sizeof(small)));
    TEST_CHECK_EQ_STRING("", small);
    TEST_CHECK(!web_static_uri_normalize("/abc", NULL, sizeof(exact)));
    char one[1U] = {'x'};
    TEST_CHECK(!web_static_uri_normalize("/", one, sizeof(one)));
    TEST_CHECK_EQ_STRING("", one);

    TEST_CHECK(web_static_uri_normalize("/assets/app.js", output, sizeof(output)));
    char absolute[APP_PATH_MAX_BYTES];
    const int written = snprintf(absolute, sizeof(absolute), STORAGE_WEB_MOUNT "%s", output);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(absolute));
    TEST_CHECK(strncmp(absolute, STORAGE_WEB_MOUNT, strlen(STORAGE_WEB_MOUNT)) == 0);
}
