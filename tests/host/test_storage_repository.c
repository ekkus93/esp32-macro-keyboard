#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "storage.h"
#include "storage_repository.h"
#include "storage_repository_internal.h"

static void fail_check(const char *expression, const char *file, int line)
{
    (void)fprintf(stderr, "check failed: %s (%s:%d)\n", expression, file, line);
    exit(EXIT_FAILURE);
}

#define CHECK(expression)                                  \
    do {                                                   \
        if (!(expression)) {                               \
            fail_check(#expression, __FILE__, __LINE__);  \
        }                                                  \
    } while (0)

static bool path_exists(const char *path)
{
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void remove_tree(const char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        CHECK(errno == ENOENT);
        return;
    }
    while (true) {
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char child[APP_PATH_MAX_BYTES];
        const int written = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        CHECK(written > 0 && (size_t)written < sizeof(child));
        struct stat metadata;
        CHECK(stat(child, &metadata) == 0);
        if (S_ISDIR(metadata.st_mode)) {
            remove_tree(child);
        } else {
            CHECK(unlink(child) == 0);
        }
    }
    CHECK(closedir(directory) == 0);
    CHECK(rmdir(path) == 0);
}

static void make_directory(const char *path)
{
    CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static void reset_store(void)
{
    remove_tree(STORAGE_DATA_MOUNT);
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
    CHECK(storage_repository_init() == APP_ERROR_NONE);
}

static macro_set_t make_set(const char *name)
{
    macro_set_t set = {0};
    set.schema_version = APP_SCHEMA_VERSION;
    CHECK(app_uuid_generate(&set.id) == APP_ERROR_NONE);
    set.revision = 1U;
    CHECK(snprintf(set.name, sizeof(set.name), "%s", name) > 0);
    CHECK(snprintf(set.description, sizeof(set.description), "Repository test") > 0);
    CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static void test_crud_and_revisions(void)
{
    reset_store();
    macro_set_t set = make_set("Test Set");
    CHECK(storage_set_create(&set) == APP_ERROR_NONE);

    storage_set_list_t list = {0};
    CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    CHECK(list.count == 1U);
    CHECK(app_uuid_equal(&list.items[0].id, &set.id));

    macro_set_t replacement = set;
    CHECK(snprintf(replacement.name, sizeof(replacement.name), "Updated Set") > 0);
    macro_set_t updated = {0};
    CHECK(storage_set_update(&replacement, 2U, &updated) == APP_ERROR_CONFLICT);
    CHECK(storage_set_update(&replacement, 1U, &updated) == APP_ERROR_NONE);
    CHECK(updated.revision == 2U);
    CHECK(strcmp(updated.name, "Updated Set") == 0);

    macro_set_t readback = {0};
    CHECK(storage_set_read(&set.id, &readback) == APP_ERROR_NONE);
    CHECK(readback.revision == 2U);
    CHECK(storage_set_delete(&set.id, 1U) == APP_ERROR_CONFLICT);
    CHECK(storage_set_delete(&set.id, 2U) == APP_ERROR_NONE);
    CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    CHECK(list.count == 0U);
    CHECK(storage_set_read(&set.id, &readback) == APP_ERROR_NOT_FOUND);
}

static void test_create_recovery(void)
{
    reset_store();
    macro_set_t set = make_set("Interrupted Create");
    CHECK(storage_set_create(&set) == APP_ERROR_NONE);

    char destination[APP_PATH_MAX_BYTES];
    CHECK(storage_make_set_path(&set.id, destination, sizeof(destination)) == APP_ERROR_NONE);
    app_uuid_t transaction_id = {0};
    CHECK(app_uuid_generate(&transaction_id) == APP_ERROR_NONE);
    char staging[APP_PATH_MAX_BYTES];
    const int staging_length = snprintf(staging, sizeof(staging),
                                        STORAGE_DATA_MOUNT "/staging/%s",
                                        transaction_id.value);
    CHECK(staging_length > 0 && (size_t)staging_length < sizeof(staging));
    CHECK(storage_repository_set_index_presence(&set.id, false) == APP_ERROR_NONE);
    CHECK(rename(destination, staging) == 0);

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_IMPORT_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .replacement_revision = 1U,
    };
    CHECK(snprintf(manifest.staging, sizeof(manifest.staging), "%s", staging) > 0);
    CHECK(snprintf(manifest.destination, sizeof(manifest.destination), "%s", destination) > 0);
    CHECK(storage_transaction_write_manifest(&manifest) == APP_ERROR_NONE);
    CHECK(storage_transaction_recover_all() == APP_ERROR_NONE);

    storage_set_list_t list = {0};
    CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    CHECK(list.count == 1U);
    CHECK(path_exists(destination));
    CHECK(!path_exists(staging));
}

static void test_delete_recovery(void)
{
    reset_store();
    macro_set_t set = make_set("Interrupted Delete");
    CHECK(storage_set_create(&set) == APP_ERROR_NONE);

    char source[APP_PATH_MAX_BYTES];
    CHECK(storage_make_set_path(&set.id, source, sizeof(source)) == APP_ERROR_NONE);
    app_uuid_t transaction_id = {0};
    CHECK(app_uuid_generate(&transaction_id) == APP_ERROR_NONE);
    char backup[APP_PATH_MAX_BYTES];
    const int backup_length = snprintf(backup, sizeof(backup),
                                       STORAGE_DATA_MOUNT "/trash/%s-%s",
                                       set.id.value, transaction_id.value);
    CHECK(backup_length > 0 && (size_t)backup_length < sizeof(backup));

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_DELETE_SET,
        .phase = STORAGE_TRANSACTION_PREPARED,
        .expected_revision = 1U,
    };
    CHECK(snprintf(manifest.source, sizeof(manifest.source), "%s", source) > 0);
    CHECK(snprintf(manifest.backup, sizeof(manifest.backup), "%s", backup) > 0);
    CHECK(storage_transaction_write_manifest(&manifest) == APP_ERROR_NONE);
    CHECK(storage_transaction_recover_all() == APP_ERROR_NONE);

    storage_set_list_t list = {0};
    CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    CHECK(list.count == 0U);
    CHECK(!path_exists(source));
    CHECK(path_exists(backup));
}

static void test_unknown_transaction_is_preserved(void)
{
    reset_store();
    app_uuid_t transaction_id = {0};
    CHECK(app_uuid_generate(&transaction_id) == APP_ERROR_NONE);
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_RESTORE,
        .phase = STORAGE_TRANSACTION_PREPARED,
    };
    CHECK(storage_transaction_write_manifest(&manifest) == APP_ERROR_NONE);
    CHECK(storage_transaction_recover_all() == APP_ERROR_STORAGE_CORRUPT);

    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path),
                                 STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 transaction_id.value);
    CHECK(written > 0 && (size_t)written < sizeof(path));
    CHECK(path_exists(path));
}

