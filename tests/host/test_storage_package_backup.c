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
#include "test_secret_sentinel.h"

#define SET_A_ID "11111111-1111-4111-8111-111111111111"
#define SET_B_ID "12121212-1212-4212-8212-121212121212"
#define LOCAL_A_ID "22222222-2222-4222-8222-222222222222"
#define LOCAL_B_ID "23232323-2323-4232-8232-232323232323"
#define PROCEDURE_A_ID "44444444-4444-4444-8444-444444444444"
#define PROCEDURE_B_ID "45454545-4545-4545-8545-454545454545"
#define STEP_A_ID "55555555-5555-4555-8555-555555555555"
#define STEP_B_ID "56565656-5656-4656-8656-565656565656"
#define SECRET_SENTINEL "phase18_5_admin_secret_N7vY5jR3xQ9mK2pL"

typedef struct {
    storage_set_list_t sets;
    storage_macro_list_t local[2];
    storage_procedure_list_t procedures[2];
    storage_progress_snapshot_t progress[2];
    app_error_code_t local_result[2];
    /* Which macro the fake reports as the one it stopped on. */
    app_uuid_t local_failed_id[2];
    bool local_failed_known[2];
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

static app_error_code_t fake_set_list(void *context, storage_set_list_t *out_list,
                                      storage_object_ref_t *out_failed,
                                      storage_skip_record_t *out_skips) {
    const fake_backup_context_t *fake = context;
    (void)out_failed;
    (void)out_skips;
    *out_list = fake->sets;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_macro_list(void *context, const app_uuid_t *set_id,
                                        storage_macro_list_t *out_list,
                                        storage_object_ref_t *out_failed,
                                        storage_skip_record_t *out_skips) {
    fake_backup_context_t *fake = context;
    memset(out_list, 0, sizeof(*out_list));
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
    const size_t index = set_index(fake, set_id);
    if (index == SIZE_MAX) {
        return APP_ERROR_NOT_FOUND;
    }
    if (fake->local_result[index] != APP_ERROR_NONE) {
        if (out_skips != NULL && app_error_is_object_fault(fake->local_result[index])) {
            ++out_skips->total;
            if (out_skips->items != NULL && out_skips->count < out_skips->capacity &&
                fake->local_failed_known[index]) {
                out_skips->items[out_skips->count].has_id = true;
                out_skips->items[out_skips->count].id = fake->local_failed_id[index];
                ++out_skips->count;
            }
            return APP_ERROR_NONE;
        }
        if (out_failed != NULL && fake->local_failed_known[index]) {
            out_failed->has_id = true;
            out_failed->id = fake->local_failed_id[index];
        }
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
                                            storage_procedure_list_t *out_list,
                                            storage_object_ref_t *out_failed,
                                            storage_skip_record_t *out_skips) {
    fake_backup_context_t *fake = context;
    (void)out_skips;
    if (out_failed != NULL) {
        memset(out_failed, 0, sizeof(*out_failed));
    }
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

static fake_backup_context_t valid_context(void) {
    static char local_a_source[] = "a";
    static char local_b_source[] = "b";
    static macro_t local_a[1];
    static macro_t local_b[1];
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
    snprintf(context.sets.items[0].keyboard_layout, sizeof(context.sets.items[0].keyboard_layout),
             "en-US");
    snprintf(context.sets.items[1].keyboard_layout, sizeof(context.sets.items[1].keyboard_layout),
             "en-US");

    local_a[0] = make_macro(LOCAL_A_ID, &context.sets.items[0].id, "Local A", local_a_source);
    local_b[0] = make_macro(LOCAL_B_ID, &context.sets.items[1].id, "Local B", local_b_source);
    context.local[0] = (storage_macro_list_t){.items = local_a, .count = 1U};
    context.local[1] = (storage_macro_list_t){.items = local_b, .count = 1U};

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
        .macro_id = local_b[0].id,
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
    context.procedures[0] = (storage_procedure_list_t){.items = procedures_a, .count = 1U};
    context.procedures[1] = (storage_procedure_list_t){.items = procedures_b, .count = 1U};

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
    TEST_CHECK(strstr(first, SECRET_SENTINEL) == NULL);

    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_package_validate(first, first_length, STORAGE_PACKAGE_KIND_BACKUP, &summary));
    TEST_CHECK_EQ_U64(2U, summary.set_count);
    TEST_CHECK_EQ_U64(2U, summary.local_macro_count);
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

static void test_backup_output_passes_secret_sentinel_scanner(void) {
    /* FIX1 18.5: production-path proof, not a reimplemented substring check.
     * A full backup covers the entire repository (no scope filtering), so
     * unlike set export there is no "referenced vs. unreferenced" boundary
     * to exploit here; the property under test is that
     * storage_package_export_backup - which only ever reads set/macro/
     * procedure/progress repository data - cannot leak a real secret that
     * exists elsewhere in the same process, checked with the real
     * scripts/check-secret-sentinel.py (all 7 encodings it checks) against
     * the real backup output. */
    fake_backup_context_t context = valid_context();
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);

    char *output = NULL;
    size_t output_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_backup(true, &output, &output_length));

    const char *outputs[] = {output};
    test_assert_no_secret_sentinel(SECRET_SENTINEL, outputs, 1U);

    storage_package_free(output);
    storage_package_reset_backup_ops_for_test();
}

static void test_progress_is_optional(void) {
    fake_backup_context_t context = valid_context();
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_package_export_backup(false, &data, &length));
    TEST_CHECK(strstr(data, "\"progress\":[]") != NULL);
    TEST_CHECK_EQ_U64(0U, context.progress_read_count);
    storage_package_free(data);
    storage_package_reset_backup_ops_for_test();
}

/* A procedure that references a macro outside its own set cannot go in the
 * package - the validator rejects it - but it must no longer take the whole
 * backup down with it. It is dropped, reported, and the rest is exported. */
static void test_cross_set_reference_is_skipped_not_fatal(void) {
    fake_backup_context_t context = valid_context();
    const app_uuid_t orphan = context.procedures[0].items[0].id;
    context.procedures[0].items[0].steps[0].macro_id = context.local[1].items[0].id;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    storage_package_skip_report_t skipped = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, storage_package_export_backup_detail(true, &data, &length, NULL, &skipped));
    TEST_CHECK(data != NULL);
    /* The offending procedure is gone from the package body. It still appears
     * in the trailing "skipped" record - that is the whole point - so only the
     * content before that record may be searched for it. */
    const char *skipped_marker = strstr(data, "\"skipped\"");
    TEST_CHECK(skipped_marker != NULL);
    const char *orphan_in_body = strstr(data, orphan.value);
    TEST_CHECK(orphan_in_body != NULL && orphan_in_body > skipped_marker);
    TEST_CHECK(strstr(data, "\"total\":1") != NULL);
    TEST_CHECK_EQ_U64(1U, skipped.total);
    TEST_CHECK_EQ_U64(1U, skipped.count);
    TEST_CHECK(skipped.items[0].kind == STORAGE_PACKAGE_OBJECT_PROCEDURE);
    TEST_CHECK(app_uuid_equal(&skipped.items[0].object_id, &orphan));
    storage_package_free(data);
    storage_package_reset_backup_ops_for_test();
}

