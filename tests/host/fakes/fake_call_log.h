#ifndef FAKE_CALL_LOG_H
#define FAKE_CALL_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FAKE_CALL_LOG_CAPACITY 128U
#define FAKE_EXPECTATION_CAPACITY 128U

typedef struct {
    const char *name;
    uint64_t argument0;
    uint64_t argument1;
} fake_call_t;

typedef struct {
    fake_call_t calls[FAKE_CALL_LOG_CAPACITY];
    size_t call_count;
    const char *expectations[FAKE_EXPECTATION_CAPACITY];
    size_t expectation_count;
    size_t expectation_index;
    const char *failure_name;
    size_t failure_occurrence;
    size_t failure_seen;
    bool strict;
} fake_call_log_t;

void fake_call_log_reset(fake_call_log_t *log);
void fake_call_log_set_strict(fake_call_log_t *log, bool strict);
void fake_call_log_expect(fake_call_log_t *log, const char *name);
void fake_call_log_verify(const fake_call_log_t *log);
void fake_call_log_fail_on(fake_call_log_t *log, const char *name, size_t occurrence);
bool fake_call_log_record(fake_call_log_t *log,
                          const char *name,
                          uint64_t argument0,
                          uint64_t argument1);
const fake_call_t *fake_call_log_at(const fake_call_log_t *log, size_t index);

#endif
