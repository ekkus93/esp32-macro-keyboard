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

static void test_expected_revision_matrix(void) {
    uint32_t revision = 99U;
    const char valid[] = "{\"expectedRevision\":7}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, web_api_json_parse_expected_revision(valid, sizeof(valid) - 1U, &revision));
    TEST_CHECK_EQ_U64(7U, revision);

    static const char *const invalid[] = {
        "{}",
        "[]",
        "{\"expectedRevision\":0}",
        "{\"expectedRevision\":1.5}",
        "{\"expectedRevision\":\"1\"}",
        "{\"expectedRevision\":7,\"extra\":true}",
        "{\"expectedRevision\":7,\"expectedRevision\":7}",
        "{\"expectedRevision\":7}x",
        "{\"expectedRevision\":",
    };
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        revision = 99U;
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             web_api_json_parse_expected_revision(
                                 invalid[index], strlen(invalid[index]), &revision));
        TEST_CHECK_EQ_U64(0U, revision);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_expected_revision(valid, sizeof(valid) - 1U, NULL));
}

static void test_resource_mutation_matrix(void) {
    const char valid[] =
        "{\"expectedRevision\":7,\"resource\":{\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":7}}";
    web_api_resource_mutation_t mutation = {0};
    const web_api_resource_parse_limits_t limits = {
        .body_length = sizeof(valid) - 1U,
        .maximum_resource_length = 512U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_resource_mutation(valid, &limits, &mutation));
    TEST_CHECK_EQ_U64(7U, mutation.expected_revision);
    TEST_CHECK(strstr(mutation.resource_json, SET_ID) != NULL);
    TEST_CHECK(mutation.resource_length == strlen(mutation.resource_json));
    web_api_json_free_resource_mutation(&mutation);
    TEST_CHECK(mutation.resource_json == NULL);
    TEST_CHECK_EQ_U64(0U, mutation.resource_length);

    static const char *const invalid[] = {
        "{\"expectedRevision\":7}",
        "{\"resource\":{}}",
        "{\"expectedRevision\":7,\"resource\":[]}",
        "{\"expectedRevision\":7,\"resource\":{},\"extra\":true}",
        "{\"expectedRevision\":7,\"expectedRevision\":7,\"resource\":{}}",
        "{\"expectedRevision\":7,\"resource\":{}}x",
    };
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        const web_api_resource_parse_limits_t invalid_limits = {
            .body_length = strlen(invalid[index]),
            .maximum_resource_length = 512U,
        };
        memset(&mutation, 0xa5, sizeof(mutation));
        TEST_CHECK_APP_ERROR(
            APP_ERROR_INVALID_ARGUMENT,
            web_api_json_parse_resource_mutation(invalid[index], &invalid_limits, &mutation));
        TEST_CHECK(mutation.resource_json == NULL);
        TEST_CHECK_EQ_U64(0U, mutation.resource_length);
    }

    const web_api_resource_parse_limits_t too_small = {
        .body_length = sizeof(valid) - 1U,
        .maximum_resource_length = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_resource_mutation(valid, &too_small, &mutation));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_resource_mutation(valid, NULL, &mutation));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_resource_mutation(valid, &limits, NULL));
    web_api_json_free_resource_mutation(NULL);
}

static void test_resource_request_boundary(void) {
    const char valid_set[] =
        "{\"schema_version\":1,\"id\":\"" SET_ID "\",\"revision\":1,\"name\":\"Set\"}";
    macro_set_t set = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_set_resource(valid_set, sizeof(valid_set) - 1U, &set));
    TEST_CHECK_EQ_STRING("Set", set.name);

    const char valid_macro[] =
        "{\"schema_version\":1,\"id\":\"" MACRO_ID "\",\"revision\":1,\"name\":\"Macro\","
        "\"source\":\"a\",\"favorite\":false,\"key_press_ms\":8,"
        "\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}";
    macro_t macro = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_macro_resource(
                                             valid_macro, sizeof(valid_macro) - 1U, &macro));
    TEST_CHECK_EQ_STRING("a", macro.source);
    macro_model_free_macro(&macro);

    const char valid_procedure[] =
        "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID "\",\"revision\":1,\"set_id\":\"" SET_ID
        "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{"
        "\"id\":\"" STEP_ID "\",\"type\":\"macro\",\"title\":\"Step\",\"macro_id\":\"" MACRO_ID
        "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}";
    procedure_t procedure = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_procedure_resource(
                             valid_procedure, sizeof(valid_procedure) - 1U, &procedure));
    TEST_CHECK_EQ_U64(1U, procedure.step_count);
    macro_model_free_procedure(&procedure);

    const char valid_progress[] =
        "{\"schema_version\":1,\"set_id\":\"" SET_ID "\",\"procedure_id\":\"" PROCEDURE_ID
        "\",\"procedure_revision\":1,\"current_step_id\":\"" STEP_ID
        "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}";
    procedure_progress_t progress = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_progress_resource(
                             valid_progress, sizeof(valid_progress) - 1U, &progress));
    TEST_CHECK_EQ_U64(1U, progress.procedure_revision);

    static const char *const invalid_sets[] = {
        "{}",
        "{\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":1,\"name\":\"Set\",,\"extra\":true}",
        "{\"schema_version\":1,\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":1,\"name\":\"Set\"}",
        "{\"schema_version\":1}x",
        "{",
    };
    for (size_t index = 0U; index < sizeof(invalid_sets) / sizeof(invalid_sets[0]); ++index) {
        memset(&set, 0xa5, sizeof(set));
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             web_api_json_parse_set_resource(invalid_sets[index],
                                                             strlen(invalid_sets[index]), &set));
        TEST_CHECK_EQ_U64(0U, set.revision);
    }

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_macro_resource("{}", 2U, &macro));
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_procedure_resource("{}", 2U, &procedure));
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_progress_resource("{}", 2U, &progress));
    TEST_CHECK_EQ_U64(0U, progress.procedure_revision);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_set_resource(valid_set, sizeof(valid_set) - 1U, NULL));
}

