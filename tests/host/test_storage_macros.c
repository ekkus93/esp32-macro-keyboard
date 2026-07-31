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
        STORAGE_DATA_MOUNT "/global",
        STORAGE_DATA_MOUNT "/global/macros",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/quarantine",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_set_t make_set(uint32_t value) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Macro repository set") > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Macro tests") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static storage_macro_location_t set_location(const macro_set_t *set) {
    return (storage_macro_location_t){
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = set->id,
    };
}

static storage_macro_location_t global_location(void) {
    return (storage_macro_location_t){
        .scope = MACRO_SCOPE_GLOBAL,
        .has_set_id = false,
    };
}

static macro_t make_macro(uint32_t value, const storage_macro_location_t *location,
                          const char *name, const char *source) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
        .scope = location->scope,
        .has_set_id = location->has_set_id,
        .set_id = location->set_id,
        .favorite = false,
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

static void create_set(macro_set_t *out_set, storage_macro_location_t *out_location) {
    *out_set = make_set(100U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(out_set));
    *out_location = set_location(out_set);
}

static void assert_macro_equal(const macro_t *expected, const macro_t *actual) {
    TEST_CHECK_EQ_UUID(&expected->id, &actual->id);
    TEST_CHECK_EQ_U64(expected->revision, actual->revision);
    TEST_CHECK_EQ_INT((int)expected->scope, (int)actual->scope);
    TEST_CHECK_EQ_INT((int)expected->has_set_id, (int)actual->has_set_id);
    if (expected->has_set_id) {
        TEST_CHECK_EQ_UUID(&expected->set_id, &actual->set_id);
    }
    TEST_CHECK_EQ_STRING(expected->name, actual->name);
    TEST_CHECK_EQ_STRING(expected->source, actual->source);
    TEST_CHECK_EQ_U64(expected->source_length, actual->source_length);
    TEST_CHECK_EQ_INT((int)expected->favorite, (int)actual->favorite);
    TEST_CHECK_EQ_U64(expected->key_press_ms, actual->key_press_ms);
    TEST_CHECK_EQ_U64(expected->inter_key_ms, actual->inter_key_ms);
}

static void test_argument_validation(void) {
    reset_store();
    macro_set_t set;
    storage_macro_location_t location;
    create_set(&set, &location);
    macro_t macro = make_macro(1U, &location, "Arguments", "TEXT hello");
    macro_t output = {0};
    storage_macro_list_t list = {0};
    storage_reference_list_t references = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_list(NULL, &list));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_list(&location, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_create(NULL, &macro));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_create(&location, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_read(&location, NULL, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_macro_read(&location, &macro.id, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_macro_update(&location, &macro, 0U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_macro_delete(&location, &macro.id, 0U, &references));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_macro_duplicate(&location, &macro.id, &macro.id, NULL, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_reorder(&location, NULL, 1U));

    storage_macro_location_t invalid = location;
    invalid.has_set_id = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_macro_create(&invalid, &macro));
    macro_model_free_macro(&macro);
}

static void test_set_local_crud_duplicate_and_order(void) {
    reset_store();
    macro_set_t set;
    storage_macro_location_t location;
    create_set(&set, &location);
    macro_t first = make_macro(10U, &location, "First", "TEXT first");
    macro_t second = make_macro(20U, &location, "Second", "TEXT second");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&location, &first));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&location, &second));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_macro_create(&location, &first));

    storage_macro_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(&location, &list));
    TEST_CHECK_EQ_U64(2U, list.count);
    assert_macro_equal(&first, &list.items[0]);
    assert_macro_equal(&second, &list.items[1]);
    storage_macro_list_free(&list);
    TEST_CHECK(list.items == NULL);
    TEST_CHECK_EQ_U64(0U, list.count);

    macro_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_read(&location, &first.id, &readback));
    assert_macro_equal(&first, &readback);
    macro_model_free_macro(&readback);

    macro_t replacement = first;
    TEST_CHECK(snprintf(replacement.name, sizeof(replacement.name), "First updated") > 0);
    replacement.favorite = true;
    macro_t updated = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_update(&location, &replacement, 2U, &updated));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_macro_update(&location, &replacement, 1U, &updated));
    TEST_CHECK_EQ_U64(2U, updated.revision);
    TEST_CHECK_EQ_STRING("First updated", updated.name);
    TEST_CHECK(updated.favorite);
    macro_model_free_macro(&updated);

    const app_uuid_t duplicate_id = make_uuid(30U);
    macro_t duplicate = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_macro_duplicate(&location, &first.id, &duplicate_id, "First copy", &duplicate));
    TEST_CHECK_EQ_UUID(&duplicate_id, &duplicate.id);
    TEST_CHECK_EQ_U64(1U, duplicate.revision);
    TEST_CHECK_EQ_STRING("First copy", duplicate.name);
    TEST_CHECK_EQ_STRING("TEXT first", duplicate.source);
    macro_model_free_macro(&duplicate);

    const app_uuid_t reordered[] = {duplicate_id, second.id, first.id};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_macro_reorder(&location, reordered, sizeof(reordered) / sizeof(reordered[0])));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(&location, &list));
    TEST_CHECK_EQ_U64(3U, list.count);
    TEST_CHECK_EQ_UUID(&duplicate_id, &list.items[0].id);
    TEST_CHECK_EQ_UUID(&second.id, &list.items[1].id);
    TEST_CHECK_EQ_UUID(&first.id, &list.items[2].id);
    storage_macro_list_free(&list);

    const app_uuid_t wrong_members[] = {first.id, second.id};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_reorder(&location, wrong_members,
                                               sizeof(wrong_members) / sizeof(wrong_members[0])));

    storage_reference_list_t references = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_delete(&location, &first.id, 1U, &references));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_macro_delete(&location, &first.id, 2U, &references));
    TEST_CHECK_EQ_U64(0U, references.count);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_macro_read(&location, &first.id, &readback));

    macro_model_free_macro(&first);
    macro_model_free_macro(&second);
}

