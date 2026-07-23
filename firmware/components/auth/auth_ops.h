#ifndef AUTH_OPS_H
#define AUTH_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *context;
    bool (*lock)(void *context);
    bool (*unlock)(void *context);
    bool (*random_fill)(void *context, uint8_t *output, size_t length);
    uint64_t (*now_us)(void *context);
    int (*derive)(void *context,
                  const char *password,
                  size_t password_length,
                  const uint8_t *salt,
                  uint32_t iterations,
                  uint8_t *output);
    void (*secure_zero)(void *context, void *memory, size_t length);
} auth_ops_t;

#endif
