#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_package.h"
#include "storage_package_internal.h"
#include "storage_repository.h"
#include "test_assert.h"
#include "test_secret_sentinel.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define LOCAL_ID "22222222-2222-4222-8222-222222222222"
#define OTHER_SET_ID "33333333-3333-4333-8333-333333333333"
#define OTHER_SET_MACRO_ID "34343434-3434-4434-8434-343434343434"
#define PROCEDURE_ID "44444444-4444-4444-8444-444444444444"
#define LOCAL_STEP_ID "55555555-5555-4555-8555-555555555555"
#define SECRET_SENTINEL "phase18_5_admin_secret_N7vY5jR3xQ9mK2pL"

typedef struct {
    macro_set_t set;
    macro_t *local_macros;
    size_t local_macro_count;
    /* Macros of a different set: present in the repository, outside this
     * export's scope. */
    macro_t *other_macros;
    size_t other_macro_count;
    procedure_t *procedures;
    size_t procedure_count;
    storage_progress_snapshot_t progress;
    app_error_code_t local_list_result;
    app_error_code_t other_list_result;
    app_error_code_t procedure_list_result;
    app_error_code_t progress_result;
    app_error_code_t unlock_result;
    size_t lock_take_count;
    size_t lock_give_count;
    size_t macro_free_count;
    size_t procedure_free_count;
    size_t progress_read_count;
} fake_export_context_t;

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static app_error_code_t fake_lock_take(void *context) {
    fake_export_context_t *fake = context;
    ++fake->lock_take_count;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_lock_give(void *context) {
    fake_export_context_t *fake = context;
    ++fake->lock_give_count;
    return fake->unlock_result;
}

static app_error_code_t fake_set_read(void *context, const app_uuid_t *set_id,
                                      macro_set_t *out_set) {
    fake_export_context_t *fake = context;
    if (!app_uuid_equal(set_id, &fake->set.id)) {
        return APP_ERROR_NOT_FOUND;
    }
    *out_set = fake->set;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_macro_list(void *context, const app_uuid_t *set_id,
                                        storage_macro_list_t *out_list) {
    fake_export_context_t *fake = context;
    memset(out_list, 0, sizeof(*out_list));
    if (app_uuid_equal(set_id, &fake->set.id)) {
        if (fake->local_list_result != APP_ERROR_NONE) {
            return fake->local_list_result;
        }
        out_list->items = fake->local_macros;
        out_list->count = fake->local_macro_count;
        return APP_ERROR_NONE;
    }
    if (fake->other_list_result != APP_ERROR_NONE) {
        return fake->other_list_result;
    }
    out_list->items = fake->other_macros;
    out_list->count = fake->other_macro_count;
    return APP_ERROR_NONE;
}

static void fake_macro_list_free(void *context, storage_macro_list_t *list) {
    fake_export_context_t *fake = context;
    ++fake->macro_free_count;
    memset(list, 0, sizeof(*list));
}

static app_error_code_t fake_procedure_list(void *context, const app_uuid_t *set_id,
                                            storage_procedure_list_t *out_list) {
    fake_export_context_t *fake = context;
    memset(out_list, 0, sizeof(*out_list));
    if (!app_uuid_equal(set_id, &fake->set.id)) {
        return APP_ERROR_NOT_FOUND;
    }
    if (fake->procedure_list_result != APP_ERROR_NONE) {
        return fake->procedure_list_result;
    }
    out_list->items = fake->procedures;
    out_list->count = fake->procedure_count;
    return APP_ERROR_NONE;
}

static void fake_procedure_list_free(void *context, storage_procedure_list_t *list) {
    fake_export_context_t *fake = context;
    ++fake->procedure_free_count;
    memset(list, 0, sizeof(*list));
}

static app_error_code_t fake_progress_read(void *context,
                                           const storage_procedure_identity_t *identity,
                                           storage_progress_snapshot_t *out_snapshot) {
    fake_export_context_t *fake = context;
    ++fake->progress_read_count;
    if (!app_uuid_equal(&identity->procedure_id, &fake->progress.progress.procedure_id)) {
        return APP_ERROR_NOT_FOUND;
    }
    if (fake->progress_result != APP_ERROR_NONE) {
        return fake->progress_result;
    }
    *out_snapshot = fake->progress;
    return APP_ERROR_NONE;
}

static storage_package_export_ops_t fake_operations(fake_export_context_t *context) {
    return (storage_package_export_ops_t){
        .context = context,
        .lock_take = fake_lock_take,
        .lock_give = fake_lock_give,
        .set_read = fake_set_read,
        .macro_list = fake_macro_list,
        .macro_list_free = fake_macro_list_free,
        .procedure_list = fake_procedure_list,
        .procedure_list_free = fake_procedure_list_free,
        .progress_read = fake_progress_read,
    };
}

static macro_t make_macro(const char *id, const app_uuid_t *set_id, const char *name,
                          char *source) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(id),
        .revision = 1U,
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

static fake_export_context_t valid_context(void) {
    static char local_source[] = "a";
    static char unused_source[] = SECRET_SENTINEL;
    static macro_t local_macros[1];
    static macro_t other_macros[1];
    static procedure_step_t steps[1];
    static procedure_t procedures[1];

    fake_export_context_t context = {0};
    context.set = (macro_set_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_ID),
        .revision = 1U,
        .sort_order = 0,
    };
    snprintf(context.set.name, sizeof(context.set.name), "Set");
    snprintf(context.set.keyboard_layout, sizeof(context.set.keyboard_layout), "en-US");

    const app_uuid_t other_set_id = uuid(OTHER_SET_ID);
    local_macros[0] = make_macro(LOCAL_ID, &context.set.id, "Local", local_source);
    other_macros[0] =
        make_macro(OTHER_SET_MACRO_ID, &other_set_id, "Unreferenced secret", unused_source);
    context.local_macros = local_macros;
    context.local_macro_count = 1U;
    context.other_macros = other_macros;
    context.other_macro_count = 1U;

    steps[0] = (procedure_step_t){
        .id = uuid(LOCAL_STEP_ID),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = local_macros[0].id,
    };
    snprintf(steps[0].title, sizeof(steps[0].title), "Local step");
    procedures[0] = (procedure_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(PROCEDURE_ID),
        .revision = 1U,
        .set_id = context.set.id,
        .steps = steps,
        .step_count = 1U,
        .sort_order = 0,
    };
    snprintf(procedures[0].name, sizeof(procedures[0].name), "Procedure");
    context.procedures = procedures;
    context.procedure_count = 1U;

    context.progress = (storage_progress_snapshot_t){
        .progress =
            {
                .schema_version = APP_SCHEMA_VERSION,
                .set_id = context.set.id,
                .procedure_id = procedures[0].id,
                .procedure_revision = procedures[0].revision,
                .current_step_id = steps[0].id,
            },
        .status = STORAGE_PROGRESS_STATUS_CURRENT,
        .current_procedure_revision = procedures[0].revision,
    };
    return context;
}

static void test_deterministic_export_and_filtering(void) {
    fake_export_context_t context = valid_context();
    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);

    char *first = NULL;
    size_t first_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_set(&context.set.id, true, &first, &first_length));
    TEST_CHECK(first != NULL);
    TEST_CHECK(first_length == strlen(first));
    TEST_CHECK(strstr(first, OTHER_SET_MACRO_ID) == NULL);
    TEST_CHECK(strstr(first, "Unreferenced secret") == NULL);
    TEST_CHECK(strstr(first, "\"progress\":[{") != NULL);
    TEST_CHECK(strstr(first, SECRET_SENTINEL) == NULL);

    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_package_validate(first, first_length, STORAGE_PACKAGE_KIND_SET, &summary));
    TEST_CHECK_EQ_U64(1U, summary.set_count);
    TEST_CHECK_EQ_U64(1U, summary.local_macro_count);
    TEST_CHECK_EQ_U64(1U, summary.procedure_count);
    TEST_CHECK_EQ_U64(1U, summary.progress_count);

    char *second = NULL;
    size_t second_length = 0U;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, storage_package_export_set(&context.set.id, true, &second, &second_length));
    TEST_CHECK_EQ_U64(first_length, second_length);
    TEST_CHECK(memcmp(first, second, first_length) == 0);
    TEST_CHECK_EQ_U64(2U, context.lock_take_count);
    TEST_CHECK_EQ_U64(2U, context.lock_give_count);
    TEST_CHECK_EQ_U64(2U, context.macro_free_count);
    TEST_CHECK_EQ_U64(2U, context.procedure_free_count);
    TEST_CHECK_EQ_U64(2U, context.progress_read_count);

    storage_package_free(first);
    storage_package_free(second);
    storage_package_reset_export_ops_for_test();
}

