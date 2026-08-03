#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "storage.h"
#include "storage_incidents.h"
#include "storage_repository.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "storage_repository_objects_json.h"

#include "test_assert.h"
#include "test_temp_dir.h"

static app_uuid_t make_uuid(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    const int written = snprintf(text, sizeof(text), "%08" PRIx32 "-0000-4000-8000-%012" PRIx64,
                                 value, (uint64_t)value);
    TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void write_file(const char *path, const char *data, size_t length) {
    const int descriptor = open(path, O_WRONLY | O_TRUNC);
    TEST_CHECK(descriptor >= 0);
    size_t offset = 0U;
    while (offset < length) {
        const ssize_t count = write(descriptor, data + offset, length - offset);
        TEST_CHECK(count > 0);
        offset += (size_t)count;
    }
    TEST_CHECK(close(descriptor) == 0);
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_package_t make_package(uint32_t value) {
    macro_package_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Macro repository set") > 0);
    return set;
}

static macro_t make_macro(uint32_t value, const app_uuid_t *set_id, const char *name,
                          const char *source) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
        .set_id = *set_id,
        .key_press_ms = 20U,
        .inter_key_ms = 15U,
    };
    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "%s", name) > 0);
    macro.source_length = strlen(source);
    macro.source = malloc(macro.source_length + 1U);
    TEST_CHECK(macro.source != NULL);
    memcpy(macro.source, source, macro.source_length + 1U);
    return macro;
}

static void create_package(macro_package_t *out_package) {
    *out_package = make_package(100U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_package_create(out_package));
}

static void assert_macro_equal(const macro_t *expected, const macro_t *actual) {
    TEST_CHECK_EQ_UUID(&expected->id, &actual->id);
    TEST_CHECK_EQ_U64(expected->revision, actual->revision);
    TEST_CHECK_EQ_UUID(&expected->set_id, &actual->set_id);
    TEST_CHECK_EQ_STRING(expected->name, actual->name);
    TEST_CHECK_EQ_STRING(expected->source, actual->source);
    TEST_CHECK_EQ_U64(expected->source_length, actual->source_length);
    TEST_CHECK_EQ_U64(expected->key_press_ms, actual->key_press_ms);
    TEST_CHECK_EQ_U64(expected->inter_key_ms, actual->inter_key_ms);
}

static void test_argument_validation(void) {
    reset_store();
    macro_package_t set;
    create_package(&set);
    const app_uuid_t *location = &set.id;
    macro_t macro = make_macro(1U, location, "Arguments", "TEXT hello");
    macro_t output = {0};
    storage_macro_list_t list = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_list(NULL, &list));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_list(location, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_create(NULL, &macro));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_create(location, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_read(location, NULL, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_read(location, &macro.id, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_macro_update(location, &macro, 0U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_delete(location, &macro.id, 0U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_macro_duplicate(location, &macro.id, &macro.id, NULL, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_reorder(location, NULL, 1U));

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_create(NULL, &macro));
    macro_model_free_macro(&macro);
}

static void test_package_local_crud_duplicate_and_order(void) {
    reset_store();
    macro_package_t set;
    create_package(&set);
    const app_uuid_t *location = &set.id;
    macro_t first = make_macro(10U, location, "First", "TEXT first");
    macro_t second = make_macro(20U, location, "Second", "TEXT second");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(location, &first));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(location, &second));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_macro_create(location, &first));

    storage_macro_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(location, &list));
    TEST_CHECK_EQ_U64(2U, list.count);
    assert_macro_equal(&first, &list.items[0]);
    assert_macro_equal(&second, &list.items[1]);
    storage_macro_list_free(&list);
    TEST_CHECK(list.items == NULL);
    TEST_CHECK_EQ_U64(0U, list.count);

    macro_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_read(location, &first.id, &readback));
    assert_macro_equal(&first, &readback);
    macro_model_free_macro(&readback);

    macro_t replacement = first;
    TEST_CHECK(snprintf(replacement.name, sizeof(replacement.name), "First updated") > 0);
    macro_t updated = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_update(location, &replacement, 2U, &updated));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_macro_update(location, &replacement, 1U, &updated));
    TEST_CHECK_EQ_U64(2U, updated.revision);
    TEST_CHECK_EQ_STRING("First updated", updated.name);
    macro_model_free_macro(&updated);

    const app_uuid_t duplicate_id = make_uuid(30U);
    macro_t duplicate = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_duplicate(location, &first.id, &duplicate_id,
                                                                 "First copy", &duplicate));
    TEST_CHECK_EQ_UUID(&duplicate_id, &duplicate.id);
    TEST_CHECK_EQ_U64(1U, duplicate.revision);
    TEST_CHECK_EQ_STRING("First copy", duplicate.name);
    TEST_CHECK_EQ_STRING("TEXT first", duplicate.source);
    macro_model_free_macro(&duplicate);

    const app_uuid_t reordered[] = {duplicate_id, second.id, first.id};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_macro_reorder(location, reordered, sizeof(reordered) / sizeof(reordered[0])));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(location, &list));
    TEST_CHECK_EQ_U64(3U, list.count);
    TEST_CHECK_EQ_UUID(&duplicate_id, &list.items[0].id);
    TEST_CHECK_EQ_UUID(&second.id, &list.items[1].id);
    TEST_CHECK_EQ_UUID(&first.id, &list.items[2].id);
    storage_macro_list_free(&list);

    const app_uuid_t wrong_members[] = {first.id, second.id};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_reorder(location, wrong_members,
                                               sizeof(wrong_members) / sizeof(wrong_members[0])));

    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_macro_delete(location, &first.id, 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_delete(location, &first.id, 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_macro_read(location, &first.id, &readback));

    macro_model_free_macro(&first);
    macro_model_free_macro(&second);
}

