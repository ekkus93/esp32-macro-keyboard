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
#include "storage_quarantine_internal.h"
#include "test_assert.h"

typedef struct {
    size_t next_value;
    size_t calls;
    size_t fail_on_call;
    app_error_code_t failure;
} uuid_sequence_t;

static int adapter_open(void *context, const char *path, int flags, mode_t mode) {
    return fake_fs_open(context, path, flags, mode);
}

static ssize_t adapter_read(void *context, int descriptor, void *buffer, size_t length) {
    return fake_fs_read(context, descriptor, buffer, length);
}

static ssize_t adapter_write(void *context, int descriptor, const void *buffer, size_t length) {
    return fake_fs_write(context, descriptor, buffer, length);
}

static int adapter_sync(void *context, int descriptor) {
    return fake_fs_sync(context, descriptor);
}

static int adapter_close(void *context, int descriptor) {
    return fake_fs_close(context, descriptor);
}

static int adapter_stat(void *context, const char *path, struct stat *metadata) {
    return fake_fs_stat(context, path, metadata);
}

static int adapter_rename(void *context, const char *source, const char *destination) {
    return fake_fs_rename(context, source, destination);
}

static int adapter_unlink(void *context, const char *path) {
    return fake_fs_unlink(context, path);
}

static int adapter_mkdir(void *context, const char *path, mode_t mode) {
    return fake_fs_mkdir(context, path, mode);
}

static void *adapter_open_directory(void *context, const char *path) {
    return fake_fs_open_dir(context, path);
}

static int adapter_read_directory(void *context, void *directory, char *name, size_t name_size,
                                  bool *out_end) {
    return fake_fs_read_dir(context, directory, name, name_size, out_end);
}

static int adapter_close_directory(void *context, void *directory) {
    return fake_fs_close_dir(context, directory);
}

static int adapter_remove_directory(void *context, const char *path) {
    return fake_fs_rmdir(context, path);
}

static storage_fs_ops_t make_operations(fake_fs_backend_t *filesystem) {
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

static app_error_code_t generate_uuid(void *context, app_uuid_t *out_uuid) {
    uuid_sequence_t *sequence = context;
    if (sequence == NULL || out_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++sequence->calls;
    if (sequence->fail_on_call != 0U && sequence->calls == sequence->fail_on_call) {
        return sequence->failure;
    }
    ++sequence->next_value;
    const int written = snprintf(out_uuid->value, sizeof(out_uuid->value),
                                 "00000000-0000-4000-8000-%012zu", sequence->next_value);
    return written == (int)APP_UUID_STRING_LENGTH ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static void create_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0700) == 0 || errno == EEXIST);
}

static void reset_storage(void) {
    char command[APP_PATH_MAX_BYTES + 32U];
    const int written = snprintf(command, sizeof(command), "rm -rf '%s'", STORAGE_DATA_MOUNT);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    TEST_CHECK_EQ_INT(0, system(command));
    create_directory(STORAGE_DATA_MOUNT);
    create_directory(STORAGE_DATA_MOUNT "/quarantine");
    create_directory(STORAGE_DATA_MOUNT "/staging");
    create_directory(STORAGE_DATA_MOUNT "/sets");
}