static void test_export_output_passes_secret_sentinel_scanner(void) {
    /* FIX1 18.5: production-path proof, not a reimplemented substring check.
     * unused_source (a macro of a different set, outside this export's scope)
     * is SECRET_SENTINEL itself, so this
     * exercises the real scripts/check-secret-sentinel.py - all 7 encodings
     * it checks - against the real export output of a repository that
     * genuinely holds the sentinel, just outside this export's scope. */
    fake_export_context_t context = valid_context();
    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);

    char *output = NULL;
    size_t output_length = 0U;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, storage_package_export_set(&context.set.id, true, &output, &output_length));

    const char *outputs[] = {output};
    test_assert_no_secret_sentinel(SECRET_SENTINEL, outputs, 1U);

    storage_package_free(output);
    storage_package_reset_export_ops_for_test();
}

static void test_progress_is_optional_and_stale_progress_is_omitted(void) {
    fake_export_context_t context = valid_context();
    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);

    char *without_progress = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_package_export_set(&context.set.id, false,
                                                                    &without_progress, &length));
    TEST_CHECK(strstr(without_progress, "\"progress\":[]") != NULL);
    TEST_CHECK_EQ_U64(0U, context.progress_read_count);
    storage_package_free(without_progress);

    context.progress.status = STORAGE_PROGRESS_STATUS_STALE;
    char *stale = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_set(&context.set.id, true, &stale, &length));
    TEST_CHECK(strstr(stale, "\"progress\":[]") != NULL);
    TEST_CHECK_EQ_U64(1U, context.progress_read_count);
    storage_package_free(stale);
    storage_package_reset_export_ops_for_test();
}

