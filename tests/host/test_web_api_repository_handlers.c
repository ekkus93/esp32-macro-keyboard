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
#include "web_api_handlers.h"
#include "web_api_response.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define SET_DUPLICATE_ID "11111111-1111-4111-8111-222222222222"
#define MACRO_ID "22222222-2222-4222-8222-222222222222"
#define MACRO_DUPLICATE_ID "22222222-2222-4222-8222-333333333333"
#define GLOBAL_MACRO_ID "22222222-2222-4222-8222-444444444444"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define STEP_ONE_ID "44444444-4444-4444-8444-444444444444"
#define STEP_TWO_ID "44444444-4444-4444-8444-555555555555"

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
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/global",
        STORAGE_DATA_MOUNT "/global/macros",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/quarantine",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < sizeof(paths) / sizeof(paths[0]); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
    settings_store = (provisioning_settings_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 1U,
        .require_physical_confirmation = true,
        .always_select_set = true,
    };
}

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static macro_set_t make_set(void) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_ID),
        .revision = 1U,
        .sort_order = 0,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Handler Set") > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Repository-backed API test") >
               0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static macro_t make_macro(const char *macro_id, macro_scope_t scope) {
    static const char source[] = "ab";
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(macro_id),
        .revision = 1U,
        .scope = scope,
        .has_set_id = scope == MACRO_SCOPE_SET,
        .set_id = uuid(SET_ID),
        .favorite = false,
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

static procedure_t make_procedure(void) {
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(PROCEDURE_ID),
        .revision = 1U,
        .set_id = uuid(SET_ID),
        .step_count = 2U,
    };
    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "Handler Procedure") > 0);
    TEST_CHECK(snprintf(procedure.description, sizeof(procedure.description), "Two steps") > 0);
    procedure.steps = calloc(procedure.step_count, sizeof(*procedure.steps));
    TEST_CHECK(procedure.steps != NULL);
    procedure.steps[0] = (procedure_step_t){
        .id = uuid(STEP_ONE_ID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .auto_complete_on_success = true,
        .has_macro_id = true,
        .macro_id = uuid(MACRO_ID),
    };
    TEST_CHECK(snprintf(procedure.steps[0].title, sizeof(procedure.steps[0].title), "Run macro") >
               0);
    static const char instruction[] = "Confirm the result";
    procedure.steps[1] = (procedure_step_t){
        .id = uuid(STEP_TWO_ID),
        .type = PROCEDURE_STEP_INSTRUCTION,
        .required = true,
        .body_length = sizeof(instruction) - 1U,
    };
    TEST_CHECK(snprintf(procedure.steps[1].title, sizeof(procedure.steps[1].title), "Confirm") > 0);
    procedure.steps[1].body = malloc(sizeof(instruction));
    TEST_CHECK(procedure.steps[1].body != NULL);
    memcpy(procedure.steps[1].body, instruction, sizeof(instruction));
    return procedure;
}

static char *serialize_set(const macro_set_t *set) {
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_set_json(set, &json, &length));
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

static char *serialize_procedure(const procedure_t *procedure) {
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_procedure_json(procedure, &json, &length));
    TEST_CHECK(json != NULL);
    TEST_CHECK(length == strlen(json));
    return json;
}

