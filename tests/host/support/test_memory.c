#include "test_memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TEST_MEMORY_MAGIC UINT64_C(0x4d4b544553544d45)

typedef struct {
    uint64_t magic;
    size_t size;
} test_memory_header_t;

static size_t outstanding_allocations;
static size_t outstanding_bytes;

void test_memory_reset(void)
{
    if (outstanding_allocations != 0U || outstanding_bytes != 0U) {
        abort();
    }
}

static void *allocate_zeroed(size_t size)
{
    if (size > SIZE_MAX - sizeof(test_memory_header_t)) {
        return NULL;
    }
    test_memory_header_t *header = calloc(1U, sizeof(*header) + size);
    if (header == NULL) {
        return NULL;
    }
    header->magic = TEST_MEMORY_MAGIC;
    header->size = size;
    ++outstanding_allocations;
    outstanding_bytes += size;
    return header + 1;
}

void *test_malloc(size_t size)
{
    return allocate_zeroed(size);
}

void *test_calloc(size_t count, size_t size)
{
    if (count != 0U && size > SIZE_MAX / count) {
        return NULL;
    }
    return allocate_zeroed(count * size);
}

void *test_realloc(void *pointer, size_t size)
{
    if (pointer == NULL) {
        return test_malloc(size);
    }
    if (size == 0U) {
        test_free(pointer);
        return NULL;
    }
    test_memory_header_t *header = ((test_memory_header_t *)pointer) - 1;
    if (header->magic != TEST_MEMORY_MAGIC) {
        abort();
    }
    const size_t previous_size = header->size;
    void *replacement = test_malloc(size);
    if (replacement == NULL) {
        return NULL;
    }
    const size_t copy_size = previous_size < size ? previous_size : size;
    memcpy(replacement, pointer, copy_size);
    test_free(pointer);
    return replacement;
}

void test_free(void *pointer)
{
    if (pointer == NULL) {
        return;
    }
    test_memory_header_t *header = ((test_memory_header_t *)pointer) - 1;
    if (header->magic != TEST_MEMORY_MAGIC || outstanding_allocations == 0U ||
        outstanding_bytes < header->size) {
        abort();
    }
    header->magic = 0U;
    --outstanding_allocations;
    outstanding_bytes -= header->size;
    free(header);
}

size_t test_memory_outstanding_allocations(void)
{
    return outstanding_allocations;
}

size_t test_memory_outstanding_bytes(void)
{
    return outstanding_bytes;
}
