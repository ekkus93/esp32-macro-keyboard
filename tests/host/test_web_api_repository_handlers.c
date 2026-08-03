#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_model.h"
#include "provisioning.h"
#include "storage_object_json.h"
#include "storage_repository.h"
#include "storage_repository_lock.h"
#include "test_assert.h"
#include "test_temp_dir.h"
#include "web_api_handler_common.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define SET_DUPLICATE_ID "11111111-1111-4111-8111-222222222222"
#define MACRO_ID "22222222-2222-4222-8222-222222222222"
#define MACRO_DUPLICATE_ID "22222222-2222-4222-8222-333333333333"

typedef app_error_code_t (*handler_fn_t)(const web_api_call_t *, web_api_response_t *);

static provisioning_settings_t settings_store;

app_error_code_t provisioning_settings_read(provisioning_settings_t *out_settings) {
    if (out_settings == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_settings = settings_store;
    return APP_ERROR_NONE;
}

app_error_code_t provisioning_settings_update(const provisioning_settings_t *replacement,
                                              uint32_t expected_revision,
                                              provisioning_settings_t *out_committed) {
    if (replacement == NULL || out_committed == NULL || expected_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (settings_store.revision != expected_revision ||
        replacement->revision != expected_revision) {
        return APP_ERROR_CONFLICT;
    }
    settings_store = *replacement;
    ++settings_store.revision;
    *out_committed = settings_store;
    return APP_ERROR_NONE;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    /* SPEC 13.3: the index file and the sets directory are the whole tree.
     * staging/, trash/, and transactions/ were created here until Phase 3
     * removed the subsystems that used them; leaving them behind would let a
     * regression that wrote to a forbidden path go unnoticed. */
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/package",
    };
    for (size_t index = 0U; index < sizeof(paths) / sizeof(paths[0]); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
    settings_store = (provisioning_settings_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 1U,
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
}

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static macro_package_t make_package(void) {
    macro_package_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_ID),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Handler Set") > 0);
    return set;
}

static macro_t make_macro(const char *macro_id) {
    static const char source[] = "ab";
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(macro_id),
        .revision = 1U,
        .set_id = uuid(SET_ID),
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
        .source_length = sizeof(source) - 1U,
    };
    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "Handler Macro") > 0);
    macro.source = malloc(sizeof(source));
    TEST_CHECK(macro.source != NULL);
    memcpy(macro.source, source, sizeof(source));
    return macro;
}

static char *serialize_package(const macro_package_t *set) {
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_package_json(set, &json, &length));
    TEST_CHECK(json != NULL);
    TEST_CHECK(length == strlen(json));
    return json;
}

static char *serialize_macro(const macro_t *macro) {
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_macro_json(macro, &json, &length));
    TEST_CHECK(json != NULL);
    TEST_CHECK(length == strlen(json));
    return json;
}

static char *mutation_body(uint32_t expected_revision, const char *resource_json) {
    cJSON *root = cJSON_CreateObject();
    cJSON *resource = cJSON_Parse(resource_json);
    TEST_CHECK(root != NULL);
    TEST_CHECK(resource != NULL);
    TEST_CHECK(cJSON_AddNumberToObject(root, "expectedRevision", expected_revision) != NULL);
    TEST_CHECK(cJSON_AddItemToObject(root, "resource", resource));
    char *body = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    TEST_CHECK(body != NULL);
    return body;
}

static web_api_response_t invoke(handler_fn_t handler, web_api_route_t route,
                                 web_api_method_t method, const char *body, const char *set_id,
                                 const char *macro_id) {
    web_api_call_t call = {
        .method = method,
        .path = {.route = route},
        .body = body,
        .body_length = body == NULL ? 0U : strlen(body),
    };
    if (set_id != NULL) {
        call.path.has_package_id = true;
        call.path.set_id = uuid(set_id);
    }
    if (macro_id != NULL) {
        call.path.has_macro_id = true;
        call.path.macro_id = uuid(macro_id);
    }
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, handler(&call, &response));
    TEST_CHECK(response.body != NULL);
    return response;
}

static void expect_status(web_api_response_t *response, unsigned int status, const char *fragment) {
    TEST_CHECK(response != NULL);
    TEST_CHECK_EQ_U64(status, response->status);
    TEST_CHECK(strstr(response->body, fragment) != NULL);
    web_api_response_free(response);
}

