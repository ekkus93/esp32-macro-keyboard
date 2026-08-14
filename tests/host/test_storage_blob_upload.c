#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_blob_internal.h"
#include "storage_blob_upload_internal.h"
#include "test_assert.h"

static storage_blob_scan_summary_t fake_inventory_state;
static size_t fake_inventory_used_bytes;

storage_blob_scan_summary_t storage_blob_scan_state(void) {
    return fake_inventory_state;
}

void storage_blob_record_committed_entry(const storage_blob_entry_t *entry) {
    if (entry == NULL || entry->id == 0U || entry->id == UINT64_MAX) {
        return;
    }
    ++fake_inventory_state.valid_count;
    fake_inventory_used_bytes += entry->stored_bytes;
    if (!fake_inventory_state.has_max_id || entry->id > fake_inventory_state.max_id) {
        fake_inventory_state.has_max_id = true;
        fake_inventory_state.max_id = entry->id;
    }
    fake_inventory_state.next_id = entry->id + UINT64_C(1);
}

#include "../../firmware/components/storage/storage_blob_upload.c"

typedef enum {
    UPLOAD_OPERATION_STAT = 0,
    UPLOAD_OPERATION_OPEN,
    UPLOAD_OPERATION_WRITE,
    UPLOAD_OPERATION_FLUSH,
    UPLOAD_OPERATION_SYNC,
    UPLOAD_OPERATION_CLOSE,
    UPLOAD_OPERATION_PERSIST,
    UPLOAD_OPERATION_RENAME,
    UPLOAD_OPERATION_SYNC_PARENT,
    UPLOAD_OPERATION_UNLINK,
    UPLOAD_OPERATION_COUNT
} upload_operation_t;

typedef struct {
    upload_operation_t calls[64U];
    size_t call_count;
    size_t operation_counts[UPLOAD_OPERATION_COUNT];
    upload_operation_t fail_operation;
    size_t fail_on_occurrence;
    int fail_errno;
    app_error_code_t persist_error;
    bool short_write;
    bool final_exists;
    bool temporary_exists;
    bool stream_open;
    uint8_t payload[64U];
    size_t payload_length;
    uint64_t persisted_next_id;
    char opened_path[STORAGE_BLOB_UPLOAD_PATH_CAPACITY];
    char renamed_source[STORAGE_BLOB_UPLOAD_PATH_CAPACITY];
    char renamed_destination[STORAGE_BLOB_UPLOAD_PATH_CAPACITY];
} upload_fake_t;

static void fake_reset(upload_fake_t *fake) {
    *fake = (upload_fake_t){
        .fail_operation = UPLOAD_OPERATION_COUNT,
        .persist_error = APP_ERROR_NONE,
    };
}

static void fake_inventory_reset(void) {
    fake_inventory_state = (storage_blob_scan_summary_t){0};
    fake_inventory_used_bytes = 0U;
}

static bool record_operation(upload_fake_t *fake, upload_operation_t operation) {
    TEST_CHECK(fake != NULL);
    TEST_CHECK(operation < UPLOAD_OPERATION_COUNT);
    TEST_CHECK(fake->call_count < sizeof(fake->calls) / sizeof(fake->calls[0]));
    fake->calls[fake->call_count++] = operation;
    ++fake->operation_counts[operation];
    if (fake->fail_operation != operation) {
        return false;
    }
    return fake->operation_counts[operation] == fake->fail_on_occurrence;
}

static bool path_is_temporary(const char *path) {
    const size_t length = strlen(path);
    return length >= 4U && strcmp(path + length - 4U, ".tmp") == 0;
}

static void copy_path(char *destination, size_t capacity, const char *source) {
    const size_t length = strlen(source);
    TEST_CHECK(length < capacity);
    memcpy(destination, source, length + 1U);
}

static void *fake_open_temporary(void *context, const char *path) {
    upload_fake_t *fake = context;
    if (record_operation(fake, UPLOAD_OPERATION_OPEN)) {
        errno = fake->fail_errno;
        return NULL;
    }
    TEST_CHECK(path_is_temporary(path));
    TEST_CHECK(!fake->temporary_exists);
    copy_path(fake->opened_path, sizeof(fake->opened_path), path);
    fake->temporary_exists = true;
    fake->stream_open = true;
    return fake;
}

