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

#define SET_OBJECT "{\"schema_version\":1,\"id\":\"" SET_ID "\",\"revision\":1,\"name\":\"Set\"}"

#define OTHER_SET_OBJECT                                                                           \
    "{\"schema_version\":1,\"id\":\"" OTHER_SET_ID "\",\"revision\":1,\"name\":\"Other\"}"

#define LOCAL_MACRO_OBJECT                                                                         \
    "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID                                               \
    "\",\"revision\":1,\"name\":\"Local\",\"source\":\"a\","                                       \
    "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}"

#define PACKAGE_PREFIX(TYPE_VALUE)                                                                 \
    "{\"schema_version\":1,\"package_type\":\"" TYPE_VALUE "\",\"sets\":["

#define PACKAGE_SUFFIX "],\"macros\":[" LOCAL_MACRO_OBJECT "]}"

static const char VALID_SET_PACKAGE[] = PACKAGE_PREFIX("set") SET_OBJECT PACKAGE_SUFFIX;
static const char VALID_BACKUP_PACKAGE[] = PACKAGE_PREFIX("backup") SET_OBJECT PACKAGE_SUFFIX;

static void test_valid_package_and_backup_documents(void) {
    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(VALID_SET_PACKAGE, sizeof(VALID_SET_PACKAGE) - 1U,
                                                  STORAGE_PACKAGE_KIND_SET, &summary));
    TEST_CHECK_EQ_U64(STORAGE_PACKAGE_KIND_SET, summary.kind);
    TEST_CHECK_EQ_U64(sizeof(VALID_SET_PACKAGE) - 1U, summary.package_bytes);
    TEST_CHECK_EQ_U64(1U, summary.set_count);
    TEST_CHECK_EQ_U64(1U, summary.local_macro_count);

    memset(&summary, 0, sizeof(summary));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(VALID_BACKUP_PACKAGE,
                                                  sizeof(VALID_BACKUP_PACKAGE) - 1U,
                                                  STORAGE_PACKAGE_KIND_BACKUP, &summary));
    TEST_CHECK_EQ_U64(STORAGE_PACKAGE_KIND_BACKUP, summary.kind);
    TEST_CHECK_EQ_U64(1U, summary.set_count);
}

/* SPEC 17: a package is a JSON document, and JSON permits whitespace between
 * tokens. The scanner used to require every value to begin immediately after
 * its colon, which the device's own writer happens to satisfy and every
 * pretty-printer does not. `GET /api/v1/backup` produces a file a user can open
 * in an editor; saving it made the file unrestorable, with a 422 and nothing to
 * explain it. Found on hardware, where the harness re-serialised with spaces. */
static void test_whitespace_between_tokens_is_accepted(void) {
    static const char SPACED_BACKUP[] =
        "{\"schema_version\": 1, \"package_type\": \"backup\", \"sets\": [ { "
        "\"schema_version\": 1, \"id\": \"" SET_ID "\", \"revision\": 1, "
        "\"name\": \"Spaced\" } ], \"macros\": [] }";
    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(SPACED_BACKUP, sizeof(SPACED_BACKUP) - 1U,
                                                  STORAGE_PACKAGE_KIND_BACKUP, &summary));
    TEST_CHECK_EQ_U64(STORAGE_PACKAGE_KIND_BACKUP, summary.kind);
    TEST_CHECK_EQ_U64(1U, summary.set_count);

    /* Newlines and tabs too: an editor writes those, not just spaces. */
    static const char PRETTY_BACKUP[] =
        "{\n\t\"schema_version\" : 1,\n\t\"package_type\" : \"backup\",\n"
        "\t\"sets\" : [\n\t\t{\n\t\t\t\"schema_version\" : 1,\n"
        "\t\t\t\"id\" : \"" SET_ID "\",\n\t\t\t\"revision\" : 1,\n"
        "\t\t\t\"name\" : \"Pretty\"\n\t\t}\n\t],\n\t\"macros\" : []\n}";
    memset(&summary, 0, sizeof(summary));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_validate(PRETTY_BACKUP, sizeof(PRETTY_BACKUP) - 1U,
                                                  STORAGE_PACKAGE_KIND_BACKUP, &summary));
    TEST_CHECK_EQ_U64(1U, summary.set_count);
}

static void test_top_level_contract(void) {
    static const char *const invalid[] = {
        "{}",
        "[]",
        "{\"schema_version\":2,\"package_type\":\"set\",\"sets\":[],\"macros\":[]}",
        "{\"schema_version\":1,\"package_type\":\"future\",\"sets\":[],\"macros\":[]}",
        "{\"schema_version\":1,\"schema_version\":1,\"package_type\":\"set\","
        "\"sets\":[],\"macros\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[],\"macros\":[],"
        "\"future\":true}",
        "{\"schema_version\":1,\"package_type\":\"se\\u0000t\",\"sets\":[],"
        "\"macros\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":{},\"macros\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[1],\"macros\":[]}",
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[],\"macros\":[]}x",
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
    const char unknown_package_field[] =
        PACKAGE_PREFIX("set") "{\"schema_version\":1,\"id\":\"" SET_ID
                              "\",\"revision\":1,\"name\":\"Set\",,"
                              "\"future\":true}" PACKAGE_SUFFIX;
    const char duplicate_package[] =
        PACKAGE_PREFIX("backup") SET_OBJECT "," SET_OBJECT PACKAGE_SUFFIX;
    const char wrong_package_count[] =
        PACKAGE_PREFIX("set") SET_OBJECT "," OTHER_SET_OBJECT PACKAGE_SUFFIX;
    const char bad_macro_syntax[] = PACKAGE_PREFIX("set") SET_OBJECT
        "],\"macros\":[{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
        "\",\"revision\":1,\"name\":\"Bad\",\"source\":\"{BAD}\","
        "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}]}";
    const char duplicate_macro[] = PACKAGE_PREFIX("set") SET_OBJECT
        "],\"macros\":[" LOCAL_MACRO_OBJECT ",{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
        "\",\"revision\":1,\"name\":\"Duplicate\",\"source\":\"b\","
        "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}]}";
    const struct {
        const char *data;
        size_t length;
        app_error_code_t expected;
        storage_package_kind_t kind;
    } cases[] = {
        {unknown_package_field, sizeof(unknown_package_field) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
        {duplicate_package, sizeof(duplicate_package) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_BACKUP},
        {wrong_package_count, sizeof(wrong_package_count) - 1U, APP_ERROR_INVALID_ARGUMENT,
         STORAGE_PACKAGE_KIND_SET},
        {bad_macro_syntax, sizeof(bad_macro_syntax) - 1U, APP_ERROR_MACRO_SYNTAX,
         STORAGE_PACKAGE_KIND_SET},
        {duplicate_macro, sizeof(duplicate_macro) - 1U, APP_ERROR_INVALID_ARGUMENT,
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
    test_valid_package_and_backup_documents();
    test_whitespace_between_tokens_is_accepted();
    test_top_level_contract();
    test_object_and_reference_validation();
    test_size_and_argument_bounds();
    puts("storage package tests passed");
    return EXIT_SUCCESS;
}
