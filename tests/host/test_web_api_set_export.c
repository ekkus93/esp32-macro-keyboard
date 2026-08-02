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
#include "storage_package.h"
#include "storage_package_internal.h"
#include "storage_repository.h"
#include "storage_repository_lock.h"
#include "storage_repository_macros_internal.h"
#include "storage_repository_procedures_internal.h"
#include "storage_repository_progress_internal.h"
#include "storage_repository_sets_internal.h"
#include "test_assert.h"
#include "test_temp_dir.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define MISSING_SET_ID "12121212-1212-4212-8212-121212121212"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
#define OTHER_SET_ID "29292929-2929-4292-8292-292929292929"
#define SECOND_MACRO_ID "23232323-2323-4232-8232-232323232323"
#define OTHER_SET_MACRO_ID "24242424-2424-4242-8242-242424242424"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define LOCAL_STEP_ID "44444444-4444-4444-8444-444444444444"
#define SECOND_STEP_ID "45454545-4545-4545-8545-454545454545"
#define UNUSED_SENTINEL "SENTINEL-OTHER-SET-MACRO"

app_error_code_t provisioning_settings_read(provisioning_settings_t *out_settings) {
    (void)out_settings;
    return APP_ERROR_INTERNAL;
}

app_error_code_t provisioning_settings_update(const provisioning_settings_t *replacement,
                                              uint32_t expected_revision,
                                              provisioning_settings_t *out_committed) {
    (void)replacement;
    (void)expected_revision;
    (void)out_committed;
    return APP_ERROR_INTERNAL;
}

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < sizeof(paths) / sizeof(paths[0]); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_set_t make_set(void) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_ID),
        .revision = 1U,
        .sort_order = 0,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Export Set") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static macro_t make_macro(const char *id, const app_uuid_t *set_id, const char *name,
                          const char *source) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(id),
        .revision = 1U,
        .favorite = false,
        .key_press_ms = APP_KEY_PRESS_DEFAULT_MS,
        .inter_key_ms = APP_INTER_KEY_DEFAULT_MS,
        .source_length = strlen(source),
    };
    if (set_id != NULL) {
        macro.set_id = *set_id;
    }
    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "%s", name) > 0);
    macro.source = malloc(macro.source_length + 1U);
    TEST_CHECK(macro.source != NULL);
    memcpy(macro.source, source, macro.source_length + 1U);
    return macro;
}

