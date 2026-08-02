#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "macro_limits.h"
#include "storage_package.h"
#include "test_assert.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define OTHER_SET_ID "12121212-1212-4212-8212-121212121212"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define STEP_ID "44444444-4444-4444-8444-444444444444"
#define OTHER_STEP_ID "45454545-4545-4545-8545-454545454545"

#define SET_OBJECT "{\"schema_version\":1,\"id\":\"" SET_ID "\",\"revision\":1,\"name\":\"Set\"}"

#define OTHER_SET_OBJECT                                                                           \
    "{\"schema_version\":1,\"id\":\"" OTHER_SET_ID "\",\"revision\":1,\"name\":\"Other\"}"

#define LOCAL_MACRO_OBJECT                                                                         \
    "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID                                               \
    "\",\"revision\":1,\"name\":\"Local\",\"source\":\"a\","                                       \
    "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}"

#define PROCEDURE_OBJECT                                                                           \
    "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID "\",\"revision\":1,\"set_id\":\"" SET_ID        \
    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{\"id\":\"" STEP_ID                 \
    "\",\"type\":\"macro\",\"title\":\"Step\",\"macro_id\":\"" LOCAL_MACRO_ID                      \
    "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}"

#define PROGRESS_OBJECT                                                                            \
    "{\"schema_version\":1,\"set_id\":\"" SET_ID "\",\"procedure_id\":\"" PROCEDURE_ID             \
    "\",\"procedure_revision\":1,\"current_step_id\":\"" STEP_ID                                   \
    "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}"

#define PACKAGE_PREFIX(TYPE_VALUE)                                                                 \
    "{\"schema_version\":1,\"package_type\":\"" TYPE_VALUE "\",\"sets\":["

#define PACKAGE_SUFFIX                                                                             \
    "],\"macros\":[" LOCAL_MACRO_OBJECT "],\"procedures\":[" PROCEDURE_OBJECT                      \
    "],\"progress\":[" PROGRESS_OBJECT "]}"

static const char VALID_SET_PACKAGE[] = PACKAGE_PREFIX("set") SET_OBJECT PACKAGE_SUFFIX;
static const char VALID_BACKUP_PACKAGE[] = PACKAGE_PREFIX("backup") SET_OBJECT PACKAGE_SUFFIX;

static void test_valid_set_and_backup_packages(void) {
    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(VALID_SET_PACKAGE, sizeof(VALID_SET_PACKAGE) - 1U,
                                                  STORAGE_PACKAGE_KIND_SET, &summary));
    TEST_CHECK_EQ_U64(STORAGE_PACKAGE_KIND_SET, summary.kind);
    TEST_CHECK_EQ_U64(sizeof(VALID_SET_PACKAGE) - 1U, summary.package_bytes);
    TEST_CHECK_EQ_U64(1U, summary.set_count);
    TEST_CHECK_EQ_U64(1U, summary.local_macro_count);
    TEST_CHECK_EQ_U64(1U, summary.procedure_count);
    TEST_CHECK_EQ_U64(1U, summary.progress_count);

    memset(&summary, 0, sizeof(summary));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(VALID_BACKUP_PACKAGE,
                                                  sizeof(VALID_BACKUP_PACKAGE) - 1U,
                                                  STORAGE_PACKAGE_KIND_BACKUP, &summary));
    TEST_CHECK_EQ_U64(STORAGE_PACKAGE_KIND_BACKUP, summary.kind);
    TEST_CHECK_EQ_U64(1U, summary.set_count);
}

static void test_top_level_contract(void) {
    static const char *const invalid[] = {
        "{}",
        "[]",
        "{\"schema_version\":2,\"package_type\":\"set\",\"sets\":[],\"macros\":[],"
        "\"procedures\":[],\"progress\":[]}",
        "{\"schema_version\":1,\"package_type\":\"future\",\"sets\":[],\"macros\":[],"
        "\"procedures\":[],\"progress\":[]}",
        "{\"schema_version\":1,\"schema_version\":1,\"package_type\":\"set\","
        "\"sets\":[],\"macros\":[],\"procedures\":[],"
        "\"progress\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[],\"macros\":[],"
        "\"procedures\":[],\"progress\":[],\"future\":true}",
        "{\"schema_version\":1,\"package_type\":\"se\\u0000t\",\"sets\":[],"
        "\"macros\":[],\"procedures\":[],\"progress\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":{},\"macros\":[],"
        "\"procedures\":[],\"progress\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[1],\"macros\":[],"
        "\"procedures\":[],\"progress\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[],\"macros\":[],"
        "\"procedures\":[],\"progress\":[]}x",
    };
    storage_package_summary_t summary = {0};
    for (size_t index = 0U; index < sizeof(invalid) / sizeof(invalid[0]); ++index) {
        memset(&summary, 0xa5, sizeof(summary));
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             storage_package_validate(invalid[index], strlen(invalid[index]),
                                                      STORAGE_PACKAGE_KIND_SET, &summary));
        TEST_CHECK_EQ_U64(0U, summary.package_bytes);
    }

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_validate(VALID_SET_PACKAGE, sizeof(VALID_SET_PACKAGE) - 1U,
                                                  STORAGE_PACKAGE_KIND_BACKUP, &summary));
}

