#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
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
#include "macro_limits.h"
#include "storage.h"
#include "storage_package.h"
#include "storage_repository_tree_internal.h"
#include "test_assert.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define LOCAL_MACRO_ID "22222222-2222-4222-8222-222222222222"
#define GLOBAL_MACRO_ID "23232323-2323-4232-8232-232323232323"
#define PROCEDURE_ID "33333333-3333-4333-8333-333333333333"
#define LOCAL_STEP_ID "44444444-4444-4444-8444-444444444444"
#define GLOBAL_STEP_ID "45454545-4545-4545-8545-454545454545"

static const char BACKUP_PACKAGE[] =
    "{\"schema_version\":1,\"package_type\":\"backup\",\"sets\":[{"
    "\"schema_version\":1,\"id\":\"" SET_ID
    "\",\"revision\":7,\"name\":\"Restored\",\"description\":\"\","
    "\"manufacturer\":\"\",\"model\":\"\",\"board\":\"\","
    "\"keyboard_layout\":\"en-US\",\"sort_order\":0}],\"macros\":[{"
    "\"schema_version\":1,\"id\":\"" LOCAL_MACRO_ID
    "\",\"revision\":1,\"scope\":\"set\",\"name\":\"Local\","
    "\"source\":\"a\",\"favorite\":false,\"key_press_ms\":8,"
    "\"inter_key_ms\":15,\"set_id\":\"" SET_ID
    "\"}],\"global_macros\":[{\"schema_version\":1,\"id\":\""
    GLOBAL_MACRO_ID
    "\",\"revision\":2,\"scope\":\"global\",\"name\":\"Global\","
    "\"source\":\"b\",\"favorite\":false,\"key_press_ms\":8,"
    "\"inter_key_ms\":15}],\"procedures\":[{\"schema_version\":1,"
    "\"id\":\"" PROCEDURE_ID "\",\"revision\":3,\"set_id\":\""
    SET_ID
    "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{"
    "\"id\":\"" LOCAL_STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Local\",\"macro_id\":\""
    LOCAL_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false},{\"id\":\""
    GLOBAL_STEP_ID
    "\",\"type\":\"macro\",\"title\":\"Global\",\"macro_id\":\""
    GLOBAL_MACRO_ID
    "\",\"required\":true,\"auto_complete_on_success\":false}],"
    "\"sort_order\":0}],\"progress\":[{\"schema_version\":1,\"set_id\":\""
    SET_ID "\",\"procedure_id\":\"" PROCEDURE_ID
    "\",\"procedure_revision\":3,\"current_step_id\":\"" LOCAL_STEP_ID
    "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}]}";

static void join_path(char *output, size_t output_size, const char *root, const char *name) {
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

static void create_empty_repository(void) {
    remove_storage();
    make_directory(STORAGE_DATA_MOUNT);
    char path[APP_PATH_MAX_BYTES];
    static const char *const directories[] = {
        "transactions", "staging", "trash", "quarantine", "sets", "global",
    };
    for (size_t index = 0U; index < sizeof(directories) / sizeof(directories[0]); ++index) {
        join_path(path, sizeof(path), STORAGE_DATA_MOUNT, directories[index]);
        make_directory(path);
    }
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "global/macros");
    make_directory(path);
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "schema.json");
    write_text(path, "{\"schema_version\":1}");
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "set-index.json");
    write_text(path, "{\"schema_version\":1,\"ids\":[]}");
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "global/macro-order.json");
    write_text(path, "{\"schema_version\":1,\"ids\":[]}");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_tree_validate(STORAGE_DATA_MOUNT));
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

static char *read_text(const char *path) {
    struct stat metadata;
    TEST_CHECK_EQ_INT(0, stat(path, &metadata));
    TEST_CHECK(metadata.st_size >= 0);
    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    TEST_CHECK(data != NULL);
    const int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    TEST_CHECK_EQ_U64(length, (size_t)read(descriptor, data, length));
    TEST_CHECK_EQ_INT(0, close(descriptor));
    data[length] = '\0';
    return data;
}

static void test_complete_backup_restores_atomically(void) {
    create_empty_repository();
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        storage_package_restore_backup(BACKUP_PACKAGE, sizeof(BACKUP_PACKAGE) - 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_tree_validate(STORAGE_DATA_MOUNT));

    char path[APP_PATH_MAX_BYTES];
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "set-index.json");
    char *index = read_text(path);
    TEST_CHECK(strstr(index, SET_ID) != NULL);
    free(index);
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT,
              "sets/" SET_ID "/set.json");
    char *set = read_text(path);
    TEST_CHECK(strstr(set, "\"revision\":7") != NULL);
    TEST_CHECK(strstr(set, "\"name\":\"Restored\"") != NULL);
    free(set);

    static const char *const evidence_roots[] = {"transactions", "staging", "trash"};
    for (size_t item = 0U; item < sizeof(evidence_roots) / sizeof(evidence_roots[0]); ++item) {
        join_path(path, sizeof(path), STORAGE_DATA_MOUNT, evidence_roots[item]);
        TEST_CHECK(directory_empty(path));
    }
}

static void test_invalid_backup_does_not_mutate_repository(void) {
    create_empty_repository();
    static const char invalid[] =
        "{\"schema_version\":1,\"package_type\":\"macro-set\",\"sets\":[],"
        "\"macros\":[],\"global_macros\":[],\"procedures\":[],\"progress\":[]}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_package_restore_backup(invalid, sizeof(invalid) - 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_tree_validate(STORAGE_DATA_MOUNT));
    char path[APP_PATH_MAX_BYTES];
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "set-index.json");
    char *index = read_text(path);
    TEST_CHECK(strcmp(index, "{\"schema_version\":1,\"ids\":[]}") == 0);
    free(index);
    join_path(path, sizeof(path), STORAGE_DATA_MOUNT, "transactions");
    TEST_CHECK(directory_empty(path));
}

int main(void) {
    test_complete_backup_restores_atomically();
    test_invalid_backup_does_not_mutate_repository();
    remove_storage();
    puts("storage package restore tests passed");
    return EXIT_SUCCESS;
}