static procedure_t make_procedure(const app_uuid_t *set_id) {
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(PROCEDURE_ID),
        .revision = 1U,
        .set_id = *set_id,
        .step_count = 2U,
        .sort_order = 0,
    };
    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "Export Procedure") > 0);
    procedure.steps = calloc(procedure.step_count, sizeof(*procedure.steps));
    TEST_CHECK(procedure.steps != NULL);
    procedure.steps[0] = (procedure_step_t){
        .id = uuid(LOCAL_STEP_ID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = uuid(LOCAL_MACRO_ID),
    };
    TEST_CHECK(snprintf(procedure.steps[0].title, sizeof(procedure.steps[0].title), "Local") > 0);
    procedure.steps[1] = (procedure_step_t){
        .id = uuid(SECOND_STEP_ID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = uuid(SECOND_MACRO_ID),
    };
    TEST_CHECK(snprintf(procedure.steps[1].title, sizeof(procedure.steps[1].title), "Second") > 0);
    return procedure;
}

static app_error_code_t export_lock_take(void *context) {
    (void)context;
    return storage_repository_lock_take();
}

static app_error_code_t export_lock_give(void *context) {
    (void)context;
    return storage_repository_lock_give();
}

static app_error_code_t export_set_read(void *context, const app_uuid_t *set_id,
                                        macro_set_t *out_set) {
    (void)context;
    return storage_set_read_locked(set_id, out_set);
}

static app_error_code_t export_macro_list(void *context, const app_uuid_t *set_id,
                                          storage_macro_list_t *out_list) {
    (void)context;
    return storage_macro_list_locked(set_id, out_list);
}

static void export_macro_list_free(void *context, storage_macro_list_t *list) {
    (void)context;
    storage_macro_list_free(list);
}

static app_error_code_t export_procedure_list(void *context, const app_uuid_t *set_id,
                                              storage_procedure_list_t *out_list) {
    (void)context;
    return storage_procedure_list_locked(set_id, out_list);
}

static void export_procedure_list_free(void *context, storage_procedure_list_t *list) {
    (void)context;
    storage_procedure_list_free(list);
}

static app_error_code_t export_progress_read(void *context,
                                             const storage_procedure_identity_t *identity,
                                             storage_progress_snapshot_t *out_snapshot) {
    (void)context;
    return storage_progress_read_locked(identity, out_snapshot);
}

static void install_export_operations(void) {
    const storage_package_export_ops_t operations = {
        .context = NULL,
        .lock_take = export_lock_take,
        .lock_give = export_lock_give,
        .set_read = export_set_read,
        .macro_list = export_macro_list,
        .macro_list_free = export_macro_list_free,
        .procedure_list = export_procedure_list,
        .procedure_list_free = export_procedure_list_free,
        .progress_read = export_progress_read,
    };
    storage_package_set_export_ops_for_test(&operations);
}

static void populate_store(void) {
    macro_set_t set = make_set();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    macro_t local = make_macro(LOCAL_MACRO_ID, &set.id, "Local Macro", "a");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&set.id, &local));
    macro_model_free_macro(&local);

    macro_t second = make_macro(SECOND_MACRO_ID, &set.id, "Second Macro", "b");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&set.id, &second));
    macro_model_free_macro(&second);

    /* A macro of a different set: present in the repository, and so outside
     * this export's scope. */
    macro_set_t other_set = make_set();
    other_set.id = uuid(OTHER_SET_ID);
    TEST_CHECK(snprintf(other_set.name, sizeof(other_set.name), "Other Set") > 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&other_set));
    macro_t unused = make_macro(OTHER_SET_MACRO_ID, &other_set.id, UNUSED_SENTINEL,
                                "unreferenced-secret-source");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&other_set.id, &unused));
    macro_model_free_macro(&unused);

    procedure_t procedure = make_procedure(&set.id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &procedure));
    macro_model_free_procedure(&procedure);

    const storage_procedure_identity_t identity = {
        .set_id = set.id,
        .procedure_id = uuid(PROCEDURE_ID),
    };
    storage_progress_snapshot_t progress = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_progress_reset(&identity, 1U, &progress));
}

static web_api_response_t invoke_export(const char *set_id) {
    const web_api_call_t call = {
        .method = WEB_API_METHOD_GET,
        .path =
            {
                .route = WEB_API_ROUTE_SET_EXPORT,
                .has_set_id = true,
                .set_id = {.value = ""},
            },
        .body = "",
        .body_length = 0U,
    };
    web_api_call_t mutable_call = call;
    mutable_call.path.set_id = uuid(set_id);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_sets(&mutable_call, &response));
    TEST_CHECK(response.body != NULL);
    return response;
}

static web_api_response_t invoke_import(const char *body) {
    const web_api_call_t call = {
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_SET_IMPORT},
        .body = body,
        .body_length = strlen(body),
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_sets(&call, &response));
    TEST_CHECK(response.body != NULL);
    return response;
}

