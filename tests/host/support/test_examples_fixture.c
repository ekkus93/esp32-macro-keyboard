#include "test_examples_fixture.h"

#include <stdio.h>
#include <stdlib.h>

#include "test_assert.h"

#ifndef EXAMPLES_JSON_PATH
#error "EXAMPLES_JSON_PATH must be defined by the build"
#endif

static cJSON *g_root;
static _Bool g_loaded;

static char *read_whole_file(const char *path) {
    FILE *file = fopen(path, "rb");
    TEST_CHECK(file != NULL);
    TEST_CHECK(fseek(file, 0, SEEK_END) == 0);
    const long length = ftell(file);
    TEST_CHECK(length >= 0);
    TEST_CHECK(fseek(file, 0, SEEK_SET) == 0);
    char *buffer = malloc((size_t)length + 1U);
    TEST_CHECK(buffer != NULL);
    TEST_CHECK(fread(buffer, 1U, (size_t)length, file) == (size_t)length);
    buffer[length] = '\0';
    TEST_CHECK(fclose(file) == 0);
    return buffer;
}

const cJSON *test_examples_fixture_get(const char *key) {
    if (!g_loaded) {
        char *text = read_whole_file(EXAMPLES_JSON_PATH);
        g_root = cJSON_Parse(text);
        free(text);
        TEST_CHECK(g_root != NULL);
        g_loaded = 1;
    }
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(g_root, key);
    TEST_CHECK(value != NULL);
    return value;
}
