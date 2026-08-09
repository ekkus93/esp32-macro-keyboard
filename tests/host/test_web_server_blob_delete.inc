/* DELETE /api/v1/blob/{blob_id} (blob_delete_handler). */

static void test_blob_delete_success_removes_record(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    blob_test_set_request_id(&fake, "delete-req-1");
    fake_storage_blob_seed(5U, "abcd", 4U);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/5", 0U);

    TEST_CHECK(blob_delete_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("204 No Content", fake.response_status);
    const char *cache_control = fake_httpd_response_header(&fake, "Cache-Control");
    TEST_CHECK(cache_control != NULL);
    TEST_CHECK_EQ_STRING("no-store", cache_control);
    blob_test_expect_response_id(&fake, "delete-req-1");
    TEST_CHECK_EQ_U64(0U, fake.response_body_length);
    TEST_CHECK(fake_storage_blob_find(5U) == NULL);
}

static void test_blob_delete_not_found(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/9", 0U);

    TEST_CHECK(blob_delete_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
}

static void test_blob_delete_rejects_malformed_path(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/00x", 0U);

    TEST_CHECK(blob_delete_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
}

static void test_blob_delete_unauthorized_without_cookie(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/blob/5", 0U);

    TEST_CHECK(blob_delete_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_delete_unauthorized_with_expired_session(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_blob_test_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_bind(&request, &fake, "/api/v1/blob/5", 0U);

    TEST_CHECK(blob_delete_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_blob_delete_maps_backend_failure_to_internal_error(void) {
    fake_httpd_request_t fake;
    httpd_req_t request;
    blob_test_reset(&fake);
    blob_test_authenticate(&fake);
    g_fake_storage_blob.force_delete_error = APP_ERROR_IO;
    fake_httpd_bind(&request, &fake, "/api/v1/blob/5", 0U);

    TEST_CHECK(blob_delete_handler(&request) == ESP_OK);

    TEST_CHECK_EQ_STRING("500 Internal Server Error", fake.response_status);
}
