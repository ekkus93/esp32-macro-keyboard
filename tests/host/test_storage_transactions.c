#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fake_fs_backend.h"
#include "storage.h"
#include "storage_repository_internal.h"
#include "storage_transaction_internal.h"
#include "test_assert.h"

#define INDEX_CALL_CAPACITY 16U

typedef struct {
    size_t next_value;
    size_t calls;
    size_t fail_on_call;
} uuid_sequence_t;

typedef struct {
    app_uuid_t ids[INDEX_CALL_CAPACITY];
    bool presence[INDEX_CALL_CAPACITY];
    size_t count;
    size_t fail_on_call;
    app_error_code_t failure;
} index_fixture_t;

static int adapter_open(void *context, const char *path, int flags, mode_t mode)
{
    return fake_fs_open(context, path, flags, mode);
}

static ssize_t adapter_read(void *context,
                            int descriptor,
                            void *buffer,
                            size_t length)
{
    return fake_fs_read(context, descriptor, buffer, length);
}

static ssize_t adapter_write(void *context,
                             int descriptor,
                             const void *buffer,
                             size_t length)
{
    return fake_fs_write(context, descriptor, buffer, length);
}

static int adapter_sync(void *context, int descriptor)
{
    return fake_fs_sync(context, descriptor);
}

static int adapter_close(void *context, int descriptor)
{
    return fake_fs_close(context, descriptor);
}

static int adapter_stat(void *context,
                        const char *path,
                        struct stat *metadata)
{
    return fake_fs_stat(context, path, metadata);
}

static int adapter_rename(void *context,
                          const char *source,
                          const char *destination)
{
    return fake_fs_rename(context, source, destination);
}

static int adapter_unlink(void *context, const char *path)
{
    return fake_fs_unlink(context, path);
}

static int adapter_mkdir(void *context, const char *path, mode_t mode)
{
    return fake_fs_mkdir(context, path, mode);
}

static void *adapter_open_directory(void *context, const char *path)
{
    return fake_fs_open_dir(context, path);
}

static int adapter_read_directory(void *context,
                                  void *directory,
                                  char *name,
                                  size_t name_size,
                                  bool *out_end)
{
    return fake_fs_read_dir(context, directory, name, name_size, out_end);
}

static int adapter_close_directory(void *context, void *directory)
{
    return fake_fs_close_dir(context, directory);
}

static int adapter_remove_directory(void *context, const char *path)
{
    return fake_fs_rmdir(context, path);
}

static storage_fs_ops_t make_operations(fake_fs_backend_t *filesystem)
{
    return (storage_fs_ops_t){
        .context = filesystem,
        .open_file = adapter_open,
        .read_file = adapter_read,
        .write_file = adapter_write,
        .sync_file = adapter_sync,
        .close_file = adapter_close,
        .stat_path = adapter_stat,
        .rename_path = adapter_rename,
        .unlink_path = adapter_unlink,
        .make_directory = adapter_mkdir,
        .open_directory = adapter_open_directory,
        .read_directory = adapter_read_directory,
        .close_directory = adapter_close_directory,
        .remove_directory = adapter_remove_directory,
    };
}