static void test_failure_cleanup_and_primary_error_preservation(void) {
    fake_export_context_t context = valid_context();
    context.local_list_result = APP_ERROR_IO;
    context.unlock_result = APP_ERROR_INTERNAL;
    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);

    char *data = (char *)1;
    size_t length = 99U;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         storage_package_export_set(&context.set.id, true, &data, &length));
    TEST_CHECK(data == NULL);
    TEST_CHECK_EQ_U64(0U, length);
    TEST_CHECK_EQ_U64(1U, context.lock_take_count);
    TEST_CHECK_EQ_U64(1U, context.lock_give_count);
    TEST_CHECK_EQ_U64(1U, context.macro_free_count);
    TEST_CHECK_EQ_U64(1U, context.procedure_free_count);
    storage_package_reset_export_ops_for_test();
}

static void test_unresolved_reference_fails_closed(void) {
    fake_export_context_t context = valid_context();
    /* A step referencing a macro the set does not contain must fail the export
     * rather than emit a dangling reference. */
    context.local_macro_count = 0U;
    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);

    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_package_export_set(&context.set.id, false, &data, &length));
    TEST_CHECK(data == NULL);
    storage_package_reset_export_ops_for_test();
}

/* With global macros gone a single set's macros can no longer reach the 512 KiB
 * package ceiling on their own (100 x 4096 bytes of source is roughly 446 KiB),
 * so the overflow is driven by procedures, which are also per-set. */