static size_t fake_write_stream(void *context, void *stream, const void *data, size_t length) {
    upload_fake_t *fake = context;
    TEST_CHECK(stream == fake);
    TEST_CHECK(fake->stream_open);
    if (record_operation(fake, UPLOAD_OPERATION_WRITE)) {
        errno = fake->fail_errno;
        return 0U;
    }
    if (fake->short_write) {
        errno = fake->fail_errno;
        return length > 0U ? length - 1U : 0U;
    }
    TEST_CHECK(fake->payload_length + length <= sizeof(fake->payload));
    memcpy(fake->payload + fake->payload_length, data, length);
    fake->payload_length += length;
    return length;
}

static int fake_flush_stream(void *context, void *stream) {
    upload_fake_t *fake = context;
    TEST_CHECK(stream == fake);
    TEST_CHECK(fake->stream_open);
    if (record_operation(fake, UPLOAD_OPERATION_FLUSH)) {
        errno = fake->fail_errno;
        return -1;
    }
    return 0;
}

static int fake_sync_stream(void *context, void *stream) {
    upload_fake_t *fake = context;
    TEST_CHECK(stream == fake);
    TEST_CHECK(fake->stream_open);
    if (record_operation(fake, UPLOAD_OPERATION_SYNC)) {
        errno = fake->fail_errno;
        return -1;
    }
    return 0;
}

static int fake_close_stream(void *context, void *stream) {
    upload_fake_t *fake = context;
    TEST_CHECK(stream == fake);
    TEST_CHECK(fake->stream_open);
    const bool fail = record_operation(fake, UPLOAD_OPERATION_CLOSE);
    fake->stream_open = false;
    if (fail) {
        errno = fake->fail_errno;
        return -1;
    }
    return 0;
}

static int fake_stat_path(void *context, const char *path, struct stat *metadata) {
    upload_fake_t *fake = context;
    if (record_operation(fake, UPLOAD_OPERATION_STAT)) {
        errno = fake->fail_errno;
        return -1;
    }
    const bool exists = path_is_temporary(path) ? fake->temporary_exists : fake->final_exists;
    if (!exists) {
        errno = ENOENT;
        return -1;
    }
    memset(metadata, 0, sizeof(*metadata));
    return 0;
}

static int fake_rename_path(void *context, const char *source, const char *destination) {
    upload_fake_t *fake = context;
    if (record_operation(fake, UPLOAD_OPERATION_RENAME)) {
        errno = fake->fail_errno;
        return -1;
    }
    TEST_CHECK(fake->temporary_exists);
    TEST_CHECK(!fake->final_exists);
    copy_path(fake->renamed_source, sizeof(fake->renamed_source), source);
    copy_path(fake->renamed_destination, sizeof(fake->renamed_destination), destination);
    fake->temporary_exists = false;
    fake->final_exists = true;
    return 0;
}

static int fake_unlink_path(void *context, const char *path) {
    upload_fake_t *fake = context;
    if (record_operation(fake, UPLOAD_OPERATION_UNLINK)) {
        errno = fake->fail_errno;
        return -1;
    }
    bool *exists = path_is_temporary(path) ? &fake->temporary_exists : &fake->final_exists;
    if (!*exists) {
        errno = ENOENT;
        return -1;
    }
    *exists = false;
    return 0;
}

static int fake_sync_parent(void *context, const char *path) {
    upload_fake_t *fake = context;
    TEST_CHECK(!path_is_temporary(path));
    if (record_operation(fake, UPLOAD_OPERATION_SYNC_PARENT)) {
        errno = fake->fail_errno;
        return -1;
    }
    return 0;
}

static app_error_code_t fake_persist_next_id(void *context, uint64_t next_id) {
    upload_fake_t *fake = context;
    (void)record_operation(fake, UPLOAD_OPERATION_PERSIST);
    if (fake->persist_error != APP_ERROR_NONE) {
        return fake->persist_error;
    }
    fake->persisted_next_id = next_id;
    return APP_ERROR_NONE;
}

