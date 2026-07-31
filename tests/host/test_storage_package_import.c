#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_package.h"
#include "storage_repository.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"
#include "test_assert.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define NEW_SET_ID_1 "55555555-5555-4555-8555-555555555555"
#define NEW_SET_ID_2 "66666666-6666-4666-8666-666666666666"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
#define GLOBAL_MACRO_ID "23232323-2323-4232-8232-232323232323"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define STEP_ID "44444444-4444-4444-8444-444444444444"

static const char PACKAGE[] =
    "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":["
    "{\"schema_version\":1,\"id\":\"" SET_ID
    "\",\"revision\":7,\"name\":\"Replacement\",\"description\":\"new\","
    "\"manufacturer\":\"Vendor\",\"model\":\"Model\",\"board\":\"Board\","
    "\"keyboard_layout\":\"en-US\",\"sort_order\":0}],\"macros\":["
    "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
    "\",\"revision\":4,\"scope\":\"set\",\"name\":\"Local\",\"source\":\"a\","
    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID
    "\"}],\"global_macros\":["
    "{\"schema_version\":1,\"id\":\"" GLOBAL_MACRO_ID
    "\",\"revision\":2,\"scope\":\"global\",\"name\":\"Global\",\"source\":\"b\","
    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}],\"procedures\":["
    "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID "\",\"revision\":3,\"set_id\":\"" SET_ID
    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{\"id\":\"" STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Step\",\"macro_id\":\"" GLOBAL_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}],"
    "\"progress\":[{\"schema_version\":1,\"set_id\":\"" SET_ID "\",\"procedure_id\":\"" PROCEDURE_ID
    "\",\"procedure_revision\":3,\"current_step_id\":\"" STEP_ID
    "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}]}";

static app_uuid_t parse_id(const char *value) {
    app_uuid_t id = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, app_uuid_parse(value, &id));
    return id;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void join_path(char *output, size_t output_size, const char *parent, const char *name) {
    const int written = snprintf(output, output_size, "%s/%s", parent, name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void reset_storage(void) {
    storage_repository_lock_deinit();
    char command[APP_PATH_MAX_BYTES + 32U];
    const int written = snprintf(command, sizeof(command), "rm -rf '%s'", STORAGE_DATA_MOUNT);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    TEST_CHECK_EQ_INT(0, system(command));
    make_directory(STORAGE_DATA_MOUNT);
    static const char *const roots[] = {
        "transactions", "staging", "sets", "trash", "global", "quarantine",
    };
    char path[APP_PATH_MAX_BYTES];
    for (size_t index = 0U; index < sizeof(roots) / sizeof(roots[0]); ++index) {
        join_path(path, sizeof(path), STORAGE_DATA_MOUNT, roots[index]);
        make_directory(path);
    }
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT "/global", "macros");
    make_directory(path);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_lock_init());
}

static void write_set_file(const char *set_path, const macro_set_t *set) {
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_serialize_set_json(set, &json, &length));
    char path[APP_PATH_MAX_BYTES];
    join_path(path, sizeof(path), set_path, "set.json");
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    cJSON_free(json);
}

static void create_current_set(void) {
    const app_uuid_t id = parse_id(SET_ID);
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = id,
        .revision = 3U,
        .sort_order = 0,
    };
    memcpy(set.name, "Current", sizeof("Current"));
    memcpy(set.keyboard_layout, "en-US", sizeof("en-US"));
    char set_path[APP_PATH_MAX_BYTES];
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_make_set_path(&id, set_path, sizeof(set_path)));
    make_directory(set_path);
    static const char *const children[] = {"macros", "procedures", "progress"};
    char path[APP_PATH_MAX_BYTES];
    for (size_t index = 0U; index < sizeof(children) / sizeof(children[0]); ++index) {
        join_path(path, sizeof(path), set_path, children[index]);
        make_directory(path);
    }
    write_set_file(set_path, &set);
    static const char empty_order[] = "{\"schema_version\":1,\"ids\":[]}";
    join_path(path, sizeof(path), set_path, "macro-order.json");
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_atomic_write(path, empty_order, strlen(empty_order), true));
    join_path(path, sizeof(path), set_path, "procedure-order.json");
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_atomic_write(path, empty_order, strlen(empty_order), true));
    storage_set_index_t index = {.ids = {id}, .count = 1U};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_write_index(&index));
}

static void create_global_macro(const char *source) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_id(GLOBAL_MACRO_ID),
        .revision = 2U,
        .scope = MACRO_SCOPE_GLOBAL,
        .has_set_id = false,
        .source = (char *)source,
        .source_length = strlen(source),
        .favorite = false,
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    memcpy(macro.name, "Global", sizeof("Global"));
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_repository_serialize_macro_json(&macro, &json, &length));
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_make_global_macro_path(&macro.id, path, sizeof(path)));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write(path, json, length, true));
    cJSON_free(json);
}

static bool directory_empty(const char *path) {
    DIR *directory = opendir(path);
    TEST_CHECK(directory != NULL);
    bool empty = true;
    while (true) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            TEST_CHECK_EQ_INT(0, errno);
            break;
        }
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            empty = false;
            break;
        }
    }
    TEST_CHECK_EQ_INT(0, closedir(directory));
    return empty;
}

static void assert_index_count(size_t expected) {
    storage_set_index_t index = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_load_index(&index));
    TEST_CHECK_EQ_U64(expected, index.count);
}

