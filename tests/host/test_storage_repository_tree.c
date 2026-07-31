#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_repository_internal.h"
#include "storage_repository_tree_internal.h"
#include "test_assert.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
#define GLOBAL_MACRO_ID "23232323-2323-4232-8232-232323232323"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define LOCAL_STEP_ID "44444444-4444-4444-8444-444444444444"
#define GLOBAL_STEP_ID "45454545-4545-4545-8545-454545454545"
#define MISSING_MACRO_ID "56565656-5656-4656-8656-565656565656"

static const char SET_INDEX_JSON[] =
    "{\"schema_version\":1,\"ids\":[\"" SET_ID "\"]}";
static const char SET_JSON[] =
    "{\"schema_version\":1,\"id\":\"" SET_ID
    "\",\"revision\":7,\"name\":\"Set\",\"description\":\"\",\"manufacturer\":\"\","
    "\"model\":\"\",\"board\":\"\",\"keyboard_layout\":\"en-US\",\"sort_order\":0}";
static const char LOCAL_MACRO_JSON[] =
    "{\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
    "\",\"revision\":1,\"scope\":\"set\",\"name\":\"Local\",\"source\":\"a\","
    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15,\"set_id\":\"" SET_ID
    "\"}";
static const char GLOBAL_MACRO_JSON[] =
    "{\"schema_version\":1,\"id\":\"" GLOBAL_MACRO_ID
    "\",\"revision\":2,\"scope\":\"global\",\"name\":\"Global\",\"source\":\"b\","
    "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}";
static const char PROCEDURE_JSON[] =
    "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID
    "\",\"revision\":3,\"set_id\":\"" SET_ID
    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{\"id\":\""
    LOCAL_STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Local\",\"macro_id\":\"" LOCAL_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false},{\"id\":\""
    GLOBAL_STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Global\",\"macro_id\":\"" GLOBAL_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}";
static const char INVALID_PROCEDURE_JSON[] =
    "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID
    "\",\"revision\":3,\"set_id\":\"" SET_ID
    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{\"id\":\""
    LOCAL_STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Missing\",\"macro_id\":\""
    MISSING_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}";
static const char PROGRESS_JSON[] =
    "{\"schema_version\":1,\"set_id\":\"" SET_ID
    "\",\"procedure_id\":\"" PROCEDURE_ID
    "\",\"procedure_revision\":3,\"current_step_id\":\"" LOCAL_STEP_ID
    "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}";
static const char MACRO_ORDER_JSON[] =
    "{\"schema_version\":1,\"ids\":[\"" LOCAL_MACRO_ID "\"]}";
static const char GLOBAL_ORDER_JSON[] =
    "{\"schema_version\":1,\"ids\":[\"" GLOBAL_MACRO_ID "\"]}";
static const char PROCEDURE_ORDER_JSON[] =
    "{\"schema_version\":1,\"ids\":[\"" PROCEDURE_ID "\"]}";

app_error_code_t storage_repository_parse_index(const char *data, size_t length,
                                                storage_set_index_t *out_index) {
    memset(out_index, 0, sizeof(*out_index));
    cJSON *root = cJSON_ParseWithLength(data, length);
    const cJSON *version =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "schema_version");
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (!cJSON_IsObject(root) || !cJSON_IsNumber(version) || version == NULL ||
        version->valueint != 1 || !cJSON_IsArray(ids)) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = cJSON_GetArraySize(ids);
    if (count < 0 || count > (int)APP_MACRO_SETS_MAX) {
        cJSON_Delete(root);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_index->ids[(size_t)index]) != APP_ERROR_NONE) {
            cJSON_Delete(root);
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    out_index->count = (size_t)count;
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}