static void write_file(const char *path, const void *data, size_t length) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const ssize_t count = write(descriptor, data, length);
    TEST_CHECK(count >= 0);
    TEST_CHECK_EQ_U64(length, (size_t)count);
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void write_text(const char *path, const char *text) {
    write_file(path, text, strlen(text));
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void make_committed_dir(char *output, size_t output_size, const char *id) {
    const int written = snprintf(output, output_size, STORAGE_DATA_MOUNT "/quarantine/%s", id);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void make_committed_child(char *output, size_t output_size, const char *id,
                                 const char *child) {
    const int written =
        snprintf(output, output_size, STORAGE_DATA_MOUNT "/quarantine/%s/%s", id, child);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void make_staging_child(char *output, size_t output_size, const char *id,
                               const char *child) {
    const int written =
        snprintf(output, output_size, STORAGE_DATA_MOUNT "/staging/quarantine-%s/%s", id, child);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void make_source_path(char *output, size_t output_size, const char *name) {
    const int written = snprintf(output, output_size, STORAGE_DATA_MOUNT "/sets/%s", name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

/* Build a committed quarantine entry directly on disk (record.json + evidence),
 * in the dir-per-entry layout, for exercising the listing path. */
static void write_committed_entry(const char *id, const char *source_path, const char *reason) {
    char directory[APP_PATH_MAX_BYTES];
    char record[APP_PATH_MAX_BYTES];
    char evidence[APP_PATH_MAX_BYTES];
    make_committed_dir(directory, sizeof(directory), id);
    make_committed_child(record, sizeof(record), id, "record.json");
    make_committed_child(evidence, sizeof(evidence), id, "evidence");
    create_directory(directory);
    write_text(evidence, "evidence");

    char json[1024U];
    const int written = snprintf(json, sizeof(json),
                                 "{\"schema_version\":1,\"id\":\"%s\","
                                 "\"source_path\":\"%s\",\"reason\":\"%s\"}",
                                 id, source_path, reason);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(json));
    write_file(record, json, (size_t)written);
}

static void assert_entry_zero(const storage_quarantine_entry_t *entry) {
    storage_quarantine_entry_t zero = {0};
    TEST_CHECK(memcmp(entry, &zero, sizeof(zero)) == 0);
}

static void test_success_and_list(void) {
    reset_storage();
    char source[APP_PATH_MAX_BYTES];
    make_source_path(source, sizeof(source), "bad.json");
    write_text(source, "bad data");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {.failure = APP_ERROR_INTERNAL};
    storage_quarantine_entry_t entry = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_quarantine_file_with_ops(source, "invalid metadata", &entry,
                                                       &operations, generate_uuid, &uuids));
    TEST_CHECK(!path_exists(source));
    TEST_CHECK_EQ_STRING("00000000-0000-4000-8000-000000000001", entry.id.value);
    TEST_CHECK_EQ_STRING("invalid metadata", entry.reason);

    char committed_dir[APP_PATH_MAX_BYTES];
    char evidence[APP_PATH_MAX_BYTES];
    char record[APP_PATH_MAX_BYTES];
    make_committed_dir(committed_dir, sizeof(committed_dir), entry.id.value);
    make_committed_child(evidence, sizeof(evidence), entry.id.value, "evidence");
    make_committed_child(record, sizeof(record), entry.id.value, "record.json");
    TEST_CHECK(path_exists(committed_dir));
    TEST_CHECK(path_exists(evidence));
    TEST_CHECK(path_exists(record));
    /* evidence_path is derived from the id, never stored in the record. */
    TEST_CHECK_EQ_STRING(evidence, entry.evidence_path);
    /* No staging residue survives a successful commit. */
    char staging_dir[APP_PATH_MAX_BYTES];
    const int staging_written =
        snprintf(staging_dir, sizeof(staging_dir), STORAGE_DATA_MOUNT "/staging/quarantine-%s",
                 entry.id.value);
    TEST_CHECK(staging_written > 0);
    TEST_CHECK(!path_exists(staging_dir));

    fake_fs_backend_reset(&filesystem);
    storage_quarantine_list_t list = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(1U, list.count);
    TEST_CHECK_EQ_U64(0U, list.damaged_count);
    TEST_CHECK_EQ_STRING(entry.id.value, list.items[0].id.value);
    TEST_CHECK_EQ_STRING(source, list.items[0].source_path);
    TEST_CHECK_EQ_STRING(entry.evidence_path, list.items[0].evidence_path);
}

static void test_invalid_arguments_and_boundaries(void) {
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {.failure = APP_ERROR_INTERNAL};
    storage_quarantine_entry_t entry;
    memset(&entry, 0xA5, sizeof(entry));

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_quarantine_file_with_ops(NULL, "reason", &entry, &operations,
                                                       generate_uuid, &uuids));
    assert_entry_zero(&entry);
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_quarantine_file_with_ops("/tmp/outside", "reason", &entry,
                                                       &operations, generate_uuid, &uuids));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_quarantine_file_with_ops(STORAGE_DATA_MOUNT "/quarantine/already",
                                                       "reason", &entry, &operations, generate_uuid,
                                                       &uuids));

    char source[APP_PATH_MAX_BYTES];
    make_source_path(source, sizeof(source), "boundary.json");
    write_text(source, "x");
    char too_long[STORAGE_QUARANTINE_REASON_MAX_BYTES + 1U];
    memset(too_long, 'x', sizeof(too_long) - 1U);
    too_long[sizeof(too_long) - 1U] = '\0';
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_quarantine_file_with_ops(source, too_long, &entry, &operations,
                                                       generate_uuid, &uuids));
    TEST_CHECK(path_exists(source));

    char maximum[STORAGE_QUARANTINE_REASON_MAX_BYTES];
    memset(maximum, 'y', sizeof(maximum) - 1U);
    maximum[sizeof(maximum) - 1U] = '\0';
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_quarantine_file_with_ops(source, maximum, &entry, &operations,
                                                       generate_uuid, &uuids));
    TEST_CHECK_EQ_U64(sizeof(maximum) - 1U, strlen(entry.reason));

    storage_quarantine_list_t list;
    memset(&list, 0xA5, sizeof(list));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, storage_quarantine_list_with_ops(&list, NULL));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK_EQ_U64(0U, list.damaged_count);
}

