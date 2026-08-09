/* GET /api/v1/blob (blob_list_handler). */

static void test_blob_list_success_reflects_seeded_blobs(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    blob_test_set_request_id(&fake, "list-req-1");
    static const uint8_t first[] = {0xAAU, 0xBBU, 0xCCU};
    static const uint8_t second[] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
    fake_storage_blob_seed(1U, first, sizeof(first));
    fake_storage_blob_seed(2U, second, sizeof(second));
    g_fake_storage_blob.partition_total_bytes = 100000U;
    g_fake_storage_blob.partition_used_bytes = 8U;
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    const char *cache_control = fake_httpd_response_header(&fake, "Cache-Control");
    TEST_CHECK(cache_control != NULL);
    TEST_CHECK_EQ_STRING("no-store", cache_control);
    blob_test_expect_response_id(&fake, "list-req-1");
    TEST_CHECK(strstr(fake.response_body, "\"id\":\"1\"") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"sizeBytes\":3") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"id\":\"2\"") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"sizeBytes\":5") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"usedBytes\":8") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"remainingBytes\":99992") != NULL);
}

static void test_blob_list_generates_request_id_when_absent(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    const char *request_id = fake_httpd_response_header(&fake, "X-Request-ID");
    TEST_CHECK(request_id != NULL);
    TEST_CHECK(request_id[0] != '\0');
}

static void test_blob_list_rejects_malformed_request_id(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    blob_test_set_request_id(&fake, "has a space");
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_list_unauthorized_without_cookie(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_list_unauthorized_with_expired_session(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_blob_test_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_list_maps_scan_failure_to_service_unavailable(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_list_error = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

static void test_blob_list_maps_capacity_query_failure_to_internal_error(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_partition_capacity_error = APP_ERROR_IO;
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("500 Internal Server Error", fake.response_status);
}

static void test_blob_list_empty_store_reports_zero_used_bytes(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_list_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK(strstr(fake.response_body, "\"usedBytes\":0") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"blobs\":[]") != NULL);
}
