#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
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

static void path_join(char *out_path, size_t path_size, const char *directory, const char *name) {
    const int written = snprintf(out_path, path_size, "%s/%s", directory, name);
    TEST_CHECK(written > 0 && (size_t)written < path_size);
}

static void create_file(const char *path, const void *data, size_t length) {
    const int descriptor = open(path, O_CREAT | O_EXCL | O_WRONLY, (mode_t)0600);
    TEST_CHECK(descriptor >= 0);
    if (length > 0U) {
        TEST_CHECK(write(descriptor, data, length) == (ssize_t)length);
    }
    TEST_CHECK(close(descriptor) == 0);
}

static void make_repository(test_temp_dir_t *directory, char *repository, size_t repository_size) {
    test_temp_dir_create(directory);
    path_join(repository, repository_size, directory->path, "repository");
    TEST_CHECK(mkdir(repository, (mode_t)0700) == 0);
}

static ssize_t overreport_read(void *context, int descriptor, void *buffer, size_t length) {
    (void)context;
    (void)descriptor;
    (void)buffer;
    (void)length;
    return 2;
}

static void test_reader_preserves_exact_bytes(void) {
    test_temp_dir_t directory = {0};
    char repository[TEST_TEMP_DIR_PATH_MAX];
    make_repository(&directory, repository, sizeof(repository));

    static const unsigned char payload[] = {0x1fU, 0x8bU, 0x00U, 0xffU, 0x41U, 0x42U, 0x43U};
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, "00000000000000000007.gz");
    create_file(path, payload, sizeof(payload));

    storage_blob_reader_t reader = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_open_with_ops(
                                             storage_fs_ops_posix(), repository, 7U, &reader));
    TEST_CHECK(reader.active);
    TEST_CHECK_EQ_U64(sizeof(payload), reader.stored_bytes);

    unsigned char output[sizeof(payload)] = {0};
    size_t count = 0U;
    bool eof = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_reader_read_with_ops(&reader, output, 2U, &count, &eof));
    TEST_CHECK_EQ_U64(2U, count);
    TEST_CHECK(!eof);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_reader_read_with_ops(&reader, output + count,
                                                           sizeof(output) - count, &count, &eof));
    TEST_CHECK_EQ_U64(sizeof(payload) - 2U, count);
    TEST_CHECK(eof);
    TEST_CHECK(memcmp(output, payload, sizeof(payload)) == 0);

    count = 99U;
    eof = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_read_with_ops(
                                             &reader, output, sizeof(output), &count, &eof));
    TEST_CHECK_EQ_U64(0U, count);
    TEST_CHECK(eof);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_close_with_ops(&reader));
    TEST_CHECK(!reader.active);

    test_temp_dir_remove(&directory);
}

static void test_reader_rejects_overreported_count(void) {
    test_temp_dir_t directory = {0};
    char repository[TEST_TEMP_DIR_PATH_MAX];
    make_repository(&directory, repository, sizeof(repository));
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, "00000000000000000008.gz");
    create_file(path, "x", 1U);

    storage_fs_ops_t operations = *storage_fs_ops_posix();
    operations.read_file = overreport_read;
    storage_blob_reader_t reader = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_reader_open_with_ops(&operations, repository, 8U, &reader));

    unsigned char output = 0U;
    size_t count = 99U;
    bool eof = true;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_STORAGE_CORRUPT,
        storage_blob_reader_read_with_ops(&reader, &output, sizeof(output), &count, &eof));
    TEST_CHECK_EQ_U64(0U, count);
    TEST_CHECK(!eof);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_close_with_ops(&reader));
    test_temp_dir_remove(&directory);
}

static void test_empty_blob_streams_as_empty(void) {
    test_temp_dir_t directory = {0};
    char repository[TEST_TEMP_DIR_PATH_MAX];
    make_repository(&directory, repository, sizeof(repository));
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, "00000000000000000001.gz");
    create_file(path, NULL, 0U);

    storage_blob_reader_t reader = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_open_with_ops(
                                             storage_fs_ops_posix(), repository, 1U, &reader));
    unsigned char byte = 0U;
    size_t count = 1U;
    bool eof = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_read_with_ops(
                                             &reader, &byte, sizeof(byte), &count, &eof));
    TEST_CHECK_EQ_U64(0U, count);
    TEST_CHECK(eof);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_reader_close_with_ops(&reader));
    test_temp_dir_remove(&directory);
}