/* A failure at any staged filesystem step must leave the source in place (the
 * rename-based creation restores it) and no committed entry behind. */
static void test_staged_failures_preserve_source(void) {
    typedef struct {
        fake_fs_operation_t operation;
        size_t occurrence;
    } failure_case_t;
    static const failure_case_t failures[] = {
        {FAKE_FS_MKDIR, 1U},  /* step 1: create staging directory */
        {FAKE_FS_RENAME, 1U}, /* step 2: move source into evidence */
        {FAKE_FS_OPEN, 1U},   /* step 3: open record for writing */
        {FAKE_FS_WRITE, 1U},  /* step 3: write record bytes */
        {FAKE_FS_SYNC, 1U},   /* step 4: sync record */
        {FAKE_FS_CLOSE, 1U},  /* step 3: close record */
        {FAKE_FS_SYNC, 2U},   /* step 4: sync evidence */
        {FAKE_FS_STAT, 4U},   /* step 5: stat record during validation */
        {FAKE_FS_READ, 1U},   /* step 5: read record during validation */
        {FAKE_FS_STAT, 5U},   /* step 5: stat evidence during validation */
        {FAKE_FS_RENAME, 2U}, /* step 7: rename staging into quarantine */
    };

    for (size_t index = 0U; index < sizeof(failures) / sizeof(failures[0]); ++index) {
        reset_storage();
        char source[APP_PATH_MAX_BYTES];
        make_source_path(source, sizeof(source), "staged-failure.json");
        write_text(source, "evidence");

        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        fake_fs_backend_fail_on(&filesystem, failures[index].operation, failures[index].occurrence,
                                EIO);
        storage_fs_ops_t operations = make_operations(&filesystem);
        uuid_sequence_t uuids = {.failure = APP_ERROR_INTERNAL};
        storage_quarantine_entry_t entry;
        memset(&entry, 0xA5, sizeof(entry));

        TEST_CHECK_EQ_INT(APP_ERROR_IO,
                          storage_quarantine_file_with_ops(source, "staged failure", &entry,
                                                           &operations, generate_uuid, &uuids));
        TEST_CHECK(path_exists(source));
        assert_entry_zero(&entry);
        TEST_CHECK(
            !path_exists(STORAGE_DATA_MOUNT "/quarantine/00000000-0000-4000-8000-000000000001"));
        TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT
                                "/staging/quarantine-00000000-0000-4000-8000-000000000001"));
    }
}

