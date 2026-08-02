#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_set_tree_internal.h"
#include "test_assert.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define LOCAL_STEP_ID "44444444-4444-4444-8444-444444444444"

static const char SET_JSON[] =
    "{\"schema_version\":1,\"id\":\"" SET_ID
    "\",\"revision\":7,\"name\":\"Set\",\"description\":\"\",\"manufacturer\":\"\","
    "\"model\":\"\",\"board\":\"\",\"keyboard_layout\":\"en-US\",\"sort_order\":0}";
static const char LOCAL_MACRO_JSON[] =
    "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
    "\",\"revision\":1,\"name\":\"Local\",\"source\":\"a\","
    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}";
static const char PROCEDURE_JSON[] =
    "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID "\",\"revision\":3,\"set_id\":\"" SET_ID
    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{\"id\":\"" LOCAL_STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Local\",\"macro_id\":\"" LOCAL_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}";
static const char PROGRESS_JSON[] =
    "{\"schema_version\":1,\"set_id\":\"" SET_ID "\",\"procedure_id\":\"" PROCEDURE_ID
    "\",\"procedure_revision\":3,\"current_step_id\":\"" LOCAL_STEP_ID
    "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}";
static const char MACRO_ORDER_JSON[] = "{\"schema_version\":1,\"ids\":[\"" LOCAL_MACRO_ID "\"]}";
static const char PROCEDURE_ORDER_JSON[] = "{\"schema_version\":1,\"ids\":[\"" PROCEDURE_ID "\"]}";

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void join_path(char *output, size_t output_size, const char *parent, const char *name) {
    const int written = snprintf(output, output_size, "%s/%s", parent, name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void write_text(const char *path, const char *text) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    TEST_CHECK_EQ_U64(length, (size_t)write(descriptor, text, length));
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void reset_storage(void) {
    char command[APP_PATH_MAX_BYTES + 32U];
    const int written = snprintf(command, sizeof(command), "rm -rf '%s'", STORAGE_DATA_MOUNT);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    TEST_CHECK_EQ_INT(0, system(command));
    make_directory(STORAGE_DATA_MOUNT);
}

static void create_valid_tree(char *out_set_path, size_t out_set_path_size) {
    const int written =
        snprintf(out_set_path, out_set_path_size, STORAGE_DATA_MOUNT "/sets/%s", SET_ID);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < out_set_path_size);
    char sets_path[APP_PATH_MAX_BYTES];
    join_path(sets_path, sizeof(sets_path), STORAGE_DATA_MOUNT, "sets");
    make_directory(sets_path);
    make_directory(out_set_path);

    static const char *const directories[] = {"macros", "procedures", "progress"};
    char path[APP_PATH_MAX_BYTES];
    for (size_t index = 0U; index < sizeof(directories) / sizeof(directories[0]); ++index) {
        join_path(path, sizeof(path), out_set_path, directories[index]);
        make_directory(path);
    }

    join_path(path, sizeof(path), out_set_path, "set.json");
    write_text(path, SET_JSON);
    join_path(path, sizeof(path), out_set_path, "macro-order.json");
    write_text(path, MACRO_ORDER_JSON);
    join_path(path, sizeof(path), out_set_path, "procedure-order.json");
    write_text(path, PROCEDURE_ORDER_JSON);

    char directory_path[APP_PATH_MAX_BYTES];
    join_path(directory_path, sizeof(directory_path), out_set_path, "macros");
    join_path(path, sizeof(path), directory_path, LOCAL_MACRO_ID ".json");
    write_text(path, LOCAL_MACRO_JSON);
    join_path(directory_path, sizeof(directory_path), out_set_path, "procedures");
    join_path(path, sizeof(path), directory_path, PROCEDURE_ID ".json");
    write_text(path, PROCEDURE_JSON);
    join_path(directory_path, sizeof(directory_path), out_set_path, "progress");
    join_path(path, sizeof(path), directory_path, PROCEDURE_ID ".json");
    write_text(path, PROGRESS_JSON);
}

static app_uuid_t set_id(void) {
    app_uuid_t id = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, app_uuid_parse(SET_ID, &id));
    return id;
}