static void path_join(char *output, size_t output_size, const char *root, const char *name) {
    const int written = snprintf(output, output_size, "%s/%s", root, name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void write_text(const char *path, const char *text) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    TEST_CHECK_EQ_U64(length, (size_t)write(descriptor, text, length));
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void remove_storage(void) {
    char command[APP_PATH_MAX_BYTES + 32U];
    const int written = snprintf(command, sizeof(command), "rm -rf -- '%s'", STORAGE_DATA_MOUNT);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    TEST_CHECK_EQ_INT(0, system(command));
}

static void create_valid_repository(const char *root) {
    make_directory(root);
    char path[APP_PATH_MAX_BYTES];
    path_join(path, sizeof(path), root, "set-index.json");
    write_text(path, SET_INDEX_JSON);
    char sets_root[APP_PATH_MAX_BYTES];
    path_join(sets_root, sizeof(sets_root), root, "sets");
    make_directory(sets_root);
    char set_root[APP_PATH_MAX_BYTES];
    path_join(set_root, sizeof(set_root), sets_root, SET_ID);
    make_directory(set_root);
    static const char *const set_directories[] = {"macros", "procedures", "progress"};
    for (size_t index = 0U; index < sizeof(set_directories) / sizeof(set_directories[0]); ++index) {
        path_join(path, sizeof(path), set_root, set_directories[index]);
        make_directory(path);
    }
    path_join(path, sizeof(path), set_root, "set.json");
    write_text(path, SET_JSON);
    path_join(path, sizeof(path), set_root, "macro-order.json");
    write_text(path, MACRO_ORDER_JSON);
    path_join(path, sizeof(path), set_root, "procedure-order.json");
    write_text(path, PROCEDURE_ORDER_JSON);
    path_join(path, sizeof(path), set_root, "macros/" LOCAL_MACRO_ID ".json");
    write_text(path, LOCAL_MACRO_JSON);
    path_join(path, sizeof(path), set_root, "procedures/" PROCEDURE_ID ".json");
    write_text(path, PROCEDURE_JSON);
    path_join(path, sizeof(path), set_root, "progress/" PROCEDURE_ID ".json");
    write_text(path, PROGRESS_JSON);

    char global_root[APP_PATH_MAX_BYTES];
    path_join(global_root, sizeof(global_root), root, "global");
    make_directory(global_root);
    path_join(path, sizeof(path), global_root, "macro-order.json");
    write_text(path, GLOBAL_ORDER_JSON);
    path_join(path, sizeof(path), global_root, "macros");
    make_directory(path);
    path_join(path, sizeof(path), global_root, "macros/" GLOBAL_MACRO_ID ".json");
    write_text(path, GLOBAL_MACRO_JSON);
}

static void test_live_and_staged_roots(void) {
    remove_storage();
    create_valid_repository(STORAGE_DATA_MOUNT);
    char extra[APP_PATH_MAX_BYTES];
    path_join(extra, sizeof(extra), STORAGE_DATA_MOUNT, "schema.json");
    write_text(extra, "{\"schema_version\":1}");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_tree_validate(STORAGE_DATA_MOUNT));

    char staged[APP_PATH_MAX_BYTES];
    path_join(staged, sizeof(staged), STORAGE_DATA_MOUNT, "staged-root");
    create_valid_repository(staged);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_tree_validate(staged));
    path_join(extra, sizeof(extra), staged, "unexpected");
    write_text(extra, "x");
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_repository_tree_validate(staged));
}

static void test_missing_and_misnamed_objects_fail(void) {
    remove_storage();
    create_valid_repository(STORAGE_DATA_MOUNT);
    char path[APP_PATH_MAX_BYTES];
    path_join(path, sizeof(path), STORAGE_DATA_MOUNT,
              "global/macros/" GLOBAL_MACRO_ID ".json");
    TEST_CHECK_EQ_INT(0, unlink(path));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_tree_validate(STORAGE_DATA_MOUNT));

    remove_storage();
    create_valid_repository(STORAGE_DATA_MOUNT);
    char source[APP_PATH_MAX_BYTES];
    char destination[APP_PATH_MAX_BYTES];
    path_join(source, sizeof(source), STORAGE_DATA_MOUNT,
              "sets/" SET_ID "/macros/" LOCAL_MACRO_ID ".json");
    path_join(destination, sizeof(destination), STORAGE_DATA_MOUNT,
              "sets/" SET_ID "/macros/" MISSING_MACRO_ID ".json");
    TEST_CHECK_EQ_INT(0, rename(source, destination));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_tree_validate(STORAGE_DATA_MOUNT));
}

static void test_cross_object_semantics_are_revalidated(void) {
    remove_storage();
    create_valid_repository(STORAGE_DATA_MOUNT);
    char path[APP_PATH_MAX_BYTES];
    path_join(path, sizeof(path), STORAGE_DATA_MOUNT,
              "sets/" SET_ID "/procedures/" PROCEDURE_ID ".json");
    write_text(path, INVALID_PROCEDURE_JSON);
    /* The bounded package validator classifies a semantically invalid package as
     * invalid input. The restore transaction adapter converts this result to
     * storage corruption before it can influence recovery. */
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_repository_tree_validate(STORAGE_DATA_MOUNT));
}

static void test_invalid_arguments(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_repository_tree_validate(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_repository_tree_validate(""));
}

int main(void) {
    test_invalid_arguments();
    test_live_and_staged_roots();
    test_missing_and_misnamed_objects_fail();
    test_cross_object_semantics_are_revalidated();
    remove_storage();
    puts("storage repository tree tests passed");
    return EXIT_SUCCESS;
}