/* When rollback cannot restore the source (the restoring rename fails too), the
 * evidence must survive under staging rather than be deleted, and the failure is
 * surfaced. */
static void test_unrestorable_rollback_preserves_evidence(void) {
    reset_storage();
    char source[APP_PATH_MAX_BYTES];
    make_source_path(source, sizeof(source), "unrestorable.json");
    write_text(source, "evidence");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    /* Fail the record open (step 3), then fail the restoring rename in rollback. */
    fake_fs_backend_add_failure(&filesystem, FAKE_FS_OPEN, 1U, EIO);
    fake_fs_backend_add_failure(&filesystem, FAKE_FS_RENAME, 2U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {.failure = APP_ERROR_INTERNAL};
    storage_quarantine_entry_t entry = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      storage_quarantine_file_with_ops(source, "unrestorable", &entry, &operations,
                                                       generate_uuid, &uuids));
    assert_entry_zero(&entry);
    TEST_CHECK(!path_exists(source));
    char staging_evidence[APP_PATH_MAX_BYTES];
    make_staging_child(staging_evidence, sizeof(staging_evidence),
                       "00000000-0000-4000-8000-000000000001", "evidence");
    TEST_CHECK(path_exists(staging_evidence));
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/quarantine/00000000-0000-4000-8000-000000000001"));
}

static void test_uuid_failure_and_collision_handling(void) {
    reset_storage();
    char source[APP_PATH_MAX_BYTES];
    make_source_path(source, sizeof(source), "uuid-failure.json");
    write_text(source, "evidence");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {
        .fail_on_call = 1U,
        .failure = APP_ERROR_INTERNAL,
    };
    storage_quarantine_entry_t entry = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL,
                      storage_quarantine_file_with_ops(source, "uuid failure", &entry, &operations,
                                                       generate_uuid, &uuids));
    TEST_CHECK(path_exists(source));

    reset_storage();
    make_source_path(source, sizeof(source), "collision.json");
    write_text(source, "evidence");
    char collision[APP_PATH_MAX_BYTES];
    make_committed_dir(collision, sizeof(collision), "00000000-0000-4000-8000-000000000001");
    create_directory(collision);
    fake_fs_backend_reset(&filesystem);
    uuids = (uuid_sequence_t){.failure = APP_ERROR_INTERNAL};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_quarantine_file_with_ops(source, "collision retry", &entry,
                                                       &operations, generate_uuid, &uuids));
    TEST_CHECK_EQ_STRING("00000000-0000-4000-8000-000000000002", entry.id.value);

    reset_storage();
    make_source_path(source, sizeof(source), "collision-exhaustion.json");
    write_text(source, "evidence");
    for (size_t value = 1U; value <= 4U; ++value) {
        char id[APP_UUID_STRING_LENGTH + 1U];
        const int written = snprintf(id, sizeof(id), "00000000-0000-4000-8000-%012zu", value);
        TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
        make_committed_dir(collision, sizeof(collision), id);
        create_directory(collision);
    }
    fake_fs_backend_reset(&filesystem);
    uuids = (uuid_sequence_t){.failure = APP_ERROR_INTERNAL};
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT,
                      storage_quarantine_file_with_ops(source, "collision exhaustion", &entry,
                                                       &operations, generate_uuid, &uuids));
    TEST_CHECK(path_exists(source));
    TEST_CHECK_EQ_U64(4U, uuids.calls);
}

static void test_list_is_sorted_and_supports_short_reads(void) {
    reset_storage();
    const char *source = STORAGE_DATA_MOUNT "/sets/original.json";
    write_committed_entry("00000000-0000-4000-8000-000000000200", source, "later");
    write_committed_entry("00000000-0000-4000-8000-000000000100", source, "earlier");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_set_short_read(&filesystem, 3U);
    storage_fs_ops_t operations = make_operations(&filesystem);
    storage_quarantine_list_t list = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(2U, list.count);
    TEST_CHECK_EQ_U64(0U, list.damaged_count);
    TEST_CHECK_EQ_STRING("00000000-0000-4000-8000-000000000100", list.items[0].id.value);
    TEST_CHECK_EQ_STRING("00000000-0000-4000-8000-000000000200", list.items[1].id.value);
    TEST_CHECK(filesystem.operation_counts[FAKE_FS_READ] > 2U);
}