/* One unreadable macro must not make the repository unbackupable. */
static void test_unreadable_macro_is_skipped_and_recorded(void) {
    fake_backup_context_t context = valid_context();
    const app_uuid_t offender = uuid(LOCAL_B_ID);
    context.local_result[1] = APP_ERROR_MACRO_SYNTAX;
    context.local_failed_known[1] = true;
    context.local_failed_id[1] = offender;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    storage_package_skip_report_t skipped = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, storage_package_export_backup_detail(true, &data, &length, NULL, &skipped));
    TEST_CHECK(data != NULL);
    TEST_CHECK(skipped.total >= 1U);
    TEST_CHECK(skipped.items[0].kind == STORAGE_PACKAGE_OBJECT_MACRO);
    TEST_CHECK(app_uuid_equal(&skipped.items[0].object_id, &offender));
    TEST_CHECK(strstr(data, "\"skipped\"") != NULL);
    TEST_CHECK(strstr(data, offender.value) != NULL);
    storage_package_free(data);
    storage_package_reset_backup_ops_for_test();
}

/* A partial package must still be a valid package: the emitted "skipped" record
 * cannot make it unrestorable. */
static void test_partial_package_still_validates(void) {
    fake_backup_context_t context = valid_context();
    context.local_result[1] = APP_ERROR_STORAGE_CORRUPT;
    context.local_failed_known[1] = true;
    context.local_failed_id[1] = uuid(LOCAL_B_ID);
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export_backup_detail(true, &data, &length, NULL, NULL));
    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_package_validate(
                                             data, length, STORAGE_PACKAGE_KIND_BACKUP, &summary));
    storage_package_free(data);
    storage_package_reset_backup_ops_for_test();
}

