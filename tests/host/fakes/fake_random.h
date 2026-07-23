#ifndef FAKE_RANDOM_H
#define FAKE_RANDOM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fake_call_log.h"

#define FAKE_RANDOM_CAPACITY 512U

typedef struct {
    uint8_t bytes[FAKE_RANDOM_CAPACITY];
    size_t length;
    size_t cursor;
    bool repeat;
    fake_call_log_t calls;
} fake_random_t;

void fake_random_reset(fake_random_t *random);
void fake_random_set(fake_random_t *random,
                     const uint8_t *bytes,
                     size_t length,
                     bool repeat);
bool fake_random_fill(fake_random_t *random, uint8_t *output, size_t length);

#endif