static storage_blob_upload_ops_t make_operations(upload_fake_t *fake) {
    return (storage_blob_upload_ops_t){
        .context = fake,
        .open_temporary = fake_open_temporary,
        .write_stream = fake_write_stream,
        .flush_stream = fake_flush_stream,
        .sync_stream = fake_sync_stream,
        .close_stream = fake_close_stream,
        .stat_path = fake_stat_path,
        .rename_path = fake_rename_path,
        .unlink_path = fake_unlink_path,
        .sync_parent = fake_sync_parent,
        .persist_next_id = fake_persist_next_id,
    };
}

static void expect_call_sequence(const upload_fake_t *fake, const upload_operation_t *expected,
                                 size_t expected_count) {
    TEST_CHECK_EQ_U64(expected_count, fake->call_count);
    for (size_t index = 0U; index < expected_count; ++index) {
        TEST_CHECK_EQ_INT(expected[index], fake->calls[index]);
    }
}

static storage_blob_upload_t begin_upload(upload_fake_t *fake,
                                          const storage_blob_upload_ops_t *operations) {
    storage_blob_upload_t upload = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_upload_begin_with_ops("/repository", UINT64_C(7), 6U, 8U,
                                                            operations, &upload));
    TEST_CHECK(upload.active);
    TEST_CHECK(!upload.final_path_owned);
    TEST_CHECK(!upload.committed);
    TEST_CHECK(fake->temporary_exists);
    TEST_CHECK_EQ_STRING("/repository/00000000000000000007.gz.tmp", upload.temporary_path);
    TEST_CHECK_EQ_STRING("/repository/00000000000000000007.gz", upload.final_path);
    return upload;
}

static void test_successful_upload_sequence(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    const storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = begin_upload(&fake, &operations);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write_with_ops(&upload, "ab", 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write_with_ops(&upload, "cdef", 4U));
    storage_blob_entry_t entry = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_commit_with_ops(&upload, &entry));

    TEST_CHECK(upload.committed);
    TEST_CHECK(upload.final_path_owned);
    TEST_CHECK(!upload.active);
    TEST_CHECK(fake.final_exists);
    TEST_CHECK(!fake.temporary_exists);
    TEST_CHECK_EQ_U64(7U, entry.id);
    TEST_CHECK_EQ_U64(6U, entry.stored_bytes);
    TEST_CHECK_EQ_U64(8U, fake.persisted_next_id);
    TEST_CHECK_EQ_U64(6U, fake.payload_length);
    TEST_CHECK(memcmp(fake.payload, "abcdef", 6U) == 0);
    TEST_CHECK_EQ_STRING(upload.temporary_path, fake.renamed_source);
    TEST_CHECK_EQ_STRING(upload.final_path, fake.renamed_destination);

    static const upload_operation_t expected[] = {
        UPLOAD_OPERATION_STAT,    UPLOAD_OPERATION_STAT,   UPLOAD_OPERATION_OPEN,
        UPLOAD_OPERATION_WRITE,   UPLOAD_OPERATION_WRITE,  UPLOAD_OPERATION_FLUSH,
        UPLOAD_OPERATION_SYNC,    UPLOAD_OPERATION_CLOSE,  UPLOAD_OPERATION_STAT,
        UPLOAD_OPERATION_PERSIST, UPLOAD_OPERATION_RENAME, UPLOAD_OPERATION_SYNC_PARENT,
    };
    expect_call_sequence(&fake, expected, sizeof(expected) / sizeof(expected[0]));
}

static void test_argument_and_size_validation(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_blob_upload_begin_with_ops(
                                                         NULL, 1U, 1U, 1U, &operations, &upload));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_upload_begin_with_ops("/repository", 0U, 1U, 1U, &operations, &upload));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_upload_begin_with_ops("/repository", 1U, 0U, 1U, &operations, &upload));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_upload_begin_with_ops("/repository", 1U, 2U, 1U, &operations, &upload));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL,
                         storage_blob_upload_begin_with_ops("/repository", UINT64_MAX, 1U, 1U,
                                                            &operations, &upload));

    operations.write_stream = NULL;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_upload_begin_with_ops("/repository", 1U, 1U, 1U, &operations, &upload));

    fake_reset(&fake);
    operations = make_operations(&fake);
    upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_upload_write_with_ops(&upload, "abcdefg", 7U));
    storage_blob_entry_t entry = {.id = 99U, .stored_bytes = 99U};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_upload_commit_with_ops(&upload, &entry));
    TEST_CHECK_EQ_U64(0U, entry.id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_abort_with_ops(&upload));
    TEST_CHECK(!fake.temporary_exists);
}

