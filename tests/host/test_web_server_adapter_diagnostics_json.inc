static web_diagnostics_snapshot_t default_diagnostics_snapshot(void)
{
    return (web_diagnostics_snapshot_t){
        .build_id = "abcdef0123456789",
        .firmware_version = "1.2.3",
        .schema_version = 1U,
        .reset_reason = "power-on",
        .uptime_ms = 123456U,
        .free_heap_bytes = 200000U,
        .min_free_heap_bytes = 150000U,
        .controls_stack_high_water_mark = 512U,
        .executor_stack_high_water_mark = 1024U,
        .webfs = {.ok = true, .total_bytes = 1000000U, .used_bytes = 400000U},
        .userdata = {.ok = true, .total_bytes = 2000000U, .used_bytes = 900000U},
        .quarantine = {.ok = true, .count = 2U, .damaged_count = 1U},
        .execution_state = "idle",
        .subsystems =
            {
                {.name = "app_core", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "storage", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "repository", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "auth", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "usb", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "executor", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "controls", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "wifi", .state = SUBSYSTEM_HEALTH_HEALTHY},
                {.name = "http", .state = SUBSYSTEM_HEALTH_HEALTHY},
            },
    };
}

static size_t count_occurrences(const char *haystack, const char *needle)
{
    size_t count = 0U;
    const char *cursor = haystack;
    while ((cursor = strstr(cursor, needle)) != NULL) {
        ++count;
        cursor += strlen(needle);
    }
    return count;
}

static void test_diagnostics_exact_allowed_fields(void)
{
    const web_diagnostics_snapshot_t snapshot = default_diagnostics_snapshot();
    char output[2048U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_diagnostics_json(&snapshot, output, sizeof(output)));

    static const char *const expected_keys[] = {
        "\"buildId\":",  "\"firmwareVersion\":", "\"schemaVersion\":", "\"resetReason\":",
        "\"uptimeMs\":", "\"freeHeapBytes\":",   "\"minFreeHeapBytes\":", "\"stack\":",
        "\"controlsWords\":", "\"executorWords\":", "\"webfs\":", "\"userdata\":",
        "\"quarantine\":", "\"count\":", "\"damagedCount\":", "\"executionState\":",
        "\"subsystems\":", "\"totalBytes\":", "\"usedBytes\":",
    };
    for (size_t index = 0U; index < sizeof(expected_keys) / sizeof(expected_keys[0]); ++index) {
        TEST_CHECK(strstr(output, expected_keys[index]) != NULL);
    }

    /* Exactly the 9 subsystems from FIX1 TODO §19.1, no more, no fewer. */
    TEST_CHECK_EQ_U64(9U, count_occurrences(output, "\"name\":"));
    TEST_CHECK(strstr(output, "\"name\":\"app_core\"") != NULL);
    TEST_CHECK(strstr(output, "\"name\":\"http\"") != NULL);
}

static void test_diagnostics_secret_sentinels_absent(void)
{
    web_diagnostics_snapshot_t snapshot = default_diagnostics_snapshot();
    /* A build id/version deliberately shaped like it could carry something
     * secret must still never introduce forbidden substrings - the struct's
     * fields are the only redaction boundary, not the JSON encoder. */
    (void)snprintf(snapshot.build_id, sizeof(snapshot.build_id), "%s", "deadbeefcafef00d");
    char output[2048U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_diagnostics_json(&snapshot, output, sizeof(output)));

    static const char *const forbidden[] = {
        "password", "passphrase", "setupCode", "setup_code", "sessionToken",
        "session_token", "csrf", "cookie", "macroSource", "macro_source", "\"ssid\"",
    };
    for (size_t index = 0U; index < sizeof(forbidden) / sizeof(forbidden[0]); ++index) {
        TEST_CHECK(strstr(output, forbidden[index]) == NULL);
    }
}