static void assert_not_created(const char *id_string) {
    const app_uuid_t id = parse_id(id_string);
    macro_set_t set = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NOT_FOUND, storage_set_read(&id, &set));
}

/* reset_storage() only creates the directory topology; unlike create_current_set(),
 * an empty repository has no set-index.json yet until something writes one (normally
 * storage_repository_init() at boot). Import must be exercisable against a truly
 * empty repository, so tests that don't call create_current_set() need this instead. */
static void write_empty_index(void) {
    storage_set_index_t index = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_write_index(&index));
}

static void test_invalid_arguments_and_collision_do_not_mutate(void) {
    reset_storage();
    create_current_set();
    create_global_macro("b");
    macro_set_t committed = {0};
    const app_uuid_t new_id = parse_id(NEW_SET_ID_1);
    const app_uuid_t existing_id = parse_id(SET_ID);
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_package_import_set(NULL, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_package_import_set(&new_id, NULL, sizeof(PACKAGE) - 1U, &committed));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_package_import_set(&new_id, PACKAGE, 0U, &committed));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_package_import_set(&new_id, "{}", 2U, &committed));
    TEST_CHECK_EQ_INT(
        APP_ERROR_CONFLICT,
        storage_package_import_set(&existing_id, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    assert_index_count(1U);
}

static void test_global_dependency_must_match(void) {
    reset_storage();
    write_empty_index();
    macro_set_t committed = {0};
    const app_uuid_t new_id = parse_id(NEW_SET_ID_1);
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT, storage_package_import_set(
                                              &new_id, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    assert_not_created(NEW_SET_ID_1);
    assert_index_count(0U);

    create_global_macro("c");
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT, storage_package_import_set(
                                              &new_id, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    assert_not_created(NEW_SET_ID_1);
    assert_index_count(0U);
}

static void test_valid_import_assigns_new_identity_and_resets_revisions(void) {
    reset_storage();
    write_empty_index();
    create_global_macro("b");
    macro_set_t committed = {0};
    const app_uuid_t new_id = parse_id(NEW_SET_ID_1);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_package_import_set(&new_id, PACKAGE,
                                                                 sizeof(PACKAGE) - 1U, &committed));
    TEST_CHECK(app_uuid_equal(&new_id, &committed.id));
    TEST_CHECK_EQ_U64(1U, committed.revision);
    TEST_CHECK_EQ_STRING("Replacement", committed.name);

    /* The package's own embedded set id must never be materialized. */
    assert_not_created(SET_ID);

    storage_macro_location_t location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = new_id,
    };
    macro_t macro = {0};
    const app_uuid_t macro_id = parse_id(LOCAL_MACRO_ID);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_macro_read(&location, &macro_id, &macro));
    TEST_CHECK_EQ_U64(1U, macro.revision);
    TEST_CHECK(macro.has_set_id);
    TEST_CHECK(app_uuid_equal(&new_id, &macro.set_id));
    macro_model_free_macro(&macro);

    procedure_t procedure = {0};
    const app_uuid_t procedure_id = parse_id(PROCEDURE_ID);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_procedure_read(&new_id, &procedure_id, &procedure));
    TEST_CHECK_EQ_U64(1U, procedure.revision);
    TEST_CHECK(app_uuid_equal(&new_id, &procedure.set_id));
    macro_model_free_procedure(&procedure);

    const storage_procedure_identity_t identity = {
        .set_id = new_id,
        .procedure_id = procedure_id,
    };
    storage_progress_snapshot_t progress = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_progress_read(&identity, &progress));
    TEST_CHECK_EQ_U64(STORAGE_PROGRESS_STATUS_CURRENT, progress.status);
    TEST_CHECK_EQ_U64(1U, progress.current_procedure_revision);
    TEST_CHECK_EQ_U64(1U, progress.progress.procedure_revision);
    TEST_CHECK(app_uuid_equal(&new_id, &progress.progress.set_id));

    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/transactions"));
    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/staging"));
    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/trash"));
    assert_index_count(1U);
}

static void test_repeated_import_produces_distinct_sets(void) {
    reset_storage();
    write_empty_index();
    create_global_macro("b");
    macro_set_t first = {0};
    macro_set_t second = {0};
    const app_uuid_t first_id = parse_id(NEW_SET_ID_1);
    const app_uuid_t second_id = parse_id(NEW_SET_ID_2);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_package_import_set(&first_id, PACKAGE, sizeof(PACKAGE) - 1U, &first));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_package_import_set(&second_id, PACKAGE,
                                                                 sizeof(PACKAGE) - 1U, &second));
    TEST_CHECK_EQ_U64(1U, first.revision);
    TEST_CHECK_EQ_U64(1U, second.revision);
    TEST_CHECK(!app_uuid_equal(&first.id, &second.id));

    macro_set_t readback = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_set_read(&first_id, &readback));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_set_read(&second_id, &readback));
    assert_index_count(2U);
}

int main(void) {
    test_invalid_arguments_and_collision_do_not_mutate();
    test_global_dependency_must_match();
    test_valid_import_assigns_new_identity_and_resets_revisions();
    test_repeated_import_produces_distinct_sets();
    reset_storage();
    storage_repository_lock_deinit();
    puts("storage package import tests passed");
    return EXIT_SUCCESS;
}
