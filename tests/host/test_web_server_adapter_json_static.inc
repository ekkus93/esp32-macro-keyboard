static void test_json_envelopes(void)
{
    char output[1024U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_error_json(APP_ERROR_IO,
                                                      "bad \"value\"\n<script>",
                                                      output,
                                                      sizeof(output)));
    TEST_CHECK_EQ_STRING(
        "{\"ok\":false,\"error\":{\"code\":\"io\","
        "\"message\":\"bad \\\"value\\\"\\n<script>\"}}",
        output);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_status_json("0.1.0",
                                                       "v5.5.5",
                                                       "ready",
                                                       "ready",
                                                       4U,
                                                       "running",
                                                       output,
                                                       sizeof(output)));
    TEST_CHECK(strstr(output, "\"wifiClients\":4") != NULL);
    TEST_CHECK(strstr(output, "\"executionState\":\"running\"") != NULL);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_limits_json(output, sizeof(output)));
    TEST_CHECK(strstr(output, "\"compiledActions\":4096") != NULL);
    TEST_CHECK(strstr(output, "\"durationMs\":300000") != NULL);

    char small[8U];
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_adapter_build_error_json(APP_ERROR_IO,
                                                      "too long",
                                                      small,
                                                      sizeof(small)));
    TEST_CHECK_EQ_STRING("", small);
}

typedef struct {
    const char *available_paths[4U];
    size_t available_count;
    char opened_paths[4U][APP_PATH_MAX_BYTES];
    size_t open_count;
    int handles[4U];
} open_fake_t;

static void *open_static_file(void *context, const char *path)
{
    open_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(fake->open_count < 4U);
    TEST_CHECK(strlen(path) < sizeof(fake->opened_paths[0]));
    memcpy(fake->opened_paths[fake->open_count], path, strlen(path) + 1U);
    ++fake->open_count;
    for (size_t index = 0U; index < fake->available_count; ++index) {
        if (strcmp(path, fake->available_paths[index]) == 0) {
            return &fake->handles[index];
        }
    }
    return NULL;
}

static void test_static_file_selection(void)
{
    web_adapter_static_file_t file;
    open_fake_t fake = {
        .available_paths = {"/web/assets/app.js.gz"},
        .available_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_open_static_file("/assets/app.js",
                                                      true,
                                                      "/web",
                                                      open_static_file,
                                                      &fake,
                                                      &file));
    TEST_CHECK(file.compressed);
    TEST_CHECK_EQ_STRING("/web/assets/app.js.gz", file.path);
    TEST_CHECK_EQ_STRING("text/javascript; charset=utf-8", file.content_type);

    fake = (open_fake_t){
        .available_paths = {"/web/index.html"},
        .available_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_open_static_file("/",
                                                      true,
                                                      "/web",
                                                      open_static_file,
                                                      &fake,
                                                      &file));
    TEST_CHECK(!file.compressed);
    TEST_CHECK_EQ_U64(2U, fake.open_count);
    TEST_CHECK_EQ_STRING("no-cache", file.cache_control);

    fake = (open_fake_t){0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_adapter_open_static_file("/api/v1/status",
                                                      false,
                                                      "/web",
                                                      open_static_file,
                                                      &fake,
                                                      &file));
    TEST_CHECK_EQ_U64(0U, fake.open_count);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         web_adapter_open_static_file("/missing.js",
                                                      false,
                                                      "/web",
                                                      open_static_file,
                                                      &fake,
                                                      &file));
}