static void test_diagnostics_output_passes_secret_sentinel_scanner(void)
{
    /* FIX1 18.5: production-path proof, not a reimplemented substring check.
     * web_diagnostics_snapshot_t has no field capable of holding a secret, so
     * unlike set export/backup there is no fixture location to embed the
     * sentinel in; this proves the invariant against a real secret that
     * exists elsewhere in the same process (e.g. a real auth session token
     * held by a concurrent request), using the real
     * scripts/check-secret-sentinel.py (all 7 encodings it checks) against
     * the real web_adapter_build_diagnostics_json output. */
    static const char secret_sentinel[] = "phase18_5_diagnostics_secret_Q4tR8wZ1nH6vX3sB";
    const web_diagnostics_snapshot_t snapshot = default_diagnostics_snapshot();
    char output[2048U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_diagnostics_json(&snapshot, output, sizeof(output)));

    const char *outputs[] = {output};
    test_assert_no_secret_sentinel(secret_sentinel, outputs, 1U);
}

static void test_diagnostics_bounded_output(void)
{
    web_diagnostics_snapshot_t snapshot = default_diagnostics_snapshot();
    memset(snapshot.build_id, 'a', sizeof(snapshot.build_id) - 1U);
    snapshot.build_id[sizeof(snapshot.build_id) - 1U] = '\0';
    memset(snapshot.firmware_version, 'b', sizeof(snapshot.firmware_version) - 1U);
    snapshot.firmware_version[sizeof(snapshot.firmware_version) - 1U] = '\0';
    snapshot.uptime_ms = UINT64_MAX;
    snapshot.free_heap_bytes = UINT32_MAX;
    snapshot.min_free_heap_bytes = UINT32_MAX;
    snapshot.controls_stack_high_water_mark = SIZE_MAX;
    snapshot.executor_stack_high_water_mark = SIZE_MAX;
    snapshot.webfs.total_bytes = SIZE_MAX;
    snapshot.webfs.used_bytes = SIZE_MAX;
    snapshot.userdata.total_bytes = SIZE_MAX;
    snapshot.userdata.used_bytes = SIZE_MAX;
    snapshot.quarantine.count = SIZE_MAX;
    snapshot.quarantine.damaged_count = SIZE_MAX;

    char output[2048U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_diagnostics_json(&snapshot, output, sizeof(output)));
    TEST_CHECK(strlen(output) < sizeof(output) - 1U);

    char small[16U];
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_adapter_build_diagnostics_json(&snapshot, small, sizeof(small)));
    TEST_CHECK_EQ_STRING("", small);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_adapter_build_diagnostics_json(NULL, output, sizeof(output)));
}

static void test_diagnostics_failed_query_never_fabricates_data(void)
{
    web_diagnostics_snapshot_t snapshot = default_diagnostics_snapshot();
    snapshot.webfs = (web_diagnostics_capacity_t){.ok = false, .total_bytes = 0U, .used_bytes = 0U};
    snapshot.quarantine =
        (web_diagnostics_quarantine_t){.ok = false, .count = 0U, .damaged_count = 0U};

    char output[2048U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_diagnostics_json(&snapshot, output, sizeof(output)));
    TEST_CHECK(strstr(output, "\"webfs\":{\"ok\":false,\"totalBytes\":0,\"usedBytes\":0}") != NULL);
    TEST_CHECK(strstr(output, "\"quarantine\":{\"ok\":false,\"count\":0,\"damagedCount\":0}") != NULL);
    /* userdata was left healthy, so its data must still come through untouched. */
    TEST_CHECK(strstr(output, "\"userdata\":{\"ok\":true,\"totalBytes\":2000000,\"usedBytes\":900000}") !=
              NULL);
}

static void test_diagnostics_no_false_healthy_state(void)
{
    web_diagnostics_snapshot_t snapshot = default_diagnostics_snapshot();
    /* The defect this exists to prevent: a subsystem reporting FAILED must
     * never read back as healthy once JSON-encoded. */
    snapshot.subsystems[6] = (web_diagnostics_subsystem_t){.name = "controls",
                                                            .state = SUBSYSTEM_HEALTH_FAILED};
    snapshot.subsystems[7] = (web_diagnostics_subsystem_t){.name = "wifi",
                                                            .state = SUBSYSTEM_HEALTH_DEGRADED};

    char output[2048U];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_adapter_build_diagnostics_json(&snapshot, output, sizeof(output)));
    TEST_CHECK(strstr(output, "\"name\":\"controls\",\"state\":\"failed\"") != NULL);
    TEST_CHECK(strstr(output, "\"name\":\"controls\",\"state\":\"healthy\"") == NULL);
    TEST_CHECK(strstr(output, "\"name\":\"wifi\",\"state\":\"degraded\"") != NULL);
}
