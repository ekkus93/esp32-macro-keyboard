static void test_json_envelopes(void) {
    char output[1024U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_error_json(APP_ERROR_IO, "bad \"value\"\n<script>",
                                                      output, sizeof(output)));
    TEST_CHECK_EQ_STRING("{\"error\":{\"code\":\"io\","
                         "\"message\":\"bad \\\"value\\\"\\n<script>\"}}",
                         output);
    TEST_CHECK(strstr(output, "\"ok\"") == NULL);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_adapter_build_limits_json(output, sizeof(output)));
    TEST_CHECK_EQ_STRING("{\"packageNameMaxBytes\":64,\"macroNameMaxBytes\":64,"
                         "\"macroSourceMaxBytes\":4096,\"compiledActionsMax\":4096,"
                         "\"delayDirectiveMaxMs\":10000,\"keyPressMaxMs\":10000,"
                         "\"interKeyMaxMs\":10000,\"estimatedDurationMaxMs\":300000,"
                         "\"executorAbsoluteDeadlineMs\":310000,\"jsonBodyMaxBytes\":8192,"
                         "\"blobMaxBytes\":131072,\"activeSessionsMax\":8,"
                         "\"sessionIdleLifetimeSeconds\":86400,"
                         "\"sessionAbsoluteLifetimeSeconds\":604800,"
                         "\"serialConfirmationTimeoutSeconds\":60,\"adminPasswordMinBytes\":12,"
                         "\"adminPasswordMaxBytes\":128,\"snapshotRetentionTargetMax\":100}",
                         output);
    TEST_CHECK(strstr(output, "\"ok\"") == NULL);
    TEST_CHECK(strstr(output, "\"data\"") == NULL);
    TEST_CHECK(strstr(output, "macrosPerPackage") == NULL);
    TEST_CHECK(strstr(output, "\"packages\"") == NULL);
    TEST_CHECK(strstr(output, "importBytes") == NULL);

    char limits_small[64U];
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_adapter_build_limits_json(limits_small, sizeof(limits_small)));
    TEST_CHECK_EQ_STRING("", limits_small);

    char small[8U];
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, web_adapter_build_error_json(APP_ERROR_IO, "too long",
                                                                          small, sizeof(small)));
    TEST_CHECK_EQ_STRING("", small);
}

typedef struct {
    const char *available_paths[4U];
    size_t available_count;
    char opened_paths[4U][APP_PATH_MAX_BYTES];
    size_t open_count;
    int handles[4U];
} open_fake_t;

static void *open_static_file(void *context, const char *path) {
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

/* SPEC 9.3: "The server MUST: stream files in bounded chunks; set correct
 * content types; set `Content-Encoding: gzip` when serving a gzip variant;
 * cache hashed assets as immutable; serve `index.html` with revalidation or
 * no-cache behavior; reject path traversal; never expose files under the
 * user-data mount through the static-file handler."
 *
 * The two cache policies are not interchangeable: hashed assets are immutable
 * because their name changes when their content does, while index.html keeps
 * the same name forever and must be revalidated or a device would serve a stale
 * application indefinitely after an update. */
/* SPEC 24.4 item: path traversal */
static void test_static_file_selection(void) {
    web_adapter_static_file_t file;
    open_fake_t fake = {
        .available_paths = {"/web/assets/app.js.gz"},
        .available_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_open_static_file("/assets/app.js", true, "/web",
                                                      open_static_file, &fake, &file));
    TEST_CHECK(file.compressed);
    TEST_CHECK_EQ_STRING("/web/assets/app.js.gz", file.path);
    TEST_CHECK_EQ_STRING("text/javascript; charset=utf-8", file.content_type);

    fake = (open_fake_t){
        .available_paths = {"/web/index.html"},
        .available_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_adapter_open_static_file(
                                             "/", true, "/web", open_static_file, &fake, &file));
    TEST_CHECK(!file.compressed);
    TEST_CHECK_EQ_U64(2U, fake.open_count);
    TEST_CHECK_EQ_STRING("no-cache", file.cache_control);

    fake = (open_fake_t){0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_adapter_open_static_file("/api/v1/status", false, "/web",
                                                      open_static_file, &fake, &file));
    TEST_CHECK_EQ_U64(0U, fake.open_count);
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NOT_FOUND,
        web_adapter_open_static_file("/missing.js", false, "/web", open_static_file, &fake, &file));

    /* SPEC 13.2: "Web-assets failure MUST NOT expose an unauthenticated fallback
     * UI." The whole web-assets partition being unavailable is not a special
     * case here -- every open simply fails, including the one for "/". The
     * requirement is that this produces nothing to serve: no handle, no
     * content type, no path left pointing at something a caller might send. A
     * built-in "storage unavailable" page substituted at this point would be
     * exactly the unauthenticated fallback the specification forbids, because
     * the static route is reachable before any session exists. */
    open_fake_t unavailable = {.available_count = 0U};
    memset(&file, 0xff, sizeof(file));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NOT_FOUND,
        web_adapter_open_static_file("/", true, "/web", open_static_file, &unavailable, &file));
    /* Both candidates were tried and both failed: the gzip variant and the
     * plain one. Nothing was invented in their place. */
    TEST_CHECK_EQ_U64(2U, unavailable.open_count);
    TEST_CHECK(file.handle == NULL);
    TEST_CHECK(file.content_type == NULL);
    TEST_CHECK_EQ_STRING("", file.path);
    TEST_CHECK(!file.compressed);
}
