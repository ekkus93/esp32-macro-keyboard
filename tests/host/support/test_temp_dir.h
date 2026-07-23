#ifndef TEST_TEMP_DIR_H
#define TEST_TEMP_DIR_H

#include <stddef.h>

#define TEST_TEMP_DIR_PATH_MAX 4096U

typedef struct {
    char path[TEST_TEMP_DIR_PATH_MAX];
    int active;
} test_temp_dir_t;

void test_temp_dir_create(test_temp_dir_t *directory);
void test_temp_dir_remove(test_temp_dir_t *directory);

#endif