/* Macros live inline in their set file, so there is no per-macro file to damage:
 * a corrupt macro is a corrupt set. The file is discarded and the failure
 * reported (SPEC 13.6) rather than the set being read back missing a macro. */
/* SPEC 3.10: "Reject malformed or unsafe state rather than silently substituting
 * defaults." Creation used to return success for a source the parser cannot
 * compile, so the failure surfaced only when something tried to use the macro --
 * on a real device, when an export tripped over it and refused to back up the
 * whole repository. The write path is where a malformed macro should be stopped. */
static void test_uncompilable_source_is_refused_on_write(void) {
    reset_store();
    macro_package_t set = {0};
    create_package(&set);

    /* The exact syntax a device accepted and could never run: the parser wants
     * DELAY:, not a space. */
    macro_t bad = make_macro(201U, &set.id, "bad delay", "ab{DELAY 3000}cd");
    TEST_CHECK_APP_ERROR(APP_ERROR_MACRO_SYNTAX, storage_macro_create(&set.id, &bad));
    free(bad.source);

    /* Refused means not stored, not stored-and-reported. */
    storage_macro_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(&set.id, &list));
    TEST_CHECK_EQ_U64(0U, list.count);
    storage_macro_list_free(&list);

    /* A good source still writes, so the check rejects the malformed rather
     * than everything. */
    macro_t good = make_macro(202U, &set.id, "good delay", "ab{DELAY:3000}cd");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&set.id, &good));
    free(good.source);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(&set.id, &list));
    TEST_CHECK_EQ_U64(1U, list.count);
    storage_macro_list_free(&list);
}

static void test_corrupt_package_file_is_discarded(void) {
    reset_store();
    macro_package_t set;
    create_package(&set);
    const app_uuid_t *location = &set.id;
    macro_t macro = make_macro(60U, location, "Corrupt", "TEXT corrupt");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(location, &macro));

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_package_path(&set.id, path, sizeof(path)));
    static const char invalid[] = "{not json";
    write_file(path, invalid, sizeof(invalid) - 1U);

    macro_t output;
    memset(&output, 0xa5, sizeof(output));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_macro_read(location, &macro.id, &output));
    TEST_CHECK(output.source == NULL);
    TEST_CHECK(!path_exists(path));

    macro_model_free_macro(&macro);
}

static void test_missing_package_and_revision_overflow(void) {
    reset_store();
    const macro_package_t absent = make_package(999U);
    macro_t macro = make_macro(70U, &absent.id, "Absent set", "TEXT absent");
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_macro_create(&absent.id, &macro));
    macro_model_free_macro(&macro);

    macro_package_t set;
    create_package(&set);
    const app_uuid_t *existing = &set.id;
    macro = make_macro(71U, existing, "Max revision", "TEXT max");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(existing, &macro));

    /* Planted by rewriting the whole set file, because that is the only file a
     * macro lives in now. */
    macro.revision = UINT32_MAX;
    char *json = NULL;
    size_t length = 0U;
    const storage_package_document_t document = {
        .set = set,
        .macros = &macro,
        .macro_count = 1U,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_package_document_serialize(&document, &json, &length));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_package_path(&set.id, path, sizeof(path)));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    cJSON_free(json);

    macro_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_update(existing, &macro, UINT32_MAX, &output));
    TEST_CHECK_EQ_U64(0U, output.revision);
    macro_model_free_macro(&macro);
}