static void test_list_io_failures_are_visible(void) {
    static const fake_fs_operation_t directory_failures[] = {
        FAKE_FS_OPEN_DIR,
        FAKE_FS_READ_DIR,
        FAKE_FS_CLOSE_DIR,
    };
    for (size_t index = 0U; index < sizeof(directory_failures) / sizeof(directory_failures[0]);
         ++index) {
        reset_storage();
        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        fake_fs_backend_fail_on(&filesystem, directory_failures[index], 1U, EIO);
        storage_fs_ops_t operations = make_operations(&filesystem);
        storage_quarantine_list_t list;
        memset(&list, 0xA5, sizeof(list));
        TEST_CHECK_EQ_INT(APP_ERROR_IO, storage_quarantine_list_with_ops(&list, &operations));
        TEST_CHECK_EQ_U64(0U, list.count);
        TEST_CHECK_EQ_U64(0U, list.damaged_count);
    }

    /* A genuine I/O fault while reading one entry is surfaced, not swallowed as
     * mere damage. */
    static const fake_fs_operation_t record_failures[] = {
        FAKE_FS_STAT,
        FAKE_FS_OPEN,
        FAKE_FS_READ,
        FAKE_FS_CLOSE,
    };
    for (size_t index = 0U; index < sizeof(record_failures) / sizeof(record_failures[0]); ++index) {
        reset_storage();
        write_committed_entry("00000000-0000-4000-8000-000000000300",
                              STORAGE_DATA_MOUNT "/sets/original.json", "record failure");
        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        fake_fs_backend_fail_on(&filesystem, record_failures[index], 1U, EIO);
        storage_fs_ops_t operations = make_operations(&filesystem);
        storage_quarantine_list_t list = {0};
        TEST_CHECK_EQ_INT(APP_ERROR_IO, storage_quarantine_list_with_ops(&list, &operations));
        TEST_CHECK_EQ_U64(0U, list.count);
    }
}

/* A damaged committed entry is counted, not fatal: valid entries are still
 * returned alongside a nonzero damaged_count (FIX1 §8.3 rule 5). */
static void test_damaged_entries_are_counted(void) {
    /* Corrupt record: parses fail -> damaged. */
    reset_storage();
    char directory[APP_PATH_MAX_BYTES];
    char record[APP_PATH_MAX_BYTES];
    char evidence[APP_PATH_MAX_BYTES];
    make_committed_dir(directory, sizeof(directory), "00000000-0000-4000-8000-000000000400");
    make_committed_child(record, sizeof(record), "00000000-0000-4000-8000-000000000400",
                         "record.json");
    make_committed_child(evidence, sizeof(evidence), "00000000-0000-4000-8000-000000000400",
                         "evidence");
    create_directory(directory);
    write_text(record, "{not-json");
    write_text(evidence, "evidence");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    storage_quarantine_list_t list = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK_EQ_U64(1U, list.damaged_count);

    /* Missing record (evidence only) -> damaged. */
    reset_storage();
    create_directory(directory);
    write_text(evidence, "evidence");
    fake_fs_backend_reset(&filesystem);
    memset(&list, 0, sizeof(list));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK_EQ_U64(1U, list.damaged_count);

    /* Missing evidence (record only) -> damaged. */
    reset_storage();
    write_committed_entry("00000000-0000-4000-8000-000000000401",
                          STORAGE_DATA_MOUNT "/sets/original.json", "orphan record");
    char orphan_evidence[APP_PATH_MAX_BYTES];
    make_committed_child(orphan_evidence, sizeof(orphan_evidence),
                         "00000000-0000-4000-8000-000000000401", "evidence");
    TEST_CHECK_EQ_INT(0, unlink(orphan_evidence));
    fake_fs_backend_reset(&filesystem);
    memset(&list, 0, sizeof(list));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK_EQ_U64(1U, list.damaged_count);

    /* Unexpected non-UUID name -> damaged, but a valid sibling still lists. */
    reset_storage();
    write_committed_entry("00000000-0000-4000-8000-000000000402",
                          STORAGE_DATA_MOUNT "/sets/original.json", "healthy");
    write_text(STORAGE_DATA_MOUNT "/quarantine/unexpected.tmp", "x");
    fake_fs_backend_reset(&filesystem);
    memset(&list, 0, sizeof(list));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(1U, list.count);
    TEST_CHECK_EQ_U64(1U, list.damaged_count);
    TEST_CHECK_EQ_STRING("00000000-0000-4000-8000-000000000402", list.items[0].id.value);
}