static void test_uuid_order_matrix(void) {
    const char valid[] = "{\"ids\":[\"" MACRO_ID "\",\"" SET_ID "\"]}";
    storage_uuid_order_t order = {0};
    const web_api_order_parse_limits_t limits = {
        .body_length = sizeof(valid) - 1U,
        .maximum_count = 4U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_uuid_order(valid, &limits, &order));
    TEST_CHECK_EQ_U64(2U, order.count);
    TEST_CHECK_EQ_STRING(MACRO_ID, order.ids[0].value);

    const char empty[] = "{\"ids\":[]}";
    const web_api_order_parse_limits_t empty_limits = {
        .body_length = sizeof(empty) - 1U,
        .maximum_count = 4U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_uuid_order(empty, &empty_limits, &order));
    TEST_CHECK_EQ_U64(0U, order.count);

    static const char *const invalid[] = {
        "{}",
        "{\"ids\":null}",
        "{\"ids\":[1]}",
        "{\"ids\":[\"not-a-uuid\"]}",
        "{\"ids\":[\"" MACRO_ID "\",\"" MACRO_ID "\"]}",
        "{\"ids\":[],\"extra\":true}",
        "{\"ids\":[],\"ids\":[]}",
        "{\"ids\":[]}x",
    };
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        const web_api_order_parse_limits_t invalid_limits = {
            .body_length = strlen(invalid[index]),
            .maximum_count = 4U,
        };
        memset(&order, 0xa5, sizeof(order));
        TEST_CHECK_APP_ERROR(
            APP_ERROR_INVALID_ARGUMENT,
            web_api_json_parse_uuid_order(invalid[index], &invalid_limits, &order));
        TEST_CHECK_EQ_U64(0U, order.count);
    }

    const char over_limit[] = "{\"ids\":[\"" SET_ID "\",\"" MACRO_ID "\"]}";
    const web_api_order_parse_limits_t one_only = {
        .body_length = sizeof(over_limit) - 1U,
        .maximum_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_uuid_order(over_limit, &one_only, &order));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_uuid_order(valid, NULL, &order));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_uuid_order(valid, &limits, NULL));
}

static void test_execution_submit_matrix(void) {
    const char base[] =
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID "\",\"macroRevision\":7}";
    web_execution_submit_request_t request = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_execution_submit(base, sizeof(base) - 1U, &request));
    TEST_CHECK(!request.has_procedure_context);
    TEST_CHECK_EQ_U64(7U, request.macro_revision);

    const char contextual[] =
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"sourceContext\":{\"procedureId\":\"" PROCEDURE_ID
        "\",\"stepId\":\"" STEP_ID "\"}}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_execution_submit(
                                             contextual, sizeof(contextual) - 1U, &request));
    TEST_CHECK(request.has_procedure_context);
    TEST_CHECK_EQ_STRING(PROCEDURE_ID, request.procedure_id.value);
    TEST_CHECK_EQ_STRING(STEP_ID, request.step_id.value);

    static const char *const invalid[] = {
        "{}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID "\"}",
        "{\"setId\":\"not-a-uuid\",\"macroId\":\"" MACRO_ID "\",\"macroRevision\":7}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID "\",\"macroRevision\":0}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"sourceContext\":null}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"sourceContext\":{\"procedureId\":\"" PROCEDURE_ID "\"}}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"sourceContext\":{\"stepId\":\"" STEP_ID "\"}}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"sourceContext\":{\"procedureId\":\"bad\",\"stepId\":\"" STEP_ID
        "\"}}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"sourceContext\":{\"procedureId\":\"" PROCEDURE_ID
        "\",\"stepId\":\"" STEP_ID "\",\"extra\":true}}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"procedureId\":\"" PROCEDURE_ID "\",\"stepId\":\"" STEP_ID "\"}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID
        "\",\"macroRevision\":7,\"extra\":true}",
        "{\"setId\":\"" SET_ID "\",\"macroId\":\"" MACRO_ID "\",\"macroRevision\":7}x",
    };
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        memset(&request, 0xa5, sizeof(request));
        TEST_CHECK_APP_ERROR(
            APP_ERROR_INVALID_ARGUMENT,
            web_api_json_parse_execution_submit(invalid[index], strlen(invalid[index]), &request));
        TEST_CHECK_EQ_U64(0U, request.macro_revision);
        TEST_CHECK(!request.has_procedure_context);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_execution_submit(base, sizeof(base) - 1U, NULL));
}

