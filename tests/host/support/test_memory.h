#ifndef TEST_MEMORY_H
#define TEST_MEMORY_H

#include <stddef.h>

void test_memory_reset(void);
void *test_malloc(size_t size);
void *test_calloc(size_t count, size_t size);
void *test_realloc(void *pointer, size_t size);
void test_free(void *pointer);
size_t test_memory_outstanding_allocations(void);
size_t test_memory_outstanding_bytes(void);

#endif
