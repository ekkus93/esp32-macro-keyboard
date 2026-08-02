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
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
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
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Procedure repository set") > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Procedure tests") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static macro_t make_macro(uint32_t value, const app_uuid_t *set_id, const char *name) {
    static const char source[] = "TEXT procedure";
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
        .set_id = *set_id,
        .source_length = sizeof(source) - 1U,
        .favorite = false,
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    TEST_CHECK(snprintf(macro.name, sizeof(macro.name), "%s", name) > 0);
    macro.source = malloc(sizeof(source));
    TEST_CHECK(macro.source != NULL);
    memcpy(macro.source, source, sizeof(source));
    return macro;
}

static void create_set_and_macros(macro_set_t *out_set, macro_t *out_first_macro,
                                  macro_t *out_second_macro) {
    *out_set = make_set(100U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(out_set));
    *out_first_macro = make_macro(10U, &out_set->id, "First macro");
    *out_second_macro = make_macro(20U, &out_set->id, "Second macro");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&out_set->id, out_first_macro));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_macro_create(&out_set->id, out_second_macro));
}

static procedure_t make_procedure(uint32_t value, const macro_set_t *set,
                                  const macro_t *first_macro, const macro_t *second_macro,
                                  const char *name) {
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
        .set_id = set->id,
        .step_count = 3U,
        .sort_order = 99,
    };
    TEST_CHECK(snprintf(procedure.name, sizeof(procedure.name), "%s", name) > 0);
    TEST_CHECK(snprintf(procedure.description, sizeof(procedure.description), "Guided procedure") >
               0);
    procedure.steps = calloc(procedure.step_count, sizeof(*procedure.steps));
    TEST_CHECK(procedure.steps != NULL);
    procedure.steps[0] = (procedure_step_t){
        .id = make_uuid(value + 1U),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .auto_complete_on_success = true,
        .has_macro_id = true,
        .macro_id = first_macro->id,
    };
    TEST_CHECK(
        snprintf(procedure.steps[0].title, sizeof(procedure.steps[0].title), "Run set macro") > 0);
    procedure.steps[1] = (procedure_step_t){
        .id = make_uuid(value + 2U),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .auto_complete_on_success = false,
        .has_macro_id = true,
        .macro_id = second_macro->id,
    };
    TEST_CHECK(snprintf(procedure.steps[1].title, sizeof(procedure.steps[1].title),
                        "Run global macro") > 0);
    static const char body[] = "Confirm the expected output.";
    procedure.steps[2] = (procedure_step_t){
        .id = make_uuid(value + 3U),
        .type = PROCEDURE_STEP_CHECKPOINT,
        .required = true,
        .body_length = sizeof(body) - 1U,
    };
    TEST_CHECK(
        snprintf(procedure.steps[2].title, sizeof(procedure.steps[2].title), "Confirm output") > 0);
    procedure.steps[2].body = malloc(sizeof(body));
    TEST_CHECK(procedure.steps[2].body != NULL);
    memcpy(procedure.steps[2].body, body, sizeof(body));
    return procedure;
}

static void assert_procedure_equal(const procedure_t *expected, const procedure_t *actual) {
    TEST_CHECK_EQ_UUID(&expected->id, &actual->id);
    TEST_CHECK_EQ_UUID(&expected->set_id, &actual->set_id);
    TEST_CHECK_EQ_U64(expected->revision, actual->revision);
    TEST_CHECK_EQ_STRING(expected->name, actual->name);
    TEST_CHECK_EQ_STRING(expected->description, actual->description);
    TEST_CHECK_EQ_U64(expected->step_count, actual->step_count);
    for (size_t index = 0U; index < expected->step_count; ++index) {
        const procedure_step_t *left = &expected->steps[index];
        const procedure_step_t *right = &actual->steps[index];
        TEST_CHECK_EQ_UUID(&left->id, &right->id);
        TEST_CHECK_EQ_INT((int)left->type, (int)right->type);
        TEST_CHECK_EQ_STRING(left->title, right->title);
        TEST_CHECK_EQ_INT((int)left->required, (int)right->required);
        TEST_CHECK_EQ_INT((int)left->has_macro_id, (int)right->has_macro_id);
        if (left->has_macro_id) {
            TEST_CHECK_EQ_UUID(&left->macro_id, &right->macro_id);
        } else {
            TEST_CHECK_EQ_STRING(left->body, right->body);
        }
    }
}

