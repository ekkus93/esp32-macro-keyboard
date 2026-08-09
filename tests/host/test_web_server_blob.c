/* HTTP-adapter-level tests for the four blob route handlers in
 * firmware/components/web_server/web_server_blob.c: GET/POST /api/v1/blob
 * and GET/DELETE /api/v1/blob/{blob_id} (SPEC_V2 13.7/13.8, TODO_V2 V2-053 /
 * V2-057). Unlike the other web_server tests in this suite, these call the
 * real httpd_req_t-taking handler functions -- exercised against a fake
 * esp_http_server.h (tests/host/fakes/esp_http_server_stub, fake_httpd.c)
 * standing in for the real ESP-IDF httpd, and a fake storage_blob backend
 * (fake_storage_blob.c) standing in for storage's NVS-dependent production
 * wrappers, which do not link on a host build. See those files' header
 * comments for exactly why. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "fake_httpd.h"
#include "fake_storage_blob.h"
#include "storage.h"
#include "storage_blob.h"
#include "test_assert.h"
#include "web_api_core.h"
#include "web_api_response.h"
#include "web_http_status.h"
#include "web_server_internal.h"

/* clang-format off
 * Order-dependent source fragments, not headers: the fixture defines the
 * fakes/doubles the suites below depend on, so it must come first. */
#include "test_web_server_blob_fixture.inc"

#include "test_web_server_blob_create.inc"
#include "test_web_server_blob_delete.inc"
#include "test_web_server_blob_list.inc"
#include "test_web_server_blob_load.inc"
// clang-format on

int main(void) {
    test_blob_list_success_reflects_seeded_blobs();
    test_blob_list_generates_request_id_when_absent();
    test_blob_list_rejects_malformed_request_id();
    test_blob_list_unauthorized_without_cookie();
    test_blob_list_unauthorized_with_expired_session();
    test_blob_list_maps_scan_failure_to_service_unavailable();
    test_blob_list_maps_capacity_query_failure_to_internal_error();
    test_blob_list_empty_store_reports_zero_used_bytes();

    test_blob_create_success_round_trips_bytes();
    test_blob_create_rejects_empty_body();
    test_blob_create_rejects_oversized_body();
    test_blob_create_rejects_wrong_content_type();
    test_blob_create_rejects_gzip_with_parameter();
    test_blob_create_rejects_missing_content_type();
    test_blob_create_unauthorized_without_cookie();
    test_blob_create_unauthorized_with_expired_session();
    test_blob_create_rejects_malformed_request_id();
    test_blob_create_maps_storage_full_at_commit_to_507();
    test_blob_create_maps_write_failure_and_aborts_upload();
    test_blob_create_maps_begin_failure_to_service_unavailable();

    test_blob_load_success_streams_exact_bytes_across_chunks();
    test_blob_load_not_found();
    test_blob_load_rejects_malformed_path_leading_zero();
    test_blob_load_rejects_malformed_path_zero();
    test_blob_load_rejects_malformed_path_non_digit();
    test_blob_load_rejects_path_traversal();
    test_blob_load_unauthorized_without_cookie();
    test_blob_load_unauthorized_with_expired_session();
    test_blob_load_maps_reader_open_failure_to_service_unavailable();

    test_blob_delete_success_removes_record();
    test_blob_delete_not_found();
    test_blob_delete_rejects_malformed_path();
    test_blob_delete_unauthorized_without_cookie();
    test_blob_delete_unauthorized_with_expired_session();
    test_blob_delete_maps_backend_failure_to_internal_error();

    fake_storage_blob_reset();
    puts("web server blob route tests passed");
    return 0;
}
