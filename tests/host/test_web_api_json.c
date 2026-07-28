#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "storage_object_json.h"
#include "test_assert.h"
#include "web_api_json.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define MACRO_ID "22222222-2222-4222-8222-222222222222"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define STEP_ID "44444444-4444-4444-8444-444444444444"

static void test_expected_revision_and_mutation(void) {
    uint32_t revision = 0U;
    const char delete_body[] = "{\"expectedRevision\":7}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_expected_revision(
                                             delete_body, sizeof(delete_body) - 1U, &revision));
    TEST_CHECK_EQ_U64(7U, revision);
    const char unknown[] = "{\"expectedRevision\":7,\"extra\":true}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_json_parse_expected_revision(
                                                         unknown, sizeof(unknown) - 1U, &revision));

    const char update[] =
        "{\"expectedRevision\":7,\"resource\":{\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":8}}";
    web_api_resource_mutation_t mutation = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_resource_mutation(
                             update,
                             &(web_api_resource_parse_limits_t){.body_length = sizeof(update) - 1U,
                                                                .maximum_resource_length = 512U},
                             &mutation));
    TEST_CHECK_EQ_U64(7U, mutation.expected_revision);
    TEST_CHECK(strstr(mutation.resource_json, SET_ID) != NULL);
    web_api_json_free_resource_mutation(&mutation);
}

static void test_order_and_execution(void) {
    const char order[] = "{\"ids\":[\"" MACRO_ID "\",\"" SET_ID "\"]}";
    storage_uuid_order_t parsed = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_json_parse_uuid_order(
            order,
            &(web_api_order_parse_limits_t){.body_length = sizeof(order) - 1U, .maximum_count = 4U},
            &parsed));
    TEST_CHECK_EQ_U64(2U, parsed.count);
    TEST_CHECK_EQ_STRING(MACRO_ID, parsed.ids[0].value);

    const char duplicate[] = "{\"ids\":[\"" MACRO_ID "\",\"" MACRO_ID "\"]}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_uuid_order(
                             duplicate,
                             &(web_api_order_parse_limits_t){.body_length = sizeof(duplicate) - 1U,
                                                             .maximum_count = 4U},
                             &parsed));

    const char execution[] =
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"procedureId\":\"" PROCEDURE_ID "\",\"stepId\":\"" STEP_ID "\"}";
    web_execution_submit_request_t request = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_execution_submit(
                                             execution, sizeof(execution) - 1U, &request));
    TEST_CHECK(request.has_procedure_context);
    TEST_CHECK_EQ_U64(7U, request.macro_revision);

    const char partial_context[] = "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
                                   "\",\"macroRevision\":7,\"procedureId\":\"" PROCEDURE_ID "\"}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_execution_submit(
                             partial_context, sizeof(partial_context) - 1U, &request));
}

static void test_progress_and_settings(void) {
    const char complete[] = "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\"}";
    web_api_progress_action_t action = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_progress_action(
                                             complete, sizeof(complete) - 1U, false, &action));
    TEST_CHECK_EQ_U64(3U, action.expected_procedure_revision);

    const char skip_unconfirmed[] =
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\",\"confirmed\":false}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_progress_action(
                             skip_unconfirmed, sizeof(skip_unconfirmed) - 1U, true, &action));

    const char settings[] = "{\"expectedRevision\":4,\"requirePhysicalConfirmation\":true,"
                            "\"alwaysSelectSet\":false,\"activeSetId\":\"" SET_ID "\"}";
    provisioning_settings_t parsed = {0};
    uint32_t revision = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_settings_update(
                                             settings, sizeof(settings) - 1U, &parsed, &revision));
    TEST_CHECK_EQ_U64(4U, revision);
    TEST_CHECK(parsed.require_physical_confirmation);
    TEST_CHECK(parsed.has_active_set);
    TEST_CHECK_EQ_STRING(SET_ID, parsed.active_set_id.value);
}

static void test_trailing_and_embedded_nul_rejected(void) {
    uint32_t revision = 0U;
    const char trailing[] = "{\"expectedRevision\":1}x";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_json_parse_expected_revision(trailing, sizeof(trailing) - 1U, &revision));
    const char nul_escape[] = "{\"expectedRevision\":1,\"x\":\"\\u0000\"}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_json_parse_expected_revision(nul_escape, sizeof(nul_escape) - 1U, &revision));
}

int main(void) {
    test_expected_revision_and_mutation();
    test_order_and_execution();
    test_progress_and_settings();
    test_trailing_and_embedded_nul_rejected();
    return 0;
}
