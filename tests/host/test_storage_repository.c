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

#include "test_assert.h"
#include "test_temp_dir.h"

static bool path_exists(const char *path)
{
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void make_directory(const char *path)
{
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static void reset_store(void)
{
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
    TEST_CHECK(storage_repository_init() == APP_ERROR_NONE);
}

static macro_set_t make_set(const char *name)
{
    macro_set_t set = {0};
    set.schema_version = APP_SCHEMA_VERSION;
    TEST_CHECK(app_uuid_generate(&set.id) == APP_ERROR_NONE);
    set.revision = 1U;
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "%s", name) > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Repository test") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static void test_crud_and_revisions(void)
{
    reset_store();
    macro_set_t set = make_set("Test Set");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK(list.count == 1U);
    TEST_CHECK_EQ_UUID(&set.id, &list.items[0].id);

    macro_set_t replacement = set;
    TEST_CHECK(snprintf(replacement.name, sizeof(replacement.name), "Updated Set") > 0);
    macro_set_t updated = {0};
    TEST_CHECK(storage_set_update(&replacement, 2U, &updated) == APP_ERROR_CONFLICT);
    TEST_CHECK(storage_set_update(&replacement, 1U, &updated) == APP_ERROR_NONE);
    TEST_CHECK(updated.revision == 2U);
    TEST_CHECK(strcmp(updated.name, "Updated Set") == 0);

    macro_set_t readback = {0};
    TEST_CHECK(storage_set_read(&set.id, &readback) == APP_ERROR_NONE);
    TEST_CHECK(readback.revision == 2U);
    TEST_CHECK(storage_set_delete(&set.id, 1U) == APP_ERROR_CONFLICT);
    TEST_CHECK(storage_set_delete(&set.id, 2U) == APP_ERROR_NONE);
    TEST_CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    TEST_CHECK(list.count == 0U);
    TEST_CHECK(storage_set_read(&set.id, &readback) == APP_ERROR_NOT_FOUND);
}

static void test_create_recovery(void)
{
    reset_store();
    macro_set_t set = make_set("Interrupted Create");
    TEST_CHECK(storage_set_create(&set) == APP_ERROR_NONE);

    char destination[APP_PATH_MAX_BYTES];
    TEST_CHECK(storage_make_set_path(&set.id, destination, sizeof(destination)) == APP_ERROR_NONE);
    app_uuid_t transaction_id = {0};
    TEST_CHECK(app_uuid_generate(&transaction_id) == APP_ERROR_NONE);
    char staging[APP_PATH_MAX_BYTES];
    const int staging_length = snprintf(staging,
                                        sizeof(staging),
                                        STORAGE_DATA_MOUNT "/staging/%s",
                                        transaction_id.value);
    TEST_CHECK(staging_length > 0 && (size_t)staging_length < sizeof(staging));
    TEST_CHECK(storage_repository_set_index_presence(&set.id, false) == APP_ERROR_NONE);
    TEST_CHECK(rename(destination, staging) == 0);

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_IMPORT_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .replacement_revision = 1U,
    };
    TEST_CHECK(snprintf(manifest.staging, sizeof(manifest.staging), "%s", staging) > 0);
    TEST_CHECK(snprintf(manifest.destination, sizeof(manifest.destination), "%s", destination) > 0);
    TEST_CHECK(storage_transaction_write_manifest(&manifest) == APP_ERROR_NONE);
    TEST_CHECK(storage_transaction_recover_all() == APP_ERROR_NONE);

    storage_set_list_t list = {0};
    TEST_CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    TEST_CHECK(list.count == 1U);
    TEST_CHECK(path_exists(destination));
    TEST_CHECK(!path_exists(staging));
}

static void test_delete_recovery(void)
{
    reset_store();
    macro_set_t set = make_set("Interrupted Delete");
    TEST_CHECK(storage_set_create(&set) == APP_ERROR_NONE);

    char source[APP_PATH_MAX_BYTES];
    TEST_CHECK(storage_make_set_path(&set.id, source, sizeof(source)) == APP_ERROR_NONE);
    app_uuid_t transaction_id = {0};
    TEST_CHECK(app_uuid_generate(&transaction_id) == APP_ERROR_NONE);
    char backup[APP_PATH_MAX_BYTES];
    const int backup_length = snprintf(backup,
                                       sizeof(backup),
                                       STORAGE_DATA_MOUNT "/trash/%s-%s",
                                       set.id.value,
                                       transaction_id.value);
    TEST_CHECK(backup_length > 0 && (size_t)backup_length < sizeof(backup));

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_DELETE_SET,
        .phase = STORAGE_TRANSACTION_PREPARED,
        .expected_revision = 1U,
    };
    TEST_CHECK(snprintf(manifest.source, sizeof(manifest.source), "%s", source) > 0);
    TEST_CHECK(snprintf(manifest.backup, sizeof(manifest.backup), "%s", backup) > 0);
    TEST_CHECK(storage_transaction_write_manifest(&manifest) == APP_ERROR_NONE);
    TEST_CHECK(storage_transaction_recover_all() == APP_ERROR_NONE);

    storage_set_list_t list = {0};
    TEST_CHECK(storage_set_list(&list) == APP_ERROR_NONE);
    TEST_CHECK(list.count == 0U);
    TEST_CHECK(!path_exists(source));
    TEST_CHECK(path_exists(backup));
}

