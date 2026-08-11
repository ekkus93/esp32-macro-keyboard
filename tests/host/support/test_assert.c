#include "test_assert.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static const char *safe_string(const char *value) {
    return value == NULL ? "<null>" : value;
}

void test_fail(const char *file, int line, const char *expression, const char *message) {
    (void)fprintf(stderr, "test failure at %s:%d: %s (%s)\n", safe_string(file), line,
                  safe_string(expression), safe_string(message));
    exit(EXIT_FAILURE);
}

void test_fail_u64(const char *file, int line, const char *expression, uint64_t expected,
                   uint64_t actual) {
    (void)expected;
    (void)actual;
    (void)fprintf(stderr, "test failure at %s:%d: %s; integer values differ\n", safe_string(file),
                  line, safe_string(expression));
    exit(EXIT_FAILURE);
}

void test_fail_string(const char *file, int line, const char *expression, const char *expected,
                      const char *actual) {
    (void)expected;
    (void)actual;
    (void)fprintf(stderr, "test failure at %s:%d: %s; string values differ\n", safe_string(file),
                  line, safe_string(expression));
    exit(EXIT_FAILURE);
}

void test_fail_buffer(const char *file, int line, const char *expression, const void *expected,
                      const void *actual, size_t length) {
    (void)expected;
    (void)actual;
    (void)fprintf(stderr, "test failure at %s:%d: %s; buffers differ across %zu byte(s)\n",
                  safe_string(file), line, safe_string(expression), length);
    exit(EXIT_FAILURE);
}
