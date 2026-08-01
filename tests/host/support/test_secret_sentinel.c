#include "test_secret_sentinel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_assert.h"
#include "test_temp_dir.h"

#ifndef TEST_SECRET_SENTINEL_SCANNER_PATH
#error "TEST_SECRET_SENTINEL_SCANNER_PATH must be defined by the build"
#endif

#define TEST_SECRET_SENTINEL_COMMAND_MAX_BYTES 8192U

static void write_file_or_fail(const char *path, const char *content) {
    FILE *file = fopen(path, "wb");
    TEST_CHECK(file != NULL);
    const size_t length = strlen(content);
    TEST_CHECK(fwrite(content, 1U, length, file) == length);
    TEST_CHECK(fclose(file) == 0);
}

void test_assert_no_secret_sentinel(const char *secret, const char *const *outputs,
                                    size_t output_count) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);

    char secret_path[TEST_TEMP_DIR_PATH_MAX];
    int written = snprintf(secret_path, sizeof(secret_path), "%s/secret.txt", directory.path);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(secret_path));
    write_file_or_fail(secret_path, secret);

    char command[TEST_SECRET_SENTINEL_COMMAND_MAX_BYTES];
    written = snprintf(command, sizeof(command), "python3 '%s' --secret-file '%s'",
                       TEST_SECRET_SENTINEL_SCANNER_PATH, secret_path);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(command));

    for (size_t index = 0U; index < output_count; ++index) {
        char output_path[TEST_TEMP_DIR_PATH_MAX];
        written =
            snprintf(output_path, sizeof(output_path), "%s/output-%zu.txt", directory.path, index);
        TEST_CHECK(written > 0 && (size_t)written < sizeof(output_path));
        write_file_or_fail(output_path, outputs[index]);

        const size_t offset = strlen(command);
        written = snprintf(command + offset, sizeof(command) - offset, " '%s'", output_path);
        TEST_CHECK(written > 0 && offset + (size_t)written < sizeof(command));
    }

    TEST_CHECK_EQ_INT(0, system(command));

    test_temp_dir_remove(&directory);
}