static void test_global_crud_is_isolated_from_set_scope(void) {
    reset_store();
    macro_set_t set;
    storage_macro_location_t local;
    create_set(&set, &local);
    const storage_macro_location_t global = global_location();
    macro_t local_macro = make_macro(40U, &local, "Local", "TEXT local");
    macro_t global_macro = make_macro(40U, &global, "Global", "TEXT global");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&local, &local_macro));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&global, &global_macro));

    macro_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_read(&local, &local_macro.id, &output));
    TEST_CHECK_EQ_STRING("Local", output.name);
    macro_model_free_macro(&output);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_read(&global, &global_macro.id, &output));
    TEST_CHECK_EQ_STRING("Global", output.name);
    macro_model_free_macro(&output);

    storage_macro_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_list(&global, &list));
    TEST_CHECK_EQ_U64(1U, list.count);
    TEST_CHECK_EQ_STRING("Global", list.items[0].name);
    storage_macro_list_free(&list);

    macro_model_free_macro(&local_macro);
    macro_model_free_macro(&global_macro);
}

static void write_procedure_reference(const macro_set_t *set, const macro_t *macro,
                                      const app_uuid_t *procedure_id) {
    procedure_step_t step = {
        .id = make_uuid(700U),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .has_macro_id = true,
        .macro_id = macro->id,
        .auto_complete_on_success = false,
    };
    TEST_CHECK(snprintf(step.title, sizeof(step.title), "Run referenced macro") > 0);
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = *procedure_id,
        .revision = 1U,
        .set_id = set->id,
        .steps = &step,
        .step_count = 1U,
    };
    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "Reference procedure") > 0);
    TEST_CHECK(snprintf(procedure.description, sizeof(procedure.description), "Reference test") >
               0);

    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_procedure_json(&procedure, &json, &length));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_procedure_path(&set->id, procedure_id, path, sizeof(path)));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    cJSON_free(json);
}

static void test_delete_reports_procedure_references(void) {
    reset_store();
    macro_set_t set;
    storage_macro_location_t location;
    create_set(&set, &location);
    macro_t macro = make_macro(50U, &location, "Referenced", "TEXT referenced");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&location, &macro));
    const app_uuid_t procedure_id = make_uuid(500U);
    write_procedure_reference(&set, &macro, &procedure_id);

    storage_reference_list_t references = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_delete(&location, &macro.id, 1U, &references));
    TEST_CHECK_EQ_U64(1U, references.count);
    TEST_CHECK(!references.truncated);
    TEST_CHECK_EQ_UUID(&procedure_id, &references.ids[0]);

    macro_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_read(&location, &macro.id, &readback));
    macro_model_free_macro(&readback);
    macro_model_free_macro(&macro);
}

static void test_corrupt_macro_is_quarantined(void) {
    reset_store();
    macro_set_t set;
    storage_macro_location_t location;
    create_set(&set, &location);
    macro_t macro = make_macro(60U, &location, "Corrupt", "TEXT corrupt");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&location, &macro));

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_macro_path(&set.id, &macro.id, path, sizeof(path)));
    static const char invalid[] = "{not json";
    write_file(path, invalid, sizeof(invalid) - 1U);

    macro_t output;
    memset(&output, 0xa5, sizeof(output));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_macro_read(&location, &macro.id, &output));
    TEST_CHECK(output.source == NULL);
    TEST_CHECK(!path_exists(path));

    storage_quarantine_list_t quarantine = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_quarantine_list(&quarantine));
    TEST_CHECK_EQ_U64(1U, quarantine.count);
    TEST_CHECK_EQ_STRING(path, quarantine.items[0].source_path);
    macro_model_free_macro(&macro);
}

static void test_missing_set_and_revision_overflow(void) {
    reset_store();
    const macro_set_t absent = make_set(999U);
    const storage_macro_location_t location = set_location(&absent);
    macro_t macro = make_macro(70U, &location, "Absent set", "TEXT absent");
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_macro_create(&location, &macro));
    macro_model_free_macro(&macro);

    macro_set_t set;
    storage_macro_location_t existing;
    create_set(&set, &existing);
    macro = make_macro(71U, &existing, "Max revision", "TEXT max");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&existing, &macro));

    macro.revision = UINT32_MAX;
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_macro_json(&macro, &json, &length));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_macro_path(&set.id, &macro.id, path, sizeof(path)));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    cJSON_free(json);

    macro_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_macro_update(&existing, &macro, UINT32_MAX, &output));
    TEST_CHECK_EQ_U64(0U, output.revision);
    macro_model_free_macro(&macro);
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_argument_validation();
    test_set_local_crud_duplicate_and_order();
    test_global_crud_is_isolated_from_set_scope();
    test_delete_reports_procedure_references();
    test_corrupt_macro_is_quarantined();
    test_missing_set_and_revision_overflow();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage macro repository tests passed");
    return EXIT_SUCCESS;
}