static void test_quarantine_preserves_evidence(void)
{
    reset_store();
    macro_set_t set = make_set("Corrupt Set");
    CHECK(storage_set_create(&set) == APP_ERROR_NONE);
    char directory[APP_PATH_MAX_BYTES];
    CHECK(storage_make_set_path(&set.id, directory, sizeof(directory)) == APP_ERROR_NONE);
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), "%s/set.json", directory);
    CHECK(written > 0 && (size_t)written < sizeof(path));

    static const char invalid[] = "{not json";
    int descriptor = open(path, O_WRONLY | O_TRUNC);
    CHECK(descriptor >= 0);
    CHECK(write(descriptor, invalid, sizeof(invalid) - 1U) ==
          (ssize_t)(sizeof(invalid) - 1U));
    CHECK(close(descriptor) == 0);

    macro_set_t output = {0};
    CHECK(storage_set_read(&set.id, &output) == APP_ERROR_STORAGE_CORRUPT);
    CHECK(!path_exists(path));
    storage_quarantine_list_t quarantine = {0};
    CHECK(storage_quarantine_list(&quarantine) == APP_ERROR_NONE);
    CHECK(quarantine.count == 1U);
    CHECK(strcmp(quarantine.items[0].source_path, path) == 0);
    CHECK(strstr(quarantine.items[0].reason, "invalid set") != NULL);
    CHECK(path_exists(quarantine.items[0].evidence_path));
}

static void test_missing_index_is_not_recreated(void)
{
    reset_store();
    CHECK(unlink(STORAGE_DATA_MOUNT "/set-index.json") == 0);
    CHECK(storage_repository_init() == APP_ERROR_STORAGE_CORRUPT);
    CHECK(!path_exists(STORAGE_DATA_MOUNT "/set-index.json"));
}

int main(void)
{
    test_crud_and_revisions();
    test_create_recovery();
    test_delete_recovery();
    test_unknown_transaction_is_preserved();
    test_quarantine_preserves_evidence();
    test_missing_index_is_not_recreated();
    remove_tree(STORAGE_DATA_MOUNT);
    puts("storage repository tests passed");
    return EXIT_SUCCESS;
}
