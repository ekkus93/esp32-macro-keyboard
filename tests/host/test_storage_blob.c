#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"
#include "test_assert.h"
#include "test_temp_dir.h"

#define CAPTURE_MAX 8U

typedef struct {
    storage_blob_entry_t entries[CAPTURE_MAX];
    size_t entry_count;
    char invalid_names[CAPTURE_MAX][STORAGE_FS_ENTRY_NAME_MAX];
    size_t invalid_count;
} capture_t;

static app_error_code_t capture_entry(void *context, const storage_blob_entry_t *entry) {
    capture_t *capture = context;
    if (capture == NULL || entry == NULL || capture->entry_count >= CAPTURE_MAX) {
        return APP_ERROR_INTERNAL;
    }
    capture->entries[capture->entry_count] = *entry;
    ++capture->entry_count;
    return APP_ERROR_NONE;
}

static app_error_code_t capture_invalid_name(void *context, const char *name) {
    capture_t *capture = context;
    if (capture == NULL || name == NULL || capture->invalid_count >= CAPTURE_MAX) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(name);
    if (length >= sizeof(capture->invalid_names[0])) {
        return APP_ERROR_INTERNAL;
    }
    memcpy(capture->invalid_names[capture->invalid_count], name, length + 1U);
    ++capture->invalid_count;
    return APP_ERROR_NONE;
}

static void path_join(char *out_path, size_t path_size, const char *directory, const char *name) {
    const int written = snprintf(out_path, path_size, "%s/%s", directory, name);
    TEST_CHECK(written > 0 && (size_t)written < path_size);
}

static void create_file_with_size(const char *path, size_t size) {
    const int descriptor = open(path, O_CREAT | O_EXCL | O_WRONLY, (mode_t)0600);
    TEST_CHECK(descriptor >= 0);
    for (size_t index = 0U; index < size; ++index) {
        const unsigned char byte = (unsigned char)index;
        TEST_CHECK(write(descriptor, &byte, sizeof(byte)) == (ssize_t)sizeof(byte));
    }
    TEST_CHECK(close(descriptor) == 0);
}

static void test_filename_contract(void) {
    uint64_t id = 0U;
    TEST_CHECK(storage_blob_parse_filename("00000000000000000001.gz", &id));
    TEST_CHECK_EQ_U64(1U, id);
    TEST_CHECK(storage_blob_parse_filename("18446744073709551615.gz", &id));
    TEST_CHECK_EQ_U64(UINT64_MAX, id);
    TEST_CHECK(!storage_blob_parse_filename("00000000000000000000.gz", &id));
    TEST_CHECK(!storage_blob_parse_filename("18446744073709551616.gz", &id));
    TEST_CHECK(!storage_blob_parse_filename("0000000000000000001.gz", &id));
    TEST_CHECK(!storage_blob_parse_filename("00000000000000000001.json", &id));
    TEST_CHECK(!storage_blob_parse_filename("0000000000000000000x.gz", &id));
    TEST_CHECK(!storage_blob_parse_filename(NULL, &id));
    TEST_CHECK(!storage_blob_parse_filename("00000000000000000001.gz", NULL));

    char name[STORAGE_BLOB_FILENAME_CAPACITY];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_format_filename(1U, name, sizeof(name)));
    TEST_CHECK_EQ_STRING("00000000000000000001.gz", name);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_format_filename(UINT64_MAX, name, sizeof(name)));
    TEST_CHECK_EQ_STRING("18446744073709551615.gz", name);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_format_filename(0U, name, sizeof(name)));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_format_filename(1U, name, STORAGE_BLOB_FILENAME_LENGTH));
}