static void test_argument_validation(void) {
    reset_store();
    macro_set_t set;
    macro_t first_macro;
    macro_t second_macro;
    create_set_and_macros(&set, &first_macro, &second_macro);
    procedure_t procedure = make_procedure(1000U, &set, &first_macro, &second_macro, "Arguments");
    procedure_t output = {0};
    storage_procedure_list_t list = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_list(NULL, &list));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_list(&set.id, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_create(NULL, &procedure));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_create(&set.id, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_procedure_read(&set.id, NULL, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_procedure_read(&set.id, &procedure.id, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_procedure_update(&set.id, &procedure, 0U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_procedure_delete(&set.id, &procedure.id, 0U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_reorder(&set.id, NULL, 1U));

    procedure.set_id = make_uuid(999U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_create(&set.id, &procedure));
    macro_model_free_procedure(&procedure);
    macro_model_free_macro(&first_macro);
    macro_model_free_macro(&second_macro);
}

static void test_crud_and_order(void) {
    reset_store();
    macro_set_t set;
    macro_t first_macro;
    macro_t second_macro;
    create_set_and_macros(&set, &first_macro, &second_macro);
    procedure_t first = make_procedure(1100U, &set, &first_macro, &second_macro, "First");
    procedure_t second = make_procedure(1200U, &set, &first_macro, &second_macro, "Second");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &first));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &second));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_procedure_create(&set.id, &first));

    storage_procedure_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_list(&set.id, &list));
    TEST_CHECK_EQ_U64(2U, list.count);
    assert_procedure_equal(&first, &list.items[0]);
    assert_procedure_equal(&second, &list.items[1]);
    TEST_CHECK_EQ_INT(0, list.items[0].sort_order);
    TEST_CHECK_EQ_INT(1, list.items[1].sort_order);
    storage_procedure_list_free(&list);
    TEST_CHECK(list.items == NULL);
    storage_procedure_list_free(&list);

    procedure_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_read(&set.id, &first.id, &readback));
    assert_procedure_equal(&first, &readback);
    TEST_CHECK_EQ_INT(0, readback.sort_order);
    macro_model_free_procedure(&readback);

    TEST_CHECK(snprintf(first.name, sizeof(first.name), "First updated") > 0);
    static const char updated_body[] = "Updated checkpoint body";
    free(first.steps[2].body);
    first.steps[2].body = malloc(sizeof(updated_body));
    TEST_CHECK(first.steps[2].body != NULL);
    memcpy(first.steps[2].body, updated_body, sizeof(updated_body));
    first.steps[2].body_length = sizeof(updated_body) - 1U;
    procedure_t updated = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_procedure_update(&set.id, &first, 2U, &updated));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_update(&set.id, &first, 1U, &updated));
    TEST_CHECK_EQ_U64(2U, updated.revision);
    TEST_CHECK_EQ_STRING("First updated", updated.name);
    TEST_CHECK_EQ_STRING(updated_body, updated.steps[2].body);
    TEST_CHECK_EQ_INT(0, updated.sort_order);
    macro_model_free_procedure(&updated);
    first.revision = 2U;

    const app_uuid_t reordered[] = {second.id, first.id};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_reorder(&set.id, reordered, 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_list(&set.id, &list));
    TEST_CHECK_EQ_UUID(&second.id, &list.items[0].id);
    TEST_CHECK_EQ_UUID(&first.id, &list.items[1].id);
    TEST_CHECK_EQ_INT(0, list.items[0].sort_order);
    TEST_CHECK_EQ_INT(1, list.items[1].sort_order);
    storage_procedure_list_free(&list);

    const app_uuid_t wrong_members[] = {first.id};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_procedure_reorder(&set.id, wrong_members, 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_procedure_delete(&set.id, &second.id, 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_delete(&set.id, &second.id, 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_procedure_read(&set.id, &second.id, &readback));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_list(&set.id, &list));
    TEST_CHECK_EQ_U64(1U, list.count);
    TEST_CHECK_EQ_UUID(&first.id, &list.items[0].id);
    TEST_CHECK_EQ_INT(0, list.items[0].sort_order);
    storage_procedure_list_free(&list);

    macro_model_free_procedure(&first);
    macro_model_free_procedure(&second);
    macro_model_free_macro(&first_macro);
    macro_model_free_macro(&second_macro);
}