static void test_package_routes(void) {
    macro_package_t set = make_package();
    char *json = serialize_package(&set);
    web_api_response_t response =
        invoke(web_api_handle_packages, WEB_API_ROUTE_SETS, WEB_API_METHOD_POST, json, NULL, NULL);
    expect_status(&response, 201U, "Handler Set");
    cJSON_free(json);

    response =
        invoke(web_api_handle_packages, WEB_API_ROUTE_SETS, WEB_API_METHOD_GET, NULL, NULL, NULL);
    expect_status(&response, 200U, SET_ID);
    response =
        invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_GET, NULL, SET_ID, NULL);
    expect_status(&response, 200U, "Handler Set");

    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Updated Handler Set") > 0);
    json = serialize_package(&set);
    char *mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_PUT, mutation,
                      SET_ID, NULL);
    expect_status(&response, 200U, "Updated Handler Set");
    cJSON_free(mutation);
    cJSON_free(json);

    json = serialize_package(&set);
    mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_PUT, mutation,
                      SET_ID, NULL);
    expect_status(&response, 409U, "could not update package");
    cJSON_free(mutation);
    cJSON_free(json);

    static const char *const invalid_package_bodies[] = {
        "{\"unknown\":true}",
        "{\"schema_version\":1}x",
        "{",
    };
    for (size_t index = 0U;
         index < sizeof(invalid_package_bodies) / sizeof(invalid_package_bodies[0]); ++index) {
        response = invoke(web_api_handle_packages, WEB_API_ROUTE_SETS, WEB_API_METHOD_POST,
                          invalid_package_bodies[index], NULL, NULL);
        expect_status(&response, 422U, "could not create package");
    }

    /* Selection takes an empty body: it is idempotent and has no revision the
     * client holds (SPEC 12.3). It used to require the settings revision, which
     * became meaningless when the active set left settings. */
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET_SELECT, WEB_API_METHOD_POST, "{}",
                      SET_ID, NULL);
    expect_status(&response, 200U, SET_ID);
    /* Selection is a repository write now (SPEC 12.3), so it is observed in the
     * index rather than in the settings store -- and it must NOT burn a settings
     * revision, because settings did not change. */
    bool has_active_package = false;
    app_uuid_t active_package_id = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_active_package_read(&has_active_package, &active_package_id));
    TEST_CHECK(has_active_package);
    TEST_CHECK_EQ_STRING(SET_ID, active_package_id.value);
    TEST_CHECK_EQ_U64(1U, settings_store.revision);

    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET_IMPORT, WEB_API_METHOD_POST, "{}",
                      NULL, NULL);
    expect_status(&response, 422U, "could not replace package");

    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET_IMPORT_NEW, WEB_API_METHOD_POST,
                      "{}", NULL, NULL);
    expect_status(&response, 422U, "could not import package as new");
}

static void test_macro_routes(void) {
    macro_t macro = make_macro(MACRO_ID);
    char *json = serialize_macro(&macro);
    web_api_response_t response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACROS,
                                         WEB_API_METHOD_POST, json, SET_ID, NULL);
    expect_status(&response, 201U, MACRO_ID);
    cJSON_free(json);

    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACROS, WEB_API_METHOD_GET, NULL,
                      SET_ID, NULL);
    expect_status(&response, 200U, MACRO_ID);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_GET, NULL,
                      SET_ID, MACRO_ID);
    expect_status(&response, 200U, "Handler Macro");

    json = serialize_macro(&macro);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO_VALIDATE, WEB_API_METHOD_POST,
                      json, SET_ID, MACRO_ID);
    expect_status(&response, 200U, "\"valid\":true");
    cJSON_free(json);

    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "Updated Handler Macro") > 0);
    json = serialize_macro(&macro);
    char *mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_PUT, mutation,
                      SET_ID, MACRO_ID);
    expect_status(&response, 200U, "Updated Handler Macro");
    cJSON_free(mutation);
    cJSON_free(json);

    char duplicate_body[160U];
    const int duplicate_length =
        snprintf(duplicate_body, sizeof(duplicate_body),
                 "{\"id\":\"%s\",\"name\":\"Duplicated Macro\"}", MACRO_DUPLICATE_ID);
    TEST_CHECK(duplicate_length > 0 && (size_t)duplicate_length < sizeof(duplicate_body));
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO_DUPLICATE, WEB_API_METHOD_POST,
                      duplicate_body, SET_ID, MACRO_ID);
    expect_status(&response, 201U, MACRO_DUPLICATE_ID);

    char order_body[192U];
    const int order_length = snprintf(order_body, sizeof(order_body), "{\"ids\":[\"%s\",\"%s\"]}",
                                      MACRO_DUPLICATE_ID, MACRO_ID);
    TEST_CHECK(order_length > 0 && (size_t)order_length < sizeof(order_body));
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACROS_REORDER, WEB_API_METHOD_POST,
                      order_body, SET_ID, NULL);
    expect_status(&response, 200U, "\"reordered\":true");

    macro_model_free_macro(&macro);
}