static void test_next_id_derivation(void) {
    uint64_t next_id = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_derive_next_id(0U, false, 0U, &next_id));
    TEST_CHECK_EQ_U64(1U, next_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_derive_next_id(17U, false, 0U, &next_id));
    TEST_CHECK_EQ_U64(17U, next_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_derive_next_id(3U, true, 9U, &next_id));
    TEST_CHECK_EQ_U64(10U, next_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_derive_next_id(20U, true, 9U, &next_id));
    TEST_CHECK_EQ_U64(20U, next_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_derive_next_id(0U, true, 42U, &next_id));
    TEST_CHECK_EQ_U64(43U, next_id);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL,
                         storage_blob_derive_next_id(0U, true, UINT64_MAX, &next_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_derive_next_id(0U, true, 0U, &next_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_derive_next_id(0U, false, 0U, NULL));
}

static void test_sort_newest_first(void) {
    storage_blob_entry_t entries[] = {
        {.id = 4U, .stored_bytes = 40U},
        {.id = 1U, .stored_bytes = 10U},
        {.id = 9U, .stored_bytes = 90U},
        {.id = 3U, .stored_bytes = 30U},
    };
    storage_blob_sort_newest_first(entries, sizeof(entries) / sizeof(entries[0]));
    TEST_CHECK_EQ_U64(9U, entries[0].id);
    TEST_CHECK_EQ_U64(4U, entries[1].id);
    TEST_CHECK_EQ_U64(3U, entries[2].id);
    TEST_CHECK_EQ_U64(1U, entries[3].id);
    storage_blob_sort_newest_first(NULL, 0U);
}

static void test_directory_prepare_and_scan(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char repository[TEST_TEMP_DIR_PATH_MAX];
    path_join(repository, sizeof(repository), directory.path, "repository");
    const storage_fs_ops_t *operations = storage_fs_ops_posix();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_prepare_directory_with_ops(operations, repository));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_prepare_directory_with_ops(operations, repository));

    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, "00000000000000000001.gz");
    create_file_with_size(path, 1U);
    path_join(path, sizeof(path), repository, "00000000000000000003.gz");
    create_file_with_size(path, 3U);
    path_join(path, sizeof(path), repository, "00000000000000000002.gz");
    TEST_CHECK(mkdir(path, (mode_t)0700) == 0);
    path_join(path, sizeof(path), repository, "00000000000000000004.gz.tmp");
    create_file_with_size(path, 4U);
    path_join(path, sizeof(path), repository, "notes.txt");
    create_file_with_size(path, 2U);

    capture_t capture = {0};
    const storage_blob_scan_observer_t observer = {
        .context = &capture,
        .visit_entry = capture_entry,
        .visit_invalid_name = capture_invalid_name,
    };
    storage_blob_scan_summary_t summary;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_scan_with_ops(operations, repository, 2U,
                                                                    &observer, &summary));
    TEST_CHECK_EQ_U64(2U, summary.valid_count);
    TEST_CHECK_EQ_U64(3U, summary.invalid_name_count);
    TEST_CHECK(summary.has_max_id);
    TEST_CHECK_EQ_U64(3U, summary.max_id);
    TEST_CHECK_EQ_U64(4U, summary.next_id);
    TEST_CHECK_EQ_U64(2U, capture.entry_count);
    TEST_CHECK_EQ_U64(3U, capture.invalid_count);

    TEST_CHECK_EQ_U64(3U, capture.entries[0].id);
    TEST_CHECK_EQ_U64(3U, capture.entries[0].stored_bytes);
    TEST_CHECK_EQ_U64(1U, capture.entries[1].id);
    TEST_CHECK_EQ_U64(1U, capture.entries[1].stored_bytes);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_scan_with_ops(operations, repository, 50U, NULL, &summary));
    TEST_CHECK_EQ_U64(50U, summary.next_id);
    test_temp_dir_remove(&directory);
}

static void test_prepare_rejects_non_directory(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), directory.path, "repository");
    create_file_with_size(path, 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_blob_prepare_directory_with_ops(storage_fs_ops_posix(), path));
    test_temp_dir_remove(&directory);
}

int main(void) {
    test_filename_contract();
    test_next_id_derivation();
    test_sort_newest_first();
    test_directory_prepare_and_scan();
    test_prepare_rejects_non_directory();
    puts("storage blob tests passed");
    return EXIT_SUCCESS;
}