static void test_existing_paths_are_not_replaced(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    fake.final_exists = true;
    storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_blob_upload_begin_with_ops(
                                                 "/repository", 7U, 1U, 8U, &operations, &upload));
    TEST_CHECK(fake.final_exists);
    TEST_CHECK_EQ_U64(0U, fake.operation_counts[UPLOAD_OPERATION_OPEN]);

    fake_reset(&fake);
    fake.temporary_exists = true;
    operations = make_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_blob_upload_begin_with_ops(
                                                 "/repository", 7U, 1U, 8U, &operations, &upload));
    TEST_CHECK(fake.temporary_exists);
    TEST_CHECK(!fake.final_exists);
    TEST_CHECK_EQ_U64(0U, fake.operation_counts[UPLOAD_OPERATION_OPEN]);
}

static void test_write_failures_abort_cleanly(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    fake.short_write = true;
    fake.fail_errno = EIO;
    const storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_blob_upload_write_with_ops(&upload, "abcdef", 6U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_abort_with_ops(&upload));
    TEST_CHECK(!fake.final_exists);
    TEST_CHECK(!fake.temporary_exists);

    fake_reset(&fake);
    fake.fail_operation = UPLOAD_OPERATION_WRITE;
    fake.fail_on_occurrence = 1U;
    fake.fail_errno = ENOSPC;
    const storage_blob_upload_ops_t full_operations = make_operations(&fake);
    upload = begin_upload(&fake, &full_operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL,
                         storage_blob_upload_write_with_ops(&upload, "abcdef", 6U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_abort_with_ops(&upload));
    TEST_CHECK(!fake.final_exists);
    TEST_CHECK(!fake.temporary_exists);
}

static void run_precommit_failure(upload_operation_t operation, size_t occurrence, int error_number,
                                  app_error_code_t persist_error, app_error_code_t expected_error) {
    upload_fake_t fake;
    fake_reset(&fake);
    fake.fail_operation = operation;
    fake.fail_on_occurrence = occurrence;
    fake.fail_errno = error_number;
    fake.persist_error = persist_error;
    const storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write_with_ops(&upload, "abcdef", 6U));
    storage_blob_entry_t entry = {.id = 99U, .stored_bytes = 99U};
    TEST_CHECK_APP_ERROR(expected_error, storage_blob_upload_commit_with_ops(&upload, &entry));
    TEST_CHECK_EQ_U64(0U, entry.id);
    TEST_CHECK(!upload.final_path_owned);
    TEST_CHECK(!upload.committed);
    TEST_CHECK(!fake.final_exists);
    TEST_CHECK(!fake.temporary_exists);
}

static void test_precommit_failures_leave_final_absent(void) {
    run_precommit_failure(UPLOAD_OPERATION_FLUSH, 1U, EIO, APP_ERROR_NONE, APP_ERROR_IO);
    run_precommit_failure(UPLOAD_OPERATION_SYNC, 1U, EIO, APP_ERROR_NONE, APP_ERROR_IO);
    run_precommit_failure(UPLOAD_OPERATION_CLOSE, 1U, EIO, APP_ERROR_NONE, APP_ERROR_IO);
    run_precommit_failure(UPLOAD_OPERATION_STAT, 3U, EIO, APP_ERROR_NONE, APP_ERROR_IO);
    run_precommit_failure(UPLOAD_OPERATION_COUNT, 0U, 0, APP_ERROR_STORAGE_UNAVAILABLE,
                          APP_ERROR_STORAGE_UNAVAILABLE);
    run_precommit_failure(UPLOAD_OPERATION_RENAME, 1U, EIO, APP_ERROR_NONE, APP_ERROR_IO);
}