static void test_output_limit_is_enforced(void) {
    fake_export_context_t context = valid_context();
    char *large_source = malloc(APP_MACRO_SOURCE_MAX_BYTES + 1U);
    macro_t *local_macros = calloc(APP_MACROS_PER_SET_MAX, sizeof(*local_macros));
    procedure_t *procedures = calloc(APP_PROCEDURES_PER_SET_MAX, sizeof(*procedures));
    procedure_step_t *steps =
        calloc((size_t)APP_PROCEDURES_PER_SET_MAX * APP_STEPS_PER_PROCEDURE_MAX, sizeof(*steps));
    TEST_CHECK(large_source != NULL);
    TEST_CHECK(local_macros != NULL);
    TEST_CHECK(procedures != NULL);
    TEST_CHECK(steps != NULL);
    memset(large_source, 'a', APP_MACRO_SOURCE_MAX_BYTES);
    large_source[APP_MACRO_SOURCE_MAX_BYTES] = '\0';

    for (size_t index = 0U; index < APP_MACROS_PER_SET_MAX; ++index) {
        char id[APP_UUID_BUFFER_LENGTH];
        snprintf(id, sizeof(id), "10000000-0000-4000-8000-%012zu", index);
        local_macros[index] = make_macro(id, &context.set.id, "Large local", large_source);
    }
    for (size_t index = 0U; index < APP_PROCEDURES_PER_SET_MAX; ++index) {
        char id[APP_UUID_BUFFER_LENGTH];
        snprintf(id, sizeof(id), "20000000-0000-4000-8000-%012zu", index);
        procedure_step_t *slice = &steps[index * APP_STEPS_PER_PROCEDURE_MAX];
        for (size_t step = 0U; step < APP_STEPS_PER_PROCEDURE_MAX; ++step) {
            char step_id[APP_UUID_BUFFER_LENGTH];
            snprintf(step_id, sizeof(step_id), "30000000-%04zu-4000-8000-%012zu", index, step);
            slice[step] = (procedure_step_t){
                .id = uuid(step_id),
                .type = PROCEDURE_STEP_MACRO,
                .required = true,
                .has_macro_id = true,
                .macro_id = local_macros[step % APP_MACROS_PER_SET_MAX].id,
            };
            snprintf(slice[step].title, sizeof(slice[step].title), "Step %zu", step);
        }
        procedures[index] = (procedure_t){
            .schema_version = APP_SCHEMA_VERSION,
            .id = uuid(id),
            .revision = 1U,
            .set_id = context.set.id,
            .steps = slice,
            .step_count = APP_STEPS_PER_PROCEDURE_MAX,
            .sort_order = (int32_t)index,
        };
        snprintf(procedures[index].name, sizeof(procedures[index].name), "Procedure %zu", index);
    }
    context.local_macros = local_macros;
    context.local_macro_count = APP_MACROS_PER_SET_MAX;
    context.procedures = procedures;
    context.procedure_count = APP_PROCEDURES_PER_SET_MAX;
    context.progress_result = APP_ERROR_NOT_FOUND;

    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_MACRO_LIMIT,
                         storage_package_export_set(&context.set.id, false, &data, &length));
    TEST_CHECK(data == NULL);

    storage_package_reset_export_ops_for_test();
    free(steps);
    free(procedures);
    free(local_macros);
    free(large_source);
}

static void test_argument_and_operations_validation(void) {
    storage_package_reset_export_ops_for_test();
    const app_uuid_t set_id = uuid(SET_ID);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_export_set(&set_id, false, &data, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_export_set(NULL, false, &data, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_export_set(&set_id, false, NULL, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_export_set(&set_id, false, &data, NULL));
}

int main(void) {
    test_deterministic_export_and_filtering();
    test_export_output_passes_secret_sentinel_scanner();
    test_progress_is_optional_and_stale_progress_is_omitted();
    test_failure_cleanup_and_primary_error_preservation();
    test_unresolved_reference_fails_closed();
    test_output_limit_is_enforced();
    test_argument_and_operations_validation();
    puts("storage package export tests passed");
    return EXIT_SUCCESS;
}