static char *make_replacement_request(uint32_t expected_revision) {
    web_api_response_t exported = invoke_export(SET_ID);
    TEST_CHECK_EQ_U64(200U, exported.status);
    const char *parse_end = NULL;
    cJSON *package =
        cJSON_ParseWithLengthOpts(exported.body, exported.body_length, &parse_end, false);
    TEST_CHECK(package != NULL);
    TEST_CHECK(parse_end == exported.body + exported.body_length);
    cJSON *sets = cJSON_GetObjectItemCaseSensitive(package, "sets");
    cJSON *set = cJSON_GetArrayItem(sets, 0);
    TEST_CHECK(cJSON_IsObject(set));
    cJSON *revision = cJSON_CreateNumber(2.0);
    cJSON *name = cJSON_CreateString("Imported Replacement");
    TEST_CHECK(revision != NULL);
    TEST_CHECK(name != NULL);
    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "revision", revision));
    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "name", name));

    cJSON *wrapper = cJSON_CreateObject();
    TEST_CHECK(wrapper != NULL);
    TEST_CHECK(cJSON_AddStringToObject(wrapper, "targetSetId", SET_ID) != NULL);
    TEST_CHECK(cJSON_AddNumberToObject(wrapper, "expectedRevision", (double)expected_revision) !=
               NULL);
    TEST_CHECK(cJSON_AddItemToObject(wrapper, "package", package));
    package = NULL;
    char *request = cJSON_PrintUnformatted(wrapper);
    TEST_CHECK(request != NULL);
    cJSON_Delete(wrapper);
    cJSON_Delete(package);
    web_api_response_free(&exported);
    return request;
}

static void test_import_route(void) {
    char *request = make_replacement_request(1U);
    web_api_response_t response = invoke_import(request);
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":true") != NULL);
    TEST_CHECK(strstr(response.body, "Imported Replacement") != NULL);
    TEST_CHECK(strstr(response.body, "\"revision\":2") != NULL);
    macro_set_t committed = {0};
    const app_uuid_t set_id = uuid(SET_ID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set_id, &committed));
    TEST_CHECK_EQ_U64(2U, committed.revision);
    TEST_CHECK_EQ_STRING("Imported Replacement", committed.name);
    web_api_response_free(&response);
    cJSON_free(request);

    request = make_replacement_request(1U);
    response = invoke_import(request);
    TEST_CHECK_EQ_U64(409U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "could not replace set") != NULL);
    web_api_response_free(&response);
    cJSON_free(request);

    response = invoke_import("{\"targetSetId\":\"" SET_ID
                             "\",\"expectedRevision\":2,\"package\":{},\"extra\":true}");
    TEST_CHECK_EQ_U64(422U, response.status);
    web_api_response_free(&response);
}

static void test_export_route(void) {
    web_api_response_t response = invoke_export(SET_ID);
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(response.body_length == strlen(response.body));
    TEST_CHECK(
        strncmp(response.body, "{\"schema_version\":1", sizeof("{\"schema_version\":1") - 1U) == 0);
    TEST_CHECK(strstr(response.body, "\"ok\":true") == NULL);
    TEST_CHECK(strstr(response.body, LOCAL_MACRO_ID) != NULL);
    TEST_CHECK(strstr(response.body, SECOND_MACRO_ID) != NULL);
    TEST_CHECK(strstr(response.body, PROCEDURE_ID) != NULL);
    TEST_CHECK(strstr(response.body, LOCAL_STEP_ID) != NULL);
    TEST_CHECK(strstr(response.body, OTHER_SET_MACRO_ID) == NULL);
    TEST_CHECK(strstr(response.body, UNUSED_SENTINEL) == NULL);

    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(response.body, response.body_length,
                                                  STORAGE_PACKAGE_KIND_SET, &summary));
    TEST_CHECK_EQ_U64(1U, summary.set_count);
    TEST_CHECK_EQ_U64(2U, summary.local_macro_count);
    TEST_CHECK_EQ_U64(1U, summary.procedure_count);
    TEST_CHECK_EQ_U64(1U, summary.progress_count);
    web_api_response_free(&response);
}

static void test_missing_set_error_envelope(void) {
    web_api_response_t response = invoke_export(MISSING_SET_ID);
    TEST_CHECK_EQ_U64(404U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "could not export set") != NULL);
    web_api_response_free(&response);
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    reset_store();
    populate_store();
    install_export_operations();
    test_export_route();
    test_import_route();
    test_missing_set_error_envelope();
    storage_package_reset_export_ops_for_test();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_deinit());
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    return 0;
}
