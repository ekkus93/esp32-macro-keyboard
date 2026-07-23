#include "test_temp_dir.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "test_assert.h"

static void remove_tree(const char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        TEST_CHECK(errno == ENOENT);
        return;
    }

    while (true) {
        errno = 0;
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            TEST_CHECK(errno == 0);
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char child[TEST_TEMP_DIR_PATH_MAX];
        const int written = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        TEST_CHECK(written > 0 && (size_t)written < sizeof(child));
        struct stat metadata;
        TEST_CHECK(lstat(child, &metadata) == 0);
        if (S_ISDIR(metadata.st_mode)) {
            remove_tree(child);
        } else {
            TEST_CHECK(unlink(child) == 0);
        }
    }
    TEST_CHECK(closedir(directory) == 0);
    TEST_CHECK(rmdir(path) == 0);
}

void test_temp_dir_create(test_temp_dir_t *directory)
{
    TEST_CHECK(directory != NULL);
    TEST_CHECK(directory->active == 0);
    static const char pattern[] = "/tmp/esp32-macro-keyboard-test-XXXXXX";
    TEST_CHECK(sizeof(pattern) <= sizeof(directory->path));
    memcpy(directory->path, pattern, sizeof(pattern));
    TEST_CHECK(mkdtemp(directory->path) != NULL);
    directory->active = 1;
}

void test_temp_dir_remove(test_temp_dir_t *directory)
{
    TEST_CHECK(directory != NULL);
    if (directory->active == 0) {
        return;
    }
    remove_tree(directory->path);
    directory->path[0] = '\0';
    directory->active = 0;
}

void test_temp_dir_remove_path(const char *path)
{
    TEST_CHECK(path != NULL);
    TEST_CHECK(path[0] != '\0');
    remove_tree(path);
}