static void test_unknown_transaction_is_preserved(void)
{
    reset_store();
    app_uuid_t transaction_id = {0};
    TEST_CHECK(app_uuid_generate(&transaction_id) == APP_ERROR_NONE);
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_RESTORE,
        .phase = STORAGE_TRANSACTION_PREPARED,
    };
    TEST_CHECK(storage_transaction_write_manifest(&manifest) == APP_ERROR_NONE);
    TEST_CHECK(storage_transaction_recover_all() == APP_ERROR_STORAGE_CORRUPT);

    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path,
                                 sizeof(path),
                                 STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 transaction_id.value);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(path));
    TEST_CHECK(path_exists(path));
}

static void test_quarantine_preserves_evidence(void)
{
    reset_store();
    macro_set_t set = make_set("Corrupt Set");
    TEST_CHECK(storage_set_create(&set) == APP_ERROR_NONE);
    char directory[APP_PATH_MAX_BYTES];
    TEST_CHECK(storage_make_set_path(&set.id, directory, sizeof(directory)) == APP_ERROR_NONE);
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), "%s/set.json", directory);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(path));

    static const char invalid[] = "{not json";
    int descriptor = open(path, O_WRONLY | O_TRUNC);
    TEST_CHECK(descriptor >= 0);
    TEST_CHECK(write(descriptor, invalid, sizeof(invalid) - 1U) ==
               (ssize_t)(sizeof(invalid) - 1U));
    TEST_CHECK(close(descriptor) == 0);

    macro_set_t output = {0};
    TEST_CHECK(storage_set_read(&set.id, &output) == APP_ERROR_STORAGE_CORRUPT);
    TEST_CHECK(!path_exists(path));
    storage_quarantine_list_t quarantine = {0};
    TEST_CHECK(storage_quarantine_list(&quarantine) == APP_ERROR_NONE);
    TEST_CHECK(quarantine.count == 1U);
    TEST_CHECK(strcmp(quarantine.items[0].source_path, path) == 0);
    TEST_CHECK(strstr(quarantine.items[0].reason, "invalid set") != NULL);
    TEST_CHECK(path_exists(quarantine.items[0].evidence_path));
}

static void test_missing_index_is_not_recreated(void)
{
    reset_store();
    TEST_CHECK(unlink(STORAGE_DATA_MOUNT "/set-index.json") == 0);
    TEST_CHECK(storage_repository_init() == APP_ERROR_STORAGE_CORRUPT);
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/set-index.json"));
}

int main(void)
{
    test_crud_and_revisions();
    test_create_recovery();
    test_delete_recovery();
    test_unknown_transaction_is_preserved();
    test_quarantine_preserves_evidence();
    test_missing_index_is_not_recreated();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    puts("storage repository tests passed");
    return EXIT_SUCCESS;
}