static void test_package_delete_and_persistent_readback(void) {
    macro_package_t current = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_read(&(app_uuid_t){.value = SET_ID}, &current));
    TEST_CHECK_EQ_U64(2U, current.revision);
    TEST_CHECK_EQ_STRING("Updated Handler Set", current.name);

    /* Duplicate-then-delete: set duplication was previously exercised inside the
       procedure route test, which SPEC 7.1 removed. Keep the coverage here. */
    char duplicate_body[192U];
    const int duplicate_length = snprintf(duplicate_body, sizeof(duplicate_body),
                                          "{\"id\":\"%s\",\"name\":\"Duplicated Handler Set\","
                                          "\"expectedRevision\":2}",
                                          SET_DUPLICATE_ID);
    TEST_CHECK(duplicate_length > 0 && (size_t)duplicate_length < sizeof(duplicate_body));
    web_api_response_t response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET_DUPLICATE,
                                         WEB_API_METHOD_POST, duplicate_body, SET_ID, NULL);
    expect_status(&response, 201U, "Duplicated Handler Set");
    macro_package_t duplicate_readback = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_package_read(&(app_uuid_t){.value = SET_DUPLICATE_ID}, &duplicate_readback));
    TEST_CHECK_EQ_U64(1U, duplicate_readback.revision);
    storage_macro_list_t duplicate_macros = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_macro_list(&(app_uuid_t){.value = SET_DUPLICATE_ID}, &duplicate_macros));
    TEST_CHECK_EQ_U64(2U, duplicate_macros.count);
    storage_macro_list_free(&duplicate_macros);

    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":1}", SET_ID, NULL);
    expect_status(&response, 409U, "could not delete package");
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":2}", SET_ID, NULL);
    expect_status(&response, 200U, "\"deleted\":true");
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_package_read(&(app_uuid_t){.value = SET_ID}, &current));
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":1}", SET_DUPLICATE_ID, NULL);
    expect_status(&response, 200U, "\"deleted\":true");
}

static void test_session_json_redaction(void) {
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handler_session_json(&json));
    TEST_CHECK(json != NULL);
    TEST_CHECK(strstr(json, "\"authenticated\":true") != NULL);
    TEST_CHECK(strstr(json, "sessionToken") == NULL);
    web_api_handler_json_free(json);
}

/* SPEC 13.7: "A stale revision returns `409 Conflict` with the current resource
 * metadata. The server MUST NOT silently overwrite a newer edit."
 *
 * The 409 is the visible half, and it was already asserted. The prohibition is
 * the other half -- that the rejected write left the stored resource alone --
 * and nothing checked it. A handler that answered 409 and wrote anyway passed
 * every existing test. That is the exact shape of the `expectedRevision`
 * defect: asserting what the handler replies instead of what the specification
 * requires. */
/* SPEC 24.4 item: stale revisions */
static void test_stale_revision_does_not_overwrite_a_newer_edit(void) {
    reset_store();
    macro_package_t set = make_package();
    char *json = serialize_package(&set);
    web_api_response_t response =
        invoke(web_api_handle_packages, WEB_API_ROUTE_SETS, WEB_API_METHOD_POST, json, NULL, NULL);
    expect_status(&response, 201U, "Handler Set");
    cJSON_free(json);

    /* One writer edits successfully, carrying the set past revision 1. */
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Winning Edit") > 0);
    json = serialize_package(&set);
    char *mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_PUT, mutation,
                      SET_ID, NULL);
    expect_status(&response, 200U, "Winning Edit");
    cJSON_free(mutation);
    cJSON_free(json);

    /* A second writer, still holding revision 1, tries to land a different name. */
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Clobbering Edit") > 0);
    json = serialize_package(&set);
    mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_PUT, mutation,
                      SET_ID, NULL);
    TEST_CHECK_EQ_U64(409U, response.status);
    web_api_response_free(&response);
    cJSON_free(mutation);
    cJSON_free(json);

    /* The requirement itself: read the resource back and confirm the newer edit
     * is intact and the stale one never landed. */
    response =
        invoke(web_api_handle_packages, WEB_API_ROUTE_SET, WEB_API_METHOD_GET, NULL, SET_ID, NULL);
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(strstr(response.body, "Winning Edit") != NULL);
    TEST_CHECK(strstr(response.body, "Clobbering Edit") == NULL);
    web_api_response_free(&response);
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    reset_store();
    test_session_json_redaction();
    test_package_routes();
    test_macro_routes();
    test_package_delete_and_persistent_readback();
    test_stale_revision_does_not_overwrite_a_newer_edit();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    return 0;
}
