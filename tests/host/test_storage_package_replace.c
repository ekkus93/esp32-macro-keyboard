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
#define OTHER_SET_ID "12121212-1212-4212-8212-121212121212"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"

static const char PACKAGE[] = "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":["
                              "{\"schema_version\":1,\"id\":\"" SET_ID
                              "\",\"revision\":7,\"name\":\"Replacement\"}],\"macros\":["
                              "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
                              "\",\"revision\":4,\"name\":\"Local\",\"source\":\"a\","
                              "\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}]}";

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
        "transactions",
        "staging",
        "sets",
        "trash",
    };
    char path[APP_PATH_MAX_BYTES];
    for (size_t index = 0U; index < sizeof(roots) / sizeof(roots[0]); ++index) {
        join_path(path, sizeof(path), STORAGE_DATA_MOUNT, roots[index]);
        make_directory(path);
    }
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
    };
    memcpy(set.name, "Current", sizeof("Current"));
    char set_path[APP_PATH_MAX_BYTES];
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_make_set_path(&id, set_path, sizeof(set_path)));
    make_directory(set_path);
    static const char *const children[] = {"macros"};
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
    storage_set_index_t index = {.ids = {id}, .count = 1U};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_write_index(&index));
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

static void prepare_valid_state(void) {
    reset_storage();
    create_current_set();
}

static void assert_current_revision(uint32_t revision) {
    macro_set_t set = {0};
    const app_uuid_t id = parse_id(SET_ID);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_set_read(&id, &set));
    TEST_CHECK_EQ_U64(revision, set.revision);
}

static void test_invalid_and_conflict_inputs_do_not_mutate(void) {
    prepare_valid_state();
    macro_set_t committed = {0};
    const app_uuid_t id = parse_id(SET_ID);
    const app_uuid_t other = parse_id(OTHER_SET_ID);
    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        storage_package_replace_set(NULL, 3U, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        storage_package_replace_set(&other, 3U, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_package_replace_set(&id, 3U, "{}", 2U, &committed));
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT, storage_package_replace_set(
                                              &id, 2U, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    assert_current_revision(3U);
}

static void test_valid_replace_commits_complete_tree(void) {
    prepare_valid_state();
    macro_set_t committed = {0};
    const app_uuid_t id = parse_id(SET_ID);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_package_replace_set(
                                          &id, 3U, PACKAGE, sizeof(PACKAGE) - 1U, &committed));
    TEST_CHECK_EQ_U64(7U, committed.revision);
    TEST_CHECK_EQ_STRING("Replacement", committed.name);
    assert_current_revision(7U);

    macro_t macro = {0};
    const app_uuid_t macro_id = parse_id(LOCAL_MACRO_ID);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_macro_read(&id, &macro_id, &macro));
    TEST_CHECK_EQ_U64(4U, macro.revision);
    macro_model_free_macro(&macro);

    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/transactions"));
    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/staging"));
    TEST_CHECK(directory_empty(STORAGE_DATA_MOUNT "/trash"));
    storage_set_index_t index = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_repository_load_index(&index));
    TEST_CHECK_EQ_U64(1U, index.count);
    TEST_CHECK(app_uuid_equal(&index.ids[0], &id));
}

int main(void) {
    test_invalid_and_conflict_inputs_do_not_mutate();
    test_valid_replace_commits_complete_tree();
    reset_storage();
    storage_repository_lock_deinit();
    puts("storage package replace tests passed");
    return EXIT_SUCCESS;
}