static void test_directory_sync_failure_is_uncertain_and_retained(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    fake.fail_operation = UPLOAD_OPERATION_SYNC_PARENT;
    fake.fail_on_occurrence = 1U;
    fake.fail_errno = EIO;
    const storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write_with_ops(&upload, "abcdef", 6U));
    storage_blob_entry_t entry = {.id = 99U, .stored_bytes = 99U};
    const app_operation_result_t result =
        storage_blob_upload_commit_with_ops_result(&upload, &entry);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, result.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result.cleanup_error);
    TEST_CHECK(!result.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_COMMIT_UNCERTAIN, result.commit_state);
    TEST_CHECK(!upload.committed);
    TEST_CHECK(upload.final_path_owned);
    TEST_CHECK(fake.final_exists);
    TEST_CHECK(!fake.temporary_exists);
    TEST_CHECK_EQ_U64(8U, fake.persisted_next_id);
    TEST_CHECK_EQ_U64(0U, entry.id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_abort_with_ops(&upload));
    TEST_CHECK(upload.final_path_owned);
    TEST_CHECK(fake.final_exists);
    TEST_CHECK_EQ_U64(0U, fake.operation_counts[UPLOAD_OPERATION_UNLINK]);
    TEST_CHECK_EQ_U64(1U, fake.operation_counts[UPLOAD_OPERATION_SYNC_PARENT]);
}

static void test_public_wrapper_records_only_durable_commit(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    fake_inventory_reset();
    storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write(&upload, "abcdef", 6U));
    storage_blob_entry_t entry = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_commit(&upload, &entry));
    TEST_CHECK_EQ_U64(1U, storage_blob_scan_state().valid_count);
    TEST_CHECK_EQ_U64(6U, fake_inventory_used_bytes);

    fake_reset(&fake);
    fake_inventory_reset();
    fake.fail_operation = UPLOAD_OPERATION_SYNC_PARENT;
    fake.fail_on_occurrence = 1U;
    fake.fail_errno = EIO;
    operations = make_operations(&fake);
    upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write(&upload, "abcdef", 6U));
    entry = (storage_blob_entry_t){.id = 99U, .stored_bytes = 99U};
    TEST_CHECK_APP_ERROR(APP_ERROR_COMMIT_UNCERTAIN, storage_blob_upload_commit(&upload, &entry));
    TEST_CHECK_EQ_U64(0U, entry.id);
    TEST_CHECK_EQ_U64(0U, storage_blob_scan_state().valid_count);
    TEST_CHECK_EQ_U64(0U, fake_inventory_used_bytes);
    TEST_CHECK(!upload.committed);
    TEST_CHECK(upload.final_path_owned);
    TEST_CHECK(fake.final_exists);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_abort(&upload));
    TEST_CHECK(fake.final_exists);
    TEST_CHECK(upload.final_path_owned);
}

static void test_cleanup_failure_preserves_primary_and_cleanup(void) {
    upload_fake_t fake;
    fake_reset(&fake);
    fake.fail_operation = UPLOAD_OPERATION_UNLINK;
    fake.fail_on_occurrence = 1U;
    fake.fail_errno = ENOSPC;
    fake.persist_error = APP_ERROR_STORAGE_UNAVAILABLE;
    const storage_blob_upload_ops_t operations = make_operations(&fake);
    storage_blob_upload_t upload = begin_upload(&fake, &operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_upload_write_with_ops(&upload, "abcdef", 6U));
    storage_blob_entry_t entry = {0};
    const app_operation_result_t result =
        storage_blob_upload_commit_with_ops_result(&upload, &entry);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, result.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, result.cleanup_error);
    TEST_CHECK(result.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_NOT_COMMITTED, result.commit_state);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, app_operation_result_error(result));
    TEST_CHECK(fake.temporary_exists);
    TEST_CHECK(!fake.final_exists);
}

int main(void) {
    test_successful_upload_sequence();
    test_argument_and_size_validation();
    test_existing_paths_are_not_replaced();
    test_write_failures_abort_cleanly();
    test_precommit_failures_leave_final_absent();
    test_directory_sync_failure_is_uncertain_and_retained();
    test_public_wrapper_records_only_durable_commit();
    test_cleanup_failure_preserves_primary_and_cleanup();
    puts("storage blob upload tests passed");
    return EXIT_SUCCESS;
}