static app_error_code_t generate_uuid(void *context, app_uuid_t *out_uuid)
{
    uuid_sequence_t *sequence = context;
    if (sequence == NULL || out_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++sequence->calls;
    if (sequence->fail_on_call != 0U &&
        sequence->calls == sequence->fail_on_call) {
        return APP_ERROR_INTERNAL;
    }
    ++sequence->next_value;
    const int written = snprintf(out_uuid->value,
                                 sizeof(out_uuid->value),
                                 "ffffffff-ffff-4fff-8fff-%012zu",
                                 sequence->next_value);
    return written == (int)APP_UUID_STRING_LENGTH ? APP_ERROR_NONE
                                                  : APP_ERROR_INTERNAL;
}

static app_error_code_t update_index(void *context,
                                     const app_uuid_t *set_id,
                                     bool should_be_present)
{
    index_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(set_id != NULL);
    TEST_CHECK(fixture->count < INDEX_CALL_CAPACITY);
    fixture->ids[fixture->count] = *set_id;
    fixture->presence[fixture->count] = should_be_present;
    ++fixture->count;
    if (fixture->fail_on_call != 0U &&
        fixture->count == fixture->fail_on_call) {
        return fixture->failure;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_set_index_presence(
    const app_uuid_t *set_id,
    bool should_be_present)
{
    (void)set_id;
    (void)should_be_present;
    return APP_ERROR_INTERNAL;
}

static app_uuid_t parse_uuid(const char *value)
{
    app_uuid_t id = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, app_uuid_parse(value, &id));
    return id;
}

static void checked_path(char *output,
                         size_t output_size,
                         const char *directory,
                         const char *name)
{
    const int written = snprintf(output,
                                 output_size,
                                 "%s/%s",
                                 directory,
                                 name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void transaction_path(char *output,
                             size_t output_size,
                             const app_uuid_t *transaction_id)
{
    const int written = snprintf(output,
                                 output_size,
                                 STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 transaction_id->value);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void staging_path(char *output,
                         size_t output_size,
                         const app_uuid_t *transaction_id)
{
    const int written = snprintf(output,
                                 output_size,
                                 STORAGE_DATA_MOUNT "/staging/%s",
                                 transaction_id->value);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void set_path(char *output,
                     size_t output_size,
                     const char *set_id)
{
    const int written = snprintf(output,
                                 output_size,
                                 STORAGE_DATA_MOUNT "/sets/%s",
                                 set_id);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void trash_path(char *output,
                       size_t output_size,
                       const char *set_id,
                       const app_uuid_t *transaction_id)
{
    const int written = snprintf(output,
                                 output_size,
                                 STORAGE_DATA_MOUNT "/trash/%s-%s",
                                 set_id,
                                 transaction_id->value);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void create_directory(const char *path)
{
    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void reset_storage(void)
{
    char command[APP_PATH_MAX_BYTES + 32U];
    const int written = snprintf(command,
                                 sizeof(command),
                                 "rm -rf '%s'",
                                 STORAGE_DATA_MOUNT);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    TEST_CHECK_EQ_INT(0, system(command));
    create_directory(STORAGE_DATA_MOUNT);

    static const char *const children[] = {
        "transactions",
        "staging",
        "sets",
        "trash",
    };
    for (size_t index = 0U;
         index < sizeof(children) / sizeof(children[0]);
         ++index) {
        char path[APP_PATH_MAX_BYTES];
        checked_path(path, sizeof(path), STORAGE_DATA_MOUNT, children[index]);
        create_directory(path);
    }
}

static bool path_exists(const char *path)
{
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void write_raw_file(const char *path, const void *data, size_t length)
{
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const ssize_t count = write(descriptor, data, length);
    TEST_CHECK(count >= 0);
    TEST_CHECK_EQ_U64(length, (size_t)count);
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static storage_transaction_manifest_t make_create_manifest(
    const char *transaction_value,
    const char *set_value,
    storage_transaction_phase_t phase)
{
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_uuid(transaction_value),
        .type = STORAGE_TRANSACTION_IMPORT_SET,
        .phase = phase,
        .expected_revision = 0U,
        .replacement_revision = 1U,
    };
    staging_path(manifest.staging,
                 sizeof(manifest.staging),
                 &manifest.id);
    set_path(manifest.destination,
             sizeof(manifest.destination),
             set_value);
    return manifest;
}

static storage_transaction_manifest_t make_delete_manifest(
    const char *transaction_value,
    const char *set_value,
    storage_transaction_phase_t phase)
{
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_uuid(transaction_value),
        .type = STORAGE_TRANSACTION_DELETE_SET,
        .phase = phase,
        .expected_revision = 3U,
        .replacement_revision = 0U,
    };
    set_path(manifest.source, sizeof(manifest.source), set_value);
    trash_path(manifest.backup,
               sizeof(manifest.backup),
               set_value,
               &manifest.id);
    return manifest;
}

static void write_manifest(storage_transaction_manifest_t *manifest,
                           storage_fs_ops_t *operations,
                           uuid_sequence_t *uuids)
{
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_transaction_write_manifest_with_ops(
                          manifest,
                          operations,
                          generate_uuid,
                          uuids));
}

static app_error_code_t recover(storage_fs_ops_t *operations,
                                uuid_sequence_t *uuids,
                                index_fixture_t *index)
{
    return storage_transaction_recover_all_with_ops(operations,
                                                    generate_uuid,
                                                    uuids,
                                                    update_index,
                                                    index);
}

static void test_invalid_arguments(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_create_manifest(
        "00000000-0000-4000-8000-000000000001",
        "10000000-0000-4000-8000-000000000001",
        STORAGE_TRANSACTION_STAGED);

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_transaction_write_manifest_with_ops(
                          NULL,
                          &operations,
                          generate_uuid,
                          &uuids));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_transaction_write_manifest_with_ops(
                          &manifest,
                          NULL,
                          generate_uuid,
                          &uuids));
    manifest.replacement_revision = 0U;
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_transaction_write_manifest_with_ops(
                          &manifest,
                          &operations,
                          generate_uuid,
                          &uuids));

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_transaction_recover_all_with_ops(
                          NULL,
                          generate_uuid,
                          &uuids,
                          update_index,
                          &index));
    operations.read_directory = NULL;
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      recover(&operations, &uuids, &index));
}

static void test_create_recovery_is_idempotent(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_create_manifest(
        "00000000-0000-4000-8000-000000000010",
        "10000000-0000-4000-8000-000000000010",
        STORAGE_TRANSACTION_STAGED);
    create_directory(manifest.staging);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK(!path_exists(manifest.staging));
    TEST_CHECK(path_exists(manifest.destination));
    char path[APP_PATH_MAX_BYTES];
    transaction_path(path, sizeof(path), &manifest.id);
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(2U, index.count);
    TEST_CHECK(index.presence[0]);
    TEST_CHECK(index.presence[1]);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK_EQ_U64(2U, index.count);
}

static void test_delete_recovery_is_idempotent(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_delete_manifest(
        "00000000-0000-4000-8000-000000000020",
        "10000000-0000-4000-8000-000000000020",
        STORAGE_TRANSACTION_PREPARED);
    create_directory(manifest.source);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK(!path_exists(manifest.source));
    TEST_CHECK(path_exists(manifest.backup));
    char path[APP_PATH_MAX_BYTES];
    transaction_path(path, sizeof(path), &manifest.id);
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(2U, index.count);
    TEST_CHECK(!index.presence[0]);
    TEST_CHECK(!index.presence[1]);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK_EQ_U64(2U, index.count);
}

static void test_conflicting_create_paths_are_preserved(void)
{
    static const bool states[][2] = {
        {false, false},
        {true, true},
    };
    for (size_t case_index = 0U;
         case_index < sizeof(states) / sizeof(states[0]);
         ++case_index) {
        reset_storage();
        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        storage_fs_ops_t operations = make_operations(&filesystem);
        uuid_sequence_t uuids = {0};
        index_fixture_t index = {.failure = APP_ERROR_IO};
        storage_transaction_manifest_t manifest = make_create_manifest(
            "00000000-0000-4000-8000-000000000030",
            "10000000-0000-4000-8000-000000000030",
            STORAGE_TRANSACTION_STAGED);
        if (states[case_index][0]) {
            create_directory(manifest.staging);
        }
        if (states[case_index][1]) {
            create_directory(manifest.destination);
        }
        write_manifest(&manifest, &operations, &uuids);
        char path[APP_PATH_MAX_BYTES];
        transaction_path(path, sizeof(path), &manifest.id);
        fake_fs_backend_reset(&filesystem);

        TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT,
                          recover(&operations, &uuids, &index));
        TEST_CHECK(path_exists(path));
        TEST_CHECK_EQ_U64(0U, index.count);
    }
}

static void test_directory_and_manifest_read_failures(void)
{
    static const fake_fs_operation_t directory_failures[] = {
        FAKE_FS_OPEN_DIR,
        FAKE_FS_READ_DIR,
        FAKE_FS_CLOSE_DIR,
    };
    for (size_t case_index = 0U;
         case_index < sizeof(directory_failures) /
                          sizeof(directory_failures[0]);
         ++case_index) {
        reset_storage();
        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        fake_fs_backend_fail_on(&filesystem,
                                directory_failures[case_index],
                                1U,
                                EIO);
        storage_fs_ops_t operations = make_operations(&filesystem);
        uuid_sequence_t uuids = {0};
        index_fixture_t index = {.failure = APP_ERROR_IO};
        TEST_CHECK_EQ_INT(APP_ERROR_IO,
                          recover(&operations, &uuids, &index));
    }

    static const fake_fs_operation_t read_failures[] = {
        FAKE_FS_STAT,
        FAKE_FS_OPEN,
        FAKE_FS_READ,
        FAKE_FS_CLOSE,
    };
    for (size_t case_index = 0U;
         case_index < sizeof(read_failures) / sizeof(read_failures[0]);
         ++case_index) {
        reset_storage();
        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        storage_fs_ops_t operations = make_operations(&filesystem);
        uuid_sequence_t uuids = {0};
        index_fixture_t index = {.failure = APP_ERROR_IO};
        storage_transaction_manifest_t manifest = make_create_manifest(
            "00000000-0000-4000-8000-000000000040",
            "10000000-0000-4000-8000-000000000040",
            STORAGE_TRANSACTION_STAGED);
        create_directory(manifest.staging);
        write_manifest(&manifest, &operations, &uuids);
        fake_fs_backend_reset(&filesystem);
        fake_fs_backend_fail_on(&filesystem,
                                read_failures[case_index],
                                1U,
                                EIO);

        TEST_CHECK_EQ_INT(APP_ERROR_IO,
                          recover(&operations, &uuids, &index));
        char path[APP_PATH_MAX_BYTES];
        transaction_path(path, sizeof(path), &manifest.id);
        TEST_CHECK(path_exists(path));
    }
}

static void test_phase_write_failure_can_be_retried(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_create_manifest(
        "00000000-0000-4000-8000-000000000050",
        "10000000-0000-4000-8000-000000000050",
        STORAGE_TRANSACTION_STAGED);
    create_directory(manifest.staging);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_WRITE, 1U, EIO);

    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    TEST_CHECK(!path_exists(manifest.staging));
    TEST_CHECK(path_exists(manifest.destination));
    TEST_CHECK_EQ_U64(0U, index.count);

    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK_EQ_U64(2U, index.count);
}

static void test_rename_index_and_unlink_failures(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_create_manifest(
        "00000000-0000-4000-8000-000000000060",
        "10000000-0000-4000-8000-000000000060",
        STORAGE_TRANSACTION_STAGED);
    create_directory(manifest.staging);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_RENAME, 1U, EIO);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    TEST_CHECK(path_exists(manifest.staging));

    reset_storage();
    fake_fs_backend_reset(&filesystem);
    operations = make_operations(&filesystem);
    uuids = (uuid_sequence_t){0};
    index = (index_fixture_t){
        .fail_on_call = 1U,
        .failure = APP_ERROR_IO,
    };
    manifest = make_create_manifest(
        "00000000-0000-4000-8000-000000000061",
        "10000000-0000-4000-8000-000000000061",
        STORAGE_TRANSACTION_ACTIVATED);
    create_directory(manifest.destination);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    TEST_CHECK_EQ_U64(1U, index.count);

    reset_storage();
    fake_fs_backend_reset(&filesystem);
    operations = make_operations(&filesystem);
    uuids = (uuid_sequence_t){0};
    index = (index_fixture_t){.failure = APP_ERROR_IO};
    manifest = make_create_manifest(
        "00000000-0000-4000-8000-000000000062",
        "10000000-0000-4000-8000-000000000062",
        STORAGE_TRANSACTION_INDEXED);
    create_directory(manifest.destination);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_UNLINK, 1U, EIO);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    char path[APP_PATH_MAX_BYTES];
    transaction_path(path, sizeof(path), &manifest.id);
    TEST_CHECK(path_exists(path));
}

