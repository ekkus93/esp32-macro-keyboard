/* POST /api/v1/blob (blob_create_handler). */

static void test_blob_create_success_round_trips_bytes(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    blob_test_set_request_id(&fake, "create-req-1");
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");

    uint8_t body[2500];
    for (size_t index = 0U; index < sizeof(body); ++index) {
        body[index] = (uint8_t)((index * 37U + 11U) & 0xFFU);
    }
    fake_httpd_set_body(&fake, body, sizeof(body), 333U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("201 Created", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    blob_test_expect_response_id(&fake, "create-req-1");
    TEST_CHECK(strstr(fake.response_body, "\"sizeBytes\":2500") != NULL);
    TEST_CHECK(strstr(fake.response_body, "\"id\":\"1\"") != NULL);

    const fake_storage_blob_record_t *stored = fake_storage_blob_find(1U);
    TEST_CHECK(stored != NULL);
    TEST_CHECK_EQ_U64(sizeof(body), stored->size);
    TEST_CHECK_EQ_BUFFER(body, stored->data, sizeof(body));
}

static void test_blob_create_rejects_empty_body(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    fake_httpd_bind(&request, &fake, "/api/v1/blob", 0U);

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_create_rejects_oversized_body(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    fake_httpd_bind(&request, &fake, "/api/v1/blob", (size_t)APP_V2_BLOB_MAX_BYTES + 1U);

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("413 Payload Too Large", fake.response_status);
    TEST_CHECK(fake_storage_blob_find(1U) == NULL);
}

static void test_blob_create_rejects_wrong_content_type(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/json");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("415 Unsupported Media Type", fake.response_status);
}

static void test_blob_create_rejects_gzip_with_parameter(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip; charset=binary");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("415 Unsupported Media Type", fake.response_status);
}

static void test_blob_create_rejects_missing_content_type(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("415 Unsupported Media Type", fake.response_status);
}

static void test_blob_create_unauthorized_without_cookie(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_create_unauthorized_with_expired_session(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_blob_test_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_create_rejects_malformed_request_id(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    blob_test_set_request_id(&fake, "bad id with spaces");
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_create_maps_storage_full_at_commit_to_507(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_upload_commit_error = APP_ERROR_STORAGE_FULL;
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("507 Insufficient Storage", fake.response_status);
    TEST_CHECK(fake_storage_blob_find(1U) == NULL);
}

static void test_blob_create_maps_write_failure_and_aborts_upload(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_upload_write_error = APP_ERROR_IO;
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("500 Internal Server Error", fake.response_status);
    TEST_CHECK(fake_storage_blob_find(1U) == NULL);
}

static void test_blob_create_maps_begin_failure_to_service_unavailable(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_upload_begin_error = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_add_request_header(&fake, "Content-Type", "application/gzip");
    uint8_t body[4] = {1U, 2U, 3U, 4U};
    fake_httpd_set_body(&fake, body, sizeof(body), 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob", sizeof(body));

    TEST_CHECK(blob_create_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}