static void test_valid_tree(void) {
    reset_storage();
    char path[APP_PATH_MAX_BYTES];
    create_valid_tree(path, sizeof(path));
    const app_uuid_t id = set_id();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_set_tree_validate(path, &id, 7U));
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(path, &id, 6U));
}

static void test_unknown_and_missing_root_entries(void) {
    reset_storage();
    char set_path[APP_PATH_MAX_BYTES];
    create_valid_tree(set_path, sizeof(set_path));
    char path[APP_PATH_MAX_BYTES];
    join_path(path, sizeof(path), set_path, "unknown.json");
    write_text(path, "{}");
    const app_uuid_t id = set_id();
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(set_path, &id, 7U));

    reset_storage();
    create_valid_tree(set_path, sizeof(set_path));
    join_path(path, sizeof(path), set_path, "procedure-order.json");
    TEST_CHECK_EQ_INT(0, unlink(path));
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(set_path, &id, 7U));
}

static void test_order_and_filename_integrity(void) {
    reset_storage();
    char set_path[APP_PATH_MAX_BYTES];
    create_valid_tree(set_path, sizeof(set_path));
    char path[APP_PATH_MAX_BYTES];
    join_path(path, sizeof(path), set_path, "macro-order.json");
    write_text(path, "{\"schema_version\":1,\"ids\":[]}");
    const app_uuid_t id = set_id();
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(set_path, &id, 7U));

    reset_storage();
    create_valid_tree(set_path, sizeof(set_path));
    char directory_path[APP_PATH_MAX_BYTES];
    join_path(directory_path, sizeof(directory_path), set_path, "macros");
    join_path(path, sizeof(path), directory_path, "not-a-uuid.json");
    write_text(path, LOCAL_MACRO_JSON);
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(set_path, &id, 7U));
}

static void test_references_and_progress_integrity(void) {
    reset_storage();
    char set_path[APP_PATH_MAX_BYTES];
    create_valid_tree(set_path, sizeof(set_path));
    char path[APP_PATH_MAX_BYTES];
    join_path(path, sizeof(path), set_path, "macros/" LOCAL_MACRO_ID ".json");
    TEST_CHECK_EQ_INT(0, unlink(path));
    const app_uuid_t id = set_id();
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(set_path, &id, 7U));

    reset_storage();
    create_valid_tree(set_path, sizeof(set_path));
    char progress_path[APP_PATH_MAX_BYTES];
    join_path(progress_path, sizeof(progress_path), set_path, "progress");
    join_path(path, sizeof(path), progress_path, PROCEDURE_ID ".json");
    static const char stale_progress[] =
        "{\"schema_version\":1,\"set_id\":\"" SET_ID "\",\"procedure_id\":\"" PROCEDURE_ID
        "\",\"procedure_revision\":2,\"current_step_id\":\"" LOCAL_STEP_ID
        "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}";
    write_text(path, stale_progress);
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT, storage_set_tree_validate(set_path, &id, 7U));
}

static void test_invalid_arguments(void) {
    const app_uuid_t id = set_id();
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, storage_set_tree_validate(NULL, &id, 7U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, storage_set_tree_validate("", &id, 7U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_set_tree_validate(STORAGE_DATA_MOUNT, NULL, 7U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_set_tree_validate(STORAGE_DATA_MOUNT, &id, 0U));
}

int main(void) {
    test_invalid_arguments();
    test_valid_tree();
    test_unknown_and_missing_root_entries();
    test_order_and_filename_integrity();
    test_references_and_progress_integrity();
    reset_storage();
    puts("storage set tree tests passed");
    return EXIT_SUCCESS;
}