static void test_object_and_reference_validation(void) {
    const char unknown_set_field[] = PACKAGE_PREFIX("set") "{\"schema_version\":1,\"id\":\"" SET_ID
                                                           "\",\"revision\":1,\"name\":\"Set\",,"
                                                           "\"future\":true}" PACKAGE_SUFFIX;
    const char duplicate_set[] = PACKAGE_PREFIX("backup") SET_OBJECT "," SET_OBJECT PACKAGE_SUFFIX;
    const char wrong_set_count[] =
        PACKAGE_PREFIX("set") SET_OBJECT "," OTHER_SET_OBJECT PACKAGE_SUFFIX;
    const char bad_macro_syntax[] = PACKAGE_PREFIX("set") SET_OBJECT
        "],\"macros\":[{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
        "\",\"revision\":1,\"name\":\"Bad\",\"source\":\"{BAD}\","
        "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID
        "\"}],\"procedures\":[],\"progress\":[]}";
    const char duplicate_macro[] = PACKAGE_PREFIX("set") SET_OBJECT
        "],\"macros\":[" LOCAL_MACRO_OBJECT ",{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
        "\",\"revision\":1,\"name\":\"Duplicate\",\"source\":\"b\","
        "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID
        "\"}],\"procedures\":[],\"progress\":[]}";
    const char missing_macro_reference[] = PACKAGE_PREFIX("set") SET_OBJECT
        "],\"macros\":[],\"procedures\":[" PROCEDURE_OBJECT "],\"progress\":[]}";
    const char bad_progress_step[] = PACKAGE_PREFIX("set") SET_OBJECT
        "],\"macros\":[" LOCAL_MACRO_OBJECT "],\"procedures\":[" PROCEDURE_OBJECT
        "],\"progress\":[{\"schema_version\":1,\"set_id\":\"" SET_ID
        "\",\"procedure_id\":\"" PROCEDURE_ID
        "\",\"procedure_revision\":1,\"current_step_id\":\"" OTHER_STEP_ID
        "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}]}";

    const struct {
        const char *data;
        size_t length;
        app_error_code_t expected;
        storage_package_kind_t kind;
    } cases[] = {
        {unknown_set_field, sizeof(unknown_set_field) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
        {duplicate_set, sizeof(duplicate_set) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_BACKUP},
        {wrong_set_count, sizeof(wrong_set_count) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
        {bad_macro_syntax, sizeof(bad_macro_syntax) - 1U, APP_ERROR_MACRO_SYNTAX,
         STORAGE_PACKAGE_KIND_SET},
        {duplicate_macro, sizeof(duplicate_macro) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
        {missing_macro_reference, sizeof(missing_macro_reference) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
        {bad_progress_step, sizeof(bad_progress_step) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
    };
    storage_package_summary_t summary = {0};
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        TEST_CHECK_APP_ERROR(cases[index].expected,
                             storage_package_validate(cases[index].data, cases[index].length,
                                                      cases[index].kind, &summary));
    }
}

static void test_size_and_argument_bounds(void) {
    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_validate(NULL, 1U, STORAGE_PACKAGE_KIND_SET, &summary));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_validate("", 0U, STORAGE_PACKAGE_KIND_SET, &summary));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_validate(VALID_SET_PACKAGE, sizeof(VALID_SET_PACKAGE) - 1U,
                                                  (storage_package_kind_t)99, &summary));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_validate(VALID_SET_PACKAGE, sizeof(VALID_SET_PACKAGE) - 1U,
                                                  STORAGE_PACKAGE_KIND_SET, NULL));

    char *oversized = malloc(APP_IMPORT_PACKAGE_MAX_BYTES + 1U);
    TEST_CHECK(oversized != NULL);
    memset(oversized, ' ', APP_IMPORT_PACKAGE_MAX_BYTES + 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_MACRO_LIMIT,
                         storage_package_validate(oversized, APP_IMPORT_PACKAGE_MAX_BYTES + 1U,
                                                  STORAGE_PACKAGE_KIND_SET, &summary));
    free(oversized);
}

int main(void) {
    test_valid_set_and_backup_packages();
    test_top_level_contract();
    test_object_and_reference_validation();
    test_size_and_argument_bounds();
    puts("storage package tests passed");
    return EXIT_SUCCESS;
}
