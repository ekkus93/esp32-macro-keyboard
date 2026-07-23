#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void test_fail(const char *file,
               int line,
               const char *expression,
               const char *message) __attribute__((noreturn));
void test_fail_u64(const char *file,
                   int line,
                   const char *expression,
                   uint64_t expected,
                   uint64_t actual) __attribute__((noreturn));
void test_fail_string(const char *file,
                      int line,
                      const char *expression,
                      const char *expected,
                      const char *actual) __attribute__((noreturn));
void test_fail_buffer(const char *file,
                      int line,
                      const char *expression,
                      const void *expected,
                      const void *actual,
                      size_t length) __attribute__((noreturn));

#define TEST_CHECK(expression)                                                         \
    do {                                                                               \
        if (!(expression)) {                                                           \
            test_fail(__FILE__, __LINE__, #expression, "condition is false");         \
        }                                                                              \
    } while (0)

#define TEST_CHECK_EQ_U64(expected_value, actual_value)                                \
    do {                                                                               \
        const uint64_t test_expected_ = (uint64_t)(expected_value);                    \
        const uint64_t test_actual_ = (uint64_t)(actual_value);                        \
        if (test_expected_ != test_actual_) {                                          \
            test_fail_u64(__FILE__,                                                    \
                          __LINE__,                                                    \
                          #actual_value " == " #expected_value,                       \
                          test_expected_,                                              \
                          test_actual_);                                               \
        }                                                                              \
    } while (0)

#define TEST_CHECK_EQ_INT(expected_value, actual_value)                                \
    TEST_CHECK_EQ_U64((uint64_t)(int64_t)(expected_value),                             \
                      (uint64_t)(int64_t)(actual_value))

#define TEST_CHECK_EQ_PTR(expected_value, actual_value)                                \
    TEST_CHECK_EQ_U64((uintptr_t)(expected_value), (uintptr_t)(actual_value))

#define TEST_CHECK_EQ_STRING(expected_value, actual_value)                             \
    do {                                                                               \
        const char *const test_expected_ = (expected_value);                           \
        const char *const test_actual_ = (actual_value);                               \
        if (test_expected_ == NULL || test_actual_ == NULL ||                          \
            strcmp(test_expected_, test_actual_) != 0) {                               \
            test_fail_string(__FILE__,                                                 \
                             __LINE__,                                                 \
                             #actual_value " == " #expected_value,                    \
                             test_expected_,                                           \
                             test_actual_);                                            \
        }                                                                              \
    } while (0)

#define TEST_CHECK_EQ_BUFFER(expected_value, actual_value, length_value)                \
    do {                                                                               \
        const size_t test_length_ = (length_value);                                    \
        const void *const test_expected_ = (expected_value);                           \
        const void *const test_actual_ = (actual_value);                               \
        if (test_expected_ == NULL || test_actual_ == NULL ||                          \
            memcmp(test_expected_, test_actual_, test_length_) != 0) {                 \
            test_fail_buffer(__FILE__,                                                 \
                             __LINE__,                                                 \
                             #actual_value " == " #expected_value,                    \
                             test_expected_,                                           \
                             test_actual_,                                             \
                             test_length_);                                            \
        }                                                                              \
    } while (0)

#define TEST_CHECK_APP_ERROR(expected_value, actual_value)                             \
    TEST_CHECK_EQ_INT((expected_value), (actual_value))

#define TEST_CHECK_EQ_UUID(expected_pointer, actual_pointer)                           \
    do {                                                                               \
        const void *const test_expected_pointer_ = (expected_pointer);                 \
        const void *const test_actual_pointer_ = (actual_pointer);                     \
        TEST_CHECK(test_expected_pointer_ != NULL);                                    \
        TEST_CHECK(test_actual_pointer_ != NULL);                                      \
        TEST_CHECK_EQ_STRING((expected_pointer)->value, (actual_pointer)->value);       \
    } while (0)

#endif