/* A device-level fault is not a bad object: it must still fail the export
 * rather than silently dropping good data. */
static void test_device_fault_still_fails_the_export(void) {
    fake_backup_context_t context = valid_context();
    context.local_result[1] = APP_ERROR_IO;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    storage_package_skip_report_t skipped = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_IO, storage_package_export_backup_detail(true, &data, &length, NULL, &skipped));
    TEST_CHECK(data == NULL);
    TEST_CHECK_EQ_U64(0U, skipped.total);
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
    TEST_CHECK_EQ_U64(2U, context.macro_free_count);
    TEST_CHECK_EQ_U64(2U, context.procedure_free_count);
    storage_package_reset_backup_ops_for_test();
}

/* A device fault still aborts the export. Reporting only that it failed gave
 * the user nothing to act on, so it must say which object stopped it. */
static void test_failure_names_the_offending_macro(void) {
    fake_backup_context_t context = valid_context();
    const app_uuid_t offender = uuid(LOCAL_B_ID);
    context.local_result[1] = APP_ERROR_IO;
    context.local_failed_known[1] = true;
    context.local_failed_id[1] = offender;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    storage_package_failure_t failure = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_IO, storage_package_export_backup_detail(true, &data, &length, &failure, NULL));
    TEST_CHECK(data == NULL);
    TEST_CHECK(failure.kind == STORAGE_PACKAGE_OBJECT_MACRO);
    TEST_CHECK(failure.has_object_id);
    TEST_CHECK(app_uuid_equal(&failure.object_id, &offender));
    TEST_CHECK(failure.has_set_id);
    TEST_CHECK(app_uuid_equal(&failure.set_id, &context.sets.items[1].id));
    storage_package_reset_backup_ops_for_test();
}

/* When the reader cannot say which object failed, the set is still reported;
 * the caller must never be handed a half-populated identity it might print. */
static void test_failure_without_object_id_still_names_the_set(void) {
    fake_backup_context_t context = valid_context();
    context.local_result[1] = APP_ERROR_IO;
    context.local_failed_known[1] = false;
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    storage_package_failure_t failure = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_IO, storage_package_export_backup_detail(true, &data, &length, &failure, NULL));
    TEST_CHECK(failure.kind == STORAGE_PACKAGE_OBJECT_MACRO);
    TEST_CHECK(!failure.has_object_id);
    TEST_CHECK(failure.has_set_id);
    TEST_CHECK(app_uuid_equal(&failure.set_id, &context.sets.items[1].id));
    storage_package_reset_backup_ops_for_test();
}

/* A successful export must not leave stale identity behind. */
static void test_success_reports_no_failure(void) {
    fake_backup_context_t context = valid_context();
    const storage_package_backup_ops_t operations = fake_operations(&context);
    storage_package_set_backup_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    storage_package_failure_t failure = {.kind = STORAGE_PACKAGE_OBJECT_MACRO,
                                         .has_object_id = true};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, storage_package_export_backup_detail(true, &data, &length, &failure, NULL));
    TEST_CHECK(failure.kind == STORAGE_PACKAGE_OBJECT_NONE);
    TEST_CHECK(!failure.has_object_id);
    TEST_CHECK(!failure.has_set_id);
    storage_package_free(data);
    storage_package_reset_backup_ops_for_test();
}

int main(void) {
    test_backup_contains_complete_repository_deterministically();
    test_backup_output_passes_secret_sentinel_scanner();
    test_progress_is_optional();
    test_cross_set_reference_is_skipped_not_fatal();
    test_unreadable_macro_is_skipped_and_recorded();
    test_partial_package_still_validates();
    test_device_fault_still_fails_the_export();
    test_failure_preserves_primary_error_and_cleans_partial_snapshot();
    test_failure_names_the_offending_macro();
    test_failure_without_object_id_still_names_the_set();
    test_success_reports_no_failure();
    puts("storage package backup tests passed");
    return 0;
}