static void test_corrupt_manifest_does_not_block_valid_manifest(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};

    storage_transaction_manifest_t corrupt = make_create_manifest(
        "00000000-0000-4000-8000-000000000070",
        "10000000-0000-4000-8000-000000000070",
        STORAGE_TRANSACTION_STAGED);
    corrupt.type = (storage_transaction_type_t)99;
    char corrupt_path[APP_PATH_MAX_BYTES];
    transaction_path(corrupt_path, sizeof(corrupt_path), &corrupt.id);
    write_raw_file(corrupt_path, &corrupt, sizeof(corrupt));

    storage_transaction_manifest_t valid = make_create_manifest(
        "00000000-0000-4000-8000-000000000071",
        "10000000-0000-4000-8000-000000000071",
        STORAGE_TRANSACTION_STAGED);
    create_directory(valid.staging);
    write_manifest(&valid, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT,
                      recover(&operations, &uuids, &index));
    TEST_CHECK(path_exists(corrupt_path));
    TEST_CHECK(path_exists(valid.destination));
    char valid_path[APP_PATH_MAX_BYTES];
    transaction_path(valid_path, sizeof(valid_path), &valid.id);
    TEST_CHECK(!path_exists(valid_path));
    TEST_CHECK_EQ_U64(2U, index.count);
}