static void test_reference_validation(void) {
    reset_store();
    macro_set_t set;
    macro_t first_macro;
    macro_t second_macro;
    create_set_and_macros(&set, &first_macro, &second_macro);
    procedure_t procedure = make_procedure(1300U, &set, &first_macro, &second_macro, "References");

    procedure.steps[0].macro_id = make_uuid(9999U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_procedure_create(&set.id, &procedure));
    procedure.steps[0].macro_id = first_macro.id;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &procedure));

    procedure_t replacement = procedure;
    replacement.steps[1].macro_id = make_uuid(9998U);
    procedure_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_procedure_update(&set.id, &replacement, 1U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_read(&set.id, &procedure.id, &output));
    TEST_CHECK_EQ_UUID(&second_macro.id, &output.steps[1].macro_id);
    macro_model_free_procedure(&output);

    macro_model_free_procedure(&procedure);
    macro_model_free_macro(&first_macro);
    macro_model_free_macro(&second_macro);
}

static void test_corrupt_and_stale_references_are_discarded(void) {
    reset_store();
    macro_set_t set;
    macro_t first_macro;
    macro_t second_macro;
    create_set_and_macros(&set, &first_macro, &second_macro);
    procedure_t procedure = make_procedure(1500U, &set, &first_macro, &second_macro, "Corrupt");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &procedure));

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_procedure_path(&set.id, &procedure.id, path, sizeof(path)));
    static const char invalid[] = "{not json";
    write_file(path, invalid, sizeof(invalid) - 1U);
    procedure_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_procedure_read(&set.id, &procedure.id, &output));
    TEST_CHECK(output.steps == NULL);
    TEST_CHECK(!path_exists(path));

    macro_model_free_procedure(&procedure);
    macro_model_free_macro(&first_macro);
    macro_model_free_macro(&second_macro);

    reset_store();
    create_set_and_macros(&set, &first_macro, &second_macro);
    procedure = make_procedure(1600U, &set, &first_macro, &second_macro, "Stale reference");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &procedure));
    char macro_path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_macro_path(&set.id, &first_macro.id,
                                                                 macro_path, sizeof(macro_path)));
    TEST_CHECK(unlink(macro_path) == 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_procedure_path(&set.id, &procedure.id, path, sizeof(path)));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_procedure_read(&set.id, &procedure.id, &output));
    TEST_CHECK(!path_exists(path));

    macro_model_free_procedure(&procedure);
    macro_model_free_macro(&first_macro);
    macro_model_free_macro(&second_macro);
}

static void test_missing_set_empty_steps_and_revision_overflow(void) {
    reset_store();
    macro_set_t set;
    macro_t first_macro;
    macro_t second_macro;
    create_set_and_macros(&set, &first_macro, &second_macro);
    procedure_t procedure = make_procedure(1700U, &set, &first_macro, &second_macro, "Limits");

    const app_uuid_t absent_set = make_uuid(999U);
    procedure.set_id = absent_set;
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_procedure_create(&absent_set, &procedure));
    procedure.set_id = set.id;

    procedure_t empty = procedure;
    empty.steps = NULL;
    empty.step_count = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_procedure_create(&set.id, &empty));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_procedure_create(&set.id, &procedure));
    procedure.revision = UINT32_MAX;
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_procedure_json(&procedure, &json, &length));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_procedure_path(&set.id, &procedure.id, path, sizeof(path)));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    cJSON_free(json);
    procedure_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_procedure_update(&set.id, &procedure, UINT32_MAX, &output));
    TEST_CHECK(output.steps == NULL);

    macro_model_free_procedure(&procedure);
    macro_model_free_macro(&first_macro);
    macro_model_free_macro(&second_macro);
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_argument_validation();
    test_crud_and_order();
    test_reference_validation();
    test_corrupt_and_stale_references_are_discarded();
    test_missing_set_empty_steps_and_revision_overflow();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage procedure repository tests passed");
    return EXIT_SUCCESS;
}