static void test_open_rejects_missing_and_nonregular(void) {
    test_temp_dir_t directory = {0};
    char repository[TEST_TEMP_DIR_PATH_MAX];
    make_repository(&directory, repository, sizeof(repository));

    storage_blob_reader_t reader = {.descriptor = 99, .active = true};
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_blob_reader_open_with_ops(
                                                  storage_fs_ops_posix(), repository, 4U, &reader));
    TEST_CHECK_EQ_INT(-1, reader.descriptor);
    TEST_CHECK(!reader.active);

    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, "00000000000000000004.gz");
    TEST_CHECK(mkdir(path, (mode_t)0700) == 0);
    TEST_CHECK_APP_ERROR(
        APP_ERROR_STORAGE_CORRUPT,
        storage_blob_reader_open_with_ops(storage_fs_ops_posix(), repository, 4U, &reader));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_reader_open_with_ops(storage_fs_ops_posix(), repository, 0U, &reader));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_reader_open_with_ops(NULL, repository, 1U, &reader));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_reader_open_with_ops(storage_fs_ops_posix(), repository, 1U, NULL));
    test_temp_dir_remove(&directory);
}

static void test_reader_argument_validation(void) {
    unsigned char byte = 0U;
    size_t count = 0U;
    bool eof = false;
    storage_blob_reader_t reader = {.descriptor = -1};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_blob_reader_read_with_ops(
                                                         NULL, &byte, sizeof(byte), &count, &eof));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_blob_reader_read_with_ops(&reader, &byte, sizeof(byte), &count, &eof));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_blob_reader_close_with_ops(&reader));
}

static void test_delete_removes_only_selected_final_blob(void) {
    test_temp_dir_t directory = {0};
    char repository[TEST_TEMP_DIR_PATH_MAX];
    make_repository(&directory, repository, sizeof(repository));

    char first[TEST_TEMP_DIR_PATH_MAX];
    char second[TEST_TEMP_DIR_PATH_MAX];
    char temporary[TEST_TEMP_DIR_PATH_MAX];
    path_join(first, sizeof(first), repository, "00000000000000000001.gz");
    path_join(second, sizeof(second), repository, "00000000000000000002.gz");
    path_join(temporary, sizeof(temporary), repository, "00000000000000000003.gz.tmp");
    create_file(first, "one", 3U);
    create_file(second, "two", 3U);
    create_file(temporary, "tmp", 3U);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_delete_with_ops(storage_fs_ops_posix(), repository, 2U));
    TEST_CHECK(access(first, F_OK) == 0);
    TEST_CHECK(access(second, F_OK) != 0);
    TEST_CHECK(access(temporary, F_OK) == 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_blob_delete_with_ops(storage_fs_ops_posix(), repository, 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_blob_delete_with_ops(storage_fs_ops_posix(), repository, 0U));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_delete_with_ops(storage_fs_ops_posix(), repository, 1U));
    TEST_CHECK(access(first, F_OK) != 0);
    TEST_CHECK(access(temporary, F_OK) == 0);
    test_temp_dir_remove(&directory);
}

static void test_delete_rejects_nonregular(void) {
    test_temp_dir_t directory = {0};
    char repository[TEST_TEMP_DIR_PATH_MAX];
    make_repository(&directory, repository, sizeof(repository));
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, "00000000000000000009.gz");
    TEST_CHECK(mkdir(path, (mode_t)0700) == 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_blob_delete_with_ops(storage_fs_ops_posix(), repository, 9U));
    TEST_CHECK(access(path, F_OK) == 0);
    test_temp_dir_remove(&directory);
}

int main(void) {
    test_reader_preserves_exact_bytes();
    test_reader_rejects_overreported_count();
    test_empty_blob_streams_as_empty();
    test_open_rejects_missing_and_nonregular();
    test_reader_argument_validation();
    test_delete_removes_only_selected_final_blob();
    test_delete_rejects_nonregular();
    puts("storage blob access tests passed");
    return EXIT_SUCCESS;
}
