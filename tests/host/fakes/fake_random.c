#include "fake_random.h"

#include <stdlib.h>
#include <string.h>

void fake_random_reset(fake_random_t *random)
{
    if (random == NULL) {
        abort();
    }
    memset(random, 0, sizeof(*random));
    fake_call_log_reset(&random->calls);
}

void fake_random_set(fake_random_t *random,
                     const uint8_t *bytes,
                     size_t length,
                     bool repeat)
{
    if (random == NULL || (bytes == NULL && length != 0U) ||
        length > sizeof(random->bytes)) {
        abort();
    }
    memset(random->bytes, 0, sizeof(random->bytes));
    if (length > 0U) {
        memcpy(random->bytes, bytes, length);
    }
    random->length = length;
    random->cursor = 0U;
    random->repeat = repeat;
}

bool fake_random_fill(fake_random_t *random, uint8_t *output, size_t length)
{
    if (random == NULL || (output == NULL && length != 0U)) {
        abort();
    }
    if (fake_call_log_record(&random->calls, "random_fill", length, random->cursor)) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        if (random->length == 0U) {
            return false;
        }
        if (random->cursor >= random->length) {
            if (!random->repeat) {
                return false;
            }
            random->cursor = 0U;
        }
        output[index] = random->bytes[random->cursor++];
    }
    return true;
}
