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
#define SECRET_SENTINEL "phase18_5_admin_secret_N7vY5jR3xQ9mK2pL"

typedef struct {
    macro_package_t set;
    macro_t *local_macros;
    size_t local_macro_count;
    /* Macros of a different set: present in the repository, outside this
     * export's scope. */
    macro_t *other_macros;
    size_t other_macro_count;
    app_error_code_t local_list_result;
    app_error_code_t other_list_result;
    app_error_code_t unlock_result;
    size_t lock_take_count;
    size_t lock_give_count;
    size_t macro_free_count;
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

static app_error_code_t fake_package_read(void *context, const app_uuid_t *set_id,
                                          macro_package_t *out_package) {
    fake_export_context_t *fake = context;
    if (!app_uuid_equal(set_id, &fake->set.id)) {
        return APP_ERROR_NOT_FOUND;
    }
    *out_package = fake->set;
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

static storage_package_export_ops_t fake_operations(fake_export_context_t *context) {
    return (storage_package_export_ops_t){
        .context = context,
        .lock_take = fake_lock_take,
        .lock_give = fake_lock_give,
        .set_read = fake_package_read,
        .macro_list = fake_macro_list,
        .macro_list_free = fake_macro_list_free,
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
    fake_export_context_t context = {0};
    context.set = (macro_package_t){
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid(SET_ID),
        .revision = 1U,
    };
    snprintf(context.set.name, sizeof(context.set.name), "Set");

    const app_uuid_t other_package_id = uuid(OTHER_SET_ID);
    local_macros[0] = make_macro(LOCAL_ID, &context.set.id, "Local", local_source);
    other_macros[0] =
        make_macro(OTHER_SET_MACRO_ID, &other_package_id, "Unreferenced secret", unused_source);
    context.local_macros = local_macros;
    context.local_macro_count = 1U;
    context.other_macros = other_macros;
    context.other_macro_count = 1U;

    return context;
}

static void test_deterministic_export_and_filtering(void) {
    fake_export_context_t context = valid_context();
    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);

    char *first = NULL;
    size_t first_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export(&context.set.id, &first, &first_length));
    TEST_CHECK(first != NULL);
    TEST_CHECK(first_length == strlen(first));
    TEST_CHECK(strstr(first, OTHER_SET_MACRO_ID) == NULL);
    TEST_CHECK(strstr(first, "Unreferenced secret") == NULL);
    TEST_CHECK(strstr(first, SECRET_SENTINEL) == NULL);

    storage_package_summary_t summary = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_package_validate(first, first_length, STORAGE_DOCUMENT_KIND_PACKAGE, &summary));
    TEST_CHECK_EQ_U64(1U, summary.set_count);
    TEST_CHECK_EQ_U64(1U, summary.local_macro_count);

    char *second = NULL;
    size_t second_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export(&context.set.id, &second, &second_length));
    TEST_CHECK_EQ_U64(first_length, second_length);
    TEST_CHECK(memcmp(first, second, first_length) == 0);
    TEST_CHECK_EQ_U64(2U, context.lock_take_count);
    TEST_CHECK_EQ_U64(2U, context.lock_give_count);
    TEST_CHECK_EQ_U64(2U, context.macro_free_count);

    storage_package_free(first);
    storage_package_free(second);
    storage_package_reset_export_ops_for_test();
}

/* SPEC 8.7 lists what a package MUST NOT contain: AP credentials, password
 * verifiers, session tokens, setup codes, device keys, other device secrets.
 * This is the test for that prohibition. The two structural guards that back it
 * up are elsewhere and deliberately not duplicated here: the package validator
 * accepts a fixed allowlist of keys, so an added top-level field fails to
 * validate; and a content scan for forbidden words cannot be written soundly,
 * because it cannot tell a leaked key from a set a user named "Password
 * Notes". */
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
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_export(&context.set.id, &output, &output_length));

    const char *outputs[] = {output};
    test_assert_no_secret_sentinel(SECRET_SENTINEL, outputs, 1U);

    storage_package_free(output);
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
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_package_export(&context.set.id, &data, &length));
    TEST_CHECK(data == NULL);
    TEST_CHECK_EQ_U64(0U, length);
    TEST_CHECK_EQ_U64(1U, context.lock_take_count);
    TEST_CHECK_EQ_U64(1U, context.lock_give_count);
    TEST_CHECK_EQ_U64(1U, context.macro_free_count);
    storage_package_reset_export_ops_for_test();
}

/* A maximal set of plain macro sources is roughly 446 KiB (100 x 4096), which no
 * longer reaches the 512 KiB package ceiling now that procedures are gone. The
 * writer's limit is still real, so drive it with sources made entirely of quote
 * characters: each escapes to two bytes on serialization, so the same 100 macros
 * emit roughly 860 KiB and the writer must refuse. */
static void test_output_limit_is_enforced(void) {
    fake_export_context_t context = valid_context();
    char *large_source = malloc(APP_MACRO_SOURCE_MAX_BYTES + 1U);
    macro_t *local_macros = calloc(APP_MACROS_PER_SET_MAX, sizeof(*local_macros));
    TEST_CHECK(large_source != NULL);
    TEST_CHECK(local_macros != NULL);
    memset(large_source, '"', APP_MACRO_SOURCE_MAX_BYTES);
    large_source[APP_MACRO_SOURCE_MAX_BYTES] = '\0';

    for (size_t index = 0U; index < APP_MACROS_PER_SET_MAX; ++index) {
        char id[APP_UUID_BUFFER_LENGTH];
        snprintf(id, sizeof(id), "10000000-0000-4000-8000-%012zu", index);
        local_macros[index] = make_macro(id, &context.set.id, "Large local", large_source);
    }
    context.local_macros = local_macros;
    context.local_macro_count = APP_MACROS_PER_SET_MAX;

    const storage_package_export_ops_t operations = fake_operations(&context);
    storage_package_set_export_ops_for_test(&operations);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_MACRO_LIMIT,
                         storage_package_export(&context.set.id, &data, &length));
    TEST_CHECK(data == NULL);

    storage_package_reset_export_ops_for_test();
    free(local_macros);
    free(large_source);
}

static void test_argument_and_operations_validation(void) {
    storage_package_reset_export_ops_for_test();
    const app_uuid_t set_id = uuid(SET_ID);
    char *data = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_export(&set_id, &data, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_package_export(NULL, &data, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_package_export(&set_id, NULL, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_package_export(&set_id, &data, NULL));
}

int main(void) {
    test_deterministic_export_and_filtering();
    test_export_output_passes_secret_sentinel_scanner();
    test_failure_cleanup_and_primary_error_preservation();
    test_output_limit_is_enforced();
    test_argument_and_operations_validation();
    puts("storage package export tests passed");
    return EXIT_SUCCESS;
}