static void test_progress_action_matrix(void) {
    const char complete[] = "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\"}";
    web_api_progress_action_t action = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_progress_action(
                                             complete, sizeof(complete) - 1U, false, &action));
    TEST_CHECK_EQ_U64(3U, action.expected_procedure_revision);
    TEST_CHECK(!action.confirmed);

    const char skip[] =
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\",\"confirmed\":true}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, web_api_json_parse_progress_action(skip, sizeof(skip) - 1U, true, &action));
    TEST_CHECK(action.confirmed);

    static const char *const invalid_complete[] = {
        "{}",
        "{\"expectedProcedureRevision\":0,\"stepId\":\"" STEP_ID "\"}",
        "{\"expectedProcedureRevision\":3,\"stepId\":\"bad\"}",
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\",\"confirmed\":true}",
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\"}x",
    };
    for (size_t index = 0U; index < sizeof(invalid_complete) / sizeof(invalid_complete[0]);
         ++index) {
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             web_api_json_parse_progress_action(invalid_complete[index],
                                                                strlen(invalid_complete[index]),
                                                                false, &action));
    }

    static const char *const invalid_skip[] = {
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\"}",
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\",\"confirmed\":false}",
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID "\",\"confirmed\":\"true\"}",
        "{\"expectedProcedureRevision\":3,\"stepId\":\"" STEP_ID
        "\",\"confirmed\":true,\"extra\":1}",
    };
    for (size_t index = 0U; index < sizeof(invalid_skip) / sizeof(invalid_skip[0]); ++index) {
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             web_api_json_parse_progress_action(
                                 invalid_skip[index], strlen(invalid_skip[index]), true, &action));
    }
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_json_parse_progress_action(complete, sizeof(complete) - 1U, false, NULL));
}

static void test_settings_update_matrix(void) {
    const char active[] = "{\"expectedRevision\":4,\"requirePhysicalConfirmation\":true,"
                          "\"alwaysSelectSet\":false,\"activeSetId\":\"" SET_ID "\"}";
    provisioning_settings_t settings = {0};
    uint32_t revision = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_json_parse_settings_update(
                                             active, sizeof(active) - 1U, &settings, &revision));
    TEST_CHECK_EQ_U64(4U, revision);
    TEST_CHECK(settings.require_physical_confirmation);
    TEST_CHECK(settings.has_active_set);
    TEST_CHECK_EQ_STRING(SET_ID, settings.active_set_id.value);

    const char no_active[] = "{\"expectedRevision\":5,\"requirePhysicalConfirmation\":false,"
                             "\"alwaysSelectSet\":true,\"activeSetId\":null}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_settings_update(no_active, sizeof(no_active) - 1U,
                                                            &settings, &revision));
    TEST_CHECK_EQ_U64(5U, revision);
    TEST_CHECK(!settings.require_physical_confirmation);
    TEST_CHECK(settings.always_select_set);
    TEST_CHECK(!settings.has_active_set);

    static const char *const invalid[] = {
        "{}",
        "{\"expectedRevision\":4,\"requirePhysicalConfirmation\":1,"
        "\"alwaysSelectSet\":false,\"activeSetId\":null}",
        "{\"expectedRevision\":4,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectSet\":false,\"activeSetId\":\"bad\"}",
        "{\"expectedRevision\":4,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectSet\":false,\"activeSetId\":null,\"extra\":true}",
        "{\"expectedRevision\":4,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectSet\":false,\"activeSetId\":null}x",
    };
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        memset(&settings, 0xa5, sizeof(settings));
        revision = 99U;
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             web_api_json_parse_settings_update(
                                 invalid[index], strlen(invalid[index]), &settings, &revision));
        TEST_CHECK_EQ_U64(0U, revision);
    }
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_json_parse_settings_update(active, sizeof(active) - 1U, NULL, &revision));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_json_parse_settings_update(active, sizeof(active) - 1U, &settings, NULL));
}

static void test_embedded_nul_rejected(void) {
    uint32_t revision = 0U;
    const char nul_escape[] = "{\"expectedRevision\":1,\"x\":\"\\u0000\"}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_json_parse_expected_revision(nul_escape, sizeof(nul_escape) - 1U, &revision));
}

int main(void) {
    test_expected_revision_matrix();
    test_resource_mutation_matrix();
    test_resource_request_boundary();
    test_uuid_order_matrix();
    test_execution_submit_matrix();
    test_progress_action_matrix();
    test_settings_update_matrix();
    test_embedded_nul_rejected();
    return 0;
}