static char *serialize_progress(const procedure_progress_t *progress) {
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_progress_json(progress, &json, &length));
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
                                 const char *macro_id, const char *procedure_id) {
    web_api_call_t call = {
        .method = method,
        .path = {.route = route},
        .body = body,
        .body_length = body == NULL ? 0U : strlen(body),
    };
    if (set_id != NULL) {
        call.path.has_set_id = true;
        call.path.set_id = uuid(set_id);
    }
    if (macro_id != NULL) {
        call.path.has_macro_id = true;
        call.path.macro_id = uuid(macro_id);
    }
    if (procedure_id != NULL) {
        call.path.has_procedure_id = true;
        call.path.procedure_id = uuid(procedure_id);
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

static void test_set_routes(void) {
    macro_set_t set = make_set();
    char *json = serialize_set(&set);
    web_api_response_t response = invoke(web_api_handle_sets, WEB_API_ROUTE_SETS,
                                         WEB_API_METHOD_POST, json, NULL, NULL, NULL);
    expect_status(&response, 201U, "Handler Set");
    cJSON_free(json);

    response =
        invoke(web_api_handle_sets, WEB_API_ROUTE_SETS, WEB_API_METHOD_GET, NULL, NULL, NULL, NULL);
    expect_status(&response, 200U, SET_ID);
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_GET, NULL, SET_ID,
                      NULL, NULL);
    expect_status(&response, 200U, "Handler Set");

    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Updated Handler Set") > 0);
    json = serialize_set(&set);
    char *mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_PUT, mutation, SET_ID,
                      NULL, NULL);
    expect_status(&response, 200U, "Updated Handler Set");
    cJSON_free(mutation);
    cJSON_free(json);

    json = serialize_set(&set);
    mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_PUT, mutation, SET_ID,
                      NULL, NULL);
    expect_status(&response, 409U, "could not update set");
    cJSON_free(mutation);
    cJSON_free(json);

    static const char *const invalid_set_bodies[] = {
        "{\"unknown\":true}",
        "{\"schema_version\":1}x",
        "{",
    };
    for (size_t index = 0U; index < sizeof(invalid_set_bodies) / sizeof(invalid_set_bodies[0]);
         ++index) {
        response = invoke(web_api_handle_sets, WEB_API_ROUTE_SETS, WEB_API_METHOD_POST,
                          invalid_set_bodies[index], NULL, NULL, NULL);
        expect_status(&response, 422U, "could not create set");
    }

    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET_SELECT, WEB_API_METHOD_POST,
                      "{\"expectedRevision\":1}", SET_ID, NULL, NULL);
    expect_status(&response, 200U, SET_ID);
    TEST_CHECK(settings_store.has_active_set);
    TEST_CHECK_EQ_STRING(SET_ID, settings_store.active_set_id.value);
    TEST_CHECK_EQ_U64(2U, settings_store.revision);

    static const web_api_route_t unavailable_routes[] = {
        WEB_API_ROUTE_SET_EXPORT,
        WEB_API_ROUTE_SET_IMPORT,
    };
    for (size_t index = 0U; index < sizeof(unavailable_routes) / sizeof(unavailable_routes[0]);
         ++index) {
        response = invoke(web_api_handle_sets, unavailable_routes[index], WEB_API_METHOD_POST, NULL,
                          SET_ID, NULL, NULL);
        expect_status(&response, 503U, "requires the Phase 18");
    }
}

static void test_macro_routes(void) {
    macro_t macro = make_macro(MACRO_ID, MACRO_SCOPE_SET);
    char *json = serialize_macro(&macro);
    web_api_response_t response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACROS,
                                         WEB_API_METHOD_POST, json, SET_ID, NULL, NULL);
    expect_status(&response, 201U, MACRO_ID);
    cJSON_free(json);

    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACROS, WEB_API_METHOD_GET, NULL,
                      SET_ID, NULL, NULL);
    expect_status(&response, 200U, MACRO_ID);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_GET, NULL,
                      SET_ID, MACRO_ID, NULL);
    expect_status(&response, 200U, "Handler Macro");

    json = serialize_macro(&macro);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO_VALIDATE, WEB_API_METHOD_POST,
                      json, SET_ID, MACRO_ID, NULL);
    expect_status(&response, 200U, "\"valid\":true");
    cJSON_free(json);

    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "Updated Handler Macro") > 0);
    json = serialize_macro(&macro);
    char *mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_PUT, mutation,
                      SET_ID, MACRO_ID, NULL);
    expect_status(&response, 200U, "Updated Handler Macro");
    cJSON_free(mutation);
    cJSON_free(json);

    char duplicate_body[160U];
    const int duplicate_length =
        snprintf(duplicate_body, sizeof(duplicate_body),
                 "{\"id\":\"%s\",\"name\":\"Duplicated Macro\"}", MACRO_DUPLICATE_ID);
    TEST_CHECK(duplicate_length > 0 && (size_t)duplicate_length < sizeof(duplicate_body));
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO_DUPLICATE, WEB_API_METHOD_POST,
                      duplicate_body, SET_ID, MACRO_ID, NULL);
    expect_status(&response, 201U, MACRO_DUPLICATE_ID);

    char order_body[192U];
    const int order_length = snprintf(order_body, sizeof(order_body), "{\"ids\":[\"%s\",\"%s\"]}",
                                      MACRO_DUPLICATE_ID, MACRO_ID);
    TEST_CHECK(order_length > 0 && (size_t)order_length < sizeof(order_body));
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACROS_REORDER, WEB_API_METHOD_POST,
                      order_body, SET_ID, NULL, NULL);
    expect_status(&response, 200U, "\"reordered\":true");

    macro_t global = make_macro(GLOBAL_MACRO_ID, MACRO_SCOPE_GLOBAL);
    global.has_set_id = false;
    memset(&global.set_id, 0, sizeof(global.set_id));
    json = serialize_macro(&global);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_GLOBAL_MACROS, WEB_API_METHOD_POST, json,
                      NULL, NULL, NULL);
    expect_status(&response, 201U, GLOBAL_MACRO_ID);
    cJSON_free(json);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_GLOBAL_MACROS, WEB_API_METHOD_GET, NULL,
                      NULL, NULL, NULL);
    expect_status(&response, 200U, GLOBAL_MACRO_ID);
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_GLOBAL_MACRO, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":1}", NULL, GLOBAL_MACRO_ID, NULL);
    expect_status(&response, 200U, "\"deleted\":true");

    macro_model_free_macro(&global);
    macro_model_free_macro(&macro);
}

