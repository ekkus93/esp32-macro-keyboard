#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_package.h"
#include "storage_package_internal.h"
#include "storage_repository.h"
#include "test_assert.h"

#define SET_A_ID "11111111-1111-4111-8111-111111111111"
#define SET_B_ID "12121212-1212-4212-8212-121212121212"
#define LOCAL_A_ID "22222222-2222-4222-8222-222222222222"
#define LOCAL_B_ID "23232323-2323-4232-8232-232323232323"
#define GLOBAL_A_ID "33333333-3333-4333-8333-333333333333"
#define GLOBAL_B_ID "34343434-3434-4434-8434-343434343434"
#define PROCEDURE_A_ID "44444444-4444-4444-8444-444444444444"
#define PROCEDURE_B_ID "45454545-4545-4545-8545-454545454545"
#define STEP_A_ID "55555555-5555-4555-8555-555555555555"
#define STEP_B_ID "56565656-5656-4656-8656-565656565656"

typedef struct {
    storage_set_list_t sets;
    storage_macro_list_t local[2];
    storage_macro_list_t globals;
    storage_procedure_list_t procedures[2];
    storage_progress_snapshot_t progress[2];
    app_error_code_t local_result[2];
    app_error_code_t unlock_result;
    size_t lock_take_count;
    size_t lock_give_count;
    size_t macro_free_count;
    size_t procedure_free_count;
    size_t progress_read_count;
} fake_backup_context_t;

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static size_t set_index(const fake_backup_context_t *context, const app_uuid_t *set_id) {
    for (size_t index = 0U; index < context->sets.count; ++index) {
        if (app_uuid_equal(&context->sets.items[index].id, set_id)) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t fake_lock_take(void *context) {
    fake_backup_context_t *fake = context;
    ++fake->lock_take_count;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_lock_give(void *context) {
    fake_backup_context_t *fake = context;
    ++fake->lock_give_count;
    return fake->unlock_result;
}

static app_error_code_t fake_set_list(void *context, storage_set_list_t *out_list) {
    const fake_backup_context_t *fake = context;
    *out_list = fake->sets;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_macro_list(void *context, const storage_macro_location_t *location,
                                        storage_macro_list_t *out_list) {
    fake_backup_context_t *fake = context;
    memset(out_list, 0, sizeof(*out_list));
    if (location->scope == MACRO_SCOPE_GLOBAL) {
        *out_list = fake->globals;
        return APP_ERROR_NONE;
    }
    const size_t index = set_index(fake, &location->set_id);
    if (index == SIZE_MAX) {
        return APP_ERROR_NOT_FOUND;
    }
    if (fake->local_result[index] != APP_ERROR_NONE) {
        return fake->local_result[index];
    }
    *out_list = fake->local[index];
    return APP_ERROR_NONE;
}

static void fake_macro_list_free(void *context, storage_macro_list_t *list) {
    fake_backup_context_t *fake = context;
    ++fake->macro_free_count;
    memset(list, 0, sizeof(*list));
}

static app_error_code_t fake_procedure_list(void *context, const app_uuid_t *set_id,
                                            storage_procedure_list_t *out_list) {
    fake_backup_context_t *fake = context;
    const size_t index = set_index(fake, set_id);
    if (index == SIZE_MAX) {
        return APP_ERROR_NOT_FOUND;
    }
    *out_list = fake->procedures[index];
    return APP_ERROR_NONE;
}

static void fake_procedure_list_free(void *context, storage_procedure_list_t *list) {
    fake_backup_context_t *fake = context;
    ++fake->procedure_free_count;
    memset(list, 0, sizeof(*list));
}

static app_error_code_t fake_progress_read(void *context,
                                           const storage_procedure_identity_t *identity,
                                           storage_progress_snapshot_t *out_snapshot) {
    fake_backup_context_t *fake = context;
    ++fake->progress_read_count;
    const size_t index = set_index(fake, &identity->set_id);
    if (index == SIZE_MAX ||
        !app_uuid_equal(&identity->procedure_id, &fake->progress[index].progress.procedure_id)) {
        return APP_ERROR_NOT_FOUND;
    }
    *out_snapshot = fake->progress[index];
    return APP_ERROR_NONE;
}

static storage_package_backup_ops_t fake_operations(fake_backup_context_t *context) {
    return (storage_package_backup_ops_t){
        .context = context,
        .lock_take = fake_lock_take,
        .lock_give = fake_lock_give,
        .set_list = fake_set_list,
        .macro_list = fake_macro_list,
        .macro_list_free = fake_macro_list_free,
        .procedure_list = fake_procedure_list,
        .procedure_list_free = fake_procedure_list_free,
        .progress_read = fake_progress_read,
    };
}

static macro_t make_macro(const char *id, macro_scope_t scope, const app_uuid_t *set_id,
                          const char *name, char *source) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(id),
        .revision = 1U,
        .scope = scope,
        .has_set_id = scope == MACRO_SCOPE_SET,
        .source = source,
        .source_length = strlen(source),
        .favorite = false,
        .key_press_ms = APP_KEY_PRESS_DEFAULT_MS,
        .inter_key_ms = APP_INTER_KEY_DEFAULT_MS,
    };
    if (set_id != NULL) {
        macro.set_id = *set_id;
    }
    snprintf(macro.name, sizeof(macro.name), "%s", name);
    return macro;
}

static fake_backup_context_t valid_context(void) {
    static char local_a_source[] = "a";
    static char local_b_source[] = "b";
    static char global_a_source[] = "c";
    static char global_b_source[] = "sentinel-unreferenced-global";
    static macro_t local_a[1];
    static macro_t local_b[1];
    static macro_t globals[2];
    static procedure_step_t steps[2];
    static procedure_t procedures_a[1];
    static procedure_t procedures_b[1];

    fake_backup_context_t context = {0};
    context.sets.count = 2U;
    context.sets.items[0] = (macro_set_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_A_ID),
        .revision = 1U,
        .sort_order = 0,
    };
    context.sets.items[1] = (macro_set_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_B_ID),
        .revision = 2U,
        .sort_order = 1,
    };
    snprintf(context.sets.items[0].name, sizeof(context.sets.items[0].name), "Set A");
    snprintf(context.sets.items[1].name, sizeof(context.sets.items[1].name), "Set B");
    snprintf(context.sets.items[0].keyboard_layout,
             sizeof(context.sets.items[0].keyboard_layout), "en-US");
    snprintf(context.sets.items[1].keyboard_layout,
             sizeof(context.sets.items[1].keyboard_layout), "en-US");

    local_a[0] = make_macro(LOCAL_A_ID, MACRO_SCOPE_SET, &context.sets.items[0].id, "Local A",
                            local_a_source);
    local_b[0] = make_macro(LOCAL_B_ID, MACRO_SCOPE_SET, &context.sets.items[1].id, "Local B",
                            local_b_source);
    globals[0] = make_macro(GLOBAL_A_ID, MACRO_SCOPE_GLOBAL, NULL, "Global A", global_a_source);
    globals[1] = make_macro(GLOBAL_B_ID, MACRO_SCOPE_GLOBAL, NULL, "Global B", global_b_source);
    context.local[0] = (storage_macro_list_t){.items = local_a, .count = 1U};
    context.local[1] = (storage_macro_list_t){.items = local_b, .count = 1U};
    context.globals = (storage_macro_list_t){.items = globals, .count = 2U};

    steps[0] = (procedure_step_t){
        .id = uuid(STEP_A_ID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = local_a[0].id,
    };
    steps[1] = (procedure_step_t){
        .id = uuid(STEP_B_ID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = globals[0].id,
    };
    snprintf(steps[0].title, sizeof(steps[0].title), "Step A");
    snprintf(steps[1].title, sizeof(steps[1].title), "Step B");
    procedures_a[0] = (procedure_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(PROCEDURE_A_ID),
        .revision = 1U,
        .set_id = context.sets.items[0].id,
        .steps = &steps[0],
        .step_count = 1U,
        .sort_order = 0,
    };
    procedures_b[0] = (procedure_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(PROCEDURE_B_ID),
        .revision = 2U,
        .set_id = context.sets.items[1].id,
        .steps = &steps[1],
        .step_count = 1U,
        .sort_order = 0,
    };
    snprintf(procedures_a[0].name, sizeof(procedures_a[0].name), "Procedure A");
    snprintf(procedures_b[0].name, sizeof(procedures_b[0].name), "Procedure B");
    context.procedures[0] =
        (storage_procedure_list_t){.items = procedures_a, .count = 1U};
    context.procedures[1] =
        (storage_procedure_list_t){.items = procedures_b, .count = 1U};

    context.progress[0] = (storage_progress_snapshot_t){
        .progress =
            {
                .schema_version = APP_SCHEMA_VERSION,
                .set_id = context.sets.items[0].id,
                .procedure_id = procedures_a[0].id,
                .procedure_revision = procedures_a[0].revision,
                .current_step_id = steps[0].id,
            },
        .status = STORAGE_PROGRESS_STATUS_CURRENT,
        .current_procedure_revision = procedures_a[0].revision,
    };
    context.progress[1] = (storage_progress_snapshot_t){
        .progress =
            {
                .schema_version = APP_SCHEMA_VERSION,
                .set_id = context.sets.items[1].id,
                .procedure_id = procedures_b[0].id,
                .procedure_revision = procedures_b[0].revision,
                .current_step_id = steps[1].id,
            },
        .status = STORAGE_PROGRESS_STATUS_CURRENT,
        .current_procedure_revision = procedures_b[0].revision,
    };
    return context;
}

static void test_backup_contains_complete_repository_deterministically(void) {
    fake_backup_context_t context = valid_context();
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);

    char *first = NULL;
    size_t first_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_backup(true, &first, &first_length));
    TEST_CHECK(first != NULL);
    TEST_CHECK(first_length == strlen(first));
    TEST_CHECK(strstr(first, "\"package_type\":\"backup\"") != NULL);
    TEST_CHECK(strstr(first, SET_A_ID) != NULL);
    TEST_CHECK(strstr(first, SET_B_ID) != NULL);
    TEST_CHECK(strstr(first, "sentinel-unreferenced-global") != NULL);

    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_package_validate(first, first_length, STORAGE_PACKAGE_KIND_BACKUP, &summary));
    TEST_CHECK_EQ_U64(2U, summary.set_count);
    TEST_CHECK_EQ_U64(2U, summary.local_macro_count);
    TEST_CHECK_EQ_U64(2U, summary.global_macro_count);
    TEST_CHECK_EQ_U64(2U, summary.procedure_count);
    TEST_CHECK_EQ_U64(2U, summary.progress_count);

    char *second = NULL;
    size_t second_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_backup(true, &second, &second_length));
    TEST_CHECK_EQ_U64(first_length, second_length);
    TEST_CHECK(memcmp(first, second, first_length) == 0);
    TEST_CHECK_EQ_U64(2U, context.lock_take_count);
    TEST_CHECK_EQ_U64(2U, context.lock_give_count);
    storage_package_free(first);
    storage_package_free(second);
    storage_package_reset_backup_ops_for_test();
}