static void test_entry_limit_is_counted_as_damage(void) {
    reset_storage();
    for (size_t value = 1U; value <= STORAGE_QUARANTINE_MAX_ENTRIES + 1U; ++value) {
        char id[APP_UUID_STRING_LENGTH + 1U];
        const int written = snprintf(id, sizeof(id), "10000000-0000-4000-8000-%012zu", value);
        TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
        write_committed_entry(id, STORAGE_DATA_MOUNT "/sets/original.json", "limit");
    }
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    storage_quarantine_list_t list = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_quarantine_list_with_ops(&list, &operations));
    TEST_CHECK_EQ_U64(STORAGE_QUARANTINE_MAX_ENTRIES, list.count);
    TEST_CHECK_EQ_U64(1U, list.damaged_count);
}

static void test_quarantine_enforces_operation_sequence(void) {
    reset_storage();
    char source[APP_PATH_MAX_BYTES];
    make_source_path(source, sizeof(source), "bad.json");
    write_text(source, "bad data");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {.failure = APP_ERROR_INTERNAL};
    storage_quarantine_entry_t entry = {0};

    /*
     * Strict fake enforcement (UNIT_TESTS1 L915) for the staged quarantine happy
     * path (FIX1 §8.2): stat the source, probe the committed and staging
     * directories for a free id, create the staging directory, move the source
     * into staged evidence, write the record durably (open/write/fsync/close),
     * fsync the evidence, validate the record read-back and evidence, then rename
     * the staging directory into quarantine. Directory fsyncs go through
     * storage_fs_sync_parent_path (raw POSIX) and are intentionally not logged
     * here. Strict mode aborts on any unexpected, missing, or out-of-order
     * operation, so dropping the evidence fsync or reordering the commit rename
     * would fail here.
     */
    static const char *const expected[] = {
        "fs_stat", "fs_stat",  "fs_stat",  "fs_mkdir", "fs_rename", "fs_open", "fs_write",
        "fs_sync", "fs_close", "fs_open",  "fs_sync",  "fs_close",  "fs_stat", "fs_open",
        "fs_read", "fs_read",  "fs_close", "fs_stat",  "fs_rename",
    };
    fake_call_log_set_strict(&filesystem.calls, true);
    for (size_t index = 0U; index < (sizeof(expected) / sizeof(expected[0])); ++index) {
        fake_call_log_expect(&filesystem.calls, expected[index]);
    }

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_quarantine_file_with_ops(source, "invalid metadata", &entry,
                                                       &operations, generate_uuid, &uuids));
    fake_call_log_verify(&filesystem.calls);
}

int main(void) {
    test_quarantine_enforces_operation_sequence();
    test_success_and_list();
    test_invalid_arguments_and_boundaries();
    test_staged_failures_preserve_source();
    test_unrestorable_rollback_preserves_evidence();
    test_uuid_failure_and_collision_handling();
    test_list_is_sorted_and_supports_short_reads();
    test_list_io_failures_are_visible();
    test_damaged_entries_are_counted();
    test_entry_limit_is_counted_as_damage();
    reset_storage();
    puts("storage quarantine tests passed");
    return EXIT_SUCCESS;
}