static void test_procedure_and_progress_routes(void) {
    procedure_t procedure = make_procedure();
    char *json = serialize_procedure(&procedure);
    web_api_response_t response = invoke(web_api_handle_procedures, WEB_API_ROUTE_SET_PROCEDURES,
                                         WEB_API_METHOD_POST, json, SET_ID, NULL, NULL);
    expect_status(&response, 201U, PROCEDURE_ID);
    cJSON_free(json);

    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_SET_PROCEDURES, WEB_API_METHOD_GET,
                      NULL, SET_ID, NULL, NULL);
    expect_status(&response, 200U, PROCEDURE_ID);
    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_SET_PROCEDURE, WEB_API_METHOD_GET,
                      NULL, SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "Handler Procedure");

    char order_body[96U];
    const int order_length =
        snprintf(order_body, sizeof(order_body), "{\"ids\":[\"%s\"]}", PROCEDURE_ID);
    TEST_CHECK(order_length > 0 && (size_t)order_length < sizeof(order_body));
    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_SET_PROCEDURES_REORDER,
                      WEB_API_METHOD_POST, order_body, SET_ID, NULL, NULL);
    expect_status(&response, 200U, "\"reordered\":true");

    procedure_progress_t progress = {
        .schema_version = APP_SCHEMA_VERSION,
        .set_id = uuid(SET_ID),
        .procedure_id = uuid(PROCEDURE_ID),
        .procedure_revision = 1U,
        .current_step_id = uuid(STEP_ONE_ID),
    };
    json = serialize_progress(&progress);
    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_PROCEDURE_PROGRESS,
                      WEB_API_METHOD_PUT, json, SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "\"status\":\"current\"");
    cJSON_free(json);

    response =
        invoke(web_api_handle_procedures, WEB_API_ROUTE_PROGRESS_COMPLETE, WEB_API_METHOD_POST,
               "{\"expectedProcedureRevision\":1,\"stepId\":\"" STEP_ONE_ID "\"}", SET_ID, NULL,
               PROCEDURE_ID);
    expect_status(&response, 200U, STEP_TWO_ID);
    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_PROGRESS_SKIP, WEB_API_METHOD_POST,
                      "{\"expectedProcedureRevision\":1,\"stepId\":\"" STEP_TWO_ID
                      "\",\"confirmed\":true}",
                      SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, STEP_TWO_ID);

    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "Updated Procedure") > 0);
    json = serialize_procedure(&procedure);
    char *mutation = mutation_body(1U, json);
    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_SET_PROCEDURE, WEB_API_METHOD_PUT,
                      mutation, SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "Updated Procedure");
    cJSON_free(mutation);
    cJSON_free(json);

    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_PROCEDURE_PROGRESS,
                      WEB_API_METHOD_GET, NULL, SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "\"status\":\"stale\"");
    response =
        invoke(web_api_handle_procedures, WEB_API_ROUTE_PROCEDURE_PROGRESS, WEB_API_METHOD_DELETE,
               "{\"expectedRevision\":2}", SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "\"status\":\"current\"");

    char duplicate_body[192U];
    const int duplicate_length = snprintf(duplicate_body, sizeof(duplicate_body),
                                          "{\"id\":\"%s\",\"name\":\"Duplicated Handler Set\","
                                          "\"expectedRevision\":2}",
                                          SET_DUPLICATE_ID);
    TEST_CHECK(duplicate_length > 0 && (size_t)duplicate_length < sizeof(duplicate_body));
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET_DUPLICATE, WEB_API_METHOD_POST,
                      duplicate_body, SET_ID, NULL, NULL);
    expect_status(&response, 201U, "Duplicated Handler Set");
    macro_set_t duplicate_readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&(app_uuid_t){.value = SET_DUPLICATE_ID},
                                                          &duplicate_readback));
    TEST_CHECK_EQ_U64(1U, duplicate_readback.revision);
    storage_macro_list_t duplicate_macros = {0};
    const storage_macro_location_t duplicate_location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = {.value = SET_DUPLICATE_ID},
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_macro_list(&duplicate_location, &duplicate_macros));
    TEST_CHECK_EQ_U64(2U, duplicate_macros.count);
    storage_macro_list_free(&duplicate_macros);
    storage_procedure_list_t duplicate_procedures = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_procedure_list(&(app_uuid_t){.value = SET_DUPLICATE_ID}, &duplicate_procedures));
    TEST_CHECK_EQ_U64(1U, duplicate_procedures.count);
    storage_procedure_list_free(&duplicate_procedures);
    storage_progress_snapshot_t duplicate_progress = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_progress_read(
                                                  &(storage_procedure_identity_t){
                                                      .set_id = {.value = SET_DUPLICATE_ID},
                                                      .procedure_id = {.value = PROCEDURE_ID},
                                                  },
                                                  &duplicate_progress));
    char set_order[192U];
    const int set_order_length = snprintf(set_order, sizeof(set_order), "{\"ids\":[\"%s\",\"%s\"]}",
                                          SET_DUPLICATE_ID, SET_ID);
    TEST_CHECK(set_order_length > 0 && (size_t)set_order_length < sizeof(set_order));
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_PUT, set_order,
                      NULL, NULL, NULL);
    expect_status(&response, 200U, SET_DUPLICATE_ID);

    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":2}", SET_ID, MACRO_ID, NULL);
    expect_status(&response, 409U, PROCEDURE_ID);

    response = invoke(web_api_handle_procedures, WEB_API_ROUTE_SET_PROCEDURE, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":2}", SET_ID, NULL, PROCEDURE_ID);
    expect_status(&response, 200U, "\"deleted\":true");
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":2}", SET_ID, MACRO_ID, NULL);
    expect_status(&response, 200U, "\"deleted\":true");
    response = invoke(web_api_handle_macros, WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":1}", SET_ID, MACRO_DUPLICATE_ID, NULL);
    expect_status(&response, 200U, "\"deleted\":true");

    macro_model_free_procedure(&procedure);
}

static void test_set_delete_and_persistent_readback(void) {
    macro_set_t current = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_set_read(&(app_uuid_t){.value = SET_ID}, &current));
    TEST_CHECK_EQ_U64(2U, current.revision);
    TEST_CHECK_EQ_STRING("Updated Handler Set", current.name);

    web_api_response_t response =
        invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,
               "{\"expectedRevision\":1}", SET_ID, NULL, NULL);
    expect_status(&response, 409U, "could not delete set");
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":2}", SET_ID, NULL, NULL);
    expect_status(&response, 200U, "\"deleted\":true");
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_set_read(&(app_uuid_t){.value = SET_ID}, &current));
    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE,
                      "{\"expectedRevision\":1}", SET_DUPLICATE_ID, NULL, NULL);
    expect_status(&response, 200U, "\"deleted\":true");
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    reset_store();
    test_set_routes();
    test_macro_routes();
    test_procedure_and_progress_routes();
    test_set_delete_and_persistent_readback();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    return 0;
}