static void test_manifest_order_is_deterministic(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};

    storage_transaction_manifest_t later = make_create_manifest(
        "00000000-0000-4000-8000-000000000082",
        "10000000-0000-4000-8000-000000000082",
        STORAGE_TRANSACTION_INDEXED);
    storage_transaction_manifest_t earlier = make_create_manifest(
        "00000000-0000-4000-8000-000000000081",
        "10000000-0000-4000-8000-000000000081",
        STORAGE_TRANSACTION_INDEXED);
    create_directory(later.destination);
    create_directory(earlier.destination);
    write_manifest(&later, &operations, &uuids);
    write_manifest(&earlier, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK_EQ_U64(2U, index.count);
    TEST_CHECK_EQ_STRING("10000000-0000-4000-8000-000000000081",
                         index.ids[0].value);
    TEST_CHECK_EQ_STRING("10000000-0000-4000-8000-000000000082",
                         index.ids[1].value);
}

static void test_orphaned_staging_is_visible(void)
{
    reset_storage();
    char orphan[APP_PATH_MAX_BYTES];
    checked_path(orphan,
                 sizeof(orphan),
                 STORAGE_DATA_MOUNT "/staging",
                 "orphan");
    create_directory(orphan);
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT,
                      recover(&operations, &uuids, &index));
}

int main(void)
{
    test_invalid_arguments();
    test_create_recovery_is_idempotent();
    test_delete_recovery_is_idempotent();
    test_conflicting_create_paths_are_preserved();
    test_directory_and_manifest_read_failures();
    test_phase_write_failure_can_be_retried();
    test_rename_index_and_unlink_failures();
    test_corrupt_manifest_does_not_block_valid_manifest();
    test_manifest_order_is_deterministic();
    test_orphaned_staging_is_visible();
    reset_storage();
    puts("storage transaction tests passed");
    return EXIT_SUCCESS;
}
