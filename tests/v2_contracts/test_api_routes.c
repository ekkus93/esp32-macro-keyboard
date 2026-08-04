#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "api_routes_v2.h"
#include "app_limits_v2.h"

_Static_assert(sizeof(app_v2_api_routes) / sizeof(app_v2_api_routes[0]) == APP_V2_API_ROUTE_COUNT,
               "v2 API route count differs");

static int failures = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            (void)fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #condition);           \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)

static size_t route_count(void) {
    return sizeof(app_v2_api_routes) / sizeof(app_v2_api_routes[0]);
}

static const app_v2_api_route_contract_t *find_route(const char *id) {
    for (size_t index = 0U; index < route_count(); ++index) {
        if (strcmp(app_v2_api_routes[index].id, id) == 0) {
            return &app_v2_api_routes[index];
        }
    }
    return NULL;
}

static void test_route_surface(void) {
    CHECK(route_count() == (size_t)APP_V2_API_ROUTE_COUNT);
    for (size_t index = 0U; index < route_count(); ++index) {
        const app_v2_api_route_contract_t *route = &app_v2_api_routes[index];
        CHECK(route->id[0] != '\0');
        CHECK(route->method[0] != '\0');
        CHECK(strncmp(route->path, "/api/v1/", sizeof("/api/v1/") - 1U) == 0);
        CHECK(route->authentication[0] != '\0');
        CHECK(route->request_body[0] != '\0');
        CHECK(route->success_status >= UINT16_C(200));
        CHECK(route->error_statuses[0] != '\0');
        CHECK(strstr(route->path, "/package") == NULL);
        CHECK(strstr(route->path, "/macro") == NULL);
        CHECK(strstr(route->path, "/executions") == NULL);
    }
}

static void test_setup_and_login_boundaries(void) {
    const app_v2_api_route_contract_t *setup_get = find_route("setupGet");
    const app_v2_api_route_contract_t *setup_post = find_route("setupPost");
    const app_v2_api_route_contract_t *login = find_route("login");
    CHECK(setup_get != NULL);
    CHECK(setup_post != NULL);
    CHECK(login != NULL);
    if (setup_get != NULL) {
        CHECK(strcmp(setup_get->method, "GET") == 0);
        CHECK(strcmp(setup_get->path, "/api/v1/setup") == 0);
        CHECK(strcmp(setup_get->authentication, "none-unprovisioned-only") == 0);
        CHECK(strcmp(setup_get->request_body, "none") == 0);
        CHECK(setup_get->request_content_type[0] == '\0');
        CHECK(setup_get->request_maximum_bytes[0] == '\0');
        CHECK(strcmp(setup_get->response_content_type, "application/json") == 0);
        CHECK(setup_get->success_status == UINT16_C(200));
    }
    if (setup_post != NULL) {
        CHECK(strcmp(setup_post->method, "POST") == 0);
        CHECK(strcmp(setup_post->request_body, "setupRequest") == 0);
        CHECK(strcmp(setup_post->request_content_type, "application/json") == 0);
        CHECK(strcmp(setup_post->request_maximum_bytes, "jsonBodyMaxBytes") == 0);
        CHECK(setup_post->success_status == UINT16_C(202));
    }
    if (login != NULL) {
        CHECK(strcmp(login->authentication, "none-provisioned-only") == 0);
    }
}

static void test_binary_and_no_content_contracts(void) {
    const app_v2_api_route_contract_t *blob_create = find_route("blobCreate");
    const app_v2_api_route_contract_t *blob_load = find_route("blobLoad");
    const app_v2_api_route_contract_t *blob_delete = find_route("blobDelete");
    const app_v2_api_route_contract_t *password_change = find_route("passwordChange");
    CHECK(blob_create != NULL);
    CHECK(blob_load != NULL);
    CHECK(blob_delete != NULL);
    CHECK(password_change != NULL);
    if (blob_create != NULL) {
        CHECK(strcmp(blob_create->request_content_type, "application/gzip") == 0);
        CHECK(strcmp(blob_create->request_maximum_bytes, "blobMaxBytes") == 0);
        CHECK(blob_create->success_status == UINT16_C(201));
    }
    if (blob_load != NULL) {
        CHECK(strcmp(blob_load->response_content_type, "application/gzip") == 0);
    }
    if (blob_delete != NULL) {
        CHECK(blob_delete->response_content_type[0] == '\0');
        CHECK(blob_delete->success_status == UINT16_C(204));
    }
    if (password_change != NULL) {
        CHECK(password_change->response_content_type[0] == '\0');
        CHECK(password_change->success_status == UINT16_C(204));
    }
}

static void test_api_limit_constants(void) {
    CHECK(APP_V2_JSON_BODY_MAX_BYTES == UINT32_C(8192));
    CHECK(APP_V2_BLOB_MAX_BYTES == UINT32_C(131072));
    CHECK(APP_V2_ACTIVE_SESSIONS_MAX == UINT32_C(8));
    CHECK(APP_V2_SESSION_IDLE_LIFETIME_SECONDS == UINT32_C(86400));
    CHECK(APP_V2_SESSION_ABSOLUTE_LIFETIME_SECONDS == UINT32_C(604800));
    CHECK(APP_V2_SERIAL_CONFIRMATION_TIMEOUT_SECONDS == UINT32_C(60));
    CHECK(APP_V2_ADMIN_PASSWORD_MIN_BYTES == UINT32_C(12));
    CHECK(APP_V2_ADMIN_PASSWORD_MAX_BYTES == UINT32_C(128));
}

int main(void) {
    test_route_surface();
    test_setup_and_login_boundaries();
    test_binary_and_no_content_contracts();
    test_api_limit_constants();

    if (failures != 0) {
        (void)fprintf(stderr, "%d v2 API route assertion(s) failed\n", failures);
        return 1;
    }
    (void)printf("all %u v2 API route contracts passed\n", (unsigned int)APP_V2_API_ROUTE_COUNT);
    return 0;
}