static void test_progress_is_optional(void) {
    fake_backup_context_t context = valid_context();
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_backup(false, &data, &length));
    TEST_CHECK(strstr(data, "\"progress\":[]") != NULL);
    TEST_CHECK_EQ_U64(0U, context.progress_read_count);
    storage_package_free(data);
    storage_package_reset_backup_ops_for_test();
}

static void test_cross_set_reference_fails_closed(void) {
    fake_backup_context_t context = valid_context();
    context.procedures[0].items[0].steps[0].macro_id = context.local[1].items[0].id;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_package_export_backup(true, &data, &length));
    TEST_CHECK(data == NULL);
    storage_package_reset_backup_ops_for_test();
}

static void test_failure_preserves_primary_error_and_cleans_partial_snapshot(void) {
    fake_backup_context_t context = valid_context();
    context.local_result[1] = APP_ERROR_IO;
    context.unlock_result = APP_ERROR_INTERNAL;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = (char *)1;
    size_t length = 99U;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_package_export_backup(true, &data, &length));
    TEST_CHECK(data == NULL);
    TEST_CHECK_EQ_U64(0U, length);
    TEST_CHECK_EQ_U64(1U, context.lock_take_count);
    TEST_CHECK_EQ_U64(1U, context.lock_give_count);
    TEST_CHECK_EQ_U64(3U, context.macro_free_count);
    TEST_CHECK_EQ_U64(2U, context.procedure_free_count);
    storage_package_reset_backup_ops_for_test();
}

int main(void) {
    test_backup_contains_complete_repository_deterministically();
    test_progress_is_optional();
    test_cross_set_reference_fails_closed();
    test_failure_preserves_primary_error_and_cleans_partial_snapshot();
    puts("storage package backup tests passed");
    return 0;
}