/* SPEC 10.7: writes are measured against the byte budget, not just the
 * per-object limits, and an over-budget write is refused as a storage-capacity
 * failure rather than being allowed to fill the partition. */
/* SPEC 24.2 item: rejection of an over-budget write with `507` */
static void test_oversized_package_file_is_refused(void) {
    reset_store();
    macro_package_t set = make_package(200U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_package_create(&set));

    /* One macro whose source alone exceeds the 32 KiB set-file budget is
       impossible (sources are capped at 4096 bytes), so fill the set until the
       serialized file passes the limit. */
    char source[APP_MACRO_SOURCE_MAX_BYTES + 1U];
    memset(source, 'a', APP_MACRO_SOURCE_MAX_BYTES);
    source[APP_MACRO_SOURCE_MAX_BYTES] = '\0';

    app_error_code_t result = APP_ERROR_NONE;
    size_t created = 0U;
    for (uint32_t index = 0U; index < APP_MACROS_PER_SET_MAX; ++index) {
        macro_t macro = {
            .schema_version = APP_SCHEMA_VERSION,
            .id = make_uuid(3000U + index),
            .revision = 1U,
            .set_id = set.id,
            .source = source,
            .source_length = APP_MACRO_SOURCE_MAX_BYTES,
            .key_press_ms = 8U,
            .inter_key_ms = 15U,
        };
        TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "Macro %" PRIu32, index) > 0);
        result = storage_macro_create(&set.id, &macro);
        if (result != APP_ERROR_NONE) {
            break;
        }
        ++created;
    }
    /* The budget, not the macro count, is what stops it. */
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, result);
    TEST_CHECK(created < APP_MACROS_PER_SET_MAX);

    /* The refusal left the set intact at its last good state. */
    storage_macro_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(&set.id, &list));
    TEST_CHECK_EQ_U64(created, list.count);
    storage_macro_list_free(&list);
}

/* SPEC 13.6: a discarded object is reported with its path AND the reason it was
 * discarded. The reason used to be dropped on the floor; this pins that it is
 * not. */
static void test_discarded_object_records_path_and_reason(void) {
    reset_store();
    storage_incidents_reset();
    macro_package_t set;
    create_package(&set);
    macro_t macro = make_macro(70U, &set.id, "Doomed", "TEXT doomed");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&set.id, &macro));
    macro_model_free_macro(&macro);

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_package_path(&set.id, path, sizeof(path)));
    static const char invalid[] = "{not json";
    write_file(path, invalid, sizeof(invalid) - 1U);

    macro_package_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_package_read(&set.id, &output));

    storage_incident_report_t report = {0};
    storage_incidents_snapshot(&report);
    TEST_CHECK_EQ_U64(1U, report.total);
    TEST_CHECK_EQ_U64(1U, report.count);
    TEST_CHECK_EQ_STRING(path, report.items[0].path);
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, report.items[0].error);
}

/* The table is bounded, and `total` still counts everything so a caller can tell
 * that more was lost than it can enumerate. */
static void test_discard_record_is_bounded_but_counts_everything(void) {
    storage_incidents_reset();
    for (size_t index = 0U; index < STORAGE_INCIDENT_MAX + 3U; ++index) {
        storage_incident_record_discard("/data/sets/x.json", APP_ERROR_STORAGE_CORRUPT);
    }
    storage_incident_report_t report = {0};
    storage_incidents_snapshot(&report);
    TEST_CHECK_EQ_U64(STORAGE_INCIDENT_MAX, report.count);
    TEST_CHECK_EQ_U64(STORAGE_INCIDENT_MAX + 3U, report.total);
    storage_incidents_reset();
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_argument_validation();
    test_discarded_object_records_path_and_reason();
    test_discard_record_is_bounded_but_counts_everything();
    test_oversized_package_file_is_refused();
    test_package_local_crud_duplicate_and_order();
    test_uncompilable_source_is_refused_on_write();
    test_corrupt_package_file_is_discarded();
    test_missing_package_and_revision_overflow();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage macro repository tests passed");
    return EXIT_SUCCESS;
}
