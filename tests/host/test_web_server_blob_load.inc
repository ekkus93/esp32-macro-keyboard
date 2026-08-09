/* GET /api/v1/blob/{blob_id} (blob_load_handler). */

static void test_blob_load_success_streams_exact_bytes_across_chunks(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    blob_test_set_request_id(&fake, "load-req-1");

    uint8_t body[3000];
    for (size_t index = 0U; index < sizeof(body); ++index) {
        body[index] = (uint8_t)((index * 7U + 3U) & 0xFFU);
    }
    fake_storage_blob_seed(42U, body, sizeof(body));
    fake_httpd_bind(&request, &fake, "/api/v1/blob/42", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("application/gzip", fake.response_type);
    const char *cache_control = fake_httpd_response_header(&fake, "Cache-Control");
    TEST_CHECK(cache_control != NULL);
    TEST_CHECK_EQ_STRING("no-store", cache_control);
    blob_test_expect_response_id(&fake, "load-req-1");
    TEST_CHECK(fake.chunk_terminated);
    TEST_CHECK_EQ_U64(sizeof(body), fake.response_body_length);
    TEST_CHECK_EQ_BUFFER(body, fake.response_body, sizeof(body));
}

static void test_blob_load_not_found(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/9", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
}

static void test_blob_load_rejects_malformed_path_leading_zero(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_storage_blob_seed(7U, "x", 1U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/0007", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_load_rejects_malformed_path_zero(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/0", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_load_rejects_malformed_path_non_digit(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/abc", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_load_rejects_path_traversal(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/1/../2", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_load_unauthorized_without_cookie(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/1", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_load_unauthorized_with_expired_session(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_blob_test_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_bind(&request, &fake, "/api/v1/blob/1", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_load_maps_reader_open_failure_to_service_unavailable(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_reader_open_error = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_bind(&request, &fake, "/api/v1/blob/1", 0U);

    TEST_CHECK(blob_load_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}
